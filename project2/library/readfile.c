#include "sifs-internal.h"
#include "../sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdbool.h>
#include "help.h"

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
            SIFS_errno	= SIFS_EINVAL;// Invalid argument
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
                bool find = false;
                for(int i=0;i<dir.nentries;i++){
                    if(bitmap[dir.entries[i].blockID]=='f'){
                        fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[i].blockID*header.blocksize,SEEK_SET);
                        fread(&file, sizeof file, 1, fp);
                        for(int j=0;j<file.nfiles;j++){    
                            if(strcmp(file.filenames[j],pname[num])==0 && dir.entries[i].fileindex==j){ //makesure the file blong to this entry
                                find=true;
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
                if(!find){
                    SIFS_errno	= SIFS_ENOTFILE; //// Not a file
                    return 1;
                }
            }
            else{
                SIFS_DIRBLOCK subdir;
                for(int i=0;i<dir.nentries;i++){
                    if(bitmap[dir.entries[i].blockID]=='d'){
                        fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[i].blockID*header.blocksize,SEEK_SET);
                        fread(&subdir, sizeof subdir, 1, fp); 
                        for(int j=0;j<subdir.nentries;j++){    
                            if(strcmp(subdir.name,pname[num])==0){
                                index = dir.entries[i].blockID;
                                num++;
                            }
                        }   
                    }
                }
                if(index==0){
                    SIFS_errno	= SIFS_ENOENT; // No such file or directory entry
                    return 1;
                }
            }
            
        }
        freenames(pname,numname);
        fclose(fp);
        return 0;
    }
    else{
        SIFS_errno	= SIFS_ENOVOL;// No such volume
        return 1;
    }
    SIFS_errno	= SIFS_ENOTYET;
    return 1;
}
