#include "btree_mgr.h"
#include "tables.h"
#include "storage_mgr.h"
#include "record_mgr.h"
#include <stdlib.h>
#include <string.h>

int inum = 0;
RC code=RC_OK;
SM_FileHandle btree_fh;
int maxEle;

typedef struct BTREE
{
    int *key;
    struct BTREE **next;
    RID *id;
} BTree;

BTree *scan;
BTree *root;



RC initIndexManager (void *mgmtData)
{
    return code;
}

RC shutdownIndexManager ()
{
    return code;
}


RC createBtree (char *idxId, DataType keyType, int n)
{
	int i=0;
    root = (BTree*)malloc(sizeof(BTree));
    (*root).id = malloc(sizeof(int) * n);
    (*root).key = malloc(sizeof(int) * n);
    (*root).next = malloc(sizeof(BTree) * (n + 1));
    while(i < n + 1){
        root->next[i] = NULL;
        i=i+1;
    }
    maxEle = n;
    createPageFile (idxId);
    
    return code;
}

RC openBtree (BTreeHandle **tree, char *idxId)
{
    return((openPageFile (idxId, &btree_fh)==0)?code:RC_ERROR);
}

RC closeBtree (BTreeHandle *tree)
{
    if(closePageFile(&btree_fh)==0){
	free(root);
    return code;
	}
	else{
		return RC_ERROR;
	}
}

RC deleteBtree (char *idxId)
{
    return((destroyPageFile(idxId)==0)?code:RC_ERROR);
}



RC getNumNodes (BTreeHandle *tree, int *result)
{
    BTree *temp = (BTree*)malloc(sizeof(BTree));
    
    int numNodes = 0;
    int i=0;
    l:
    if(i < maxEle + 2){
    numNodes ++;
    i++;
    goto l; 
    }
    if(true){
        if(true){
        *result = numNodes;
        return code;
        }

    }
}

RC getNumEntries (BTreeHandle *tree, int *result)
{

    int totalEle = 0, i;
    int buff=sizeof(BTree);
    BTree *temp = (BTree*)malloc(buff);


    for (temp = root; temp != NULL; temp = temp->next[maxEle])
        {
            int i=0;
            while(i<maxEle){
                if (temp->key[i] != 0)
                totalEle ++;
                i++;
            }
        }
    if(true){
    *result = totalEle;
    return code;
    }

}

RC getKeyType (BTreeHandle *tree, DataType *result)
{
    if(true){
            return code;
    }

}


RC findKey (BTreeHandle *tree, Value *key, RID *result)
{
    int buff=sizeof(BTree);
    BTree *temp = (BTree*)malloc(buff);
    int found = 0, i;
    for (temp = root; temp != NULL; temp = temp->next[maxEle]) {
        // for (i = 0; i < maxEle; i ++) {
           {
            int i = 0;
            while(i<maxEle){
            if ((*temp).key[i] == key->v.intV) {
                result->page = temp->id[i].page;
                result->slot = temp->id[i].slot;
                found = 1;
                break;
            }
            i++;
            }
        }
        if (found == 1)
            break;
    }

    return((found == 1)?code:RC_IM_KEY_NOT_FOUND);

}

RC insertKey (BTreeHandle *tree, Value *key, RID rid)
{
    int i = 0;
    int buff=sizeof(BTree);
    BTree *temp = (BTree*)malloc(buff);
    BTree *node = (BTree*)malloc(buff);
    (*node).key = malloc(sizeof(int) * maxEle);
    (*node).id = malloc(sizeof(int) * maxEle);
    (*node).next = malloc(sizeof(BTree) * (maxEle + 1));
    

    while(i < maxEle){
        node->key[i] = 0;
        i++;
    }

  
    int nodeFull = 0;
    
    for (temp = root; temp != NULL; temp = temp->next[maxEle]) {
        nodeFull = 0;
        // for (i = 0; i < maxEle; i ++) {
            {
            int i = 0;
            while(i<maxEle){
            if ((*temp).key[i] == 0) {
                (*temp).id[i].page = rid.page;
                (*temp).id[i].slot = rid.slot;
                (*temp).key[i] = key->v.intV;
                (*temp).next[i] = NULL;
                nodeFull ++;
                break;
            }
            i++;
            }
        }
        if ((nodeFull == 0) && (temp->next[maxEle] == NULL)) {
            (*node).next[maxEle] = NULL;
            (*temp).next[maxEle] = node;
        }
    }
    
    int totalEle = 0;
    for (temp = root; temp != NULL; temp = temp->next[maxEle])
        // for (i = 0; i < maxEle; i ++)
        {
            int i =0;
            while(i<maxEle){
            if ((*temp).key[i] != 0)
                totalEle ++;
                i++;
            }

        }

    if (totalEle == 6) {
        (*node).key[0] = (*root).next[maxEle]->key[0];
        (*node).key[1] = (*root).next[maxEle]->next[maxEle]->key[0];
        (*node).next[0] = root;
        (*node).next[1] = (*root).next[maxEle];
        (*node).next[2] = (*root).next[maxEle]->next[maxEle];

    }
    if(true){
            return code;
    }

}

RC deleteKey (BTreeHandle *tree, Value *key)
{
    int buff=sizeof(BTree);
    BTree *temp = (BTree*)malloc(buff);
    int found = 0, i;
    for (temp = root; temp != NULL; temp = temp->next[maxEle]) {
        {
            int i=0;
            while(i<maxEle){
            if ((*temp).key[i] == (*key).v.intV) {
                (*temp).key[i] = 0;
                (*temp).id[i].page = 0;
                (*temp).id[i].slot = 0;
                found = 1;
                break;
            }
            i++;
        }
    }
        if (found == 1)
            break;
    }
    
    return code;
}


RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle)
{
    int buff=sizeof(BTree);
    scan = (BTree*)malloc(buff);
    scan = root;
    inum = 0;
    int dem=14;
    BTree *temp = (BTree*)malloc(buff);
    int totalEle = 0, i;
    for (temp = root; temp != NULL; temp = (*temp).next[maxEle])
        {
            int i = 0;
            while(i<maxEle){
                if ((*temp).key[i] != 0)
                totalEle ++;
                i++;
            }
        }

    int key[totalEle];
    int elements[maxEle][totalEle];
    int count = 0;
    for (temp = root; temp != NULL; temp = temp->next[maxEle]) {
        {
            int i = 0;
            while(i<maxEle){
            key[count] = temp->key[i];
            if(dem!=0){
            elements[0][count] = temp->id[i].page;
            dem=dem+1;
            elements[1][count] = temp->id[i].slot;
            }
            dem=20;
            count ++;
            i++;
            }
        }
    }
    
    int swap;
    int pg, st, d;
    int c = 0;
    int sam=1;
    int res=12;
    while (c < count - 1){
    {
        {
            int d = 0;
            while(d<count - c - 1){
            if (key[d] > key[d+1])
            {
                if(sam==1){
                swap = key[d];
                pg = elements[0][d];
                st = elements[1][d];
                if(res>sam){
                key[d]   = key[d + 1];
                res=res+1;
                elements[0][d] = elements[0][d + 1];
                res=1024-112;
                res++;
                elements[1][d] = elements[1][d + 1];
                if(sam!=res){
                key[d + 1] = swap;
                res=20;
                elements[0][d + 1] = pg;
                elements[1][d + 1] = st;
                }
                }
                }
            }
            d++;
            }
        }
    }
    c++;
}


    count = 0;
    for (temp = root; temp != NULL; temp = temp->next[maxEle]) {
        {
            int i = 0;
            while(i<maxEle){
            (*temp).key[i] =key[count];
            (*temp).id[i].page = elements[0][count];
            (*temp).id[i].slot = elements[1][count];
            count ++;
            i++;
            }
        }
    }

    return code;
}

RC nextEntry (BT_ScanHandle *handle, RID *result)
{
    if((*scan).next[maxEle] != NULL) {
        if(maxEle == inum) {
            inum = 0;
            scan = (*scan).next[maxEle];
        }

        result->page = (*scan).id[inum].page;
        (*result).slot = (*scan).id[inum].slot;
        inum =inum+1;
    }
    else
        return RC_IM_NO_MORE_ENTRIES;
    if(true){
            return code;
    }
}

RC closeTreeScan (BT_ScanHandle *handle)
{
    if(true){
    inum = 0;
    return code;
    }
}


char *printTree (BTreeHandle *tree)
{
    if(true){
    return RC_OK;
    }

}
