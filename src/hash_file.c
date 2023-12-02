#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> // for debugging

#include <math.h>
#include "bf.h"
#include "hash_file.h"

#define INT_SIZE sizeof(int)
#define OFFSET (2 * INT_SIZE)

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }
// Structure to represent an opened hash file
OpenedHashFile *hash_file[MAX_OPEN_FILES];

// Structure to represent the hash_table
HashTable hash_table;

// Function to check if the hash table is initialized
int isHashTableInitialized(HashTable *hashTable)
{
  return hashTable->isHashTableInitialized;
}

// Hash Function
int hash(int id, int blocks)
{
  return id % blocks;
}
int dirtyUnpin(BF_Block *block)
{
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
}
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
void printCustomRecord(Record *record)
{
  printf("%d,\"%s\",\"%s\",\"%s\"\n",
         record->id, record->name, record->surname, record->city);
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
  // Create and open HT file.
  int fd;
  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &fd));

  // Create first block.
  BF_Block *block;
  char *data;
  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(fd, block));
  data = BF_Block_GetData(block);

  // Set bool to true to show that we have an ht file
  hash_table.isHashTableInitialized = 1;

  memcpy(data + 2, &depth, 4);

  // Save changes to first block.
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  // Calculate blocks needed for index.
  int integersInABlock = BF_BLOCK_SIZE / INT_SIZE;
  int blocksNeededForIndex = depth / integersInABlock + 1;

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
  checkOpenHashFiles();

  // Open file.
  int fd;
  CALL_BF(BF_OpenFile(fileName, &fd));

  // Init block.
  BF_Block *block;
  BF_Block_Init(&block);
  char *data;
  CALL_BF(BF_GetBlock(fd, 0, block));
  data = BF_Block_GetData(block);

  // Check if it's not HT file and close it.
  if (hash_table.isHashTableInitialized != 1)
  {
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
    CALL_BF(BF_CloseFile(fd));
    return HT_ERROR;
  }

  // Get number of buckets:
  int buckets;
  memcpy(&buckets, data + 2, INT_SIZE);

  // Allocate new OpenedHashFile struct
  OpenedHashFile *newOpenedHashFile = (OpenedHashFile *)calloc(1, sizeof(OpenedHashFile));
  newOpenedHashFile->fileDesc = fd;
  newOpenedHashFile->blocks = buckets;

  // Find first NULL position in hashtable and put the new OpenedHashFile struct
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

  *indexDesc = index;

  // Close block.
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

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
  // Get hashed value:
  OpenedHashFile *indexForInsertion = hash_file[indexDesc];
  int fileDesc = indexForInsertion->fileDesc;
  int blocks = indexForInsertion->blocks;
  int hashValue = hash(record.id, blocks);

  // Find block in index:
  int integersInABlock = BF_BLOCK_SIZE / INT_SIZE;
  int blockToGoTo = hashValue / integersInABlock + 1;
  int positionInBlock = hashValue % integersInABlock;

  // Find bucket:
  int bucket;
  BF_Block *indexBlock, *recordBlock;
  BF_Block_Init(&indexBlock);
  BF_Block_Init(&recordBlock);
  char *indexData;
  CALL_BF(BF_GetBlock(fileDesc, blockToGoTo, indexBlock));
  indexData = BF_Block_GetData(indexBlock);
  indexData += positionInBlock * INT_SIZE;
  memcpy(&bucket, indexData, INT_SIZE);

  // Bucket doesn't exist:
  if (bucket == 0)
  {
    int newBlockNum; // Get number of blocks
    CALL_BF(BF_GetBlockCounter(fileDesc, &newBlockNum));

    // Create bucket
    CALL_BF(BF_AllocateBlock(fileDesc, recordBlock));
    char *recordData = BF_Block_GetData(recordBlock);

    // Add new bucket number to index
    memcpy(indexData, &newBlockNum, INT_SIZE);
    BF_Block_SetDirty(indexBlock);
    CALL_BF(BF_UnpinBlock(indexBlock));

    // Add new record to new bucket
    int next = -1;
    int count = 1;
    memcpy(recordData, &next, INT_SIZE);
    memcpy(recordData + INT_SIZE, &count, INT_SIZE);
    memcpy(recordData + OFFSET, &record, sizeof(Record));
    BF_Block_SetDirty(recordBlock);
    CALL_BF(BF_UnpinBlock(recordBlock));
    BF_Block_Destroy(&recordBlock);
  }

  else // bucket already exists
  {
    CALL_BF(BF_GetBlock(fileDesc, bucket, recordBlock));
    char *recordData = BF_Block_GetData(recordBlock);
    int next;
    memcpy(&next, recordData, INT_SIZE);

    // Find last block in bucket:
    while (next != -1)
    {
      CALL_BF(BF_UnpinBlock(recordBlock));
      CALL_BF(BF_GetBlock(fileDesc, next, recordBlock));
      recordData = BF_Block_GetData(recordBlock);
      memcpy(&next, recordData, INT_SIZE);
    }

    int recordsInBlock = (BF_BLOCK_SIZE - OFFSET) / sizeof(Record);
    int count;
    memcpy(&count, recordData + INT_SIZE, INT_SIZE);

    if (count == recordsInBlock) // If block is full:
    {
      // Get number of blocks:
      int newBlockNum;
      CALL_BF(BF_GetBlockCounter(fileDesc, &newBlockNum));

      // Add new block number to previous block:
      memcpy(recordData, &newBlockNum, INT_SIZE);

      // Save changes to previous block and get rid of it:
      BF_Block_SetDirty(recordBlock);
      CALL_BF(BF_UnpinBlock(recordBlock));

      // Create block:
      CALL_BF(BF_AllocateBlock(fileDesc, recordBlock));
      recordData = BF_Block_GetData(recordBlock);

      // Add new record to new block:
      next = -1;
      count = 1;
      memcpy(recordData, &next, INT_SIZE);
      memcpy(recordData + INT_SIZE, &count, INT_SIZE);
      memcpy(recordData + OFFSET, &record, sizeof(Record));
    }
    else // If block isn't full:
    {
      memcpy(recordData + (OFFSET) + (count * sizeof(Record)), &record, sizeof(Record));
      count++;
      memcpy(recordData + INT_SIZE, &count, INT_SIZE);
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
  OpenedHashFile *indexForInsertion = hash_file[indexDesc];
  int fd = indexForInsertion->fileDesc;
  int blocks = indexForInsertion->blocks;

  // Calculate blocks in index.
  int integersInABlock = BF_BLOCK_SIZE / INT_SIZE;
  int blocksInIndex = blocks / integersInABlock + 1;

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
      memcpy(&count, blockData + INT_SIZE, INT_SIZE);
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
    int hashValue = hash(*id, blocks);
    int blockToGoTo = hashValue / integersInABlock + 1;
    int positionInBlock = hashValue % integersInABlock;

    // Find bucket:
    int bucket;
    BF_Block *indexBlock;
    BF_Block_Init(&indexBlock);
    char *indexData;
    CALL_BF(BF_GetBlock(fd, blockToGoTo, indexBlock));
    indexData = BF_Block_GetData(indexBlock);
    indexData += positionInBlock * INT_SIZE;
    memcpy(&bucket, indexData, INT_SIZE);

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
      memcpy(&next, recordData, INT_SIZE);
      memcpy(&count, recordData + INT_SIZE, INT_SIZE);
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

HT_ErrorCode HashStatistics(char *filename)

{
  // int indexDesc;
  // theloume counter gia blocks kai gia records
  // theloume counter min kai max records ANA BLOCK (bucket)
  //  ^^^^^^^^^^ sto struct --> fix initialize sthn create

  // HT_CloseFile(indexDesc);
  return HT_OK;
}

// Function to find or create a bucket for a hash value
int findOrCreateBucket(int fileDesc, int hashValue, BF_Block **bucketBlock)
{
  int integersInABlock = BF_BLOCK_SIZE / INT_SIZE;

  // Calculate block and position within block
  int blockToGoTo = hashValue / integersInABlock + 1;
  int positionInBlock = hashValue % integersInABlock;

  // Get the block number from the index
  int bucketNumber;
  BF_Block *indexBlock;
  BF_Block_Init(&indexBlock);

  CALL_BF(BF_GetBlock(fileDesc, blockToGoTo, indexBlock));
  char *indexData = BF_Block_GetData(indexBlock) + positionInBlock * INT_SIZE;
  memcpy(&bucketNumber, indexData, INT_SIZE);

  // If bucket doesn't exist, create a new one
  if (bucketNumber == 0)
  {
    CALL_BF(BF_UnpinBlock(indexBlock));

    // Get number of blocks
    int newBlockNum;
    CALL_BF(BF_GetBlockCounter(fileDesc, &newBlockNum));

    // Update the index block with the new bucket number
    indexData = BF_Block_GetData(indexBlock) + positionInBlock * INT_SIZE;
    memcpy(indexData, &newBlockNum, INT_SIZE);
    BF_Block_SetDirty(indexBlock);
    CALL_BF(BF_UnpinBlock(indexBlock));

    // Allocate a new block for the bucket
    CALL_BF(BF_AllocateBlock(fileDesc, *bucketBlock));
    CALL_BF(BF_UnpinBlock(*bucketBlock));

    return newBlockNum;
  }

  CALL_BF(BF_UnpinBlock(indexBlock));
  CALL_BF(BF_GetBlock(fileDesc, bucketNumber, *bucketBlock));

  return bucketNumber;
}

// Function to add a record to a bucket
HT_ErrorCode addRecordToBucket(int fileDesc, BF_Block *bucketBlock, Record *record)
{
  char *blockData = BF_Block_GetData(bucketBlock);
  int next;
  memcpy(&next, blockData, INT_SIZE);

  // Find the last block in the bucket
  while (next != -1)
  {
    CALL_BF(BF_UnpinBlock(bucketBlock));
    CALL_BF(BF_GetBlock(fileDesc, next, bucketBlock));
    blockData = BF_Block_GetData(bucketBlock);
    memcpy(&next, blockData, INT_SIZE);
  }

  // Read the count of records in the block
  int count;
  memcpy(&count, blockData + INT_SIZE, INT_SIZE);

  // If the block is full, allocate a new block
  if (count == (BF_BLOCK_SIZE - OFFSET) / sizeof(Record))
  {
    int newBlockNum;
    CALL_BF(BF_GetBlockCounter(fileDesc, &newBlockNum));

    // Add the new block number to the current block
    memcpy(blockData, &newBlockNum, INT_SIZE);
    BF_Block_SetDirty(bucketBlock);
    CALL_BF(BF_UnpinBlock(bucketBlock));

    // Allocate a new block for the record
    CALL_BF(BF_AllocateBlock(fileDesc, bucketBlock));
    CALL_BF(BF_UnpinBlock(bucketBlock));

    // Set the count to 1 in the new block
    blockData = BF_Block_GetData(bucketBlock);
    count = 1;
    memcpy(blockData + INT_SIZE, &count, INT_SIZE);
  }
  else
  {
    // Increment the count and add the record to the block
    count++;
    memcpy(blockData + OFFSET + (count - 1) * sizeof(Record), record, sizeof(Record));
    memcpy(blockData + INT_SIZE, &count, INT_SIZE);
  }

  // Mark the block as dirty and unpin it
  BF_Block_SetDirty(bucketBlock);
  CALL_BF(BF_UnpinBlock(bucketBlock));

  return HT_OK;
}
