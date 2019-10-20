#include "library/sifs-internal.h"
#include "sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


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

// read the contents of an existing file from an existing volume
int SIFS_readfile(const char *volumename, const char *pathname,
		  void **data, size_t *nbytes)
{

    FILE* fp = fopen(volumename, "r" );
    if(fp!=NULL){
        
        //initialize the name of the path
        int numname=0;
        char **pname=NULL;
        getnamefrompath(pathname,&numname,&pname);
        
        if(numname==0){
            freenames(pname,numname);   
            //SIFS_errno	= SIFS_EINVAL;// Invalid argument
            return 1;
        }

        //get the info of the header
        SIFS_VOLUME_HEADER  header;
        fread(&header, sizeof header, 1, fp);
        
        //get the bitmap
        SIFS_BIT bitmap[header.nblocks];
        fseek(fp,sizeof header,SEEK_SET);
        fread(&bitmap, sizeof bitmap , 1 , fp);

                int num=0;
        int index=0;
        for(int q=0;q<numname;q++){
            //num++;
            SIFS_DIRBLOCK dir;
            fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
            fread(&dir, sizeof dir, 1, fp);

            if(num==numname-1){
                SIFS_FILEBLOCK file;
                for(int i=0;i<dir.nentries;i++){
                    if(bitmap[dir.entries[i].blockID]=='f'){
                        fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[i].blockID*header.blocksize,SEEK_SET);
                        fread(&file, sizeof file, 1, fp);
                        for(int j=0;j<file.nfiles;j++){    
                            if(strcmp(file.filenames[j],pname[num])==0 && dir.entries[i].fileindex==j){ //makesure the file blong to this entry
                                
                                char oneblock[header.blocksize];
                                *nbytes=file.length;
                                char * newdata = malloc(file.length);
                                int needblock = (file.length/header.blocksize)+1; //find out how many blocks we need to check
                                int remain = file.length-(header.blocksize*(needblock-1));
                                int seq=0;

                                for(int w=1;w<=needblock;w++){
                                    fseek(fp,sizeof(header)+sizeof(bitmap)+(dir.entries[i].blockID+w)*header.blocksize,SEEK_SET);
                                    fread(oneblock, sizeof oneblock, 1 , fp);
                                    if(w==needblock-1){
                                        for(int p=0;p<remain;p++){
                                            newdata[seq]=oneblock[p];
                                            seq++;
                                        }
                                    }
                                    else{    
                                        for(int p=0;p<header.blocksize;p++){
                                            newdata[seq]=oneblock[p];
                                            seq++;
                                        }
                                    }
                                }
                                *data=newdata;
                            }
                        }
                    }
                }
            }
            else{
                SIFS_DIRBLOCK subdir;
                for(int i=0;i<dir.nentries;i++){
                    if(bitmap[dir.entries[i].blockID]=='d'){
                        fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[i].blockID*header.blocksize,SEEK_SET);
                        fread(&subdir, sizeof subdir, 1, fp); 
                        if(strcmp(subdir.name,pname[num])==0){
                            index = dir.entries[i].blockID;
                            num++;
                        }    
                    }
                }
            }
            
        }
        freenames(pname,numname);
        return 0;
    }
    fclose(fp);
    //SIFS_errno	= SIFS_ENOTYET;
    return 1;
}


int main(int argcount, char *argvalue[])
{
    char	*volumename;    // filename storing the SIFS volume
    char	*pathname;      // the required directory name on the volume


	volumename	= argvalue[1];
	pathname	= argvalue[2];
    
    
    void * data ;
    size_t nbytes;
    
  
//  PASS THE ADDRESS OF OUR VARIABLES SO THAT SIFS_dirinfo() MAY MODIFY THEM
    if(SIFS_readfile(volumename, pathname, &data, &nbytes) != 0) {
	//SIFS_perror(argvalue[0]);
    printf("fail\n");
	exit(EXIT_FAILURE);
    }

    char * p = data;

    printf("%s\n",p);
    free(data);
//  SIFS_dirinfo() ALLOCATED MEMORY TO PROVIDE THE entrynames - WE MUST FREE IT
    //free_entrynames(entrynames, nentries);

    return EXIT_SUCCESS;
}