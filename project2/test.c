#include "library/sifs-internal.h"
#include "sifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "library/md5.c"
#include "library/md5.h"

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
        //printf("%s %d\n",pname[0],numname);
        
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
        //printf("%d %d %d\n",position,find,update);
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
            //SIFS_errno	= SIFS_ENOSPC;// No space left on volume
            freenames(pname,numname);
            return 1;
        }


        int num=0;
        int index=0;

        for(int q=0;q<numname;q++){
            //num++;
            SIFS_DIRBLOCK dir;
            fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
            fread(&dir, sizeof dir, 1, fp);

            if(num==numname-1){
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
                    //char oneblock[header.blocksize];
                    memset(oneblock, 0, sizeof oneblock);
                    memcpy(oneblock, &dir, sizeof dir);
                    fseek(fp,sizeof(header)+sizeof(bitmap)+index*header.blocksize,SEEK_SET);
                    fwrite(oneblock, sizeof oneblock, 1, fp);
                }
                else{ //need a creat a few new block
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
	pathname	= argvalue[3];
    
    
    
    //printf("0:%s,1:%s,2:%s,3:%s\n",argvalue[0],argvalue[1],argvalue[2],argvalue[3]);
    FILE * fp = fopen(argvalue[2],"rb");
    
    
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char * data = malloc(size);

    fread(data,1, size, fp);
    // for(int i=0;i<size;i++){
    //     printf("%c",data[i]);
    // }
    // printf("\n");
    
  
//  PASS THE ADDRESS OF OUR VARIABLES SO THAT SIFS_dirinfo() MAY MODIFY THEM
    if(SIFS_writefile(volumename, pathname, data, size) != 0) {
	//SIFS_perror(argvalue[0]);
    printf("fail\n");
	exit(EXIT_FAILURE);
    }

    free(data);
//  SIFS_dirinfo() ALLOCATED MEMORY TO PROVIDE THE entrynames - WE MUST FREE IT
    //free_entrynames(entrynames, nentries);

    return EXIT_SUCCESS;
}
