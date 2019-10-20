#include "sifs-internal.h"
#include "../sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "md5.c"
#include "md5.h"
#include "help.h"


// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
		   void *data, size_t nbytes)
{

    FILE* fp = fopen(volumename, "r+" );
    if(fp!=NULL){
        
        //initialize the name of the path
        int numname=0;
        char **pname=NULL;
        getnamefrompath(pathname,&numname,&pname);
        

        //get the info of the header
        SIFS_VOLUME_HEADER  header;
        fread(&header, sizeof header, 1, fp);
        
        //get the bitmap
        SIFS_BIT bitmap[header.nblocks];
        fseek(fp,sizeof header,SEEK_SET);
        fread(&bitmap, sizeof bitmap , 1 , fp);

        //try find the file contain the same content
        int position=0;
        bool find = false;
        bool update = false;
        unsigned char newmd5[MD5_BYTELEN];
        MD5_buffer(data, nbytes, newmd5);       
        for(int i=0;i<header.nblocks;i++){
            if(bitmap[i]=='f'){
                SIFS_FILEBLOCK file;
                fseek(fp,sizeof(header)+sizeof(bitmap)+i*header.blocksize,SEEK_SET);
                fread(&file,sizeof(file),1,fp);
                if(memcmp(newmd5, file.md5, MD5_BYTELEN)==0){
                    find = true;
                    position=i;
                    update = true;
                }
            }
        }
        
        int needblock=(nbytes/header.blocksize)+2; //file block also need a block
        
        //if there no file contain the same content, find the proper position in vol to store the file
        if(!find){
            int num=0;
            for(int i=0;i<=(header.nblocks-needblock);i++){
                num=0;
                for(int j=0;j<needblock;j++){
                    if(bitmap[j+i]=='u'){
                        num++;
                    }
                    else break;                   
                }
                if(num==needblock){
                    position=i;
                    find = true;
                    break;
                }
            }
        }
        if(!find){
            SIFS_errno	= SIFS_ENOSPC;// No space left on volume
            freenames(pname,numname);
            return 1;
        }


        int num=0;
        int index=0;

        for(int q=0;q<numname;q++){
            SIFS_DIRBLOCK dir;
            fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
            fread(&dir, sizeof dir, 1, fp);

            if(num==numname-1){
                //check the same name problem
                for(int g=0;g<dir.nentries;g++){
                    if(bitmap[dir.entries[g].blockID]=='f'){
                        SIFS_FILEBLOCK checkfile;
                        fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[g].blockID*header.blocksize, SEEK_SET);
                        fread(&checkfile,sizeof(checkfile),1,fp);
                        for(int r=0;r<checkfile.nfiles;r++){
                            if(strcmp(checkfile.filenames[r],pname[numname-1])==0){
                                SIFS_errno	= SIFS_EEXIST; // Volume, file or directory already exists
                                return 1;
                            }
                        }                            
                    }
                }
                SIFS_FILEBLOCK file;
                fseek(fp,sizeof(header)+sizeof(bitmap)+position*header.blocksize,SEEK_SET);
                if(update){ //have same content 
                    //update file block
                    fread(&file, sizeof file, 1, fp);
                    file.modtime=time(NULL);
                    file.nfiles++;
                    strcpy(file.filenames[file.nfiles-1],pname[numname-1]);
                    char oneblock[header.blocksize];
                    memset(oneblock, 0, sizeof oneblock);
                    memcpy(oneblock, &file, sizeof file);
                    fseek(fp,sizeof(header)+sizeof(bitmap)+position*header.blocksize,SEEK_SET);
                    fwrite(oneblock, sizeof oneblock, 1, fp);

                    //update super dir
                    dir.nentries++;
                    dir.modtime=time(NULL);
                    dir.entries[dir.nentries-1].blockID=position;
                    dir.entries[dir.nentries-1].fileindex=file.nfiles-1;
                    memset(oneblock, 0, sizeof oneblock);
                    memcpy(oneblock, &dir, sizeof dir);
                    fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
                    fwrite(oneblock, sizeof oneblock, 1, fp);
                }
                else{ //need a creat a few new block
                    //check the same name problem
                    for(int g=0;g<dir.nentries;g++){
                        if(bitmap[dir.entries[g].blockID]=='f'){
                            SIFS_FILEBLOCK checkfile;
                            fseek(fp,sizeof(header)+sizeof(bitmap)+dir.entries[g].blockID*header.blocksize, SEEK_SET);
                            fread(&checkfile,sizeof(checkfile),1,fp);
                            for(int r=0;r<checkfile.nfiles;r++){
                                if(strcmp(checkfile.filenames[r],pname[numname-1])==0){
                                    SIFS_errno	= SIFS_EEXIST; // Volume, file or directory already exists
                                    return 1;
                                }
                            }                            
                        }
                    }
                    //update file block
                    file.modtime=time(NULL);
                    file.nfiles=1;
                    file.firstblockID=position+1;
                    file.length=nbytes;
                    memcpy(&file.md5, &newmd5, sizeof newmd5);
                    strcpy(file.filenames[file.nfiles-1],pname[numname-1]);
                    char oneblock[header.blocksize];
                    memset(oneblock, 0, sizeof oneblock);
                    memcpy(oneblock, &file, sizeof file);
                    fseek(fp,sizeof(header)+sizeof(bitmap)+position*header.blocksize,SEEK_SET);
                    fwrite(oneblock, sizeof oneblock, 1, fp);
                    
                    //update the data block
                    char * datacopy = data;
                    int seq=0;
                    int remain = nbytes-(header.blocksize*(needblock-2));
                    for(int i=1;i<needblock;i++){
                        memset(oneblock, 0, sizeof oneblock);
                        if(i==needblock-1){
                            for(int p=0;p<remain;p++){
                                oneblock[p]=datacopy[seq];
                                seq++;
                            }
                        }
                        else{    
                            for(int p=0;p<header.blocksize;p++){
                                oneblock[p]=datacopy[seq];
                                seq++;
                            }
                        }
                        
                        fseek(fp,sizeof(header)+sizeof(bitmap)+(position+i)*header.blocksize,SEEK_SET);
                        fwrite(oneblock, sizeof oneblock, 1, fp);
                    }
                    //update the bitmap
                    for(int l=position;l<position+needblock;l++){
                        if(l==position){
                            bitmap[l]='f';
                        }
                        else bitmap[l]='b';
                    }
                    fseek(fp,sizeof header,SEEK_SET);
                    fwrite(bitmap, sizeof bitmap,1,fp);
                    //update the super dir
                    dir.nentries++;
                    dir.modtime=time(NULL);
                    dir.entries[dir.nentries-1].blockID=position;
                    dir.entries[dir.nentries-1].fileindex=file.nfiles-1;
                    memset(oneblock, 0, sizeof oneblock);
                    memcpy(oneblock, &dir, sizeof dir);
                    fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
                    fwrite(oneblock, sizeof oneblock, 1, fp);
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
