#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>
#include "helper.h"
#include "buffer_helper.h"

RC Rcode;

extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
	(*bm).pageFile = (char *)pageFileName;
	(*bm).numPages = numPages;
	(*bm).strategy = strategy;
	Rcode=RC_OK;

	int buff=sizeof(Frame) * numPages;
	Frame *page = malloc(buff);
	buffSize = numPages;
	int i=0;
	l:
	if(i<buffSize){
	{
		page[i]=(Frame){.data=NULL,.pageNum = -1,.dirtySegment = 0,.fixCount = 0,.hitNum = 0,.index = 0};
	}
	i=i+1;
	goto l;	
	}

	IF_NULL(bm,RC_BUFF_POOL_NOT_FOUND,"The Buffer Pool does not exist");
	IF_NULL(pageFileName,RC_FILE_NOT_FOUND,"The File does not exist");

	(*bm).mgmtData = page;
	wCount = 0;
	clockPtr = 0;
	lfuPtr = 0;
	return Rcode;
}

extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
	Frame *pageFrame = (Frame *)(*bm).mgmtData;
	Rcode=RC_OK;
	IF_NULL(bm,RC_BUFF_POOL_NOT_FOUND,"The Buffer Pool does not exist");

	int res_stst = forceFlushPool(bm);
	return(res_stst!=Rcode?RC_WRITE_BACK_FAILED:0);
	
	int n=0;
	l:
	if(n < buffSize){
	{
	return(pageFrame[n].fixCount != 0?RC_PINNED_PAGES_IN_BUFFER:0);
	}
	n++;
	goto l;	
	}
	free(pageFrame);
	(*bm).mgmtData = NULL;
	return Rcode;
}


extern RC forceFlushPool(BM_BufferPool *const bm)
{
	Frame *pageFrame = (Frame *)(*bm).mgmtData;
	Rcode=RC_OK;	

	IF_NULL(bm,RC_BUFF_POOL_NOT_FOUND,"The Buffer Pool does not exist");
	
	int n=0;
	l:
	if(n < buffSize){
	{
	if(pageFrame[n].fixCount == 0 && pageFrame[n].dirtySegment == 1)
		{
			SM_FileHandle fh;
			int filestatflush=openPageFile(bm->pageFile, &fh);
			(filestatflush != RC_OK?printf("File Opening Error"):0);

			writeBlock(pageFrame[n].pageNum, &fh, pageFrame[n].data);
			int zro=0;
			pageFrame[n].dirtySegment = zro;
			wCount=wCount+1;
			
		}
	}
	n++;
	goto l;	
	}
	return Rcode;
}



extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	Frame *pageFrame = (Frame *)(*bm).mgmtData;
	Rcode=RC_OK;

	IF_NULL(bm,RC_BUFF_POOL_NOT_FOUND,"The Buffer Pool does not exist");
	IF_NULL(page,RC_PAGE_ERROR,"Invalid Page");

	int n=0;
	l:
	if(n < buffSize){
	{

	if(pageFrame[n].pageNum == (*page).pageNum)
		{
			pageFrame[n].dirtySegment = 1;
			return Rcode;		
		}
	}
	n++;
	goto l;	
	}
	return RC_ERROR;
}


extern RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{	
	Frame *pageFrame = (Frame *)(*bm).mgmtData;

	IF_NULL(bm,RC_BUFF_POOL_NOT_FOUND,"The Buffer Pool does not exist");
    IF_NULL(page,RC_PAGE_ERROR,"Invalid Page");

	int n=0;
	while(n < buffSize){

	((pageFrame[n].pageNum == (*page).pageNum)?pageFrame[n].fixCount--:0);
		n++;	
	}
	return RC_OK;

}

extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	Frame *pageFrame = (Frame *)(*bm).mgmtData;
	Rcode=RC_OK;
	int i=0;
	a:
	if(i < buffSize){
		if(pageFrame[i].pageNum == (*page).pageNum)
		{		
			SM_FileHandle fh;
			int filestatforce = openPageFile((*bm).pageFile, &fh);
			((filestatforce != RC_OK)?printf("File Opening Error"):0);

			writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
			int buf=0;
			pageFrame[i].dirtySegment = buf;
			wCount=wCount+1;
		
		}
	i++;
	goto a;
	}
	return Rcode;
}



extern void FIFO(BM_BufferPool *const bm, Frame *page)
{
	Frame *pageFrame = (Frame *) (*bm).mgmtData;
	int firstIndex;
	firstIndex = lastIndex % buffSize;

	int n=0;
	while(n < buffSize){
	if(pageFrame[firstIndex].fixCount == 0)
		{
			if(pageFrame[firstIndex].dirtySegment == 1)
			{
				SM_FileHandle fh;
			 	int filestat=openPageFile((*bm).pageFile, &fh);
				((filestat != RC_OK)?printf("File Opening Error"):0);
				writeBlock(pageFrame[firstIndex].pageNum, &fh, pageFrame[firstIndex].data);
				wCount++;
			}
			pageFrame[firstIndex]=(Frame){.data = (*page).data,.pageNum = (*page).pageNum, .dirtySegment = (*page).dirtySegment, .fixCount = (*page).fixCount };
			break;
		}
		else
		{
			firstIndex++;
			((firstIndex % buffSize == 0)?firstIndex=0:0);
		}
	n++;
	}
}


extern void LFU(BM_BufferPool *const bm, Frame *page)
{

	Frame *pageFrame = (Frame *) (*bm).mgmtData;
	int leastFreqIndex, leastFreqRef;
	leastFreqIndex = lfuPtr;	

	int i=0;
	while(i < buffSize){
	((pageFrame[leastFreqIndex].fixCount == 0)?(leastFreqIndex = (leastFreqIndex + i) % buffSize, leastFreqRef = pageFrame[leastFreqIndex].index):0);
	i++;
	}
	i = (leastFreqIndex + 1) % buffSize;
	int j=0;
	while(j < buffSize){
		((pageFrame[i].index < leastFreqRef)?(leastFreqIndex = i, leastFreqRef = pageFrame[i].index):0);
		i = (i + 1) % buffSize;
	j++;
	}

	if(pageFrame[leastFreqIndex].dirtySegment == 1)
	{
		SM_FileHandle fh;
		int filestat=openPageFile( (*bm).pageFile, &fh);
		((filestat != RC_OK)?printf("File Opening Error"):0);
		writeBlock(pageFrame[leastFreqIndex].pageNum, &fh, pageFrame[leastFreqIndex].data);
		wCount=wCount+1;
	}
	
	pageFrame[leastFreqIndex]=(Frame){.data = (*page).data, .pageNum = (*page).pageNum, .dirtySegment = (*page).dirtySegment, .fixCount = (*page).fixCount };
	lfuPtr = leastFreqIndex + 1+0;
	
}

extern void LRU(BM_BufferPool *const bm, Frame *page)
{	
	Frame *pageFrame = (Frame *) (*bm).mgmtData;
	int loop,LHI, LHN;

loop=0;
while(loop< buffSize){
	if(pageFrame[loop].fixCount == 0)
		{
			LHI = loop;
			LHN = pageFrame[loop].hitNum;
			break;
		}
		loop++;
}	

loop=LHI + 1;
	while(loop < buffSize){
	((pageFrame[loop].hitNum < LHN)?(LHI = loop,LHN = pageFrame[loop].hitNum):0);
	loop++;
	}

	if(pageFrame[LHI].dirtySegment == 1)
	{
		SM_FileHandle fh;
		int filestatlru=openPageFile( (*bm).pageFile, &fh);
		((filestatlru != RC_OK)?printf("File Opening Error"):0);
		writeBlock(pageFrame[LHI].pageNum, &fh, pageFrame[LHI].data);
		wCount=wCount+1;
	}

	pageFrame[LHI]=(Frame){.data = (*page).data, .pageNum = (*page).pageNum, .dirtySegment = (*page).dirtySegment, .fixCount = (*page).fixCount, .hitNum = (*page).hitNum };
}



extern void CLOCK(BM_BufferPool *const bm, Frame *page)
{	
	Frame *pageFrame = (Frame *)  (*bm).mgmtData;
	while(true)
	{
		if(clockPtr % buffSize == 0){
			clockPtr=0;
		}
		else{
			clockPtr=clockPtr;
		}

		if(pageFrame[clockPtr].hitNum == 0)
		{
			SM_FileHandle fh;
			int filestatclk;
			((pageFrame[clockPtr].dirtySegment == 1)?(filestatclk=openPageFile(bm->pageFile, &fh),((filestatclk != RC_OK)?printf("File Opening Error"):0),writeBlock(pageFrame[clockPtr].pageNum, &fh, pageFrame[clockPtr].data),wCount++):0);

			pageFrame[clockPtr]=(Frame){.data = (*page).data, .pageNum = (*page).pageNum,.dirtySegment = (*page).dirtySegment, .fixCount = (*page).fixCount, .hitNum = (*page).hitNum };
			clockPtr++;
			break;	
		}
		else
		{
			pageFrame[clockPtr++].hitNum = 0;		
		}
	}
}


extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
	Frame *pageFrame = (Frame *)(*bm).mgmtData;
	Rcode=RC_OK;

IF_NULL(bm,RC_BUFF_POOL_NOT_FOUND,"The Buffer Pool does not exist");
IF_NULL(page,RC_PAGE_ERROR,"Invalid Page");
	if(pageFrame[0].pageNum == -1)
	{
		SM_FileHandle fh;
		openPageFile((*bm).pageFile, &fh);
		int pp=PAGE_SIZE;
		pageFrame[0]=(Frame){.data = (SM_PageHandle) malloc(pp)};
		ensureCapacity(pageNum,&fh);
		readBlock(pageNum, &fh, pageFrame[0].data);
		pageFrame[0].pageNum = pageNum;
		pageFrame[0].fixCount=pageFrame[0].fixCount+1+0;
		lastIndex = hitCount = 0;
		pageFrame[0].hitNum = hitCount;	
		pageFrame[0].index = 0;
		(*page).pageNum = pageNum;
		(*page).data = pageFrame[0].data;
		return Rcode;		
	}
	else
	{	
		
		bool isBufferFull = true;
	int n=0;	
	while(n < buffSize){
	if(pageFrame[n].pageNum != -1)
			{	
				if(pageFrame[n].pageNum == pageNum)
				{
					pageFrame[n].fixCount=pageFrame[n].fixCount+1;
					isBufferFull = false;
					hitCount=hitCount+1; 
					switch (bm->strategy)
					{
					case RS_LRU:
						pageFrame[n].hitNum = hitCount;
						break;
					case RS_CLOCK:
						pageFrame[n].hitNum = 1;
						break;
					case RS_LFU:
						pageFrame[n].index++;
						break;
					default:
						break;
					}
					(*page).pageNum = pageNum;
					(*page).data = pageFrame[n].data;
					clockPtr=clockPtr+1;
					break;
				}				
			} else {
				SM_FileHandle fh;
				openPageFile((*bm).pageFile, &fh);
				int pg=PAGE_SIZE;
				pageFrame[n]=(Frame){.data = (SM_PageHandle) malloc(pg)};
				readBlock(pageNum, &fh, pageFrame[n].data);
				pageFrame[n].pageNum = pageNum;
				int cnt=1;
				int zri=0;
				pageFrame[n].fixCount = cnt;
				pageFrame[n].index = zri;
				lastIndex=lastIndex+1;	
				hitCount=hitCount+1; 

				switch (bm->strategy)
				{
				case RS_LRU:
					pageFrame[n].hitNum = hitCount;	
					break;
				case RS_CLOCK:
					pageFrame[n].hitNum = 1;
					break;
				default:
					break;
				}	
				(*page).pageNum = pageNum;
				(*page).data = pageFrame[n].data;
				isBufferFull = false;
				break;
			}
	n++;
	}

		if(isBufferFull == true)
		{
			Frame *newPage = (Frame *) malloc(sizeof(Frame));
			SM_FileHandle fh;
			openPageFile((*bm).pageFile, &fh);
			(*newPage).data = (SM_PageHandle) malloc(PAGE_SIZE);
			readBlock(pageNum, &fh, newPage->data);
			(*newPage).pageNum = pageNum;
			(*newPage).dirtySegment = 0;		
			(*newPage).fixCount = 1;
			(*newPage).index = 0;
			lastIndex=lastIndex+1;
			hitCount=hitCount+1;

			switch (bm->strategy)
			{
			case RS_LRU:
				newPage->hitNum = hitCount;
				break;
			case RS_CLOCK:
				newPage->hitNum = 1;
				break;
			default:
				break;
			}
			(*page).pageNum = pageNum;
			(*page).data = newPage->data;			

			if(bm->strategy==RS_FIFO){
				FIFO(bm, newPage);
			}
			else if(bm->strategy==RS_LRU){
				LRU(bm, newPage);
			}
			else if(bm->strategy==RS_CLOCK){
				CLOCK(bm, newPage);
			}
			else if(bm->strategy==RS_LFU){
				LFU(bm, newPage);
			}
			else{
				return Rcode;
			}			
		}		
		return Rcode;
	}	
}

extern PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	int buff=sizeof(PageNumber) * buffSize;
	PageNumber *FContents = malloc(buff);
	Frame *pageFrame = (Frame *) (*bm).mgmtData;

	for(int n=0;n < buffSize;n++){
	((pageFrame[n].pageNum != -1)?FContents[n]=pageFrame[n].pageNum:(FContents[n]=NO_PAGE));
	}
	return FContents;
}


extern bool *getDirtyFlags (BM_BufferPool *const bm)
{
	int fun=sizeof(bool) * buffSize;
	bool *dirtyFlags = malloc(fun);
	Frame *pageFrame = (Frame *)(*bm).mgmtData;
	int n=0;
	while(n < buffSize){
		if(pageFrame[n].dirtySegment == 1){
			dirtyFlags[n] = true;
		}
		else{
			dirtyFlags[n] = false;
		}
	n++;
	}	
	return dirtyFlags;
}

extern int *getFixCounts (BM_BufferPool *const bm)
{
	int buff=sizeof(int) * buffSize;
	int *fixCounts = malloc(buff);
	Frame *pageFrame= (Frame *)(*bm).mgmtData;

	for(int n=0;n < buffSize;n++){
		if(pageFrame[n].fixCount != -1){
			fixCounts[n] = pageFrame[n].fixCount;
		}
		else{
			fixCounts[n] =0;
		}
	}	
	return fixCounts;
}

extern int getNumReadIO (BM_BufferPool *const bm)
{
	if(true){
	int lastI=lastIndex + 1;
	return lastI;
	}
}
extern int getNumWriteIO (BM_BufferPool *const bm)
{
	if(true){
	return wCount;
	}
}

