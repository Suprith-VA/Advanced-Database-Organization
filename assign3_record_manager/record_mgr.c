#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <ctype.h>

typedef int *dtINTPtr;
typedef char *dtCHARPtr;

RecMgr *objRecMgr;
RC iReturnCode;
 
size_t intTypeSize = sizeof(int);
size_t szSchema = sizeof(Schema);
int ATTRIBUTE_SZ = 15;

void call_memcopy(void *dst, const void *src, int datatype);
RC funcPinOrUnpinPage(RecMgr *objRecMgr, bool unpin, int iPgNum);
RC funTwoObjPinUnpin(BM_BufferPool* buffPool, BM_PageHandle* pgHandle, bool unpin, int pgNumber);

int findFreeSlot(dtCHARPtr chData, int iRecSize)
{
	bool bFlag = FALSE;
	int iIdx_Cntr = 0;
	int iSlot = -1;

	// To determine the total number of records that can be fit in
	int iDiv = PAGE_SIZE / iRecSize;
	while (iIdx_Cntr < iDiv && iRecSize > 0)
	{
		int iIndex = iIdx_Cntr * iRecSize;

		// check if the slot has already been filled
		if (chData[iIndex] == '+')
		{  
			bFlag = FALSE;
			iSlot = -1;
		}
		else
		{
			bFlag = TRUE;
			iSlot = iIdx_Cntr;
		}

		if (iSlot > -1 && bFlag)
			return iSlot;

		// increment to iterate
		iIdx_Cntr++;
	}

	return iSlot;
}

extern RC initRecordManager(void *mgmtData)
{
	// initialise the storage manager 
	initStorageManager();
	return RC_OK;
}

extern RC shutdownRecordManager()
{
	// shutting the buffer pool down 
	shutdownBufferPool(&objRecMgr->buff_pool);
	objRecMgr = NULL;

	// free the mem of object
	free(objRecMgr);

	iReturnCode = RC_OK;
	return iReturnCode;
}

// Var name here and there touched. Need to come back here
extern RC createTable(dtCHARPtr chName, Schema *objSchema)
{
	// declaring the record manager data using array
	char recData[PAGE_SIZE];
	int check = -1;

	// setting the meemory for the record manager
	int iOne = 1;
	int iZero = 0;
	int iRecSize = sizeof(RecMgr);
	objRecMgr = (RecMgr *)malloc(iRecSize);

	dtCHARPtr pgDataSpace = recData;
	SM_FileHandle fileHandle;

	// intialising the buffer pool
	initBufferPool(&objRecMgr->buff_pool, chName, 100, RS_LRU, NULL);

	// set the tuple count to zero at the beginning of the page
	*(dtINTPtr)pgDataSpace = iZero;
	pgDataSpace = pgDataSpace + intTypeSize;

	// marking the next byte wuith one
	*(dtINTPtr)pgDataSpace = iOne;
	pgDataSpace = pgDataSpace + intTypeSize;

	// setting the number of attributes
	*(dtINTPtr)pgDataSpace = objSchema->numAttr;
	pgDataSpace = pgDataSpace + intTypeSize;

	// setting the key size
	*(dtINTPtr)pgDataSpace = objSchema->keySize;
	pgDataSpace = pgDataSpace + intTypeSize;

	for (int iIt_idx = 0; iIt_idx < objSchema->numAttr; iIt_idx++)
	{
		// copy the name of the attributes
		strncpy(pgDataSpace, objSchema->attrNames[iIt_idx], 15);
		pgDataSpace = pgDataSpace + 15;

		// copying the datatype
		*(dtINTPtr)pgDataSpace = (int)objSchema->dataTypes[iIt_idx];

		// size of the data type 
		pgDataSpace = pgDataSpace + intTypeSize;
		*(dtINTPtr)pgDataSpace = (int)objSchema->typeLength[iIt_idx];

		pgDataSpace = pgDataSpace + intTypeSize;
	}

	// creating the new page and open
	if (createPageFile(chName) == RC_OK)
	{
		if(check != 0)
		{
			// opening the page
			openPageFile(chName, &fileHandle);
		}
	}
		

	// write thhe page 
	if (writeBlock(0, &fileHandle, recData) == RC_OK)
		if(check != 0)
		{
			//closing the page 
			closePageFile(&fileHandle);
		}
	

	return RC_OK;
}

//Opening the table 
extern RC openTable(RM_TableData *rel, dtCHARPtr name)
{
	
	SM_PageHandle smPgHandle;
	int iAttributeCount = 0;
	int iCheck;
	int iDTSize = sizeof(DataType);
	//setting the metadata with the object manager
	rel->mgmtData = objRecMgr;

	//pinning the page in the buffer pool
	if (intTypeSize != -1)
	{
		pinPage(&objRecMgr->buff_pool, &objRecMgr->pg_hndl, 0);
		//setting the page handle with the recird managers page handle data
		smPgHandle = (dtCHARPtr)objRecMgr->pg_hndl.data;
		// setting the tuple count
		objRecMgr->t_count = *(dtINTPtr)smPgHandle;
		smPgHandle = smPgHandle + intTypeSize;


		objRecMgr->freePg = *(dtINTPtr)smPgHandle;
		smPgHandle = smPgHandle + intTypeSize;
	}
	//setting the attribute count
	iAttributeCount = *(dtINTPtr)smPgHandle;
	smPgHandle = smPgHandle + intTypeSize;
	//allocating the memory to schema and allocating the values to it
	Schema *objSchema = (Schema *)malloc(szSchema);
	if (szSchema != 0)
	{
		objSchema->dataTypes = (DataType *)malloc(iDTSize * iAttributeCount);
		objSchema->attrNames = (dtCHARPtr *)malloc(sizeof(dtCHARPtr) * iAttributeCount);
		objSchema->numAttr = iAttributeCount;
	}

	int iItr_idx = 0;
	objSchema->typeLength = (dtINTPtr)malloc(intTypeSize * iAttributeCount);
	//iterating through each of the attribute and allocating memory for each of them
	while (iItr_idx < iAttributeCount)
	{
		objSchema->attrNames[iItr_idx] = (dtCHARPtr)malloc(ATTRIBUTE_SZ);
		iItr_idx++;
	}
	//iterating through each of the attribute and copying its data type and size of the data type
	iItr_idx = 0;
	do
	{
		if(objSchema->attrNames != NULL)
		{
			//copying the name of the attribute
			strncpy(objSchema->attrNames[iItr_idx], smPgHandle, ATTRIBUTE_SZ);

			iCheck = ATTRIBUTE_SZ;
			if(iCheck == 15)
				smPgHandle = smPgHandle + iCheck;
		}
		
		//checking if the end of the attribute has been reached
		if (iItr_idx < objSchema->numAttr)
		{	
			//copying the attribute data type
			if(true) objSchema->dataTypes[iItr_idx] = *(dtINTPtr)smPgHandle;
			smPgHandle = intTypeSize + smPgHandle;
			//copying the size of the data type
			objSchema->typeLength[iItr_idx] = *(dtINTPtr)smPgHandle;
		}
		else
			break;

		smPgHandle = intTypeSize + smPgHandle;
		iItr_idx++;
	} while (iItr_idx < objSchema->numAttr);

	rel->schema = objSchema;

	// unpin the page
	RC iRet = funcPinOrUnpinPage(objRecMgr, true, 0);

	// if pages have changes we write it back to disk before shutdown
	if (iRet == RC_OK)
		forcePage(&objRecMgr->buff_pool, &objRecMgr->pg_hndl);

	return RC_OK;
}

//Closing the table by taking in the relation of the table as the input parameter
extern RC closeTable(RM_TableData *rel)
{
	//picking up the meta data related to the record manager
	RecMgr *objRecMgr = rel->mgmtData;

	// shutdown the buffer pool
	if (!rel->mgmtData)
	// shuting down the bufferpool once the operation has been performed
		shutdownBufferPool(&objRecMgr->buff_pool);

	return RC_OK;
}

// deleting a table by taking the table name as the input parameter
extern RC deleteTable(dtCHARPtr iTableName) { return destroyPageFile(iTableName); }

//Getting the tuples by taking in the relation of the data 
extern int getNumTuples(RM_TableData *rel)
{
	//picking up the meta data related to the record manager
	RecMgr *objRecMgr = rel->mgmtData;

	//tuple count
	int iCount = (*objRecMgr).t_count;
	iCount = iCount > 0 ? iCount : 0;
	//returning the number of tuples 
	return iCount;
}

//Inserting the record into the table by taking in the relation and the records
extern RC insertRecord(RM_TableData *iRelation, Record *iRecords)
{
	//picking up the meta data related to the record manager
	RecMgr *objRecMgr = iRelation->mgmtData;
	bool bFlag = true;
	int rSize;

	//Picking up the record 
	RID *ridRecord = &iRecords->id;

	//declaring the char data pointer to handle the old page handle data and new handle data
	dtCHARPtr chPageHandleData;
	dtCHARPtr chNewPageHandleData;
	
	int iRecordSize = 0;


	if (iRecordSize == 0)
	{
		//getting the record size
		iRecordSize = getRecordSize(iRelation->schema);
		ridRecord->page = objRecMgr->freePg;
		//Pining the page associated with the record manager
		funcPinOrUnpinPage(objRecMgr, false, ridRecord->page);
	}

	//getting the data of the page
	chPageHandleData = objRecMgr->pg_hndl.data;
	
	//finding the free slot in the page to fit in the newly created record
	ridRecord->slot = findFreeSlot(chPageHandleData, iRecordSize);

	//If the free slot is not available the following condition will be executed
	if (ridRecord->slot < 0)
	{	
		//Looping through the entire page until the free slot has been found
		while ((ridRecord->slot == -1))
		{
			//Unpinning the current page where the free slot hasn't been found
			funcPinOrUnpinPage(objRecMgr, true, 0);
			//Incrementing the page number to next page to find a free slot
			int iPageNumber = ridRecord->page + 1;
			ridRecord->page = iPageNumber;
			//Pin the associated page into the record manager
			funcPinOrUnpinPage(objRecMgr, false, iPageNumber);
			chPageHandleData = objRecMgr->pg_hndl.data;

			rSize = iRecordSize;
			ridRecord->slot = findFreeSlot(chPageHandleData, rSize);
		}
	}

	//Once the free slot has been found and the data has been entered we mark the record as dirty bits
	if (chPageHandleData != NULL)
	{
		chNewPageHandleData = chPageHandleData;
		if (bFlag == true)	markDirty(&objRecMgr->buff_pool, &objRecMgr->pg_hndl);
	}

	//Pointer cursor has been changed to the right slot position 
	chNewPageHandleData = chNewPageHandleData + (ridRecord->slot * iRecordSize);
	//Indicating that the slot has been occupied 
	*chNewPageHandleData = '+';
	//Copying the data into the slot
	memcpy(++chNewPageHandleData, (*iRecords).data + 1, iRecordSize - 1);

	if (chNewPageHandleData != chPageHandleData)
	{
		//Unpinning the page after the process has been completed 
		funcPinOrUnpinPage(objRecMgr, true, 0);
		objRecMgr->t_count++;
	}
	
	//Pinning the first page of the table
	funcPinOrUnpinPage(objRecMgr, false, 0);

	return RC_OK;
}


RC funcPinOrUnpinPage(RecMgr *objRecMgr, bool unpin, int iPgNum)
{
	int iRet;
	//If the bool value is true we will be unpinning the page
	if (unpin)
		iRet = unpinPage(&objRecMgr->buff_pool, &objRecMgr->pg_hndl);
	else
	//If the bool value is false we will be pinning the page
		iRet = pinPage(&objRecMgr->buff_pool, &objRecMgr->pg_hndl, iPgNum);

	return iRet;
}

//Deleting the record from the table 
extern RC deleteRecord(RM_TableData *iRelation, RID iRelationID)
{
	//picking up the meta data related to the record manager
	RecMgr *objRecMgr = iRelation->mgmtData;
	dtCHARPtr iRecordData;
	int iRecordPageID = iRelationID.page;

	//Pinning the page associated with that record  
	funcPinOrUnpinPage(objRecMgr, false, iRecordPageID);

	//Setting the pageid
	objRecMgr->freePg = iRecordPageID;

	//retreving the current data from the page
	objRecMgr->pg_hndl.data != NULL ? iRecordData = objRecMgr->pg_hndl.data : NULL;

	//Retreving the record size
	int iSzRecord = getRecordSize(iRelation->schema);
	char dummyChar = '-';

	//Making sure the it goes to right record position
	for (int iItr_cntr = 0; iItr_cntr < iSzRecord; iItr_cntr++)
	{
		int iMul = (iRelationID.slot * iSzRecord);
		int iAdd = iMul + iItr_cntr;
		iRecordData[iAdd] = dummyChar;
	}

	//marking the page as dirty so that the changes reflects in the disk
	if ((iRecordData != NULL))
	{
		bool bFlag = true;
		if ( markDirty(&objRecMgr->buff_pool, &objRecMgr->pg_hndl) == RC_OK && bFlag)
			forcePage(&objRecMgr->buff_pool, &objRecMgr->pg_hndl);
	}

	//Deleting the record from the table
	return RC_OK;
}


// function used to update the record 
extern RC updateRecord(RM_TableData *iRelation, Record *iRecords)
{
	dtCHARPtr iRecordData;
	int iRelationSize = 0;
	bool bFlag = true;
	char dummyPlusVar = '+';
	bool bRecordSz = true;

	if (iRelation->mgmtData != NULL)
	{   //take the meta data
		RecMgr *objRecMgr = iRelation->mgmtData;
		//pin the page to the buffer manager to use the page where record is used 
		funcPinOrUnpinPage(objRecMgr, false, iRecords->id.page);
	}
	else
	{
		return RC_ERROR;
	}
    //get the size of the record 
	while (bRecordSz)
	{
		//get the record size of the record
		int iRet = getRecordSize(iRelation->schema);
		if (iRet > 0)
		{
			iRelationSize = getRecordSize(iRelation->schema);
		}
		break;
	}

    //iterate through the data so that the pointer points the data that has to be changed
	for (int iItr = 0; iRecordData != NULL && iItr < 1; iItr++)
	{
		RID iRecordID = iRecords->id;
		iRecordData = objRecMgr->pg_hndl.data;
		iRecordData = iRecordData + (iRecordID.slot * iRelationSize);
		*iRecordData = dummyPlusVar;
	}

	++iRecordData;
	//copy the data to the record that has to be updated
	memcpy(iRecordData, iRecords->data + 1, iRelationSize - 1);

	int iUpdatedRecord;
	if(bFlag == true)
	{
		//once the record is update mark the page as dirty page
		iUpdatedRecord = markDirty(&objRecMgr->buff_pool, &objRecMgr->pg_hndl);
	}
    
	//once the record id updated unpin the page 
	int pageNum = 0;
	if (iUpdatedRecord == RC_OK)
		funcPinOrUnpinPage(objRecMgr, true, pageNum);

	return iUpdatedRecord;
}

// to get the record requested by the user
extern RC getRecord(RM_TableData *iRelation, RID iRecordiD, Record *iRecords)
{
	//meta data 
	RecMgr *objRecMgr = iRelation->mgmtData;
	//bring the oage that has the paticular record to the buffer
	funcPinOrUnpinPage(objRecMgr, false, iRecordiD.page);
	//get the recordsize of the record
	int iRecordSize = getRecordSize(iRelation->schema);
	
	dtCHARPtr dataString = objRecMgr->pg_hndl.data;
	//move the pointer to the slot where the record is present
	int iRecSlotSize = iRecordSize * iRecordiD.slot;
     
	dataString = dataString + iRecSlotSize;
	char chData = *dataString;
	//check if the data is already inserted in the table
	if(chData == '+')
	{
		iRecords->id = iRecordiD;
		dtCHARPtr data = iRecords->data;
		memcpy(++data, dataString + 1, iRecordSize - 1);
		funcPinOrUnpinPage(objRecMgr, true, 0);
		return RC_OK;
	}

	return RC_ERROR;
}

//Start the scan 
extern RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
	if (cond == NULL){ return RC_SCAN_CONDITION_NOT_FOUND; }
	else
	{
		int iZero = 0;
		int iOne = 1;
		RecMgr *objScanMgr;
		RecMgr *objTblMgr;
		//open the table that has to be scanned
		RC iRet = openTable(rel, "ScanTable");
		//check if tablw has been opened sucessfully
		if (iRet ==RC_OK)
		{
			//get the size of the record manager
			int iRecMgrSize = sizeof(RecMgr);
			if (iRecMgrSize > 0)
			{
				//allocate memory for object scan manager
				objScanMgr = (RecMgr *)malloc(iRecMgrSize);
				
				//set the condition associated with the scan manager
				objScanMgr->cond = cond;
				//initliase the slot of scan manager to zero
				objScanMgr->rec_ID.slot = iZero;

				scan->mgmtData = objScanMgr;
			}
            //initliase record id to value 1
			objScanMgr->rec_ID.page = iOne;

			//initliase the scan count to value 0
			objScanMgr->scn_count = iZero;
			for (int rcdMgr = 0; objScanMgr != NULL && rcdMgr != 1; rcdMgr++)
			{
				objTblMgr = rel->mgmtData;
				objTblMgr->t_count = ATTRIBUTE_SZ;
			}

			scan->rel = rel;
		}
		return RC_OK;
	}

	
}

extern RC next(RM_ScanHandle *scan, Record *record)
{
	iReturnCode = RC_OK;
	int iIncrement = 1;
	char chHiphen = '-';

	if(NULL == scan->rel->mgmtData)
	{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}
	else
	{
		RecMgr *objScanMgr = (*scan).mgmtData;

		if(objScanMgr->cond == NULL)
			return RC_SCAN_CONDITION_NOT_FOUND;
			
		if (objScanMgr->cond != NULL)
		{
			RecMgr *objTblMgr = (*scan).rel->mgmtData;
			dtCHARPtr chPtrData;
			int iValSize = sizeof(Value);

			if (objTblMgr != NULL)
			{
				Schema *objSchema = scan->rel->schema;

				if (objScanMgr->cond == NULL)
					return RC_SCAN_CONDITION_NOT_FOUND;

				Value *result = (Value *)malloc(iValSize);
				int iRecSz = getRecordSize(objSchema);

				int iTotalSlots = PAGE_SIZE / iRecSz;
				int iSchemaCount = 0;

				iSchemaCount = objScanMgr->scn_count;
				int iTabPtrCount = objTblMgr->t_count;

				if (iTabPtrCount == 0)
					return RC_RM_NO_MORE_TUPLES;
				
				while (iSchemaCount <= iTabPtrCount)
				{
					if ( 0 > iSchemaCount)
					{
						objScanMgr->rec_ID.slot = 0;
						objScanMgr->rec_ID.page = 1;
					}
					else
					{
						objScanMgr->rec_ID.slot = objScanMgr->rec_ID.slot+iIncrement;
						if (objScanMgr->rec_ID.slot >= iTotalSlots)
						{
							if (iSchemaCount > 0)
								objScanMgr->rec_ID.page = objScanMgr->rec_ID.page+iIncrement;
							objScanMgr->rec_ID.slot = 0;
						}
					}

					funTwoObjPinUnpin(&objTblMgr->buff_pool, &objScanMgr->pg_hndl, false, objScanMgr->rec_ID.page);
					// pinPage(&objTblMgr->buff_pool, &objScanMgr->pg_hndl, objScanMgr->rec_ID.page);
					chPtrData = objScanMgr->pg_hndl.data;
						
					iRecSz = getRecordSize(objSchema);
					int iSlot = objScanMgr->rec_ID.slot;

					chPtrData = chPtrData + (iSlot * iRecSz);
					record->id.page = objScanMgr->rec_ID.page;
					record->id.slot = objScanMgr->rec_ID.slot;

					dtCHARPtr chDtPntr = record->data;
					*chDtPntr = chHiphen;

					iRecSz = getRecordSize(objSchema);
					memcpy(++chDtPntr, chPtrData + 1, iRecSz - 1);

					iSchemaCount = iSchemaCount + iIncrement;
					objScanMgr->scn_count = objScanMgr->scn_count + iIncrement;

					evalExpr(record, objSchema, objScanMgr->cond, &result);

					bool bIsBool = true;
					if (bIsBool == result->v.boolV){
						funTwoObjPinUnpin(&objTblMgr->buff_pool, &objScanMgr->pg_hndl, true, 0);
						return RC_OK;
					}
				}

				funTwoObjPinUnpin(&objTblMgr->buff_pool, &objScanMgr->pg_hndl, false, 0);
				
				objScanMgr->rec_ID.slot = 0;
				objScanMgr->scn_count = 0;
				objScanMgr->rec_ID.page = 1;
				
				return RC_RM_NO_MORE_TUPLES;
			}
		}
	}

	return RC_RM_NO_MORE_TUPLES;
}


RC funTwoObjPinUnpin(BM_BufferPool* buffPool, BM_PageHandle* pgHandle, bool unpin, int pgNumber)
{
	RC iRet;

	// check which function to call
	if(! unpin)
		iRet = pinPage(buffPool, pgHandle, pgNumber); // call pin page on the buffer pool
	else
		iRet = unpinPage(buffPool, pgHandle); // call unpin page on the buffer pool

	return iRet;
}

//function used to set the value to inital value that is used when the next scan is initiated
extern RC closeScan(RM_ScanHandle *scan)
{
	RecMgr *objScanMgr;
	//check if the meta data is not equal to null
	if (scan->mgmtData!= NULL )
		objScanMgr = scan->mgmtData;
	else
	 objScanMgr =  NULL;
	
	RecMgr *objRecMgr = (*scan).rel->mgmtData;
	do{ //check if the scanning is over
		if(funTwoObjPinUnpin(&objRecMgr->buff_pool, &objScanMgr->pg_hndl, true, 0) == RC_OK)
		{   //set all the value to inital value in the record manager
			objScanMgr -> scn_count = 0;
			objScanMgr -> rec_ID.page = 1;
			objScanMgr -> rec_ID.slot = 0;
		}
		scan -> mgmtData = NULL;
		free(scan -> mgmtData);
		break;
	}while((!(objScanMgr->scn_count < 0)));

	return RC_OK;
}

//return the record size of the record
extern int getRecordSize (Schema *schema)
{
	int iIdx_Cntr = 0;
	int iRecordSize = 0;

	while(iIdx_Cntr < schema -> numAttr)
	{   //check if the datatype is String 
		if (schema ->dataTypes[iIdx_Cntr] == DT_STRING){
			        //size of the string data is the size allocated when creating hence use typelength
					iRecordSize = iRecordSize + schema ->typeLength[iIdx_Cntr];
		}
		//check if the datatype is int
		else if(schema ->dataTypes[iIdx_Cntr] == DT_INT){
			        //add the size of integer to recordsize
					iRecordSize = iRecordSize + intTypeSize;
		}
		//check if the datatype is float
		else if(schema ->dataTypes[iIdx_Cntr] == DT_FLOAT){
			         //add the size of float to recordsize
					iRecordSize = iRecordSize + sizeof(float);
		}
		//check if the datatype is boolean
		else if(schema ->dataTypes[iIdx_Cntr] == DT_BOOL){
			        //add the size of boolean to recordsize
					iRecordSize = iRecordSize + sizeof(bool);
		}
		iIdx_Cntr++;
	}
	iRecordSize = iRecordSize + 1;

	return iRecordSize;
}
//fucntion to create the schema 
extern Schema *createSchema(int numAttr, dtCHARPtr *attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	Schema *objSchema;

	if (szSchema > 0)
	{
		objSchema = (Schema *)malloc(szSchema);
        //assign the attribute datatype if its datatype is not null
		
		//assign the attribute typelength if type length is not null
		if(typeLength != NULL) objSchema->typeLength = typeLength;
		else objSchema->typeLength = 0;

		//assign the number of attributes present in the schema
		objSchema->numAttr = numAttr;

		//assign the attribute names if its not null
		if(attrNames != NULL) objSchema->attrNames =attrNames;
		else objSchema->attrNames = NULL;

		//assign the keyattribute in the keys
		objSchema->keyAttrs = keys;
		
		if(dataTypes != NULL) objSchema->dataTypes = dataTypes;
		else objSchema->dataTypes = NULL;
		//assign the keyssize of the attribute
		objSchema->keySize = keySize;
	}

	return objSchema;
}

extern RC freeSchema(Schema *schema)
{
	if(schema != NULL)
	{
		free(schema);
	}
		
	return RC_OK;
}

extern RC createRecord (Record **record, Schema *schema)
{
	char chHyphen;
	Record * objRecord;
	char chNull;
	do{
		objRecord = (Record*) calloc(1,sizeof(Record));
		objRecord -> data= (dtCHARPtr) calloc(1,getRecordSize(schema));
		objRecord -> id.page = objRecord -> id.slot = -1;
        //check if the record is not NULL
		if(!(objRecord == NULL)){
			chHyphen ='-';
			chNull = '\0';
			dtCHARPtr objNObj = objRecord -> data;
			dtCHARPtr objPtrData = objNObj;
			// assign "-" to indicate its a new record and has not yet been inserted to the table
			*objPtrData = chHyphen;
			 //assign the last byte with value "\0" to indicate end of string 
			*(++objPtrData) = chNull;
		}
		*record = objRecord;
		return RC_OK;

	}while(sizeof(Record)>0);

	return RC_ERROR;
}
//set the pointer to particular attribute position passed as parameter
RC attrOffset (Schema *schema, int attrNum, int *result)
{
	*result = 1;
	for (int iCounter=0;iCounter<attrNum;iCounter++){
        //check if the attribute is string
		if (schema -> dataTypes[iCounter] == DT_STRING){
			//add the size allocated to the string attribute
			int schema_length=schema->typeLength[iCounter];
	        *result=*result + schema_length;
		}
		else{
		//add the size based on the datatype of the attribute
		*result = (schema -> dataTypes[iCounter] == DT_INT ? *result + intTypeSize :
											(schema -> dataTypes[iCounter] == DT_FLOAT ? *result + sizeof(float) : *result + sizeof(bool)));
		}
	}
	return RC_OK;

}
extern RC freeRecord (Record *record)
{
	if(record != NULL){
		free(record);	
	}
	
	return RC_OK;
}

extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
	int offset = 0;

	if(record == NULL)
		return RC_ERROR;

	attrOffset(schema, attrNum, &offset);

	iReturnCode = RC_OK;
	dtCHARPtr dataPointer = (*record).data;
	

	Value *attribute = (Value*) malloc(sizeof(Value));
	dataPointer += offset;
	//check if the value of the attribute to be returned is the first attribute
	 if (attrNum == 1){
		schema ->dataTypes[attrNum] =1;

	} else{
		schema ->dataTypes[attrNum]=schema ->dataTypes[attrNum];

	}  
    //check the datatype of the attribute if its an integer
	if(schema ->dataTypes[attrNum]==DT_INT){
					//copy value from the datapointer to a temp variable
					memcpy(&(*attribute).v.intV, dataPointer, intTypeSize);
					//set the datatype to the attribute
					(*attribute).dt=DT_INT;
					
				
	}
	//check the datatype of the attribute if its an string
	if(schema ->dataTypes[attrNum]==DT_STRING) {
					int length = schema ->typeLength[attrNum]; 
					(*attribute).v.stringV = (dtCHARPtr) malloc(length + 1);

					char* chPtr = (*attribute).v.stringV;
					//copy the string value to the attribue
					strncpy(chPtr, dataPointer, length);

					DataType dataType = DT_STRING;
					//set the dtype of the attribute
					(*attribute).dt = dataType;

					char chNULL = '\0';
					//assign null value to the last byte of the string
					(*attribute).v.stringV[length] = chNULL;
	}
	
	if (schema ->dataTypes[attrNum]==DT_BOOL){
		if (true){
			        //copy the boolean value to the attribue
					call_memcopy(&(*attribute).v.boolV,dataPointer,3);
					//set the dtype of the attribute
					(*attribute).dt=DT_BOOL;
		}
					
	}
	if (schema ->dataTypes[attrNum]==DT_FLOAT){
		            //copy the float value to the attribue
					call_memcopy(&(*attribute).v.floatV, dataPointer,2);
					//set the dtype of the attribute
					(*attribute).dt=DT_FLOAT;
			
	}
			
	*value=attribute;
	*value = *value;

	return iReturnCode;
}

extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{  
	dtCHARPtr charDataPointer;
	int iAttributeOffset = 0;
	iReturnCode = RC_OK;
	int iFloatSizeType=sizeof(float),iBoolSizeType= sizeof(bool);

    attrOffset(schema, attrNum, &iAttributeOffset);
	do{

		charDataPointer = record-> data;
		charDataPointer += iAttributeOffset;

        //check if the datatype of the attribute is string
		if((schema -> dataTypes[attrNum])==DT_STRING){

            //assign the value  to the attribute
			strncpy(charDataPointer, (*value).v.stringV, schema -> typeLength[attrNum]);
			//increamenting the position of data pointer
			charDataPointer+= schema -> typeLength[attrNum];
		}
        
		//check if the datatype of the attribute is int
		if((schema -> dataTypes[attrNum])==DT_INT){
        
			*(int *) charDataPointer = value ->v.intV;
			charDataPointer  += intTypeSize;
		}

        //check if the datatype of the attribute is float
		if((schema -> dataTypes[attrNum])==DT_FLOAT){

			*(float *) charDataPointer = value ->v.floatV;	  
			charDataPointer  += iFloatSizeType;
		}

        //check if the datatype of the attribute is boolean
		if((schema -> dataTypes[attrNum])==DT_BOOL){

			*(bool *) charDataPointer = value ->v.boolV;	  
			charDataPointer  += iBoolSizeType;
		}

		break;
	}while(true);

	return iReturnCode;
}

void call_memcopy(void *dst, const void *src, int datatype){
	     /*
	 1=intdatatype
	 2=floatdatatype
     3=booldatatype
	 */
	//check the dataype is int
	if (datatype==1){
		memcpy(&dst, src,intTypeSize);
	}
	//check if the datatype is float
	if(datatype==2){
		memcpy(&dst, src,sizeof(float));
	}
	//check if the datatype is boolean 
	if (datatype==3){
		memcpy(&dst, src,sizeof(bool));
	}
}