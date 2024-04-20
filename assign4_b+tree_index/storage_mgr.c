#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>
#include <stdbool.h> 
#include "storage_mgr.h"
#include "helper.h"
#define RC_FAILED_DEL 10

FILE *SMPageFile;
FILE *pFile;
FILE *file;
RC Rcode;


void initStorageManager (void){
  
}

RC createPageFile (char *fileName){
    IF_NULL(fileName,RC_FILE_NOT_FOUND,"The File does not exist");
    Rcode=RC_OK;
    FILE *pgf = fopen(fileName, "w");
    char *tpg_str, *f_pg;
    if(1){
    tpg_str = (char *) calloc(PAGE_SIZE, sizeof(char)); 
    f_pg = (char *) calloc(PAGE_SIZE, sizeof(char));   
    strcat(tpg_str,"1\n");
    }
    if(1){
    fwrite(tpg_str, sizeof(char), PAGE_SIZE, pgf);
    fwrite(f_pg, sizeof(char), PAGE_SIZE, pgf);
    }
    if(1){
    free(tpg_str);
    free(f_pg);
    fclose(pgf);
    }
    return Rcode;
}


RC openPageFile (char *SMPageFileName, SM_FileHandle *smFileHandle)
{
    struct stat FileInfo;
	Rcode=RC_OK;
    SMPageFile = fopen(SMPageFileName, "r");
    bool isOpenFile = (SMPageFile != NULL);
	if(isOpenFile){
		(*smFileHandle).fileName = SMPageFileName;
        (*smFileHandle).curPagePos = 0;
		return((fstat(fileno(SMPageFile), &FileInfo) == -1)?RC_READ_NON_EXISTING_PAGE:0);
        (*smFileHandle).totalNumPages = FileInfo.st_size / PAGE_SIZE;;
		fclose(SMPageFile);
		return Rcode;
	}else{ 
		return RC_FILE_NOT_FOUND;
	}
}

RC closePageFile (SM_FileHandle *smFileHandle)
{
     return((fopen(smFileHandle->fileName, "r+")!= NULL )?RC_OK:RC_FILE_NOT_FOUND);
}

RC destroyPageFile (char *fileName){
  IF_NULL(fileName,RC_FILE_NOT_FOUND,"The File does not exist");
  Rcode=RC_OK;
        if(remove(fileName)==0){
            return Rcode;
        }
        else{
            return RC_FAILED_DEL;
        }
        return Rcode;
}



RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){

    int seekSuccess;
    int seekpage;
    Rcode = RC_OK;

    if (pageNum > (*fHandle).totalNumPages || pageNum < 0){
        return RC_READ_NON_EXISTING_PAGE;
    }


    if ((*fHandle).mgmtInfo == NULL){
        return RC_FILE_NOT_FOUND;
    }

    seekpage = (pageNum+1)*PAGE_SIZE*sizeof(char);
    seekSuccess = fseek((*fHandle).mgmtInfo, seekpage, SEEK_SET);


    if (seekSuccess == 0){
        fread(memPage, sizeof(char), PAGE_SIZE, (*fHandle).mgmtInfo);
        (*fHandle).curPagePos = pageNum;
        return Rcode;
    }
    else{
        return RC_READ_NON_EXISTING_PAGE;
    }
}


int getBlockPos (SM_FileHandle *fHandle){
    IF_NULL((*fHandle).fileName,RC_FILE_NOT_FOUND,"The File does not exist");
    return (*fHandle).curPagePos;
}

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    IF_NULL((*fHandle).fileName,RC_FILE_NOT_FOUND,"The File does not exist");
    return readBlock(0, fHandle, memPage);
}


RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    IF_NULL((*fHandle).fileName,RC_FILE_NOT_FOUND,"The File does not exist");
    int PrevBlock = (*fHandle).curPagePos-1;
    return (readBlock(PrevBlock, fHandle, memPage));
}


RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    IF_NULL((*fHandle).fileName,RC_FILE_NOT_FOUND,"The File does not exist");
    return readBlock((*fHandle).curPagePos, fHandle, memPage);
}


RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    IF_NULL((*fHandle).fileName,RC_FILE_NOT_FOUND,"The File does not exist");
    int NextBlock = (*fHandle).curPagePos+1;
    return readBlock(NextBlock, fHandle, memPage);
}


RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int LastBlock = (*fHandle).totalNumPages;
    return readBlock(LastBlock, fHandle, memPage);
}

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){

    int seekSuccess;
    int seekpage;
    
    Rcode = RC_OK;
    if (pageNum > ((*fHandle).totalNumPages) || (pageNum < 0)){
        return RC_WRITE_FAILED;
    }

    seekpage = (pageNum+1)*PAGE_SIZE*sizeof(char);
    seekSuccess = fseek((*fHandle).mgmtInfo, seekpage, SEEK_SET); 

    if (seekSuccess == 0){
        fwrite(memPage, sizeof(char), PAGE_SIZE, (*fHandle).mgmtInfo); 
        (*fHandle).curPagePos = pageNum;

        return Rcode;
    }
    else{
        return RC_WRITE_FAILED;
    }
}


RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int curPos = (*fHandle).curPagePos;
	return writeBlock (curPos, fHandle, memPage); 
}

RC appendEmptyBlock (SM_FileHandle *fHandle){
	int seekSuccess;
    int seekpage;
	
    SM_PageHandle eb;
    Rcode = RC_OK;
    eb = (char *) calloc(PAGE_SIZE, sizeof(char)); 
    seekpage = ((*fHandle).totalNumPages + 1)*PAGE_SIZE*sizeof(char);
    seekSuccess = fseek((*fHandle).mgmtInfo, seekpage, SEEK_END); 

    if (seekSuccess == 0){
      fwrite(eb, sizeof(char), PAGE_SIZE, (*fHandle).mgmtInfo);
        (*fHandle).totalNumPages = (*fHandle).totalNumPages + 1;
        (*fHandle).curPagePos = (*fHandle).totalNumPages;
		rewind((*fHandle).mgmtInfo);
		fprintf((*fHandle).mgmtInfo, "%d\n" , (*fHandle).totalNumPages); 
        fseek((*fHandle).mgmtInfo, ((*fHandle).totalNumPages + 1)*PAGE_SIZE*sizeof(char), SEEK_SET);
        free(eb);
        return Rcode;
	}
	else{
        free(eb);
		return RC_WRITE_FAILED;
	}
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){

	Rcode = RC_OK;
    if ((*fHandle).totalNumPages < numberOfPages){
		int numPages = numberOfPages - (*fHandle).totalNumPages;
        int i=0;
        while(i < numPages){
            appendEmptyBlock(fHandle); 
            i++;
        }
    }
    return Rcode;
}


