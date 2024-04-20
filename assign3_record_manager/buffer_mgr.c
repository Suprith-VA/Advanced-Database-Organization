#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

int iCheck = 0;
int buff_size = 0;
int l_indx = 0,count = 0;
int h_cnt = 0;
int clk_pntr = 0,lfu_pntr = 0;
int iFlag = 0;
RC Return_code;
bool bCheckIdx = true;

extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{

    checkStorageManagerValid();
    while(bm != NULL && pageFileName !=NULL){

        checkBufferPoolEmpty();
        bm->pageFile=(char *)pageFileName;
        if(iCheck == iFlag)
        bm->strategy=strategy;

        if(true)
        bm->numPages=numPages;

        int size = sizeof(PageFrame);
        buff_size= numPages;
        int cnt=0;
        if(iCheck == iFlag)
        {
            if(size== 0){
                return RC_PAGE_NOT_FOUND_ERROR;
            }
        }
        
        PageFrame *pf=malloc(bm->numPages*size);  
        do{
            checkStorageManagerValid();

            pf[cnt].data = NULL;
            int temp_pgNum = -1;
            if(iCheck == iFlag)
                pf[cnt].pageNum = temp_pgNum;
            pf[cnt].dirty_seg = 0;

            checkBufferPoolEmpty();
            pf[cnt].fixCount = 0;

            if(iCheck == iFlag)
                pf[cnt].hit_cnt = 0;

            if(true)
                cnt++;
        }while(cnt<numPages);

        bm->mgmtData = pf;
        if(true)
        lfu_pntr = 0,clk_pntr = 0; 
        
        bufferPoolContents();

        return RC_OK;
    }

    if(bm == NULL){
        return RC_BUFFER_POOL_NOT_FOUND ;

    } 
    else{

    return  RC_FILE_NOT_FOUND;
    }
}

extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
    PageFrame *pf = (PageFrame *)bm->mgmtData;

    while(bCheckIdx)
    {
        if(forceFlushPool(bm) == RC_OK){
        int cntr=0;
        bufferPoolContents();

        Top: 
            if(pf[cntr].fixCount == 0)
                checkBufferPoolEmpty();
            else
            {
                if(iCheck == iFlag)
                    return RC_PINNED_PAGES_IN_BUFFER;
            }
                
            cntr = 0;    
            if(!(cntr >=buff_size)){
                bufferPoolContents();
                cntr++;
                goto Top;
            }
            
            bm->mgmtData = NULL;
            
        }
        break;
    }
    
    free(pf);
    return RC_OK;
}

extern RC forceFlushPool(BM_BufferPool *const bm)
{
    int iFlushState = 0;
    while(bm!=NULL){
        PageFrame *pf = (PageFrame *)bm->mgmtData;
        checkStorageManagerValid();
        SM_FileHandle fh;
        int cntr=0;
        isPageFrameEmpty();

        while(cntr<buff_size && pf[cntr].dirty_seg < 1){
            checkBufferPoolEmpty();
            if((! pf[cntr].fixCount!=0) && pf[cntr].dirty_seg ==0){
                iFlushState = 5;
                openPageFile(bm->pageFile,&fh);
                int num = pf[cntr].pageNum;

                if(iCheck == iFlag)
                    writeBlock(num,&fh,pf[cntr].data);
            }
        count++;
        validatePageHandler(iFlushState);
        pf[cntr].dirty_seg=0;

        cntr++;
        }
    return RC_OK;
    }
    return RC_BUFFER_POOL_NOT_FOUND;
}

extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const p)
{

    int iPageEmpty = 1;
    while(bCheckIdx)
    {
        if(bm == NULL || p == NULL)
            return bm == NULL ? RC_BUFFER_POOL_NOT_FOUND : RC_PAGE_NOT_FOUND_ERROR;
        else{
            checkStorageManagerValid();
            PageFrame *pf = (PageFrame *)bm->mgmtData;
            if(pf != NULL){
                checkBufferPoolEmpty();
                for(int i=0;i<buff_size;i++){
                    validatePageHandler(iPageEmpty);
                    if(!(pf[i].pageNum <-1))
                        pf[i].dirty_seg=1;
                }
            }
        }
        break;
    } 
    
    return RC_OK;
}

void validatePageHandler(int iPageNum) {
    checkBufferPoolEmpty();
    iPageNum  = 100;
}

extern RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const p)
{ 
    
    if(iCheck == iFlag)
    {
        while(bm != NULL && p != NULL){
        int iPageFrame = -1;
        PageFrame *pf = (PageFrame *)bm->mgmtData;

        validatePageHandler(iPageFrame);
        int cntr =0;

        if(iCheck == iFlag)
        {
            Top: 
            if(pf[cntr].pageNum == p->pageNum)
                pf[cntr].fixCount--;

            checkStorageManagerValid();
            cntr++;
            bufferPoolContents();
            if(!(cntr == buff_size || cntr>buff_size))
            goto Top;

            return RC_OK;
        }
        
        }
    }
    
    return RC_PAGE_NOT_FOUND_ERROR;
}

void bufferPoolContents() {}

extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const p)
{
    int iWriteBlockCheck = 10;
    if(bm == NULL || p == NULL)
        return bm == NULL ? RC_BUFFER_POOL_NOT_FOUND : RC_PAGE_NOT_FOUND_ERROR;
    
    checkStorageManagerValid();
    PageFrame *pf = (PageFrame*)bm->mgmtData; 
    Return_code=RC_OK;

    validatePageHandler(iWriteBlockCheck);
    int cntr=0;
     SM_FileHandle fh;

    setPageFixCount(iCheck);
    do{
        if(pf!= NULL && pf[cntr].pageNum == p->pageNum){
            checkBufferPoolEmpty();
            while(openPageFile((*bm).pageFile, &fh) == RC_OK){
                checkBufferPoolEmpty();
                int pg_num = pf[cntr].pageNum;
                writeBlock(pg_num, &fh, pf[cntr].data); 

                if(iCheck == iFlag)
                {
                    if(pf[cntr].dirty_seg < -1){
                        pf[cntr].dirty_seg= pf[cntr].dirty_seg;

                    }
                    else{
                        pf[cntr].dirty_seg=0;

                    }
                }
                    
                break;
            }
        }
        bufferPoolContents();
        if(iCheck == iFlag)
            cntr++;

    }while(cntr < buff_size);
    return Return_code;
}

double iBuffSize = 0.0, iPoolSize = 1.0;
RC setPageFixCount(int iFrameCount)
{
    validatePageHandler(iFrameCount);
    iFrameCount++;

    return RC_OK;
}

extern void FIFO(BM_BufferPool *const bm, PageFrame *p)
{
    bool bPoolFull = false;
    int pos = l_indx % buff_size;

    bPoolFull = isPageFrameEmpty();
    if (bm != NULL &&  p!=NULL){
        
        PageFrame *pf = (PageFrame *)bm->mgmtData;
        checkBufferPoolEmpty();
        SM_FileHandle fh;
        int cntr=0;
        Top: 
            if(pf[cntr].fixCount != 0){
                if(!bPoolFull)
                    pos = (pos+1)%buff_size == 0 ? 0 : pos+1; 
            }
            else{
                while(pf[cntr].dirty_seg != 1){
                    setPageFixCount(pos);
                    if(openPageFile(bm->pageFile, &fh) == RC_OK)
                    {
                        isPageFrameEmpty();
                        writeBlock(pf[cntr].pageNum,&fh,pf[cntr].data);
                    }
                        
                    if(iCheck == iFlag)
                        count++;
                    break;
                }


                // pf[cntr]=(PageFrame){.fixCount= p->fixCount,.data=p->data,.pageNum=p->pageNum,  
                //     .hit_cnt=p->hit_cnt,.dirty_seg=p->dirty_seg
                //     };
                
                pf[cntr].data = p->data;
                int temp_pgNum = p->pageNum;
                if(iBuffSize != iPoolSize)
                    pf[cntr].pageNum = temp_pgNum;
                int temp_dirtyBit = p->dirty_seg;
                pf[cntr].dirty_seg = temp_dirtyBit;

                checkBufferPoolEmpty();
                int temp_accessCount = p->fixCount;
                pf[cntr].fixCount = temp_accessCount;

                if(iCheck == iFlag)
                    pf[cntr].hit_cnt = p->hit_cnt;
                    
            }
            if(iBuffSize != iPoolSize)
                cntr++;
            while(cntr <buff_size)
                goto Top;

    }
}

void checkBufferPoolEmpty(){} 


bool isPageFrameEmpty()
{
    bool bFrameValidate = false;
    bufferPoolContents();

    return bFrameValidate;
}

extern void LRU(BM_BufferPool *const bm, PageFrame *p)
{ 
    
    int cntr=0,l_hd =0, hd=0;
    bool bRet = false;
    PageFrame *pf = (PageFrame *) (*bm).mgmtData;

    checkStorageManagerValid();
    SM_FileHandle fh;
    do{
        if(iBuffSize != iPoolSize)
            if(pf[cntr].fixCount == 0)
                hd = pf[cntr].hit_cnt;

        
        if(iCheck == iFlag)
            if(pf[cntr].fixCount == 0)
                l_hd = cntr;

        if(iCheck == iFlag)
            cntr++;
        break;
    }while(cntr <buff_size);

    cntr = l_hd +1;
    bRet = isPageFrameEmpty();
    for(;cntr<buff_size;cntr++){

        hd = pf[cntr].hit_cnt< hd ? pf[cntr].hit_cnt : hd;

        if(!bRet)
            l_hd = pf[cntr].hit_cnt< hd ? cntr : l_hd;
        }
        
    if(pf[l_hd].dirty_seg == 1)
        bufferPoolContents();
    else{

        if(true)
        {
             while(openPageFile(bm->mgmtData,&fh) == RC_OK){

                int pg_num = pf[l_hd].pageNum;
                validatePageHandler(pg_num);
                writeBlock(pg_num,&fh,pf[l_hd].data);
                count = (count*2) +1 - count;
                setPageFixCount(pg_num);
                break;
            }    

        }
          
    }
    while(!bRet)
    {
        pf[l_hd].data = p->data;
        int temp_pgNum = p->pageNum;
        if(iBuffSize != iPoolSize)
            pf[l_hd].pageNum = temp_pgNum;
        int temp_dirtyBit = p->dirty_seg;
        pf[l_hd].dirty_seg = temp_dirtyBit;

        checkBufferPoolEmpty();
        int temp_accessCount = p->fixCount;
        pf[l_hd].fixCount = temp_accessCount;

        if(iCheck == iFlag)
            pf[l_hd].hit_cnt = p->hit_cnt;

        break;
    }
}

void checkStorageManagerValid(){}

 // To be done
extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const p, const PageNumber pageNum)
{
    Return_code=RC_OK;
    int iNumFrame = 1;
    int temp=1;
    if(bm==NULL){
        isPageFrameEmpty();
        temp=0;   
    }

    checkStorageManagerValid();
    if(temp==0){
        return RC_BUFFER_POOL_NOT_FOUND;
    }
    else{
            validatePageHandler(iNumFrame); 
            if(p==NULL){
                return RC_PAGE_NOT_FOUND_ERROR;
            }
            else{
                    iNumFrame++;
                    PageFrame *pf = (PageFrame *)(*bm).mgmtData;
                    if(pf[0].pageNum!=-1)
                    {
                        bufferPoolContents();
                        bool flag=true;
                        bool isBufferFull = true;

                        isPageFrameEmpty();
                        int temp=0; 
                        int pg;
                        a:
                        if(!(temp>buff_size || temp==buff_size)){
                            bool bTailCheck = false;
                            if(pf[temp].pageNum==-1){
                                SM_FileHandle file_handle;
                                
                                pg=PAGE_SIZE;
                                if(!bTailCheck)
                                {
                                    if(pg==PAGE_SIZE)
                                        openPageFile((*bm).pageFile,&file_handle);
                                }
                                
                                pf[temp]=(PageFrame){
                                    .data = (SM_PageHandle) malloc(pg)
                                };

                                checkBufferPoolEmpty();
                                pf[temp].pageNum = pageNum;

                                int zri=0;

                                while(!bTailCheck)
                                {
                                    if(pf[temp].pageNum == pageNum)
                                        readBlock(pageNum,&file_handle,pf[temp].data);
                                    break;
                                }
                                
                                int cnt=1; 
                                h_cnt++; 

                                validatePageHandler(iNumFrame);

                                if(cnt==1)
                                    pf[temp].index = zri; 
                                checkStorageManagerValid();
                                pf[temp].fixCount = cnt; 

                                if(pf[temp].fixCount == cnt)
                                    l_indx++; 
                                
                                if(iBuffSize != iPoolSize)
                                {
                                    if(RS_LRU==(*bm).strategy){
                                    if(true)
                                        pf[temp].hit_cnt=h_cnt;
                                    }
                                    bufferPoolContents();
                                }
                                
                                else if(RS_LRU==(*bm).strategy){
                                    if(true)
                                    {
                                        isPageFrameEmpty(iNumFrame);
                                        pf[temp].hit_cnt=1;
                                    }
                                }

                                iNumFrame += 2;

                                isBufferFull=false;
                                    if(!isBufferFull)
                                    {
                                        p->pageNum=pageNum;
                                        iNumFrame = iNumFrame -1 ;
                                    }
                                        
                                    p->data=pf[temp].data;

                                    if(!isBufferFull)
                                    {
                                        checkStorageManagerValid();
                                        flag = false;
                                    }
                                        
                            }
                            else{
                                if(pf[temp].pageNum==pageNum){
                                    if(iBuffSize != iPoolSize)
                                        isBufferFull = false;
                                    if(true)
                                    {
                                        pf[temp].fixCount=pf[temp].fixCount+1;
                                        bufferPoolContents();
                                    }
                                    h_cnt++; 
                                    if((*bm).strategy==RS_CLOCK){
                                        iNumFrame = 0 + iNumFrame;
                                        if(true)
                                            pf[temp].hit_cnt=1;
                                    }
                                    else if((*bm).strategy==RS_LRU){
                                        isPageFrameEmpty();
                                        pf[temp].hit_cnt = h_cnt;
                                    }

                                    flag=false;
                                    if(!isBufferFull)
                                    {
                                        setPageFixCount(iNumFrame);
                                        p->pageNum=pageNum;
                                    }
                                        
                                    if(!isBufferFull)
                                    {
                                        p->data=pf[temp].data;
                                        checkBufferPoolEmpty();
                                    }
                                        
                                    if(!isBufferFull)
                                        clk_pntr=clk_pntr+1;
                                }
                            } 
                            iNumFrame++;     
                        }   
            
            temp++;

            if(temp<buff_size)
            {
                bufferPoolContents();
                if(flag)
                    goto a;
            }
                
            validatePageHandler(iNumFrame);
            if(isBufferFull){
                SM_FileHandle file_handle;

                checkBufferPoolEmpty();
                int temp=1;
                PageFrame *newPage=(PageFrame*)malloc(sizeof(PageFrame));

                while(bCheckIdx)
                {
                    if(temp==1)
                        openPageFile((*bm).pageFile,&file_handle);

                    break;
                }
                
                newPage->data=(SM_PageHandle)malloc(PAGE_SIZE);
                if(temp==1)
                {
                    bufferPoolContents();
                    readBlock(pageNum,&file_handle,(*newPage).data);
                }
                    
                l_indx++;
                iNumFrame = iNumFrame +1;
                if(flag){
                    newPage->dirty_seg=0;
                    if(iBuffSize != iPoolSize)
                        newPage->pageNum=pageNum;
                    newPage->index=0;
                    newPage->fixCount=1; 
                    checkBufferPoolEmpty();
                }
                h_cnt++;
                if(RS_CLOCK==(*bm).strategy){
                    isPageFrameEmpty();
                    (*newPage).hit_cnt=1;
                } 
                else if((*bm).strategy==RS_LRU){
                    (*newPage).hit_cnt=h_cnt;
                }
                if(iCheck == iFlag)
                    iNumFrame++;
                p->data=newPage->data;
                p->pageNum=pageNum;
                setPageFixCount(iNumFrame);
                switch ((*bm).strategy)
                {
                case RS_LRU:
                    checkBufferPoolEmpty();
                    if(true)
                        LRU(bm, newPage);
                    isPageFrameEmpty();
                    break;
                case RS_FIFO:
                    if(true)
                    {
                        FIFO(bm, newPage);
                        iNumFrame--;
                    }
                    break;
                default:
                    checkBufferPoolEmpty();
                    break;  
            } 
        } 
        return Return_code; 
        }
        else{ 
            isPageFrameEmpty();
            SM_FileHandle file_handle;

            if(true)
            {
                openPageFile(bm->pageFile,&file_handle);
            }
            int pp=PAGE_SIZE;

            while(iBuffSize != iPoolSize)
            {
                if(pp == PAGE_SIZE)
                pf[0]=(PageFrame){.data=(SM_PageHandle) malloc(pp)};
                break;
            }
            

            ensureCapacity(pageNum,&file_handle);
            if(true)
            {
                if(iCheck == iFlag)
                    readBlock(pageNum,&file_handle,pf[0].data); 
            }

            setPageFixCount(iPoolSize);
            pf[0].fixCount=pf[0].fixCount+1+0;
            checkStorageManagerValid();
            pf[0].pageNum=pageNum; 

            if(true)
                l_indx=0;
            
            iNumFrame ++;
            if(h_cnt==0)
                h_cnt=0; 
            
            isPageFrameEmpty();
            pf[0].index=0;
            if(pf[0].index == 0)
                if(iCheck == iFlag)
                    pf[0].hit_cnt=h_cnt; 
            p->data=pf[0].data; 

            bufferPoolContents();
            if(p->data==pf[0].data)
                p->pageNum=pageNum; 

            validatePageHandler(iNumFrame);
        return Return_code; 
        } 
    }
    if(iCheck)
    {
        setPageFixCount(iNumFrame);
    }
  } 
}

extern PageNumber *getFrameContents (BM_BufferPool *const bm)
{
    PageNumber *container;
    PageFrame *pf=(PageFrame *)bm-> mgmtData;
    if(iBuffSize != iPoolSize)
        container =malloc(sizeof(PageNumber)*buff_size);

    while(bCheckIdx)
    {
        for(int i=0;i<buff_size;i++){
            checkStorageManagerValid();
            if(pf[i].pageNum < -1 )
                container[i] = pf[i].pageNum;
            else
            {
                if(pf[i].pageNum > -1)
                    container[i] = pf[i].pageNum;
                else
                    container[i] = NO_PAGE;
            }
            //container[i] = pf[i].pageNum < -1 ? pf[i].pageNum :  (pf[i].pageNum > -1 ? pf[i].pageNum : NO_PAGE);
        }
        break;
    }
    
    return container;
}

extern bool *getDirtyFlags (BM_BufferPool *const bm)
{
    PageFrame *pf = (PageFrame *)bm->mgmtData;
    while(buff_size > 0){
        bufferPoolContents();
        int cntr=0;
        bool *dirty_Flg =  malloc(sizeof(bool) * buff_size);

        validatePageHandler(buff_size);
        Top: 
            dirty_Flg[cntr] = pf[cntr].dirty_seg== 1 ? TRUE : FALSE;

            if(iCheck == iFlag)
            {
                if(cntr < buff_size){
                    cntr ++;
                    isPageFrameEmpty();
                    goto Top;
                } 
                else
                {
                    bufferPoolContents();
                    return dirty_Flg;
                }        
            } 

        checkStorageManagerValid();
    }
 return NULL;
}

extern int *getFixCounts (BM_BufferPool *const bm)
{
    int *fix_cnt = malloc(sizeof(int) * buff_size);
    checkBufferPoolEmpty();
    PageFrame *pf= (PageFrame *)bm-> mgmtData;

    bool bFixCountValue = true;
    while(buff_size >0){
        int cntr =0;

        bFixCountValue = !bFixCountValue;
        Top :
            if(pf[cntr].fixCount != -1){
                fix_cnt[cntr] =pf[cntr].fixCount ;

            } else{ 
                fix_cnt[cntr] = 0 ;
            }
            if(cntr< buff_size){
                bFixCountValue = isPageFrameEmpty();
                cntr++;
                goto Top;
            }
            else
            {
                validatePageHandler(iBuffSize);
                return fix_cnt;
            }    
    }
 return NULL;

}

extern int getNumReadIO (BM_BufferPool *const bm)
{
    if(bm == NULL)
        return RC_BUFFER_POOL_NOT_FOUND;
    
    double dReadCount = 1.0;
    int num = l_indx*2;
    l_indx++;

    setPageFixCount((int) dReadCount);
    
    if(num > 0)
        return (num/2) +1;
    else 
    {  
        bufferPoolContents();
        return l_indx+1;
    }
}

extern int getNumWriteIO (BM_BufferPool *const bm)
{
    if(iBuffSize != iPoolSize)
        return (count*2) == (count +count) ? count : 0; 

    return RC_OK;
}
