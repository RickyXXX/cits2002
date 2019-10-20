#include "sifs-internal.h"
#include "../sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>



int SIFS_defra(const char *volumename,int startposition, int length)
{

    FILE* fp = fopen(volumename, "r+" );
    if(fp!=NULL){

        //get the info of the header
        SIFS_VOLUME_HEADER  header;
        fread(&header, sizeof header, 1, fp);
        
        //get the bitmap
        SIFS_BIT bitmap[header.nblocks];
        fseek(fp,sizeof header,SEEK_SET);
        fread(&bitmap, sizeof bitmap , 1 , fp);

        //update the super dir
        for(int i=startposition;i<startposition+length;i++){
            for(int j=0;j<header.nblocks;j++){
                if(bitmap[j]=='d'){
                    SIFS_DIRBLOCK dir;
                    fseek(fp,sizeof(header)+sizeof(bitmap)+j*header.blocksize,SEEK_SET);
                    fread(&dir, sizeof dir, 1, fp);
                    for(int a=0;a<dir.nentries;a++){
                        if(dir.entries[a].blockID==i){
                            dir.entries[a].blockID--;
                            char oneblock[header.blocksize]; 
                            memset(oneblock, 0, sizeof oneblock);
                            memcpy(oneblock, &dir, sizeof dir);
                            fseek(fp,sizeof(header)+sizeof(bitmap)+j*header.blocksize,SEEK_SET);
                            fwrite(oneblock, sizeof oneblock, 1, fp);
                        }
                    }
                }
            }   
        }

        //update the file in moving blocks 
        for(int i=startposition;i<startposition+length;i++){
            if(bitmap[i]=='f'){
                SIFS_FILEBLOCK file;
                fseek(fp,sizeof(header)+sizeof(bitmap)+i*header.blocksize,SEEK_SET);
                fread(&file, sizeof file, 1, fp);
                file.firstblockID--;
                char oneblock[header.blocksize];
                memset(oneblock, 0, sizeof oneblock);
                memcpy(oneblock, &file, sizeof file);
                fseek(fp,sizeof(header)+sizeof(bitmap)+i*header.blocksize,SEEK_SET);
                fwrite(oneblock, sizeof oneblock, 1, fp);
            }
        }

        //find the position we want to move into it
        int position=0;
        for(int i=0;i<header.nblocks;i++){
            if(bitmap[i]=='u'){
                position=i;
                break;
            }
        }

        //move the blocks
        char blocks[length*header.blocksize];
        fseek(fp,sizeof(header)+sizeof(bitmap)+startposition*header.blocksize,SEEK_SET);
        fread(blocks, sizeof blocks, 1, fp);

        fseek(fp,sizeof(header)+sizeof(bitmap)+startposition*header.blocksize,SEEK_SET);
        char empty[length*header.blocksize];
        memset(empty, 0, sizeof empty);
        fwrite(empty, sizeof empty, 1, fp);

        fseek(fp,sizeof(header)+sizeof(bitmap)+position*header.blocksize,SEEK_SET);
        fwrite(blocks, sizeof blocks, 1, fp);

        //update the bitmap
        for(int i=position;i<position+length;i++){
            bitmap[i]=bitmap[startposition+(i-position)];
        }
        for(int i=position+length;i<position+length+length;i++){
            bitmap[i]='u';
        }
        fseek(fp,sizeof(header),SEEK_SET);
        fwrite(bitmap, sizeof bitmap, 1, fp);

        fclose(fp);
        return 0;
    }
    
    fclose(fp);
    //SIFS_errno	= SIFS_ENOTYET;
    return 1;
}

