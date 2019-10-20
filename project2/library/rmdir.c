#include "sifs-internal.h"
#include "../sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "help.h"


// remove an existing directory from an existing volume
int SIFS_rmdir(const char *volumename, const char *dirname)
{   
    FILE* fp = fopen(volumename, "r+" );
    if(fp!=NULL){
        
        //initialize the name of the path
        int numname=0;
        char **pname=NULL;
        getnamefrompath(dirname,&numname,&pname);
        
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
            SIFS_DIRBLOCK dir;
            fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
            fread(&dir, sizeof dir, 1, fp);

            if(num==numname-1){
                SIFS_DIRBLOCK subdir;
                bool find =false;
                for(int i=0;i<dir.nentries;i++){
                    if(bitmap[dir.entries[i].blockID]=='d'){
                        fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[i].blockID*header.blocksize,SEEK_SET);
                        fread(&subdir, sizeof subdir, 1, fp);
                          
                        if(strcmp(pname[num],subdir.name)==0){
                            find = true;
                            if(subdir.nentries>0){
                                SIFS_errno	= SIFS_ENOTEMPTY; //Directory is not empty
                                return 1;
                            }
                            char oneblock[header.blocksize]; //delete the current dir block
                            memset(oneblock, 0, sizeof oneblock);
                            fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[i].blockID*header.blocksize,SEEK_SET);
                            fwrite(oneblock, sizeof oneblock, 1, fp);
                            
                            //update the bitmap
                            bitmap[dir.entries[i].blockID]='u';
                            fseek(fp,sizeof(header), SEEK_SET);
                            fwrite(&bitmap, sizeof(bitmap), 1, fp); 

                            //update the super dir
                            for(int t=i;t<dir.nentries;t++){ 
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
                    }
                }
                if(!find){
                    SIFS_errno	= SIFS_ENOENT; // No such file or directory entry
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
    else{
        SIFS_errno	= SIFS_ENOVOL;// No such volume
        return 1;
    }
    SIFS_errno	= SIFS_ENOTYET;
    return 1;
}
