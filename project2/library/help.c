#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "help.h"

// get the name individully from pathname
void getnamefrompath(const char *pathname, int* n, char*** name){
    char **na=NULL;
    char *token;
    char *string = malloc(1+strlen(pathname));
    int num=0;
    const char s[2]="/";
    strcpy(string,pathname);
    token = strtok(string,s);
    while(token!=NULL){
        
        na = realloc(na,(num+1)*sizeof(na[0]));
        na[num] = strdup(token);
        
        num++;
        token = strtok(NULL,s);
        
    }
    *n=num;
    *name=na;
    free(string);
}

void freenames(char **entrynames, uint32_t nentries)
{
    if(entrynames != NULL) {
        for(int e=0 ; e<nentries ; ++e) {
            free(entrynames[e]);
        }
        free(entrynames);
    }
}
