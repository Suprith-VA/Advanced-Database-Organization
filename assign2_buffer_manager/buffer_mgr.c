#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dberror.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

typedef char* char_ptr;
int iWritingCounter;
int iReadingCounter;

// file handle and queue
SM_FileHandle* fHandle;
stPageLinkedList* stPtrLList;

void ClearMemory()
{
	if(fHandle != NULL)
	{
		free(fHandle);
		fHandle = NULL;
	}

	if(stPtrLList != NULL && stPtrLList->iTotalFramesCount > 0)
	{
		for(int iIterator = 0; iIterator <  stPtrLList->iTotalFramesCount; iIterator++)
		{
			stFrameInfo* objFrameInfo = stPtrLList->start;
			if(objFrameInfo->ptrNextPageInfo != NULL)
			{
				stFrameInfo* temp = objFrameInfo->ptrNextPageInfo;
				free(objFrameInfo->c_ptrFrameData);
				free(objFrameInfo);
				objFrameInfo = temp;
			}
			else
			{
				free(objFrameInfo);
			}
		}
		free(stPtrLList);
	}
}

//Creates a new buffer pool with numPages page frames 
RC initBufferPool(BM_BufferPool *const bm, const char* const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
	fHandle = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));
	stPtrLList = (stPageLinkedList *)malloc(sizeof(stPageLinkedList));
	iWritingCounter = 0;
	iReadingCounter = 0;

	bm->pageFile = (char_ptr) pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	bm->mgmtData = (char_ptr) calloc (numPages,sizeof(char)*PAGE_SIZE);;

	stFrameInfo *arrObjPageInfo[bm->numPages];
	int iNumPages = (bm->numPages) - 1;

	int iIterator = 0;
	//Create Page and Initialise its attributes
	for(iIterator = 0; iIterator <= iNumPages; iIterator++)
	{
		arrObjPageInfo[iIterator] = (stFrameInfo*) malloc(sizeof(stFrameInfo));
		arrObjPageInfo[iIterator]->iFixCount = 0;
		arrObjPageInfo[iIterator]->pageNum = -1;
		arrObjPageInfo[iIterator]->iFrameNum = iIterator;
		arrObjPageInfo[iIterator]->iDirtyPage = 0;
		arrObjPageInfo[iIterator]->c_ptrFrameData = (char_ptr) calloc(PAGE_SIZE, sizeof(char));
	}
	 	
	for(iIterator = 0; iIterator <= iNumPages; iIterator++)
	{
		if(iIterator == iNumPages && iIterator == 0)
		{
			//head node(frame)
			if(iIterator == 0)
			{
				arrObjPageInfo[iIterator]->ptrPrevPageInfo = NULL;
				arrObjPageInfo[iIterator]->ptrNextPageInfo = arrObjPageInfo[iIterator+1];
			}
			//tail node(frame)
			else
			{
				arrObjPageInfo[iIterator]->ptrPrevPageInfo =  arrObjPageInfo[iIterator-1];
				arrObjPageInfo[iIterator]->ptrNextPageInfo = NULL;
			}
		}
		//link each node (frame) to previous and next 
		else
		{				
			arrObjPageInfo[iIterator]->ptrNextPageInfo = arrObjPageInfo[iIterator + 1];
			arrObjPageInfo[iIterator]->ptrPrevPageInfo = arrObjPageInfo[iIterator - 1];
		}		
	}	
	//Initalize buffer attributes
	stPtrLList->iFrameFilled = 0;
	stPtrLList->iTotalFramesCount = bm->numPages;
	stPtrLList->start = arrObjPageInfo[0];
	stPtrLList->end = arrObjPageInfo[iNumPages];	

	if(openPageFile(bm->pageFile, fHandle) != RC_OK)
	{
		ClearMemory();	
	}	

    return RC_OK;
}

//Here we destory the buffer pool
RC shutdownBufferPool(BM_BufferPool *const bm)
{
	if(bm != NULL)
	{
		//Write back changes to the disk
		int iRet = forceFlushPool(bm);
		if(iRet == RC_OK)
		{
			stFrameInfo* objPageInfo = stPtrLList->start;
			//Check if any page is being accessed 
			for(int iIterator = 0; iIterator < stPtrLList->iFrameFilled; iIterator++)
			{
				if(objPageInfo != NULL)
				{
					if(objPageInfo->iFixCount != 0)
					{
						return RC_OK;
					}
					if(objPageInfo->ptrNextPageInfo != NULL)
						objPageInfo = objPageInfo->ptrNextPageInfo;
				}
			}

			objPageInfo = stPtrLList->start;
			//Free memory 
			for(int iIterator = 0; iIterator <  stPtrLList->iTotalFramesCount; iIterator++)
			{
				if(objPageInfo->ptrNextPageInfo != NULL)
				{
					stFrameInfo* temp = objPageInfo->ptrNextPageInfo;
					free(objPageInfo);
					objPageInfo = temp;
				}
				else
				{
					free(objPageInfo);
				}
			}

			closePageFile(fHandle);
			if(fHandle != NULL) 
			{
				free(fHandle);
				fHandle = NULL;
			}

			return RC_OK;
		}
		//Return status incase of disk write failure
		else
		{
			ClearMemory();
			return RC_FAILED_WRITEBACK;
		}
	}
	else
	{
		ClearMemory();
		return RC_BUFFER_POOL_NOTFOUND;
	}
		
}

//Causes dirty pages from the buffer pool to be written to disk
RC forceFlushPool(BM_BufferPool *const bm)
{
	if(bm!=NULL)
	{
		int iTotFrames = stPtrLList->iTotalFramesCount;
		stFrameInfo* objPageInfo = stPtrLList->start;
		int iIterator;
		for(iIterator = 0; iIterator < iTotFrames; iIterator++)
		{
			if(objPageInfo != NULL)
			{
				//check if frame is being acessed 
				if(objPageInfo->iFixCount == 0)
				{
					//check if the page content is changed
					if(objPageInfo->iDirtyPage == 1)
					{
						int iPageNum = objPageInfo->pageNum;
						writeBlock(iPageNum, fHandle, objPageInfo->c_ptrFrameData);
						objPageInfo->iDirtyPage = 0;
						iWritingCounter++;
					}
				}		
				if(objPageInfo->ptrNextPageInfo != NULL)
						objPageInfo = objPageInfo->ptrNextPageInfo;
			}	
		}

		return RC_OK;
	}
	return RC_BUFFER_POOL_NOTFOUND;
}

//Marks a page as a dirty
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	if(bm != NULL)
	{
		if(page != NULL)
		{
			stFrameInfo* objPageInfo = stPtrLList->start;
			int iIterator;
			//set dirty bit if changes has been made
			for(iIterator = 0; iIterator < bm->numPages; iIterator++)
			{
				if(objPageInfo != NULL)
				{
					if(objPageInfo->pageNum == page->pageNum)
					{
						objPageInfo->iDirtyPage = 1;
						break;	
					}
					if(objPageInfo->ptrNextPageInfo != NULL)
						objPageInfo = objPageInfo->ptrNextPageInfo;
				}	
			}

			return RC_OK;
		}//return status if file is not present
		else
		{
			ClearMemory();
			return RC_FILE_NOT_FOUND;
		}
			
	}
   //return status if buffer pool is not found
	else
	{
		ClearMemory();
		return RC_BUFFER_POOL_NOTFOUND;
	}		
}

//Unpin the input page
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	if(bm != NULL)
	{
		if(page != NULL)
		{
			stFrameInfo* objPageInfo = stPtrLList->start;
			
			int iIterator;
			//check for the frame that is not no longer accessed by user
			for(iIterator = 0; iIterator < bm->numPages; iIterator++)
			{
				if(objPageInfo != NULL)
				{
					if(objPageInfo->pageNum == page->pageNum)
					{
						break;
					}
					//shift the pointer to next frame in bufffer
					if(objPageInfo->ptrNextPageInfo != NULL)
						objPageInfo = objPageInfo->ptrNextPageInfo;
				}	
			}
            //decrease the acess count of the page which the user has stopped using
			if(iIterator < bm->numPages || iIterator > bm->numPages)
				objPageInfo->iFixCount = objPageInfo->iFixCount-1;
			else
			{
				ClearMemory();
				return RC_READ_NON_EXISTING_PAGE;
			}

			return RC_OK;
		}
		else
		{
			ClearMemory();
			return RC_FILE_NOT_FOUND;
		}
	}
	else
	{
		ClearMemory();
		return RC_BUFFER_POOL_NOTFOUND;
	}	
}

//Write the current content of the page back to the page file
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	if(bm != NULL)
	{
		if(page != NULL)
		{
			stFrameInfo* objPageInfo = stPtrLList->start;
			
			int iIterator;
			//check for the frame in the buffer that has be written to disk
			for(iIterator = 0; iIterator < bm->numPages; iIterator++)
			{
				if(objPageInfo != NULL)
				{
					if(objPageInfo->pageNum == page->pageNum)
					{    //write the frame back to disk
						if(writeBlock(objPageInfo->pageNum, fHandle, objPageInfo->c_ptrFrameData)!=0)
						{
							ClearMemory();
							return RC_WRITE_FAILED;
						}
						else
						    //increment the write counter
							iWritingCounter++;

						break;
					}
					//shift the pointer to next frame in bufffer
					if(objPageInfo->ptrNextPageInfo != NULL)
						objPageInfo = objPageInfo->ptrNextPageInfo;
				}	
			}

			return RC_OK;
		}
		else
		{
			ClearMemory();
			return RC_FILE_NOT_FOUND;
		}		
	}
	else
	{
		ClearMemory();
		return RC_BUFFER_POOL_NOTFOUND;
	}		
}

//Creates integer array that is equal to size buffer pool and store the page number if exists w.r.t each frame 
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	if(bm != NULL)
	{
		PageNumber* iArrFrameContents[bm->numPages];
		*iArrFrameContents = malloc(bm->numPages * sizeof(int));
	
		bool bFrameFound;
		int iIterator;
		for(iIterator = 0; iIterator < bm->numPages; iIterator++)
		{
			stFrameInfo* objPageInfo = stPtrLList->start;
			bFrameFound = true;
            
			while(bFrameFound && objPageInfo != NULL)
			{
				//if particular frame is present in the buffer then store page number of page in the frame
				if(objPageInfo->iFrameNum == iIterator){
					(*iArrFrameContents)[iIterator] = objPageInfo->pageNum;
					bFrameFound = false;
				}

				if(bFrameFound)
					objPageInfo = objPageInfo->ptrNextPageInfo;
			}
		}
		return *iArrFrameContents;
	}
}

//Creates boolean array that is equal to size buffer pool and indicate if Dirty bit is set w.r.t each frame 
bool *getDirtyFlags (BM_BufferPool *const bm)
{
	if(bm != NULL)
	{
		bool (*iArrDirtyFlags)[bm->numPages];
		iArrDirtyFlags = malloc(bm->numPages * sizeof(bool));

		int iIterator;
		for(iIterator = 0; iIterator < bm->numPages; iIterator++)
		{
			stFrameInfo* objPageInfo = stPtrLList->start;

			while(objPageInfo != NULL)
			{
				//set the value to true if dirty bit is 1 in the frame
				if(objPageInfo->iFrameNum == iIterator){
					if(objPageInfo->iDirtyPage != 0 ){
						(*iArrDirtyFlags)[iIterator] = TRUE;
					}
					//set the boolean value false if dirty bit is 0
					else{
						(*iArrDirtyFlags)[iIterator] = FALSE;
					}
					break;
				}
				if(objPageInfo->ptrNextPageInfo != NULL)
					objPageInfo = objPageInfo->ptrNextPageInfo;
				else
					break;
			}
		}
		return (*iArrDirtyFlags);
	}
}

//Creates integer array and stores the access count w.r.t each frame 
int *getFixCounts (BM_BufferPool *const bm)
{
	if(bm != NULL)
	{
		int* iArrFixCounts[bm->numPages];
		*iArrFixCounts = malloc(bm->numPages * sizeof(int));

		int iIterator;
		//iterating throught frames
		for(iIterator = 0; iIterator < bm->numPages; iIterator++)
		{
			stFrameInfo* objPageInfo = stPtrLList->start;
			while(objPageInfo != NULL)
			{   //stores the access count value to the array
				if(objPageInfo->iFrameNum == iIterator){
					(*iArrFixCounts)[iIterator] = objPageInfo->iFixCount;
					break;
				}

				if(objPageInfo->ptrNextPageInfo != NULL)
					objPageInfo = objPageInfo->ptrNextPageInfo;
				else
					break;
			}
		}

		return *iArrFixCounts;
	}
}

//gettter function that returns the readcount
int getNumReadIO (BM_BufferPool *const bm)
{
	return iReadingCounter;
}

//getter function that returns the writecount
int getNumWriteIO (BM_BufferPool *const bm)
{
	return iWritingCounter;
}
//Here the page is pinned with page number using the replacement strategy choosen by user
RC pinPage(BM_BufferPool * const bm, BM_PageHandle * const page,const PageNumber pageNum)
{
	int iRet = 0;
	
	ReplacementStrategy strategy = bm->strategy;
	switch(strategy)
	{
		//First In First Out strategy is used for page replacement
		case RS_FIFO:
			iRet = FIFOpin(bm,page,pageNum);\
			break;
		
		//Least Recently Used strategy is used for page replace
		case RS_LRU:
			iRet = LRUpin(bm,page,pageNum);
			break;
	}

	if(iRet != RC_OK)
	{
		ClearMemory();
	}

	return iRet;
}

//framelist is remove
RC removePageFrameLList() 
{
	int itr;
	stFrameInfo* objPageInfo = stPtrLList->start;

	for(itr=0; itr < stPtrLList->iFrameFilled; itr++){
		if(itr==(stPtrLList->iFrameFilled-1))
			stPtrLList->end = objPageInfo;
		else
			objPageInfo = objPageInfo->ptrNextPageInfo;
	}

	// Setting temp variables
	int iNum; 
	int iDelPage = 0;
	stFrameInfo *objPageInfo1 = stPtrLList->end;
	itr = 0;
	
	for(itr = 0; itr < stPtrLList -> iTotalFramesCount; itr++){  
		if(objPageInfo1->iFixCount==0){
			iDelPage = objPageInfo1->pageNum; 
			if(objPageInfo1->pageNum != stPtrLList -> end -> pageNum){ //check if it not the last page 
				objPageInfo1->ptrNextPageInfo-> ptrPrevPageInfo = objPageInfo1->ptrPrevPageInfo;
				objPageInfo1->ptrPrevPageInfo->ptrNextPageInfo = objPageInfo1->ptrNextPageInfo;
			}
			else{
				stPtrLList -> end = stPtrLList -> end -> ptrPrevPageInfo;
				stPtrLList -> end -> ptrNextPageInfo = NULL;
			}
		}
		else{
			iNum = objPageInfo1->pageNum;
			objPageInfo1 = objPageInfo1->ptrPrevPageInfo;
		}
	}

    //Write page to disk if it is modified before removing from buffer
	if(objPageInfo1->iDirtyPage==1){
		iWritingCounter++;
		writeBlock(objPageInfo1->pageNum, fHandle, objPageInfo1->c_ptrFrameData);
	}
	stPtrLList->iFrameFilled--;

	if(iNum == stPtrLList->end->pageNum){
		return 0;
	}

	return iDelPage;
}

//enqueing the page frame
RC addPageFrameLList(BM_PageHandle * const page, const PageNumber pageNum,BM_BufferPool * const bm) 
{
		stFrameInfo* objNewPageInfo = (stFrameInfo*) malloc(sizeof(stFrameInfo));
		
		objNewPageInfo->iFrameNum = 0;
		objNewPageInfo->pageNum = pageNum;
		objNewPageInfo->iFixCount = 1;
		objNewPageInfo->iDirtyPage = 0;
		objNewPageInfo->ptrPrevPageInfo=NULL;
		objNewPageInfo->ptrNextPageInfo = NULL;
		
		//allocating memory to the page 
		char_ptr cChar = (char_ptr) malloc(PAGE_SIZE * sizeof(char));
		objNewPageInfo->c_ptrFrameData = cChar;		
		
		stFrameInfo* objPageInfo = objNewPageInfo;
		int iDel = -1;
		
		if (!(stPtrLList->iFrameFilled != stPtrLList->iTotalFramesCount)) { // remove pages if the buffer is full
			iDel = removePageFrameLList();
		}

		if (stPtrLList->iFrameFilled != 0) { //If the buffer has some prefilled frames 
			readBlock(pageNum, fHandle, objPageInfo->c_ptrFrameData);	// read block of data
			if(iDel==-1)
				objPageInfo->iFrameNum = stPtrLList->start->iFrameNum+1;
			else
				objPageInfo->iFrameNum=iDel;//store the frame number of page that was deleted lastly
			page->data = objPageInfo->c_ptrFrameData;
			iReadingCounter++;
			objPageInfo->ptrNextPageInfo = stPtrLList->start;
			stPtrLList->start->ptrPrevPageInfo = objPageInfo;
			stPtrLList->start = objPageInfo;
			page->pageNum = pageNum ;
		} 
		else {  //If there are no frames in buffer that are filled
			readBlock(objPageInfo->pageNum,fHandle,objPageInfo->c_ptrFrameData);
			page->data = objPageInfo->c_ptrFrameData;
			objPageInfo->pageNum=pageNum;
			page->pageNum = pageNum;			
			objPageInfo->ptrNextPageInfo = stPtrLList->start;
			stPtrLList->start->ptrPrevPageInfo = objPageInfo;
			objPageInfo->iFrameNum=stPtrLList->start->iFrameNum;
			stPtrLList->start=objPageInfo;
			iReadingCounter++;
		}
		stPtrLList->iFrameFilled++;

		return RC_OK; 
}

//Here the page is pinned with page number along with LRU replacement strategy
RC LRUpin(BM_BufferPool * const bm, BM_PageHandle * const page,const PageNumber pageNum)
{
	bool bPageFound = false;
	stFrameInfo* objPageInfo = stPtrLList->start;
	int iIterator;
	for(iIterator = 0; iIterator < bm->numPages; iIterator++)
	{
		if(objPageInfo != NULL)
		{
			if (objPageInfo->pageNum == pageNum)// Check if the page is found 
			{
				bPageFound = true;
				break;
			}
			if(objPageInfo->ptrNextPageInfo != NULL)
				objPageInfo = objPageInfo->ptrNextPageInfo;	
		}	
	}

	if(!bPageFound)//Add the page to the buffer if its not found
	{
		addPageFrameLList(page, pageNum, bm);
		return RC_OK;	
	}

	page->pageNum = pageNum;
	page->data = objPageInfo->c_ptrFrameData;
	objPageInfo->iFixCount++;
     
	 //Add the page at the first frame 
	if(objPageInfo == stPtrLList->start)
	{
		stPtrLList->start = objPageInfo;
		objPageInfo->ptrNextPageInfo = stPtrLList->start;
		stPtrLList->start->ptrPrevPageInfo = objPageInfo;	
	}
	else
	{
		objPageInfo->ptrPrevPageInfo->ptrNextPageInfo = objPageInfo->ptrNextPageInfo;	
	}

	if(objPageInfo->ptrNextPageInfo && objPageInfo == stPtrLList->start)
	{
		return 0;
	}
	else if(objPageInfo != stPtrLList->start && objPageInfo->ptrNextPageInfo)
	{
		objPageInfo->ptrNextPageInfo->ptrPrevPageInfo = objPageInfo->ptrPrevPageInfo;
        //If it is the only page in the the frame
		if(objPageInfo == stPtrLList->end)
		{
			stPtrLList->end = objPageInfo->ptrPrevPageInfo;
			stPtrLList->end->ptrNextPageInfo = NULL;				
			objPageInfo->ptrPrevPageInfo = NULL;
			objPageInfo->ptrNextPageInfo = stPtrLList->start;
			objPageInfo->ptrNextPageInfo->ptrPrevPageInfo = objPageInfo;
			stPtrLList->start = objPageInfo;
		}
		else{
			objPageInfo->ptrPrevPageInfo = NULL;
			objPageInfo->ptrNextPageInfo = stPtrLList->start;			
			objPageInfo->ptrNextPageInfo->ptrPrevPageInfo = objPageInfo;
			stPtrLList->start = objPageInfo;		
		}
	}		

	return RC_OK;
}


//Here the page is pinned with page number along with FIFO replacement strategy
RC FIFOpin(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{		
	stFrameInfo *objPageInfoEx =stPtrLList->start;

	// Here we are finding the node for the input pagenumber
	int iIterator = 0;
	for(iIterator = 0; iIterator < bm->numPages; iIterator++)
	{
			if (objPageInfoEx->pageNum == pageNum)
			{  
				objPageInfoEx->iFixCount++;   
				page->data = objPageInfoEx->c_ptrFrameData;
				page->pageNum = pageNum;

				return RC_OK;						
			}
			else
			{
				objPageInfoEx = objPageInfoEx->ptrNextPageInfo;				
			}
	}
				
	stFrameInfo *objPageInfo = stPtrLList->start;
	int iRet = -1;

	while(stPtrLList->iFrameFilled < stPtrLList->iTotalFramesCount)
	{
		if (objPageInfo->pageNum == -1)
		{    
			objPageInfo->iFixCount = 1; //Initliase page attributes if page is not previously present
			objPageInfo->iDirtyPage = 0;
			objPageInfo->pageNum = pageNum;
			page->pageNum= pageNum;
			
			stPtrLList->iFrameFilled = stPtrLList->iFrameFilled + 1 ;
			
			readBlock(objPageInfo->pageNum, fHandle, objPageInfo->c_ptrFrameData);
			
			page->data = objPageInfo->c_ptrFrameData;
			iReadingCounter++;  //increase the read counter once the page is read
			iRet = 0;
			break;	
		}
		else
		{
			objPageInfo = objPageInfo->ptrNextPageInfo;		
		}
	}
	
	if(iRet == 0)
	{
		return RC_OK;	
	}
	
	// Creating new node
	stFrameInfo *newPageInfoNode = (stFrameInfo *) malloc (sizeof(stFrameInfo));
	page->pageNum = pageNum;
	newPageInfoNode->iDirtyPage = 0;
	newPageInfoNode->iFixCount = 1;
	newPageInfoNode->ptrPrevPageInfo = stPtrLList->end;
	newPageInfoNode->c_ptrFrameData = NULL;
	newPageInfoNode->pageNum = pageNum;
	newPageInfoNode->ptrNextPageInfo = NULL;

	objPageInfo = stPtrLList->start;
	for(iIterator = 0; iIterator < bm->numPages; iIterator++)
	{
		if(objPageInfo != NULL)
		{
			if(objPageInfo->iFixCount != 0) 
			{
				if(objPageInfo->ptrNextPageInfo != NULL)  //check if it not the last node 
					objPageInfo = objPageInfo->ptrNextPageInfo;
			}	
			else
			 	break;
		}	
	}
	
	stFrameInfo *objNewPageInfo = objPageInfo;

	if(iIterator == bm->numPages)
	{
		return RC_NO_FREESPACE_ERROR;
	}
    
	if(objPageInfo!=stPtrLList->start && objPageInfo!= stPtrLList->end)
	{
		objPageInfo->ptrPrevPageInfo->ptrNextPageInfo = objPageInfo->ptrNextPageInfo;
		objPageInfo->ptrNextPageInfo->ptrPrevPageInfo=objPageInfo->ptrPrevPageInfo;
	}
    //check if it is the head node
	if(stPtrLList->start==objPageInfo)
	{
		stPtrLList->start = stPtrLList->start->ptrNextPageInfo;
		stPtrLList->start->ptrPrevPageInfo = NULL;
	}
    // Add the new page at the tail
	if(stPtrLList->end == objPageInfo)
	{
		stPtrLList->end = objPageInfo->ptrPrevPageInfo;
		newPageInfoNode->ptrPrevPageInfo=stPtrLList->end;
	}
	
	// If page is dirty, we write back to the disk
	if(objNewPageInfo->iDirtyPage == 1)
	{
		iWritingCounter++;
		writeBlock(objNewPageInfo->pageNum,fHandle,objNewPageInfo->c_ptrFrameData);
	}
     
	newPageInfoNode->iFrameNum = objNewPageInfo->iFrameNum;	
	newPageInfoNode->c_ptrFrameData = objNewPageInfo->c_ptrFrameData;


	readBlock(pageNum, fHandle, newPageInfoNode->c_ptrFrameData);
	page->data = newPageInfoNode->c_ptrFrameData;
	iReadingCounter++;	//Increament the read count after the page is read
		
	stPtrLList->end->ptrNextPageInfo = newPageInfoNode;
	stPtrLList->end = newPageInfoNode;  
    
    return RC_OK;
}


