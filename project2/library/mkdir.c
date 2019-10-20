#include "sifs-internal.h"
#include "../sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "help.h"

// make a new directory within an existing volume
int SIFS_mkdir(const char *volumename, const char *dirname)
{   
    FILE * fp = fopen(volumename,"r+");

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
        fseek(fp,0,SEEK_SET);
        fread(&header, sizeof header, 1, fp);
        
        //get the bitmap
        SIFS_BIT bitmap[header.nblocks];
        fseek(fp,sizeof header,SEEK_SET);
        fread(&bitmap, sizeof bitmap , 1 , fp);
        
        //find the postion which is unused
        int position=0;
        for(int i=0;i<header.nblocks;i++){  
            if(bitmap[i]=='u'){
                position = i;
                break;
            }
        }
        if(position == 0){
            SIFS_errno	= SIFS_ENOSPC;// No space left on volume
            return 1;
        }
        

        if(numname==1){
            SIFS_DIRBLOCK dir;
            fseek(fp,sizeof(header)+sizeof(bitmap), SEEK_SET);
            fread(&dir,sizeof(dir),1,fp);

            //check the same name problem
            for(int g=0;g<dir.nentries;g++){
                if(bitmap[dir.entries[g].blockID]=='d'){
                    SIFS_DIRBLOCK checkdir;
                    fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[g].blockID*header.blocksize, SEEK_SET);
                    fread(&checkdir,sizeof(checkdir),1,fp);
                    if(strcmp(checkdir.name,pname[numname-1])==0){
                        SIFS_errno	= SIFS_EEXIST; // Volume, file or directory already exists
                        return 1;
                    }
                }
            }

            dir.nentries++;
            dir.modtime=time(NULL);
            dir.entries[dir.nentries-1].blockID=position;
            char oneblock[header.blocksize];
            memset(oneblock, 0, sizeof oneblock);
            memcpy(oneblock, &dir, sizeof dir);
            fseek(fp,sizeof(header)+sizeof(bitmap), SEEK_SET);
            fwrite(oneblock, sizeof oneblock, 1, fp);
        }
        else{
            int index =0;
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

                if(i==numname-2){
                    if(subdir.nentries==SIFS_MAX_ENTRIES){ // Too many directory or file entries
                        SIFS_errno	= SIFS_EMAXENTRY;
                        return 1;
                    }
                    subdir.modtime=time(NULL);
                    subdir.nentries++;
                    subdir.entries[subdir.nentries-1].blockID=position;
                    char oneblock[header.blocksize];
                    memset(oneblock, 0, sizeof oneblock);
                    memcpy(oneblock, &subdir, sizeof subdir);
                    fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize, SEEK_SET);
                    fwrite(oneblock, sizeof oneblock, 1, fp);
                    break;
                }

            }
        }

        //create a new dir and write it to the vol
        SIFS_DIRBLOCK nameddir;
        nameddir.modtime=time(NULL);
        strcpy(nameddir.name,pname[numname-1]);
        nameddir.nentries=0;
        char oneblock[header.blocksize];
        memset(oneblock, 0, sizeof oneblock);        // cleared to all zeroes
        memcpy(oneblock, &nameddir, sizeof nameddir);
        fseek(fp,sizeof header + sizeof bitmap + header.blocksize * position,SEEK_SET);
        fwrite(oneblock, sizeof oneblock, 1, fp);

        //update the bitmap
        bitmap[position]='d';
        fseek(fp,sizeof header,SEEK_SET);
        fwrite(bitmap, sizeof(bitmap), 1, fp);

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
