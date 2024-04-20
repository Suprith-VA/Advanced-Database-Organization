#include "storage_mgr.h"
#include "string.h"
#include "stdio.h"
#include "dberror.h"
#include "unistd.h"
#include "stdlib.h"

typedef FILE file_datatype;
typedef char* char_ptr;

file_datatype *filePtr = NULL;

void initStorageManager(void)
{
  // Initialize the Storage Manager
  printf("Start Storage Manager");
}

RC createPageFile(char *fileName)
{
  // creating a page file
  filePtr = fopen(fileName, "w+");
 	
  // check if the file could be opened
  if(filePtr == NULL)
  {
    return RC_FILE_NOT_FOUND;
  }

  // construct memory of PAGE_SIZE and set it to '\0'
  char_ptr chWriteMemory = (char_ptr) malloc(PAGE_SIZE * sizeof(char));
  memset(chWriteMemory,'\0', PAGE_SIZE);

  // write the block content to file
  fwrite(chWriteMemory, sizeof(char), PAGE_SIZE, filePtr);

  // free allocated memory
  free(chWriteMemory);
  
  // file close
  fclose(filePtr);
  
  return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle)         
{
  // open the page file and set the meta data for SM_FileHandle
  filePtr = fopen(fileName, "r+");    
       
  if(filePtr != NULL)                                                   
  { 
    if(fseek(filePtr, 0, SEEK_END) == 0)
    {
      /* It is a valid page file fseek is successfull */
      
      // find the size and check if it is valid page file
      int iSizeBytes = ftell(filePtr);
      if(iSizeBytes != -1)   
      {
        // update the meta data
        fHandle->fileName = fileName;
        
        // check if the position goes to the next page and update accordingly
        if((iSizeBytes % PAGE_SIZE) != 0)
        {
          fHandle->totalNumPages = (iSizeBytes/PAGE_SIZE) + 1;
        }                  
        else
        {
          fHandle->totalNumPages = (iSizeBytes/PAGE_SIZE);
        }
        
        // make sure the current page position is the 1st one
        rewind(filePtr);
        fHandle->curPagePos = 0;    
                                          
        return RC_OK;
      }
      else
      {
        // the current position is non existing
        
        // return non existing page error
	return RC_READ_NON_EXISTING_PAGE;
      }                
        
    }
    else
    {
      // invalid page file
      
      // return non existing page error
      return RC_READ_NON_EXISTING_PAGE;
    }                                                               
  }
  else
  {
    return RC_FILE_NOT_FOUND;       
  }                           
}

RC closePageFile(SM_FileHandle *fHandle)
{
  /* closing the page file which is opened */
  
  // check for valid file pointer
  if(filePtr != NULL)     
  {
    // close the file
    if(fclose(filePtr) != 0)                                       
      return RC_FILE_NOT_FOUND;                              
    else
      return RC_OK; 
  }
  else
  {
    return RC_FILE_NOT_FOUND;                                      
  }
}

RC destroyPageFile (char *Fname)
{
  /*Check if file exists and Delete the page file*/  
   
  // check if the file exists
  if (access(Fname, F_OK) == 0) 
  {
    // file exists
    // remove the file
    char_ptr chMemBlock = malloc(PAGE_SIZE * sizeof(char)); 
    free(chMemBlock);
    
    if(remove(Fname) != 0)                                 
      return RC_FILE_NOT_FOUND;
    else
      return RC_OK;
  } 
  else 
  {
    // file doesn't exist
    return RC_FILE_NOT_FOUND;
  }       
}

RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
  /*Read the given page in the memory block*/
  
  // check if valid file
  if(filePtr == NULL)
  {
    return RC_FILE_HANDLE_NOT_INIT;
  }
  else
  {
    // check if the page number specified is more than total number ofpafes
    if (pageNum > fHandle->totalNumPages) 
    {
      return RC_READ_NON_EXISTING_PAGE;        
    }                                 
    else 
    {
      // perform seek and get the page position
      int iPageSeek = pageNum * PAGE_SIZE;
      
      // get the cursor position
      int iCurPos = fseek(filePtr, iPageSeek, SEEK_SET);
      
      if(iCurPos != 0) 
      {
        return -1;
      }
      else
      {
        // valid  cursor position
        fread(memPage, sizeof(char), PAGE_SIZE, filePtr);  
        
        // update the curPagePos
        fHandle->curPagePos = ftell(filePtr);   
                              
        return RC_OK;                 
      }                                                     
    }
  } 
}

RC getBlockPos (SM_FileHandle *fHandle)
{
  // get the current block position
  if(fHandle != NULL)                                                     
  {
    int iPosPage = fHandle->curPagePos;
    
    // verify the page position
    if(iPosPage < -1)
    {
      printf("File not inilized or invalid file handle");
    }
    
    return iPosPage;                          
  }
  else
  {
    return RC_FILE_HANDLE_NOT_INIT;
  }	
}    

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // Reading first block
    // file handle check
    if(fHandle == NULL)                                                     
    {
      return RC_FILE_NOT_FOUND;                                  
    }
    else
    {
      RC iRet = readBlock (0, fHandle, memPage);
      return iRet;       
    }
} 

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // Reading previous block
    
    // find the previous block and read 
    if(fHandle != NULL)                                                     
    {
      return RC_FILE_NOT_FOUND;                                 
    }
    else
    {
      // find the previous block
      int iPrevious = fHandle->curPagePos - 1;
      RC iRet = readBlock(iPrevious, fHandle, memPage);
      
      return iRet;
    }	
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // Reading current block
    
    // find the previous block and read 
    if(fHandle != NULL)                                                     
    {
      return RC_FILE_NOT_FOUND;                                 
    }
    else
    {
      // find the previous block
      int iPrevious = fHandle->curPagePos - 1;
      
      // pass the current block
      int iCurrent = fHandle->curPagePos;
      RC iRet = readBlock(iCurrent, fHandle, memPage);
      
      return iRet;
    }	
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // Reading next block
    
    // find the previous block and read 
    if(fHandle != NULL)                                                     
    {
      return RC_FILE_NOT_FOUND;                                 
    }
    else
    {
      // find the previous block
      int iPrevious = fHandle->curPagePos - 1;
      
      // pass the next block
      int iCurrent = fHandle->curPagePos;
      int iNext = fHandle->curPagePos + 1;
      RC iRet = readBlock(iNext, fHandle, memPage);
      
      return iRet;
    }	
}	

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
  // Write page content to into the page given
  
  // check if page number is more than zero
  // also check if page number is less than total pages
  if (pageNum > 0 || pageNum < fHandle->totalNumPages)
  {
    // check for valid file handle 
    if(fHandle == NULL)  
    {
      return RC_FILE_NOT_FOUND;
    }     
    else
    {
      // check file pointer validity
      if(filePtr == NULL)
      {
       return RC_FILE_HANDLE_NOT_INIT;
      }
      
      // searching the position
      int iSeeked = fseek(filePtr, PAGE_SIZE * pageNum, SEEK_SET);
      if (iSeeked != 0) 
      {
        return RC_WRITE_FAILED;
      } 
      else
      {
        // bytes seeked
        fwrite(memPage, sizeof(char), PAGE_SIZE, filePtr);
        
        // current position in the file and update handler
        fHandle->curPagePos = ftell(filePtr) / PAGE_SIZE; 
        
        fseek(filePtr, 0, SEEK_END);
        int iSize = ftell(filePtr);
        
        // update the total number of pages to the new one and handle flow to next page
        if((iSize % PAGE_SIZE) == 0)
        {
          fHandle->totalNumPages = (iSize/PAGE_SIZE);
        }
        else
        {
          fHandle->totalNumPages = (iSize/PAGE_SIZE) + 1;
        }
        
        return  RC_OK;              
      }            
    }	
  }
  else
  {
    return RC_WRITE_FAILED;      
  }
}

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
  // write page contents to the current block
    
  // check file handle
  if(fHandle == NULL)                                                     
  {
    return RC_FILE_NOT_FOUND;                                   
  }
  else
  {
    // writing the current block
    int iCurrent = fHandle->curPagePos;
    RC iRet = writeBlock(iCurrent, fHandle, memPage);
    
    return iRet;         
  }	
}

RC appendEmptyBlock(SM_FileHandle *fHandle) 
{
  // file handle check
  if(fHandle == NULL)                                                     
  {
    return RC_FILE_NOT_FOUND;                              
  }
  else
  {
    // create the memory and fill it with null
    char_ptr chMemBlock = (char_ptr)calloc(PAGE_SIZE, sizeof(char));  
    
    // seek till end  
    int iSeek = fseek(filePtr, 0, SEEK_END);
    
    if(iSeek != 0)
    {
      free(chMemBlock);
      return RC_WRITE_FAILED;
    }
    
    // perform write on the file
    size_t iSize = fwrite(chMemBlock, sizeof(char), PAGE_SIZE, filePtr);
    if(iSize == 0)  
    {
      free(chMemBlock);  
      return RC_WRITE_FAILED;
    }                                              
    else
    {
      // update the total number of pages and current page position
      fHandle->totalNumPages++;         
      //fHandle->curPagePos = fHandle->totalNumPages - 1; 
      
      // free the allocated memeory
      free(chMemBlock);                                           
      return RC_OK;                                         
    }
  }	
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) 
{
    // ensure the capacity of the page file before appending empty block
    if (fHandle == NULL)
    {
      return RC_FILE_HANDLE_NOT_INIT; 
    }
    else
    {
      // check if the given number is less than the total number of pages available  
      if(fHandle->totalNumPages < numberOfPages)                                                           
      {
        // add difference if pages as empty blocks 
        int iNumDiff = numberOfPages - fHandle->totalNumPages;
        for(int i = 0; i < iNumDiff; i++)
        {
          appendEmptyBlock(fHandle); 
        }                                                   
      }
      return RC_OK;    
    }
}


        


