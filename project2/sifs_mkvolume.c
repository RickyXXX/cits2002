#include <stdio.h>
#include <stdlib.h>
#include "sifs.h"

//  Written by Chris.McDonald@uwa.edu.au, September 2019

//  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename blocksize nblocks\n", progname);
    fprintf(stderr, "or     %s blocksize nblocks\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    char	*volumename;    // filename storing the SIFS volume
    size_t	blocksize;
    uint32_t	nblocks;

//  ATTEMPT TO OBTAIN THE volumename FROM AN ENVIRONMENT VARIABLE
    if(argcount == 3) {
	volumename	= getenv("SIFS_VOLUME");
	if(volumename == NULL) {
	    usage(argvalue[0]);
	}
	blocksize	= atoi(argvalue[1]);
	nblocks		= atoi(argvalue[2]);
    }
//  ... OR FROM A COMMAND-LINE PARAMETER
    else if(argcount == 4) {
	volumename	= argvalue[1];
	blocksize	= atoi(argvalue[2]);
	nblocks		= atoi(argvalue[3]);
    }
    else {
	usage(argvalue[0]);
	exit(EXIT_FAILURE);
    }

//  ATTEMPT TO CREATE THE NEW VOLUME
    if(SIFS_mkvolume(volumename, blocksize, nblocks) != 0) {
	SIFS_perror(argvalue[0]);
	exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
