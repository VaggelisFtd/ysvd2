#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> // for debugging

#include <math.h>
#include "bf.h"
#include "hash_file.h"

#define INT_SIZE sizeof(int)
#define OFFSET (2 * INT_SIZE)
#define RECORD_SIZE sizeof(Record)
#define CHAR_SIZE sizeof(char)

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }

/*Structs to represent:an opened hash file and a hash table*/

OpenedHashFile *hash_file[MAX_OPEN_FILES];
HashTable hash_table;

/* Function to check if the hash table is initialized */
int isHashTableInitialized(HashTable *hashTable)
{
  return hashTable->isHashTableInitialized;
}

/*Hash function*/
int hash(int id, int blocks)
{
  return id % blocks;
}

/*Finds a dirty block and unpins it*/
int dirtyUnpin(BF_Block *block)
{
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
}

/*Checks if a hash file is open*/
int checkOpenHashFiles()
{
  int i;

  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (hash_file[i] == NULL)
      break;
  }
  if (i == MAX_OPEN_FILES)
  {
    printf("Open files are at maximum - more files can't be opened");
    return HT_ERROR;
  }
  return HT_OK;
}
/*Function to split bucket */

int splitBucket(OpenedHashFile *fileToInsert, BF_Block *indexBlock, int hashValue)
{
  int fileDesc = fileToInsert->fileDesc;
  int localDepth = fileToInsert->localDepth;
  int intsInBlock = BF_BLOCK_SIZE / INT_SIZE;

  /*Find the block position and block index */

  int blockPosition = hashValue % intsInBlock;
  int blockIndex = hashValue / intsInBlock + 1;

  /*Create two new buckets (blocks) to split the entries and allocate them */
  BF_Block *newBucket1, *newBucket2;
  BF_Block_Init(&newBucket1);
  BF_Block_Init(&newBucket2);
  CALL_BF(BF_AllocateBlock(fileDesc, newBucket1));
  CALL_BF(BF_AllocateBlock(fileDesc, newBucket2));

  /* Get data pointers for the new buckets */
  char *data1 = BF_Block_GetData(newBucket1);
  char *data2 = BF_Block_GetData(newBucket2);

  /*Counters for the 2 new blocks*/
  int counter1 = 0, counter2 = 0;

  /* Iterate through the existing bucket and re-enter entries */

  BF_Block *existingBucket;
  BF_Block_Init(&existingBucket);
  CALL_BF(BF_GetBlock(fileDesc, blockIndex, existingBucket));
  char *existingData = BF_Block_GetData(existingBucket);
  int existingCounter;
  memcpy(&existingCounter, existingData + INT_SIZE, INT_SIZE);

  for (int i = 0; i < existingCounter; i++)
  {
    Record *record = (Record *)(existingData + OFFSET + i * RECORD_SIZE);
    int newHash = hash(record->id, 2 * intsInBlock);

    if (newHash % 2 == 0)
    {
      /*Place record in 1st bucket*/
      memcpy(data1 + OFFSET + counter1 * RECORD_SIZE, record, RECORD_SIZE);
      counter1++;
    }
    else
    {
      /*Place record in 2nd bucket*/
      memcpy(data2 + OFFSET + counter2 * RECORD_SIZE, record, RECORD_SIZE);
      counter2++;
    }
  }

  /*Counter updates */
  memcpy(data1 + INT_SIZE, &counter1, INT_SIZE);
  memcpy(data2 + INT_SIZE, &counter2, INT_SIZE);

  /* Update the index to point to the new buckets */
  char *indexData = BF_Block_GetData(indexBlock);
  indexData += blockPosition * INT_SIZE;
  memcpy(indexData, &blockIndex, INT_SIZE);

  /* Unpin and destroy */
  dirtyUnpin(existingBucket);
  BF_Block_Destroy(&existingBucket);

  dirtyUnpin(newBucket1);
  dirtyUnpin(newBucket2);
  BF_Block_Destroy(&newBucket1);
  BF_Block_Destroy(&newBucket2);

  /* Update local depth */
  fileToInsert->localDepth++;

  printf("Bucket split completed for hash value %d\n", hashValue);

  return HT_OK;
}
HT_ErrorCode HT_Init()
{
  CALL_BF(BF_Init(LRU));
  for (int i = 0; i < MAX_OPEN_FILES; i++)
    hash_file[i] = NULL;

  return HT_OK;
}
int expandHashTable(OpenedHashFile *fileToInsert, BF_Block *indexBlock)
{
  int fileDesc = fileToInsert->fileDesc;
  int globalDepth = hash_table.globalDepth;
  int newGlobalDepth = 2 * globalDepth; // Double the global depth

  /* Allocate new blocks for the expanded hash table */
  BF_Block *newBlock1, *newBlock2;
  BF_Block_Init(&newBlock1), BF_Block_Init(&newBlock2);
  CALL_BF(BF_AllocateBlock(fileDesc, newBlock1));
  CALL_BF(BF_AllocateBlock(fileDesc, newBlock2));

  char *indexData = BF_Block_GetData(indexBlock); // Copy existing data to the new blocks

  /* Update the index to point to the new blocks */
  int blockIndex1, blockIndex2;
  int intsInBlock = BF_BLOCK_SIZE / INT_SIZE;

  /* Calculate block indexes for the new blocks */
  blockIndex1 = (1 + (newGlobalDepth - 1)) + hash_table.splitPointer;
  blockIndex2 = blockIndex1 + (1 + (newGlobalDepth - 1));

  memcpy(indexData, &blockIndex1, INT_SIZE);
  memcpy(indexData + INT_SIZE, &blockIndex2, INT_SIZE);

  /* Copy existing records to new blocks */
  for (int i = 0; i < pow(2, globalDepth); i++)
  {
    {
      BF_Block *existingBucket;
      BF_Block_Init(&existingBucket);
      CALL_BF(BF_GetBlock(fileDesc, i, existingBucket));

      char *existingData = BF_Block_GetData(existingBucket);
      int existingCounter;
      memcpy(&existingCounter, existingData + INT_SIZE, INT_SIZE);

      for (int j = 0; j < existingCounter; j++)
      {
        Record *record = (Record *)(existingData + OFFSET + j * RECORD_SIZE);
        int newHash = hash(record->id, pow(2, newGlobalDepth));
        BF_Block *newBucket;
        BF_Block_Init(&newBucket);
        int newBlockIndex = newHash % intsInBlock + 1;

        /*Get the new block to copy the record */
        CALL_BF(BF_GetBlock(fileDesc, blockIndex1 + newBlockIndex, newBucket));
        char *newBucketData = BF_Block_GetData(newBucket);

        int newCounter;
        memcpy(&newCounter, newBucketData + INT_SIZE, INT_SIZE);

        /* Copy the record to the new block */
        memcpy(newBucketData + OFFSET + newCounter * RECORD_SIZE, record, RECORD_SIZE);
        newCounter++;

        /* Update the counter for the new block */
        memcpy(newBucketData + INT_SIZE, &newCounter, INT_SIZE);

        dirtyUnpin(newBucket);
        BF_Block_Destroy(&newBucket);
      }

      dirtyUnpin(existingBucket);
      BF_Block_Destroy(&existingBucket);
    }

    /* Update local depth and global depth */
    fileToInsert->localDepth = newGlobalDepth;
    hash_table.globalDepth = newGlobalDepth;

    /*  Unpin and destroy */
    dirtyUnpin(newBlock1);
    dirtyUnpin(newBlock2);
    BF_Block_Destroy(&newBlock1);
    BF_Block_Destroy(&newBlock2);

    return HT_OK;
  }
}
HT_ErrorCode HT_CreateIndex(const char *filename, int depth)
{
  /*Create file and open it*/
  int fileDesc;
  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &fileDesc));

  /*Allocate first block*/
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(fileDesc, block));

  /*Get Block data and save them*/
  char *data;
  data = BF_Block_GetData(block);
  memcpy(data + 2 * CHAR_SIZE, &depth, INT_SIZE);

  /*Set bool to true to initialize the hash table*/
  hash_table.isHashTableInitialized = 1;

  /*Unpin the first block*/
  dirtyUnpin(block);

  /*Find the blocks we need */
  int intsInBlock = BF_BLOCK_SIZE / INT_SIZE;
  int blocksForTheIndex = depth / BF_BLOCK_SIZE / INT_SIZE + 1;

  /*Allocate index blocks */
  for (int i = 0; i < blocksForTheIndex; i++)
  {
    CALL_BF(BF_AllocateBlock(fileDesc, block));
    CALL_BF(BF_UnpinBlock(block));
  }

  /*Closing the file*/
  BF_Block_Destroy(&block);
  CALL_BF(BF_CloseFile(fileDesc));
  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc)
{
  /*Check if there are other open files*/
  checkOpenHashFiles();

  /*Opening the file*/
  int fileDesc;
  CALL_BF(BF_OpenFile(fileName, &fileDesc));

  /*Block initialization*/
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(fileDesc, 0, block));

  /*Get the data*/
  char *data;
  data = BF_Block_GetData(block);

  /*Check if is hash table,if not unpin and close it*/
  if (hash_table.isHashTableInitialized != 1)
  {
    dirtyUnpin(block);
    BF_Block_Destroy(&block);
    CALL_BF(BF_CloseFile(fileDesc));
    return HT_ERROR;
  }

  /*get number of buckets*/
  int buckets;
  memcpy(&buckets, data + 2 * CHAR_SIZE, INT_SIZE);

  /*Initialize and allocate new struct*/
  OpenedHashFile *newOpenedHashFile = (OpenedHashFile *)calloc(1, sizeof(OpenedHashFile));
  newOpenedHashFile->fileDesc = fileDesc;
  newOpenedHashFile->blocks = buckets;

  /*Find first empty position and place the struct above*/
  int index = 0;
  while (index < MAX_OPEN_FILES)
  {
    if (hash_file[index] == NULL)
    {
      hash_file[index] = newOpenedHashFile;
      break;
    }
    index++;
  }
  /*Closing the block*/
  dirtyUnpin(block);

  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc)
{
  /*Closing open files and free them*/
  CALL_BF(BF_CloseFile(indexDesc));
  free(hash_file[indexDesc]);
  hash_file[indexDesc] = NULL;

  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record)
{
  OpenedHashFile *fileToInsert = hash_file[indexDesc];
  int fileDesc = fileToInsert->fileDesc;
  int blocks = fileToInsert->blocks;
  int localDepth = fileToInsert->localDepth;
  int hashValue = hash(record.id, blocks);

  /*Find block */
  int intsInBlock = BF_BLOCK_SIZE / INT_SIZE;
  int blockTo = hashValue / intsInBlock + 1;
  int blockPosition = hashValue % intsInBlock;

  /* Find the bucket */
  int bucket;
  BF_Block *indexBlock, *recordBlock;
  BF_Block_Init(&indexBlock), BF_Block_Init(&recordBlock);
  CALL_BF(BF_GetBlock(fileDesc, blockTo, indexBlock));

  char *indexData;
  indexData = BF_Block_GetData(indexBlock);
  indexData += blockPosition * INT_SIZE;
  memcpy(&bucket, indexData, INT_SIZE);

  /*1st case: bucket doesn't exist*/
  if (bucket == 0)
  {
    int newBlocksNeeded; // New number of blocks needed
    CALL_BF(BF_GetBlockCounter(fileDesc, &newBlocksNeeded));

    /*Create the block*/
    CALL_BF(BF_AllocateBlock(fileDesc, recordBlock));
    char *recordData = BF_Block_GetData(recordBlock);

    // Added new block number to index to insert
    memcpy(indexData, &newBlocksNeeded, INT_SIZE);

    dirtyUnpin(indexBlock);

    /*Add new record*/
    int next = -1;
    int counter = 1;
    memcpy(recordData, &next, INT_SIZE);
    memcpy(recordData + INT_SIZE, &counter, INT_SIZE);
    memcpy(recordData + OFFSET, &record, RECORD_SIZE);

    dirtyUnpin(recordBlock);
    BF_Block_Destroy(&recordBlock);
  }
  /*2nd case: bucket exists*/
  else
  {
    CALL_BF(BF_GetBlock(fileDesc, bucket, recordBlock));
    char *recordData = BF_Block_GetData(recordBlock);
    int next;
    memcpy(&next, recordData, INT_SIZE);

    /* Iterate to find last block */
    while (next != -1)
    {
      CALL_BF(BF_UnpinBlock(recordBlock));
      CALL_BF(BF_GetBlock(fileDesc, next, recordBlock));
      recordData = BF_Block_GetData(recordBlock);
      memcpy(&next, recordData, INT_SIZE);
    }

    int recordsInBlock = (BF_BLOCK_SIZE - OFFSET) / RECORD_SIZE;
    int counter;
    memcpy(&counter, recordData + INT_SIZE, INT_SIZE);

    /*Check is split is needed*/
    if (counter == recordsInBlock)
    {
      /*Split the bucket */
      splitBucket(fileToInsert, indexBlock, hashValue);

      /*Retrieve info after the split */
      memcpy(&bucket, indexData, INT_SIZE);

      dirtyUnpin(recordBlock);
      BF_Block_Destroy(&recordBlock);

      /* Get the new last block in the bucket */
      CALL_BF(BF_GetBlock(fileDesc, bucket, recordBlock));
      recordData = BF_Block_GetData(recordBlock);
    }

    /*Case where block hasn't capacity */
    if (counter == recordsInBlock)
    {
      int newBlocksNeeded;
      CALL_BF(BF_GetBlockCounter(fileDesc, &newBlocksNeeded));

      /* Add new block number to previous */
      memcpy(recordData, &newBlocksNeeded, INT_SIZE);

      dirtyUnpin(recordBlock);

      /*Create new block*/
      CALL_BF(BF_AllocateBlock(fileDesc, recordBlock));
      recordData = BF_Block_GetData(recordBlock);

      /*Add record */
      next = -1;
      counter = 1;
      memcpy(recordData, &next, INT_SIZE);
      memcpy(recordData + INT_SIZE, &counter, INT_SIZE);
      memcpy(recordData + OFFSET, &record, RECORD_SIZE);
    }
    /* Case where block exists and it has capacity for the record */
    else
    {
      memcpy(recordData + (OFFSET) + (counter * RECORD_SIZE), &record, RECORD_SIZE);
      counter++;
      memcpy(recordData + INT_SIZE, &counter, INT_SIZE);
    }

    dirtyUnpin(recordBlock);
    BF_Block_Destroy(&recordBlock);
  }

  dirtyUnpin(indexBlock);
  BF_Block_Destroy(&indexBlock);

  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id)
{
  /*Same logic as insert*/
  OpenedHashFile *fileToInsert = hash_file[indexDesc];
  int fileDesc = fileToInsert->fileDesc;
  int blocks = fileToInsert->blocks;

  /*Find blocks inside index */
  int intsInBlock = BF_BLOCK_SIZE / INT_SIZE;
  int blocksInIndex = blocks / intsInBlock + 1;

  /*Initialize record block to print*/
  BF_Block *recordBlock;
  BF_Block_Init(&recordBlock);

  int totalBlocks; // counter
  CALL_BF(BF_GetBlockCounter(fileDesc, &totalBlocks));

  /*Case 1: We don't specify id,so print all records */

  if (id == NULL)
  {
    Record *record;
    int currentBlock = 1 + blocksInIndex;
    for (currentBlock; currentBlock < totalBlocks; currentBlock++)
    {
      CALL_BF(BF_GetBlock(fileDesc, currentBlock, recordBlock));
      char *blockData;
      blockData = BF_Block_GetData(recordBlock);
      int counter;
      memcpy(&counter, blockData + INT_SIZE, INT_SIZE);
      for (int i = 0; i < counter; i++)
      {
        record = (Record *)(blockData + OFFSET + i * RECORD_SIZE);
        printf("ID: %d, name: %s, surname: %s, city: %s\n", record->id, record->name,
               record->surname, record->city);
      }
      dirtyUnpin(recordBlock);
    }
  }
  /*Case 2: find block with specified id with same logic as insert and print it*/
  else
  {
    /*Initialize record*/
    Record *record;
    int hashValue = hash(*id, blocks);
    int blockTo = hashValue / intsInBlock + 1;
    int blockPosition = hashValue % intsInBlock;

    /*Find bucket and initialize index block*/
    int bucket;
    BF_Block *indexBlock;
    BF_Block_Init(&indexBlock);
    CALL_BF(BF_GetBlock(fileDesc, blockTo, indexBlock));

    char *indexData;
    indexData = BF_Block_GetData(indexBlock);
    indexData += blockPosition * INT_SIZE;
    memcpy(&bucket, indexData, INT_SIZE);

    /*Case where bucket doesn't exist*/
    if (bucket == 0)
    {
      dirtyUnpin(recordBlock);
      BF_Block_Destroy(&recordBlock);
      printf("Bucket doesn't exist\n");
      return HT_OK;
    }

    int next = bucket;
    int counter;
    int printedRecords = 0;

    /*Loop inside blocks to find the record*/
    while (next != -1)
    {
      CALL_BF(BF_GetBlock(fileDesc, next, recordBlock));
      char *recordData;
      recordData = BF_Block_GetData(recordBlock);
      memcpy(&next, recordData, INT_SIZE);
      memcpy(&counter, recordData + INT_SIZE, INT_SIZE);
      for (int i = 0; i < counter; i++)
      {
        record = (Record *)(recordData + OFFSET + i * RECORD_SIZE);
        if (record->id == *id)
        {
          printf("id: %d, name: %s, surname: %s, city: %s\n", record->id, record->name,
                 record->surname, record->city);
          printedRecords++;
        }
      }
      dirtyUnpin(recordBlock);
    }

    if (printedRecords == 0)
    {
      printf("Could not find id%d \n", *id);
    }
    dirtyUnpin(indexBlock);
    BF_Block_Destroy(&indexBlock);
  }

  BF_Block_Destroy(&recordBlock);
  return HT_OK;
}
