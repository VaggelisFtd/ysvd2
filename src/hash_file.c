#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> // for debugging
#include <math.h>
#include "bf.h"
#include "hash_file.h"

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
void printCustomRecord(Record *record)
{
  printf("%d,\"%s\",\"%s\",\"%s\"\n",
         record->id, record->name, record->surname, record->city);
}

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
  // HT_info ht_info;
  // BF_Block *block;
  // BF_Block *ht_block;
  // BF_Block *next_ht_block;
  // void *data;
  // int file_desc, N, required_blocks, i, curr_id;

  // /*create file and open it*/
  // CALL_BF(BF_CreateFile(filename));
  // CALL_BF(BF_OpenFile(filename, &ht_info.fileDesc));

  // /* 1st block: init metadata */
  // BF_Block_Init(&block);
  // CALL_BF(BF_AllocateBlock(ht_info.fileDesc, block));
  // data = BF_Block_GetData(block);

  // /*ht info initialization*/
  // ht_info.is_ht = true;
  // ht_info.global_depth = depth;
  // ht_info.ht_id = -1;
  // ht_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
  // ht_info.max_ht = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(HT_block_info);

  // /*2nd block: hashtable */
  // BF_Block_Init(&ht_block);
  // CALL_BF(BF_AllocateBlock(ht_info.fileDesc, ht_block));
  // CALL_BF(BF_GetBlockCounter(ht_info.fileDesc, &ht_info.ht_id)); // number 2 will be saved in ht_id (ht block is 2nd block, ht_id is the id of this block)
  // data = BF_Block_GetData(ht_block);                             // initialize ht block
  // memcpy(data, &ht_info.ht_id, sizeof(int));                     // save ht_id in data

  // /*Hash Table can be stored in multiple blocks --> Create & Initialize more if needed */

  // // N = pow(2, ht_info.global_depth);                // 2^depth --> number of entries
  // N = 2 ^ ht_info.global_depth;  //cant read pow
  // required_blocks = ceil(N / ht_info.max_records); // number of blocks we need for hash table

  // if (required_blocks > 1) //  if we need more blocks (we already have 1 ht block)
  // {
  //   BF_Block_Init(&next_ht_block);
  //   for (int i = 1; i < required_blocks; i++)
  //   {
  //     CALL_BF(BF_AllocateBlock(file_desc, next_ht_block));
  //     CALL_BF(BF_GetBlockCounter(ht_info.fileDesc, &curr_id)); // Get number (id) of the new block

  //     memcpy(data, &curr_id, sizeof(int));
  //     data = BF_Block_GetData(next_ht_block); // initialize ht block

  //     // memcpy(ht_info.max_ht)    // memcpy gia max_ht --> loop apo 0 ews max_ht ++ alla ti mpainei sthn memcpy???
  //     dirtyUnpin(next_ht_block);
  //     ht_block = next_ht_block; // h next_ht_block = ht_block???
  //   }
  // }
  // memcpy(data, &ht_info, sizeof(HT_info)); // Write the meta-data to the first block

  // /* Set blocks as dirty & unpin them, so that they are saved in the disc */
  // dirtyUnpin(block);
  // dirtyUnpin(ht_block);

  // /* Call Destroy to free the memory */
  // BF_Block_Destroy(&block);
  // BF_Block_Destroy(&ht_block);

  // CALL_BF(BF_CloseFile(ht_info.fileDesc)); // Close the file

  // return HT_OK;

  int fd;
  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &fd));

  // Create first block.
  BF_Block *block;
  char *data;
  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(fd, block));
  data = BF_Block_GetData(block);

  // Write "HT" in the beginning of first block to signify
  // we have an HT file.
  char str1[3];
  strcpy(str1, "HT");
  memcpy(data, str1, 2);

  // Write number of buckets after "HT" string.
  memcpy(data + 2, &buckets, 4); //!!!

  // Save changes to first block.
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  // Calculate blocks needed for index.
  int integersInABlock = BF_BLOCK_SIZE / sizeof(int);
  int blocksNeededForIndex = buckets / integersInABlock + 1;

  // Allocate blocks for index.
  for (int i = 0; i < blocksNeededForIndex; i++)
  {
    CALL_BF(BF_AllocateBlock(fd, block));
    CALL_BF(BF_UnpinBlock(block));
  }

  // Make changes and close HT file.
  BF_Block_Destroy(&block);
  CALL_BF(BF_CloseFile(fd));
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
    int newBlockNum; // Get number of blocks:
    CALL_BF(BF_GetBlockCounter(fileDescriptor, &newBlockNum));

    // Create bucket:
    CALL_BF(BF_AllocateBlock(fileDescriptor, recordBlock));
    char *recordData = BF_Block_GetData(recordBlock);

    // Add new bucket number to index:
    memcpy(indexData, &newBlockNum, sizeof(int));
    BF_Block_SetDirty(indexBlock);
    CALL_BF(BF_UnpinBlock(indexBlock));

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
  else // bucket already exists
  {
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

    if (count == recordsInBlock) // If block is full:
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
    else // If block isn't full:
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
  openedIndex *indexForInsertion = hash_table[indexDesc];
  int fd = indexForInsertion->fileDesc;
  int buckets = indexForInsertion->buckets;

  // Calculate blocks in index.
  int integersInABlock = BF_BLOCK_SIZE / sizeof(int);
  int blocksInIndex = buckets / integersInABlock + 1;

  BF_Block *recordBlock;
  BF_Block_Init(&recordBlock);

  int totalBlocks;
  CALL_BF(BF_GetBlockCounter(fd, &totalBlocks));

  // If id is NULL, print every record in the hash file
  if (id == NULL)
  {
    Record *record;
    for (int currentBlock = 1 + blocksInIndex; currentBlock < totalBlocks; currentBlock++)
    {
      CALL_BF(BF_GetBlock(fd, currentBlock, recordBlock));
      char *blockData;
      blockData = BF_Block_GetData(recordBlock);
      int count;
      memcpy(&count, blockData + sizeof(int), sizeof(int));
      for (int i = 0; i < count; i++)
      {
        record = (Record *)(blockData + OFFSET + i * sizeof(Record));
        printCustomRecord(record);
      }
      CALL_BF(BF_UnpinBlock(recordBlock));
    }
  }

  else
  { // Otherwise find the record in the hash table using the same operations as in InsertEntry
    Record *record;
    int hashValue = hash(*id, buckets);
    int blockToGoTo = hashValue / integersInABlock + 1;
    int positionInBlock = hashValue % integersInABlock;

    // Find bucket:
    int bucket;
    BF_Block *indexBlock;
    BF_Block_Init(&indexBlock);
    char *indexData;
    CALL_BF(BF_GetBlock(fd, blockToGoTo, indexBlock));
    indexData = BF_Block_GetData(indexBlock);
    indexData += positionInBlock * sizeof(int);
    memcpy(&bucket, indexData, sizeof(int));

    if (bucket == 0)
    {
      CALL_BF(BF_UnpinBlock(recordBlock));
      BF_Block_Destroy(&recordBlock);
      printf("Bucket doesn't exist yet.\n");
      return HT_OK;
    }

    int next = bucket;
    int count;
    int printed = 0;

    do
    { // Searching each block in the bucket for the record
      CALL_BF(BF_GetBlock(fd, next, recordBlock));
      char *recordData;
      recordData = BF_Block_GetData(recordBlock);
      memcpy(&next, recordData, sizeof(int));
      memcpy(&count, recordData + sizeof(int), sizeof(int));
      for (int i = 0; i < count; i++)
      {
        record = (Record *)(recordData + OFFSET + i * sizeof(Record));
        if (record->id == *id)
        {
          printCustomRecord(record);
          printed++;
        }
      }
      CALL_BF(BF_UnpinBlock(recordBlock));
    } while (next != -1);

    if (printed == 0)
    {
      printf("Could not find ID %d sadly...\n", *id);
    }
    CALL_BF(BF_UnpinBlock(indexBlock));
    BF_Block_Destroy(&indexBlock);
  }

  BF_Block_Destroy(&recordBlock);
  return HT_OK;
}