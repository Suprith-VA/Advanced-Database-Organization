#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <ctype.h>

typedef struct RecordManager
{
	BM_PageHandle pgHandle;	
	BM_BufferPool buffPool;	
	RID recID;
	Expr *cond;
	int tupCount;
	int freePg;
	int scnCount;
} RManager;


const int MAX_NUMBER_OF_PAGES = 100;
const int ATR_SIZE = 15; 
RManager *rMgr;
RC Rcode;


int findFreeSlot(char *data, int recordSize)
{
	int i = 0,totalSlots = PAGE_SIZE / recordSize; 
	while (i < totalSlots){
		if (data[i * recordSize] != '+')
	    return i;
		i=i+1;
	}
	return -1;
}


extern RC initRecordManager (void *mgmtData)
{
	Rcode = RC_OK;
	initStorageManager();
	return Rcode;
}


extern RC shutdownRecordManager ()
{
	Rcode = RC_OK;
	shutdownBufferPool(&rMgr->buffPool);
	rMgr = NULL;
	free(rMgr);
	return Rcode;
}

extern RC createTable (char *name, Schema *schema)
{
	Rcode = RC_OK;
	rMgr = (RManager*) malloc(sizeof(RManager));
	initBufferPool(&rMgr->buffPool, name, 100, RS_LRU, NULL);
	char data[PAGE_SIZE];
	char *pgHandle = data;
	int result;

	int buf=sizeof(int);
	*(int*)pgHandle = 0; 
	pgHandle = pgHandle + buf;
	*(int*)pgHandle = 1;
	pgHandle = pgHandle + buf;
	*(int*)pgHandle = (*schema).numAttr;
	pgHandle = pgHandle + buf; 
	*(int*)pgHandle = (*schema).keySize;
	pgHandle = pgHandle + buf;

	int k = 0;
	while (k < (*schema).numAttr){
		strncpy(pgHandle, (*schema).attrNames[k], ATR_SIZE);
		pgHandle = pgHandle + ATR_SIZE;
		*(int*)pgHandle = (int)(*schema).dataTypes[k];
		pgHandle = pgHandle + sizeof(int);
		*(int*)pgHandle = (int) (*schema).typeLength[k];
		pgHandle = pgHandle + sizeof(int);
		k=k+1;
	}
	SM_FileHandle fh;



	if((result = createPageFile(name)) == Rcode){
	}
	else{
		return result;
	}
	if((result = openPageFile(name, &fh)) == Rcode)
	{
	}
	else{
		return result;
	}
	if((result = writeBlock(0, &fh, data)) == Rcode)
	{}
	else{
		return result;
	}
	if((result = closePageFile(&fh)) == Rcode)
	{
	}
	else{
		return result;
	}
	return Rcode;
}

extern RC openTable (RM_TableData *rel, char *name)
{
	Rcode = RC_OK;
	SM_PageHandle pgHandle;    
	int attributeCount, l;
	(*rel).mgmtData = rMgr;
	(*rel).name = name;
	int buf=sizeof(int);
	pinPage(&rMgr->buffPool, &rMgr->pgHandle, 0);
	pgHandle = (char*)(*rMgr).pgHandle.data;
	(*rMgr).tupCount= *(int*)pgHandle;
	pgHandle = pgHandle + buf;
	(*rMgr).freePg= *(int*) pgHandle;
    pgHandle = pgHandle + buf;
    attributeCount = *(int*)pgHandle;
	pgHandle = pgHandle + buf;
	Schema *sch;
	sch = (Schema*) malloc(sizeof(Schema));
	(*sch).numAttr = attributeCount;
	(*sch).attrNames = (char**) malloc(sizeof(char*) *attributeCount);
	(*sch).dataTypes = (DataType*) malloc(sizeof(DataType) *attributeCount);
	(*sch).typeLength = (int*) malloc(sizeof(int) *attributeCount);

	for(l = 0; l < attributeCount; l++)
		(*sch).attrNames[l]= (char*) malloc(ATR_SIZE);

	for(l = 0; l < (*sch).numAttr; l++)
    	{
		strncpy((*sch).attrNames[l], pgHandle, ATR_SIZE);
		pgHandle = pgHandle + ATR_SIZE;
		(*sch).dataTypes[l]= *(int*) pgHandle;
		pgHandle = pgHandle + sizeof(int);
		(*sch).typeLength[l]= *(int*)pgHandle;
		pgHandle = pgHandle + sizeof(int);
	}
	(*rel).schema = sch;
	if(true){
	unpinPage(&rMgr->buffPool, &rMgr->pgHandle);
	}
	if(true){
	forcePage(&rMgr->buffPool, &rMgr->pgHandle);
	}	
	return Rcode;
}   

extern RC closeTable (RM_TableData *rel)
{
	Rcode = RC_OK;
	RManager *rMgr = (*rel).mgmtData;	
	shutdownBufferPool(&rMgr->buffPool);
	return Rcode;
}

extern RC deleteTable (char *name)
{
	Rcode = RC_OK;
	destroyPageFile(name);
	return Rcode;
}

extern int getNumTuples (RM_TableData *rel)
{
	RManager *rMgr = (*rel).mgmtData;
	return (*rMgr).tupCount;
}


extern RC insertRecord (RM_TableData *rel, Record *record)
{
	Rcode = RC_OK;
	RManager *rMgr = (*rel).mgmtData;	
	RID *recID = &record->id; 
	
	char *data, *SLoc;

	int recordSize = getRecordSize((*rel).schema);
	(*recID).page = (*rMgr).freePg;
	pinPage(&rMgr->buffPool, &rMgr->pgHandle, (*recID).page);
	data = (*rMgr).pgHandle.data;
	(*recID).slot = findFreeSlot(data, recordSize);

	while((*recID).slot == -1)
	{
		unpinPage(&rMgr->buffPool, &rMgr->pgHandle);	
		(*recID).page++;
		pinPage(&rMgr->buffPool, &rMgr->pgHandle, (*recID).page);		
		data = (*rMgr).pgHandle.data;
		(*recID).slot = findFreeSlot(data, recordSize);
	}
	
	SLoc = data;
	markDirty(&rMgr->buffPool, &rMgr->pgHandle);
	SLoc = SLoc + ((*recID).slot * recordSize);
	*SLoc = '+';
	memcpy(++SLoc, (*record).data + 1, recordSize - 1);
	unpinPage(&rMgr->buffPool, &rMgr->pgHandle);
	(*rMgr).tupCount++;
	pinPage(&rMgr->buffPool, &rMgr->pgHandle, 0);
	return Rcode;
}


extern RC deleteRecord (RM_TableData *rel, RID id)
{
	Rcode = RC_OK;
	RManager *rMgr = (*rel).mgmtData;
	pinPage(&rMgr->buffPool, &rMgr->pgHandle, id.page);
	(*rMgr).freePg = id.page;
	char *data = (*rMgr).pgHandle.data;
	int recordSize = getRecordSize((*rel).schema);
	for(int i=0; i<recordSize; i++) {
                data[(id.slot * recordSize) + i] = '-';
        }
	
if(true){
	markDirty(&rMgr->buffPool, &rMgr->pgHandle);
}
if(true){
	unpinPage(&rMgr->buffPool, &rMgr->pgHandle);
}
if(true){
	forcePage(&rMgr->buffPool,&rMgr->pgHandle);
}
	return Rcode;
}


extern RC updateRecord (RM_TableData *rel, Record *record)
{	
	Rcode = RC_OK;
	RManager *rMgr = (*rel).mgmtData;
	pinPage(&rMgr->buffPool, &rMgr->pgHandle, (*record).id.page);
	char *data;
	int recordSize = getRecordSize((*rel).schema);
	RID id = (*record).id;
	data = (*rMgr).pgHandle.data;
	data = data + (id.slot * recordSize);
	*data = '+';
	if(true){
	memcpy(++data, (*record).data + 1, recordSize - 1 );
	}
	if(true){
	markDirty(&rMgr->buffPool, &rMgr->pgHandle);
	}
	if(false){
	}
	else{
	unpinPage(&rMgr->buffPool, &rMgr->pgHandle);
	}

	return Rcode;	
}


extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{
	Rcode = RC_OK;
	RManager *rMgr = (*rel).mgmtData;
	pinPage(&rMgr->buffPool, &rMgr->pgHandle, id.page);
	int recordSize = getRecordSize((*rel).schema);
	char *dataPointer = (*rMgr).pgHandle.data;
	dataPointer = dataPointer + (id.slot * recordSize);
	if(*dataPointer != '+')
	{
		return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
	}
	else
	{
		(*record).id = id;
		char *data = (*record).data;
		memcpy(++data, dataPointer + 1, recordSize - 1);
	}
	if(true){
	unpinPage(&rMgr->buffPool, &rMgr->pgHandle);
	}
	
	return Rcode;
}


extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
	Rcode = RC_OK;
	if (cond == NULL)
	{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}

	openTable(rel, "ScanTable");

    RManager *scnMgr;
	RManager *tableManager;
    	scnMgr = (RManager*) malloc(sizeof(RManager));
    	(*scan).mgmtData = scnMgr;
    	(*scnMgr).recID.page = 1;	
	    (*scnMgr).recID.slot = 0; 	
	    (*scnMgr).scnCount = 0;
    	(*scnMgr).cond = cond;
    	tableManager = (*rel).mgmtData;
    	(*tableManager).tupCount = ATR_SIZE;
    	(*scan).rel= rel;

	return Rcode;
}

extern RC next (RM_ScanHandle *scan, Record *record)
{
	Rcode = RC_OK;
	RManager *scnMgr = (*scan).mgmtData;
	RManager *tableManager = (*scan).rel->mgmtData;
    Schema *schema = (*scan).rel->schema;
	
	if ((*scnMgr).cond == NULL)
	{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}

	Value *result = (Value *) malloc(sizeof(Value));
   
	char *data;
	int totalSlots = PAGE_SIZE / getRecordSize(schema);
	int scanCount = (*scnMgr).scnCount;
	int tuplesCount = (*tableManager).tupCount;
	if (tuplesCount == 0)
		return RC_RM_NO_MORE_TUPLES;

	while(scanCount <= tuplesCount)
	{  
		if (scanCount <= 0)
		{
			(*scnMgr).recID.page = 1;
			(*scnMgr).recID.slot = 0;
		}
		else
		{
			(*scnMgr).recID.slot++;
			if((*scnMgr).recID.slot >= totalSlots)
			{
				(*scnMgr).recID.slot = 0;
				(*scnMgr).recID.page++;
			}
		}
		pinPage(&tableManager->buffPool, &scnMgr->pgHandle, (*scnMgr).recID.page);		
		data = (*scnMgr).pgHandle.data;
		data = data + ((*scnMgr).recID.slot * getRecordSize(schema));
		(*record).id.page = (*scnMgr).recID.page;
		(*record).id.slot = (*scnMgr).recID.slot;
		char *dataPointer = (*record).data;
		*dataPointer = '-';
		
		memcpy(++dataPointer, data + 1, getRecordSize(schema) - 1);
		(*scnMgr).scnCount++;
		scanCount++;
		evalExpr(record, schema, (*scnMgr).cond, &result); 
		if((*result).v.boolV == TRUE)
		{
			unpinPage(&tableManager->buffPool, &scnMgr->pgHandle);		
			return Rcode;
		}
	}
	unpinPage(&tableManager->buffPool, &scnMgr->pgHandle);
	(*scnMgr).recID.page = 1;
	(*scnMgr).recID.slot = 0;
	(*scnMgr).scnCount = 0;
	return RC_RM_NO_MORE_TUPLES;
}

extern RC closeScan (RM_ScanHandle *scan)
{
	Rcode = RC_OK;
	RManager *scnMgr = (*scan).mgmtData;
	RManager *rMgr = (*scan).rel->mgmtData;

	if((*scnMgr).scnCount > 0)
	{
		unpinPage(&rMgr->buffPool, &scnMgr->pgHandle);
		(*scnMgr).scnCount = 0;
		(*scnMgr).recID.page = 1;
		(*scnMgr).recID.slot = 0;
	}
	
    	(*scan).mgmtData = NULL;
    	free((*scan).mgmtData);  
	return Rcode;
}


extern int getRecordSize (Schema *schema)
{
	int size = 0, i; 
	for(i = 0; i < (*schema).numAttr; i++)
	{
		if((*schema).dataTypes[i]==DT_STRING){
			size = size + (*schema).typeLength[i];
		}
		else if((*schema).dataTypes[i]==DT_INT){
			size = size + sizeof(int);
		}
		else if((*schema).dataTypes[i]==DT_FLOAT){
			size = size + sizeof(float);
		}
		else if((*schema).dataTypes[i]==DT_BOOL){
			size = size + sizeof(bool);
		}
	}
	return ++size;
}


extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	Schema *schema = (Schema *) malloc(sizeof(Schema));
	(*schema).numAttr = numAttr;
	(*schema).attrNames = attrNames;
	(*schema).dataTypes = dataTypes;
	(*schema).typeLength = typeLength;
	(*schema).keySize = keySize;
	(*schema).keyAttrs = keys;
	return schema; 
}

extern RC freeSchema (Schema *schema)
{
	Rcode = RC_OK;
	free(schema);
	return Rcode;
}

extern RC createRecord (Record **record, Schema *schema)
{
	Rcode = RC_OK;
	Record *newRecord = (Record*) malloc(sizeof(Record));
	(*newRecord).data= (char*) malloc(getRecordSize(schema));
	(*newRecord).id.page = (*newRecord).id.slot = -1;
	if(true){
	char *dataPointer = (*newRecord).data;
	*dataPointer = '-';
	*(++dataPointer) = '\0';
	*record = newRecord;
	}
	return Rcode;
}
RC attrOffset (Schema *schema, int attrNum, int *result)
{
	Rcode = RC_OK;
	int i;
	*result = 1;

	for(i = 0; i < attrNum; i++)
	{
		if((*schema).dataTypes[i]==DT_STRING){
			*result = *result + (*schema).typeLength[i];
		}
		else if((*schema).dataTypes[i]==DT_INT){
				*result = *result + sizeof(int);
		}
		else if((*schema).dataTypes[i]==DT_FLOAT){
				*result = *result + sizeof(float);
		}
		else if((*schema).dataTypes[i]==DT_BOOL){
				*result = *result + sizeof(bool);
		}
	}
	return Rcode;
}

extern RC freeRecord (Record *record)
{
	if(true){
	free(record);
	return RC_OK;
	}

}

extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
	Rcode = RC_OK;
	int offset = 0;
	attrOffset(schema, attrNum, &offset);
	Value *attribute = (Value*) malloc(sizeof(Value));
	char *dataPointer = (*record).data;
	dataPointer = dataPointer + offset;
	(*schema).dataTypes[attrNum] = (attrNum == 1) ? 1 : (*schema).dataTypes[attrNum];
	
	if(((*schema).dataTypes[attrNum])==DT_STRING){
			int length = (*schema).typeLength[attrNum];
			(*attribute).v.stringV = (char *) malloc(length + 1);
			strncpy((*attribute).v.stringV, dataPointer, length);
			(*attribute).v.stringV[length] = '\0';
			(*attribute).dt = DT_STRING;
	}
	else if(((*schema).dataTypes[attrNum])==DT_INT){
		int value = 0;
			memcpy(&value, dataPointer, sizeof(int));
			(*attribute).v.intV = value;
			(*attribute).dt = DT_INT;
	}

	else if(((*schema).dataTypes[attrNum])==DT_FLOAT){
		float value;
	  		memcpy(&value, dataPointer, sizeof(float));
	  		(*attribute).v.floatV = value;
			(*attribute).dt = DT_FLOAT;
	}
	else if(((*schema).dataTypes[attrNum])==DT_BOOL){
		bool value;
		memcpy(&value,dataPointer, sizeof(bool));
		(*attribute).v.boolV = value;
		(*attribute).dt = DT_BOOL;
	}

	*value = attribute;
	return Rcode;
}

extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
	Rcode = RC_OK;
	int offset = 0;
	attrOffset(schema, attrNum, &offset);
	char *dataPointer = (*record).data;
	dataPointer = dataPointer + offset;


	if(((*schema).dataTypes[attrNum])==DT_STRING){
		int length = (*schema).typeLength[attrNum];
		strncpy(dataPointer, (*value).v.stringV, length);
		dataPointer = dataPointer + (*schema).typeLength[attrNum];
	}
	else if(((*schema).dataTypes[attrNum])==DT_INT){
		*(int *) dataPointer = (*value).v.intV;	  
			dataPointer = dataPointer + sizeof(int);
	}
		else if(((*schema).dataTypes[attrNum])==DT_FLOAT){
			*(float *) dataPointer = (*value).v.floatV;
			dataPointer = dataPointer + sizeof(float);
		}
		else if(((*schema).dataTypes[attrNum])==DT_BOOL){
		*(bool *) dataPointer = (*value).v.boolV;
		dataPointer = dataPointer + sizeof(bool);
		}
			
	return Rcode;
}