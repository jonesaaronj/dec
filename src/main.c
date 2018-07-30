#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "partition.h"

void handleFiles(struct DiscInfo * discInfo, char *inputFile, char *outputFile) {

}

/**
 * Create a shrunken image from the input file and disc info
 */
void createShrunkImage(struct DiscInfo * discInfo, char *inputFile, char *outputFile) {

    if (!discInfo->isGC && !discInfo->isWII) {
        fprintf(stderr, "ERROR: We are not a GC or WII disc\n");
        return;
    }

    size_t discBlockNum = discInfo->isGC ? GC_BLOCK_NUM :
        discInfo->isWII && discInfo->isDualLayer ? WII_DL_BLOCK_NUM : WII_BLOCK_NUM;

    size_t lastBlockSize = discInfo->isGC ? GC_LAST_BLOCK_SIZE :
        discInfo->isWII && discInfo->isDualLayer ? WII_DL_LAST_BLOCK_SIZE : WII_LAST_BLOCK_SIZE;

    // if file pointer is empty read from stdin
    FILE *inputF = (inputFile != NULL) ? fopen(inputFile, "rb") : stdin;
    // if file pointer is empty read from stdout
    FILE *outputF = (outputFile != NULL) ? fopen(outputFile, "wb") : stdout;

    // Do all of our reading in 0x40000 byte blocks
    unsigned char * buffer = calloc(1, BLOCK_SIZE);
    
    // write partition block
    if (fwrite(discInfo->table, BLOCK_SIZE, 1, outputF) != 1) {
        fprintf(stderr, "ERROR: could not write block\n");
        return;
    }

    uint64_t dataBlockNum = 1;
    uint64_t blockNum = 0;
    size_t read;
    while((read = fread(buffer, 1, BLOCK_SIZE, inputF)) > 0) {

        // set the block size to write
        size_t writeSize = (blockNum == discBlockNum - 1) ? lastBlockSize : BLOCK_SIZE;

        // get the junk block
        unsigned char * junk = getJunkBlock(blockNum, discInfo->discId, discInfo->discNumber);
        
        // make sure the first data block has the correct id and disc number
        if (blockNum == 0) {
            dataBlockNum++;
            if (memcmp(buffer, discInfo->table + 8, 7) == 0) {
                if (fwrite(buffer, 1, writeSize, outputF) != writeSize) {
                    fprintf(stderr, "ERROR: could not write block\n");
                    break;
                }
            } else {
                fprintf(stderr, "ERROR: disc info does not match\n");
                break;
            }
        }

        // if this is a junk block skip writing it
        else if (isSame(buffer, junk, read)) {
            if (memcmp(&JUNK_BLOCK_MAGIC_WORD, discInfo->table + ((blockNum + 1) * 8), 8) != 0) {
                fprintf(stderr, "ERROR: Saw a junk block at %llu but expected something else\n", blockNum);
                break;
            }
        }

        // if we got this far we should be a data block
        // make sure our table has the correct address
        else if (memcmp(&dataBlockNum, discInfo->table + ((blockNum + 1) * 8), 8) == 0) {
            if (fwrite(buffer, 1, writeSize, outputF) != writeSize) {
                fprintf(stderr, "ERROR: could not write block\n");
                break;
            }
            dataBlockNum++;
        }

        else {
            uint64_t address =  ((uint64_t) discInfo->table[((blockNum + 1) * 8) + 7] << 0) +
                                ((uint64_t) discInfo->table[((blockNum + 1) * 8) + 6] << 8) +
                                ((uint64_t) discInfo->table[((blockNum + 1) * 8) + 5] << 16) +
                                ((uint64_t) discInfo->table[((blockNum + 1) * 8) + 4] << 24) +
                                ((uint64_t) discInfo->table[((blockNum + 1) * 8) + 3] << 32) +
                                ((uint64_t) discInfo->table[((blockNum + 1) * 8) + 2] << 40) +
                                ((uint64_t) discInfo->table[((blockNum + 1) * 8) + 1] << 48) +
                                ((uint64_t) discInfo->table[((blockNum + 1) * 8) + 0] << 56);

            fprintf(stderr, "ERROR: Saw a data block but address was wrong at %llu ", blockNum);
            fprintf(stderr, "expected %llu but %llu is in the table\n", dataBlockNum, address);
            break;
        }

        blockNum++;
    }
}

int main(int argc, char *argv[])
{
    char *inputFile = NULL;
    char *outputFile = NULL;
    bool doProfile = false;

    int opt;
    while ((opt = getopt(argc, argv, "i:o:pq")) != -1) {
        switch (opt) {
            case 'p':
                doProfile = true;
                break;
            case 'i':
                inputFile = optarg; 
                break;
            case 'o': 
                outputFile = optarg;
                break;
            case '?':
                if (optopt == 'i' || optopt == 'i') {
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                }            
            default:
                fprintf(stderr, "Usage: %s [-i inputFile] [-o outputFile]\n", argv[0]);
                return 1;
            }
    }

    if (doProfile) {
        struct DiscInfo * discInfo = profileImage(inputFile);
        printDiscInfo(discInfo);
    } else {
        // Creating a shrunken image will take two passes.
        // One to prifile the disc and one to do the write the shrunken image
        struct DiscInfo * discInfo = profileImage(inputFile);
        createShrunkImage(discInfo, inputFile, outputFile);
    }
}