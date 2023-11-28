#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> // for debugging
#include <math.h>
#include "bf.h"
#include "../include/hash_file.h"

#define MAX_OPEN_FILES 20
#define OFFSET 2 * sizeof(int)
#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }

struct openedIndex
{
  int fileDesc;
  int buckets;
} typedef openedIndex;

openedIndex *hash_table[MAX_OPEN_FILES];

int openFiles = 0;

// Hash Function
int hash(int id, int buckets)
{
  return id % buckets;
}
int dirtyUnpin(BF_Block *block)
{
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
}
int checkOpenFiles()
{
  int i;

  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (hash_table[i] == NULL)
      break;
  }
  if (i == MAX_OPEN_FILES)
  {
    printf("Open files are at maximum - more files can't be opened");
    return HT_ERROR;
  }
  return HT_OK;
}
// HT_info *hash_table[MAX_OPEN_FILES]; // hash table for open files

HT_ErrorCode HT_Init()
{
  CALL_BF(BF_Init(LRU));
  for (int i = 0; i < MAX_OPEN_FILES; i++)
    hash_table[i] = NULL;

  return HT_OK;
}

/*we don't check for max open files beacause we can create, as many as we want, but we can only have 20 open*/
HT_ErrorCode HT_CreateIndex(const char *filename, int depth)
{
  HT_info ht_info;
  BF_Block *block;
  BF_Block *ht_block;
  BF_Block *next_ht_block;
  void *data;
  int file_desc, N, required_blocks, i, curr_id;

  /*create file and open it*/
  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &ht_info.fileDesc));

  /* 1st block: init metadata */
  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(ht_info.fileDesc, block));
  data = BF_Block_GetData(block);

  /*ht info initialization*/
  ht_info.is_ht = true;
  ht_info.global_depth = depth;
  ht_info.ht_id = -1;
  ht_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
  ht_info.max_ht = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(HT_block_info);

  /*2nd block: hashtable */
  BF_Block_Init(&ht_block);
  CALL_BF(BF_AllocateBlock(ht_info.fileDesc, ht_block));
  CALL_BF(BF_GetBlockCounter(ht_info.fileDesc, &ht_info.ht_id)); // number 2 will be saved in ht_id (ht block is 2nd block, ht_id is the id of this block)
  data = BF_Block_GetData(ht_block);                             // initialize ht block
  memcpy(data, &ht_info.ht_id, sizeof(int));                     // save ht_id in data

  /*Hash Table can be stored in multiple blocks --> Create & Initialize more if needed */

  N = pow(2, ht_info.global_depth);                // 2^depth --> number of entries
  required_blocks = ceil(N / ht_info.max_records); // number of blocks we need for hash table

  if (required_blocks > 1) //  if we need more blocks (we already have 1 ht block)
  {
    BF_Block_Init(&next_ht_block);
    for (int i = 1; i < required_blocks; i++)
    {
      CALL_BF(BF_AllocateBlock(file_desc, next_ht_block));
      CALL_BF(BF_GetBlockCounter(ht_info.fileDesc, &curr_id)); // Get number (id) of the new block

      memcpy(data, &curr_id, sizeof(int));
      data = BF_Block_GetData(next_ht_block); // initialize ht block

      // memcpy(ht_info.max_ht)    // memcpy gia max_ht --> loop apo 0 ews max_ht ++ alla ti mpainei sthn memcpy???
      dirtyUnpin(next_ht_block);
      ht_block = next_ht_block; // h next_ht_block = ht_block???
    }
  }
  memcpy(data, &ht_info, sizeof(HT_info)); // Write the meta-data to the first block

  /* Set blocks as dirty & unpin them, so that they are saved in the disc */
  dirtyUnpin(block);
  dirtyUnpin(ht_block);

  /* Call Destroy to free the memory */
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&ht_block);

  CALL_BF(BF_CloseFile(ht_info.fileDesc)); // Close the file

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc)
{
  /* Open files are at maximum - we can't open more */
  if (checkOpenFiles() == HT_ERROR)
    return HT_ERROR;

  // HT_info *ht_info;
  BF_Block *block;
  void *data;

  /*Open the file*/
  CALL_BF(BF_OpenFile(fileName, indexDesc));
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(*indexDesc, 0, block));

  /* Find empty place and write the file's data */
  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (hash_table[i] == NULL)
    {
      // hash_table[i] = (HT_info *)malloc(sizeof(HT_info));
      hash_table[i] = malloc(sizeof(openedIndex));
      data = BF_Block_GetData(block);
      // ht_info = data;
      break;
    }
  }
  CALL_BF(BF_UnpinBlock(block));
  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc)
{
  /*Closing open files and free them*/
  CALL_BF(BF_CloseFile(indexDesc));
  free(hash_table[indexDesc]);
  hash_table[indexDesc] = NULL;

  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record)
{
  // Get hashed value:
  openedIndex *indexForInsertion = hash_table[indexDesc];
  int fileDescriptor = indexForInsertion->fileDesc;
  int buckets = indexForInsertion->buckets;
  int hashValue = hash(record.id, buckets);

  // Find block in index:
  int integersInABlock = BF_BLOCK_SIZE / sizeof(int);
  int blockToGoTo = hashValue / integersInABlock + 1;
  int positionInBlock = hashValue % integersInABlock;

  // Find bucket:
  int bucket;
  BF_Block *indexBlock, *recordBlock;
  BF_Block_Init(&indexBlock);
  BF_Block_Init(&recordBlock);
  char *indexData;
  CALL_BF(BF_GetBlock(fileDescriptor, blockToGoTo, indexBlock));
  indexData = BF_Block_GetData(indexBlock);
  indexData += positionInBlock * sizeof(int);
  memcpy(&bucket, indexData, sizeof(int));

  // Bucket doesn't exist:
  if (bucket == 0)
  {
    // Get number of blocks:
    int newBlockNum;
    CALL_BF(BF_GetBlockCounter(fileDescriptor, &newBlockNum));

    // Create bucket:
    char *recordData;
    CALL_BF(BF_AllocateBlock(fileDescriptor, recordBlock));
    recordData = BF_Block_GetData(recordBlock);

    // Add new bucket number to index:
    memcpy(indexData, &newBlockNum, sizeof(int));
    BF_Block_SetDirty(indexBlock);
    // CALL_BF(BF_UnpinBlock(indexBlock));

    // Add new record to new bucket:
    int next = -1;
    int count = 1;
    memcpy(recordData, &next, sizeof(int));
    memcpy(recordData + sizeof(int), &count, sizeof(int));
    memcpy(recordData + OFFSET, &record, sizeof(Record));
    BF_Block_SetDirty(recordBlock);
    CALL_BF(BF_UnpinBlock(recordBlock));
    BF_Block_Destroy(&recordBlock);
  }
  else
  { // Bucket already exists:
    CALL_BF(BF_GetBlock(fileDescriptor, bucket, recordBlock));
    char *recordData = BF_Block_GetData(recordBlock);
    int next;
    memcpy(&next, recordData, sizeof(int));

    // Find last block in bucket:
    while (next != -1)
    {
      CALL_BF(BF_UnpinBlock(recordBlock));
      CALL_BF(BF_GetBlock(fileDescriptor, next, recordBlock));
      recordData = BF_Block_GetData(recordBlock);
      memcpy(&next, recordData, sizeof(int));
    }

    int recordsInBlock = (BF_BLOCK_SIZE - OFFSET) / sizeof(Record);
    int count;
    memcpy(&count, recordData + sizeof(int), sizeof(int));

    // If block is full:
    if (count == recordsInBlock)
    {
      // Get number of blocks:
      int newBlockNum;
      CALL_BF(BF_GetBlockCounter(fileDescriptor, &newBlockNum));

      // Add new block number to previous block:
      memcpy(recordData, &newBlockNum, sizeof(int));

      // Save changes to previous block and get rid of it:
      BF_Block_SetDirty(recordBlock);
      CALL_BF(BF_UnpinBlock(recordBlock));

      // Create block:
      CALL_BF(BF_AllocateBlock(fileDescriptor, recordBlock));
      recordData = BF_Block_GetData(recordBlock);

      // Add new record to new block:
      next = -1;
      count = 1;
      memcpy(recordData, &next, sizeof(int));
      memcpy(recordData + sizeof(int), &count, sizeof(int));
      memcpy(recordData + OFFSET, &record, sizeof(Record));
    }
    // If block isn't full:
    else
    {
      memcpy(recordData + (OFFSET) + (count * sizeof(Record)), &record, sizeof(Record));
      count++;
      memcpy(recordData + sizeof(int), &count, sizeof(int));
    }

    BF_Block_SetDirty(recordBlock);
    CALL_BF(BF_UnpinBlock(recordBlock));
    BF_Block_Destroy(&recordBlock);
  }

  CALL_BF(BF_UnpinBlock(indexBlock));
  BF_Block_Destroy(&indexBlock);
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id)
{
  /*
    // void* data;
    char *data;
    BF_Block *block;
    BF_Block_Init(&block);
    HT_info ht_info;
    HT_block_info ht_block_info;
    Record record;

    if ((BF_GetBlock(indexDesc, 0, block)) < 0)
    {
      printf("Error getting block in HT_InsertEntry\n");
      return -1;
    }

    // get pointer to block's 0 data
    data = BF_Block_GetData(block);
    // get the metadata of this block so that we can access ht_info.fileDesc
    memcpy(&ht_info, data, sizeof(HT_info));

    // Firstly get the total num of blocks in heap file
    int total_blocks;
    if ((BF_GetBlockCounter(ht_info.fileDesc, &total_blocks)) < 0)
    {
      printf("Error getting num of blocks in HT_InsertEntry\n");
      return -1;
    }

    // Unpin block 0 since we dont need it anymore
    if ((BF_UnpinBlock(block)) < 0)
    {
      printf("Error unpinning block in HT_InsertEntry\n");
      return -1;
    }

    for (int i = 1; i < total_blocks; i++)
    {
      // for (int i=1 ; i<2 ; i++) {
      // Bring this block to memory
      if ((BF_GetBlock(ht_info.fileDesc, i, block)) < 0)
      {
        return -1;
      }

      // get pointer to this block's data
      data = BF_Block_GetData(block);

      // get the metadata of this last block
      memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));

      // for every record in this block
      for (int k = 0; k < ht_block_info.num_records * sizeof(Record); k = k + sizeof(Record))
      {
        // copy in our variable record, the contents of the current record we are reading
        memcpy(&record, data + k, sizeof(Record));

        // check if it's the one we are looking for
        // printf("record.id = %d\n", record.id);
        // printf("id = %d\n", id);
        if (id != NULL)
        {
          if (record.id == *id)
          {
            // we store as "number of total blocks read" the numbber of the block we just found the last occurance of the id we are searching for
            // num_blocks_read = i;
            // we print it each time it occures
            printf("Record in offset %d in Block %d: id=%d, name=%s, surname=%s, city=%s\n", k, i, record.id, record.name, record.surname, record.city);
          }
        }
        else
        {
          printf("Record in offset %d in Block %d: id=%d, name=%s, surname=%s, city=%s\n", k, i, record.id, record.name, record.surname, record.city);
        }
      }

      // Write babck on disk the block we just read
      if ((BF_UnpinBlock(block)) < 0)
      {
        printf("Error unpinning block in HT_GetAllEntries\n");
        return -1;
      }
    }

    BF_Block_Destroy(&block);

    return HT_OK;
  }
  */
  /* HT_ErrorCode HT_HashStatistics(char *filename)
  {
    int fileDesc, indexDesc;
    HT_OpenIndex(filename, &indexDesc);
    /* if ((indexDesc < MAX_OPEN_FILES) && (indexDesc > -1) && (ht_index.fileDesc[indexDesc] != -1)) {
    //     fileDesc = ht_index.fileDesc[indexDesc];
    // } else
    //     return HT_ERROR;


    int num_of_blocks; // computing total blocks in the file
    CALL_BF(BF_GetBlockCounter(fileDesc, &num_of_blocks));

    printf("File '%s' has %d Blocks\n", filename, num_of_blocks);
    if (num_of_blocks == 1)
    {
      printf("No data yet in the file!\n");
      return HT_OK;
    }

    int total_buckets = 0; // counter buckets
    int total_records = 0; // counter for records
    int min_records = 20;
    int max_records = 0;

    char *ht;
    BF_Block *HashBlock;
    BF_Block_Init(&HashBlock);

    int HTindex = 0;
    LL *explorer = NULL; // traverse blocks

    while (HTindex != -1)
    {
      insertLL(HTindex, &explorer); // get which blocks are hashblocks
      CALL_BF(BF_GetBlock(fileDesc, HTindex, HashBlock));
      ht = BF_Block_GetData(HashBlock);
      HTindex = ((HT *)ht)->nextHT; // the next SHTindex
      BF_UnpinBlock(HashBlock);
    }

    char *data;
    BF_Block *Bucket;
    BF_Block_Init(&Bucket);

    freeLL(explorer);
    if (total_buckets != 0)
    {
      double average = total_records / total_buckets; // compute using total records by buckets

      if ((min_records == 20) || (max_records == 0))
        return HT_ERROR;

      //print statistics
      printf("Minimum Records: %d\n", min_records);
      printf("Average Records: %f\n", average);
      printf("Maximum Records: %d\n", max_records);
    }
    return HT_OK;
  }
  */
}