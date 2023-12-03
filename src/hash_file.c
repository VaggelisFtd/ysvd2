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

HT_ErrorCode HT_Init()
{
  CALL_BF(BF_Init(LRU));
  for (int i = 0; i < MAX_OPEN_FILES; i++)
    hash_file[i] = NULL;

  return HT_OK;
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
  memcpy(data + 2 * sizeof(char), &depth, INT_SIZE);

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
  memcpy(&buckets, data + 2 * sizeof(char), INT_SIZE);

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
  /*Retrieve hash value*/
  OpenedHashFile *fileToInsert = hash_file[indexDesc];
  int fileDesc = fileToInsert->fileDesc;
  int blocks = fileToInsert->blocks;
  int hashValue = hash(record.id, blocks);

  /*Find block */
  int intsInBlock = BF_BLOCK_SIZE / INT_SIZE;
  int blockTo = hashValue / intsInBlock + 1;
  int blockPosition = hashValue % intsInBlock;

  /*Find the bucket */
  int bucket;
  BF_Block *indexBlock, *recordBlock;
  BF_Block_Init(&indexBlock), BF_Block_Init(&recordBlock);
  CALL_BF(BF_GetBlock(fileDesc, blockTo, indexBlock));

  /*Metadata */
  char *indexData;
  indexData = BF_Block_GetData(indexBlock);
  indexData += blockPosition * INT_SIZE;
  memcpy(&bucket, indexData, INT_SIZE);

  /*First case: bucket doesn't exist*/
  if (bucket == 0)
  {
    int newBlocksNeeded; // New number of blocks needed
    CALL_BF(BF_GetBlockCounter(fileDesc, &newBlocksNeeded));

    /*Create the bucket*/
    CALL_BF(BF_AllocateBlock(fileDesc, recordBlock));
    char *recordData = BF_Block_GetData(recordBlock);

    /*Added new block number to index to insert*/
    memcpy(indexData, &newBlocksNeeded, INT_SIZE);

    dirtyUnpin(indexBlock);

    /*Add new record */
    int next = -1;
    int counter = 1;
    memcpy(recordData, &next, INT_SIZE);
    memcpy(recordData + INT_SIZE, &counter, INT_SIZE);
    memcpy(recordData + OFFSET, &record, RECORD_SIZE);

    dirtyUnpin(recordBlock);
    BF_Block_Destroy(&recordBlock);
  }

  /*Second case: bucket exists*/
  else
  {
    CALL_BF(BF_GetBlock(fileDesc, bucket, recordBlock));
    char *recordData = BF_Block_GetData(recordBlock);
    int next;
    memcpy(&next, recordData, INT_SIZE);

    /*Iterate to find the last block in bucket */
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

    /*If block is full with records*/
    if (counter == recordsInBlock)
    {
      int newBlocksNeeded;
      CALL_BF(BF_GetBlockCounter(fileDesc, &newBlocksNeeded));

      /*Add new block number to previous */
      memcpy(recordData, &newBlocksNeeded, INT_SIZE);

      /*Save and unpin the block*/
      dirtyUnpin(recordBlock);

      /*Create the new block*/
      CALL_BF(BF_AllocateBlock(fileDesc, recordBlock));
      recordData = BF_Block_GetData(recordBlock);

      /*Add new record to new block */
      next = -1;
      counter = 1;
      memcpy(recordData, &next, INT_SIZE);
      memcpy(recordData + INT_SIZE, &counter, INT_SIZE);
      memcpy(recordData + OFFSET, &record, RECORD_SIZE);
    }
    /*Case where block exists and its has capacity for the record*/
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
          printf("ID: %d, name: %s, surname: %s, city: %s\n", record->id, record->name,
                 record->surname, record->city);
          printedRecords++;
        }
      }
      dirtyUnpin(recordBlock);
    }

    if (printedRecords == 0)
    {
      printf("Could not find ID %d \n", *id);
    }
    dirtyUnpin(indexBlock);
    BF_Block_Destroy(&indexBlock);
  }

  BF_Block_Destroy(&recordBlock);
  return HT_OK;
}

HT_ErrorCode HashStatistics(char *filename)

{
  // int indexDesc;
  // theloume counterer gia blocks kai gia records
  // theloume counterer min kai max records ANA BLOCK (bucket)
  //  ^^^^^^^^^^ sto struct --> fix initialize sthn create

  // HT_CloseFile(indexDesc);
  return HT_OK;
}
