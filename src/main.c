#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "image.h"
#include "disc_info.h"
#include "crc32.h"

int main(int argc, char *argv[])
{
    char *inputFile = NULL;
    char *outputFile = NULL;
    bool doProfile = false;
    bool doShrink = false;
    bool doUnshrink = false;

    int opt;
    while ((opt = getopt(argc, argv, "i:o:hpsu")) != -1) {
        switch (opt) {
            case 'p':
                doProfile = true;
                break;
            case 's':
                doShrink = true;
                break;
            case 'u':
                doUnshrink = true;
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
            case 'h':
            default:
                fprintf(stderr, "Usage: %s -s|-u [-i inputFile] [-o outputFile]\n", argv[0]);
                return 1;
            }
    }

    if (doProfile) {
        struct DiscInfo * discInfo = profileImage(inputFile);
        printDiscInfo(discInfo);
    } else if(doShrink){
        // Creating a shrunken image will take two passes.
        // One to prifile the disc and one to write the shrunken image
        struct DiscInfo * discInfo = profileImage(inputFile);
        printDiscInfo(discInfo);
        shrinkImage(discInfo, inputFile, outputFile);
    } else if(doUnshrink){
        // Unshrinking an image can be done in a single pass
        unshrinkImage(inputFile, outputFile);
    }
}
