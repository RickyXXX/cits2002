#include "sifs-internal.h"
#include "../sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "help.h"

//  CITS2002 Project 2 2019
//  Name(s):             haoran zhang , Shehjad Omar Ibrahim
//  Student number(s):   22289211 , 22566957

int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
{   
    
    FILE * fp = fopen(volumename,"r");

    if(fp!=NULL){
        //initialize the name of the path
        int numname=0;
        char **pname=NULL;
        getnamefrompath(pathname,&numname,&pname);
        

        //get the info of the header
        SIFS_VOLUME_HEADER  header;
        fseek(fp,0,SEEK_SET);
        fread(&header, sizeof header, 1, fp);
        
        //get the bitmap
        SIFS_BIT bitmap[header.nblocks];
        fseek(fp,sizeof header,SEEK_SET);
        fread(&bitmap, sizeof bitmap , 1 , fp);

        int index =0;// except get the root dir 
        for(int i=0;i<numname;i++){ 
            SIFS_DIRBLOCK dir;
            SIFS_DIRBLOCK subdir;
            fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize, SEEK_SET);
            fread(&dir,sizeof(dir),1,fp);

            for(int j=0;j<dir.nentries;j++){ //find the sub dir
                
                if(bitmap[dir.entries[j].blockID]=='d'){
                    fseek(fp,sizeof(header)+sizeof(bitmap)+(dir.entries[j].blockID)*header.blocksize, SEEK_SET);
                    fread(&subdir,sizeof(subdir),1,fp);

                    if(strcmp(pname[i],subdir.name)==0){
                        index = dir.entries[j].blockID;
                        break;
                    }
                }
            }

            if(index==0){
                SIFS_errno	= SIFS_ENOENT; // No such file or directory entry
                return 1;
            }
            
            if(i==numname-1){
                *modtime=subdir.modtime;
                *nentries=subdir.nentries;
                char** name = malloc(subdir.nentries * sizeof name[0]);
                for(int z=0;z<subdir.nentries;z++){
                    name[z]=malloc(SIFS_MAX_NAME_LENGTH * sizeof(char));
                }
                for(int w=0;w<subdir.nentries;w++){
                    int indexblock = subdir.entries[w].blockID;
                    int indexfile = subdir.entries[w].fileindex;
                    SIFS_DIRBLOCK dir1;
                    SIFS_FILEBLOCK fil;
                    fseek(fp,sizeof header + sizeof bitmap + indexblock * header.blocksize,SEEK_SET);

                    if(bitmap[indexblock]=='f'){ //if the entry is file
                        fread(&fil, sizeof fil , 1 , fp);
                        name[w]=strdup(fil.filenames[indexfile]);
                    }
                    else if(bitmap[indexblock]=='d'){
                        fread(&dir1, sizeof dir1 , 1 , fp);
                        name[w]=strdup(dir1.name);
                    }
                    else continue;
                }
                *entrynames = name;
                break;
            }
        }
        if(*pathname=='/' && numname==0){ //if we want to get the info from root dir
                SIFS_DIRBLOCK dir;
                fseek(fp,sizeof(header)+sizeof(bitmap), SEEK_SET);
                fread(&dir,sizeof(dir),1,fp);

                
                *modtime=dir.modtime;
                *nentries=dir.nentries;
                char** name = malloc(dir.nentries * sizeof name[0]);
                for(int z=0;z<dir.nentries;z++){
                    name[z]=malloc(SIFS_MAX_NAME_LENGTH * sizeof(char));
                }
                for(int w=0;w<dir.nentries;w++){
                    int indexblock = dir.entries[w].blockID;
                    int indexfile = dir.entries[w].fileindex;
                    SIFS_DIRBLOCK dir1;
                    SIFS_FILEBLOCK fil;
                    fseek(fp,sizeof header + sizeof bitmap + indexblock * header.blocksize,SEEK_SET);

                    if(bitmap[indexblock]=='f'){ //if the entry is file
                        fread(&fil, sizeof fil , 1 , fp);
                        name[w]=strdup(fil.filenames[indexfile]);
                    }
                    else if(bitmap[indexblock]=='d'){
                        fread(&dir1, sizeof dir1 , 1 , fp);
                        name[w]=strdup(dir1.name);
                    }
                    else continue;
                }
                *entrynames = name;
            }
        //FINISHED, free the memory
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
    

    SIFS_errno	= SIFS_ENOTYET;// Not yet implemented
    return 1;
}
