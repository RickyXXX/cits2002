#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sifs.h"


//  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename pathname\n", progname);
    fprintf(stderr, "or     %s pathname\n", progname);
    exit(EXIT_FAILURE);
}

//  SIFS_dirinfo() ALLOCATED MEMORY TO PROVIDE THE entrynames - WE MUST FREE IT
void free_entrynames(char **entrynames, uint32_t nentries)
{
    if(entrynames != NULL) {
        for(int e=0 ; e<nentries ; ++e) {
            free(entrynames[e]);
        }
        free(entrynames);
    }
}

int main(int argcount, char *argvalue[])
{
    char	*volumename;    // filename storing the SIFS volume
    char	*pathname;      // the required directory name on the volume

//  ATTEMPT TO OBTAIN THE volumename FROM AN ENVIRONMENT VARIABLE
    if(argcount == 2) {
	volumename	= getenv("SIFS_VOLUME");
	if(volumename == NULL) {
	    usage(argvalue[0]);
	}
	pathname	= argvalue[1];
    }
//  ... OR FROM A COMMAND-LINE PARAMETER
    else if(argcount == 3) {
	volumename	= argvalue[1];
	pathname	= argvalue[2];
    }
    else {
	usage(argvalue[0]);
	exit(EXIT_FAILURE);
    }

//  VARIABLES TO STORE THE INFORMATION RETURNED FROM SIFS_dirinfo()
    char        **entrynames;
    uint32_t    nentries;
    time_t      modtime;

//  PASS THE ADDRESS OF OUR VARIABLES SO THAT SIFS_dirinfo() MAY MODIFY THEM
    if(SIFS_dirinfo(volumename, pathname, &entrynames, &nentries, &modtime) != 0) {
	SIFS_perror(argvalue[0]);
	exit(EXIT_FAILURE);
    }

//  PRINT WHAT WE NOW KNOW ABOUT THE DIRECTORY
    printf("modified %s\n", ctime(&modtime) );
    for(int e=0 ; e<nentries ; ++e) {
        printf("%s\n", entrynames[e]);
    }

//  SIFS_dirinfo() ALLOCATED MEMORY TO PROVIDE THE entrynames - WE MUST FREE IT
    free_entrynames(entrynames, nentries);

    return EXIT_SUCCESS;
}
