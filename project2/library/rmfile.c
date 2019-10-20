#include "sifs-internal.h"
#include "../sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "help.h"


// remove an existing file from an existing volume
int SIFS_rmfile(const char *volumename, const char *pathname)
{   
    FILE* fp = fopen(volumename, "r+" );
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
                            if(strcmp(file.filenames[j],pname[num])==0 && dir.entries[i].fileindex==j){ //makesure the remove file blong to this entry
                                find=true;
                                if(file.nfiles>1){ //if there are more than 1 file using this content
                                    for(int w=j;w<file.nfiles;w++){ //update the current file block
                                        strcpy(file.filenames[w],file.filenames[w+1]);                             
                                    }
                                    file.nfiles--;                            
                                    file.modtime=time(NULL);
                                    char oneblock[header.blocksize];
                                    memset(oneblock, 0, sizeof oneblock);
                                    memcpy(oneblock, &file, sizeof file);
                                    fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[i].blockID*header.blocksize,SEEK_SET);
                                    fwrite(oneblock, sizeof oneblock, 1, fp);

                                    for(int t=i;t<dir.nentries;t++){ //update the super dir
                                        dir.entries[t].blockID=dir.entries[t+1].blockID;
                                        dir.entries[t].fileindex=dir.entries[t+1].fileindex;                                
                                    }
                                    dir.nentries--;                            
                                    dir.modtime=time(NULL);
                                    memset(oneblock, 0, sizeof oneblock);
                                    memcpy(oneblock, &dir, sizeof dir);
                                    fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
                                    fwrite(oneblock, sizeof oneblock, 1, fp);

                                }
                                else{ //if there are only one file using this content
                                    int numblock = (file.length/header.blocksize)+2;
                                    char oneblock[header.blocksize];
                                    memset(oneblock, 0, sizeof oneblock);
                                    fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[i].blockID*header.blocksize,SEEK_SET);
                                    for(int a=0;a<numblock;a++){ 
                                        fwrite(oneblock, sizeof oneblock, 1, fp);
                                        bitmap[dir.entries[i].blockID+a]='u';
                                    }
                                    fseek(fp,sizeof(header), SEEK_SET);
                                    fwrite(&bitmap, sizeof(bitmap), 1, fp); //update the bitmap
                                    
                                    for(int w=i;w<dir.nentries;w++){ //uddate the super dir
                                        dir.entries[w].blockID=dir.entries[w+1].blockID;
                                        dir.entries[w].fileindex=dir.entries[w+1].fileindex;                                
                                    }
                                    dir.nentries--;                            
                                    dir.modtime=time(NULL);
                                    memset(oneblock, 0, sizeof oneblock);
                                    memcpy(oneblock, &dir, sizeof dir);
                                    fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
                                    fwrite(oneblock, sizeof oneblock, 1, fp);
                                    break;
                                }
                            }
                        }
                    }
                }
                if(!find){
                    SIFS_errno	= SIFS_ENOENT; //// No such file or directory entry
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
        //  FINISHED, CLOSE THE VOLUME
        fclose(fp);
        //  AND RETURN INDICATING SUCCESS
        return 0;
    }
    
    
    SIFS_errno	= SIFS_ENOTYET;
    return 1;
}
