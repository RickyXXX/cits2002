#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sifs.h"

 //  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename sourcefilename pathname\n", progname);
    fprintf(stderr, "or     %s sourcefilename pathname\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    char	*volumename;    // filename storing the SIFS volume
    char	*pathname;      // the required directory name on the volume

//  ATTEMPT TO OBTAIN THE volumename FROM AN ENVIRONMENT VARIABLE
    if(argcount == 3) {
	volumename	= getenv("SIFS_VOLUME");
	if(volumename == NULL) {
	    usage(argvalue[0]);
	}
	pathname	= argvalue[2];
    }
//  ... OR FROM A COMMAND-LINE PARAMETER
    else if(argcount == 4) {
	volumename	= argvalue[1];
	pathname	= argvalue[3];
    }
    else {
	usage(argvalue[0]);
	exit(EXIT_FAILURE);
    }
    
    FILE * fp = fopen(argvalue[2],"rb");
    
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char * data = malloc(size);

    fread(data,1, size, fp);
    

    if(SIFS_writefile(volumename, pathname, data, size) != 0) {
	SIFS_perror(argvalue[0]);
	exit(EXIT_FAILURE);
    }

    free(data);


    return EXIT_SUCCESS;
}
