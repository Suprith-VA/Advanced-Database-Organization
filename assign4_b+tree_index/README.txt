Assignment 4 - B+ Tree

- - - - - - - - - - - - - - - - -
        Group Members:
- - - - - - - - - - - - - - - - -

Name - V.C Chandra Kishore
CWID - A20491272
Email- cvuppalachalapathy@hawk.iit.edu 


Name - Sai Sumeeth Reddy Manda
CWID - A20482283
Email- mreddy8@hawk.iit.edu 


Name - Preethi Vempati
CWID - A20466855
Email- pvempati1@hawk.iit.edu 



- - - - - - - - - - - - - - - - -
      Instructions To Run:
- - - - - - - - - - - - - - - - -
-> Go to Project root (assign4) using Terminal.

-> Type ls to list the files and check that we are in the correct directory.

-> Type "make clean" to delete old compiled .o files.

-> Type "make" to compile all project files including "test_assign4.c" file 

-> Type "./test_expr" to compile test expression related files including "test_assign4.c".

-> Type "./test_assign4" to run "test_assign4.c" file.


- - - - - - - - - - - - - - - - -
          Functions:
- - - - - - - - - - - - - - - - -


Functions created to implement the B+ Tree are as below:


- - - - - - - - - - - - - - - - - - - - - - - -
       B+ Tree Functions:
- - - - - - - - - - - - - - - - - - - - - - - -



1) initIndexManager(...)

-> This function Initializes B+tree.

2) shutdownIndexManager(...)

-> This function is used to shutdown the Index manager.

3) createBtree(...)

-> This function is used to allocate memory to all elements of Btree struct.
-> It creates a page file with name idxId.

4) openBtree(...)

-> This function opens page file of the name idxID

5) closeBtree(...)

-> This function frees all the allocated memory.

6) deleteBtree(...)

-> This function deletes all the records of the mentioned Btree.


- - - - - - - - - - - - - - - - -
    Access Functions:
- - - - - - - - - - - - - - - - -


1) getNumNodes(...)

-> This function returns the numbers of nodes in a Btree.

2) getNumEntries(...)

-> This function returns the total number of entries in a Btree.

3) getKeyType(...)

-> This function returns the key type i.e Integer, String etc.


- - - - - - - - - - - - - - - - -
   Index Access Functions:
- - - - - - - - - - - - - - - - -


1) findKey(...)

-> This function returns the key which needs to be found.


2) InsertKey(...)

-> This function inserts the given key in a Btree.


3) deleteKey(...) 

-> This function deletes corresponding key in a Btree.


4) openTreeScan(...)

-> This function scans all entries of the tree.


5) nextEntry(...)

-> This function reads next entry in the Btree.


6) closeTreeScan(...)

-> This function closes scanning of tree elements

