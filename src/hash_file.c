#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }

static int file_count=0;
HT_info *HT_table[MAX_OPEN_FILES]; // hash table for open files

// int hash_function(int id, int buckets) // hash function
// {
//   return id % buckets;
// }

// Hash Function
int hash(int id, int buckets){
    return (id * (id+3)) % buckets;
}
int hash2(int id, int buckets){
    return id % buckets;
}

HT_ErrorCode HT_Init()
{
  CALL_BF(BF_Init(LRU));

  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    HT_table[i] = NULL;
  }

  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth)    //we don't check for max open files, we can create as many as we want, but we can only have 20 open
{
  HT_info ht_info;
  BF_Block* block;
  BF_Block* ht_block;
  BF_Block* next_ht_block;
  void* data;
  int file_desc, N, required_blocks, i;

  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, ht_info.fileDesc));

  // META DATA BLOCK --> first

  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(ht_info.fileDesc, block));

  data = BF_Block_GetData(block);

  // na doume analoga me ta structs 
  ht_info.is_ht = true;
  ht_info.global_depth = depth;
  ht_info.ht_id = -1;
  ht_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record); // floor? --> bfr
  ht_info.max_HT = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(HT_block_info); // mallon????

  // HASH TABLE BLOCK --> second
  /*----------------------------------------------------------------------------------------------------------------------------*/

  BF_Block_Init(&ht_block);
  CALL_BF(BF_AllocateBlock(file_desc, ht_block));

  ////??????????????

  N = pow(2, ht_info.global_depth); // 2^depth --> number of entries

  data = BF_Block_GetData(ht_block);

  memcpy(data, &ht_info.ht_id, sizeof(int));
  // for(i=0;i<ht_info.max_records;i++)
  //  memcpy

  // Hash Table can be stored in multiple blocks --> Create & Initialize more if needed
  required_blocks = ceil(N / ht_info.max_records);

  if (required_blocks > 1){     //if we need more blocks (we already have 1 ht block)
    BF_Block_Init(&next_ht_block);
    for(i=1; i<required_blocks; i++){
      CALL_BF(BF_AllocateBlock(file_desc, next_ht_block));

      // memcpy

      //initialize ht block
      data = BF_Block_GetData(next_ht_block);
      //memcpy

      BF_Block_SetDirty(next_ht_block);
      CALL_BF(BF_UnpinBlock(next_ht_block));

      next_ht_block = ht_block;
    }
  }

  /*----------------------------------------------------------------------------------------------------------------------------*/

  // Write meta-data to meta-data block (1st)
  memcpy(data, &ht_info, sizeof(HT_info));

  // Set blocks as dirty & unpin them, so that they are saved in disc
  BF_Block_SetDirty(block);
  BF_Block_SetDirty(ht_block);
  CALL_BF(BF_UnpinBlock(block));
  CALL_BF(BF_UnpinBlock(ht_block));

  // Call Destroy to free the memory
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&ht_block);

  // Close the file
  CALL_BF(BF_CloseFile(ht_info.fileDesc));

  //file_count++; // edw h sthn open?????

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc)
{

  // Open files are at maximum - we can't open more
  if (file_count == MAX_OPEN_FILES)
  {
    return HT_ERROR;
  }

  HT_info *ht_info;
  BF_Block *block;
  // void* data;

  CALL_BF(BF_OpenFile(fileName, indexDesc));

  BF_Block_Init(&block);

  CALL_BF(BF_GetBlock(*indexDesc, 0, block));

  // vres thesi k vale to data

  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (HT_table[i] == NULL)
    {
      // thelei malloc??????
      HT_table[i] = (HT_info*)malloc(sizeof(HT_info));
      ht_info = BF_Block_GetData(block); // prin: ht_info = data;
      // memcpy?
      *indexDesc = i;
      break;
    }
    if (i == MAX_OPEN_FILES)
    { // edw h sthn create????
      return HT_ERROR;
    }
  }

  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  file_count++;

  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc)
{

  CALL_BF(BF_CloseFile(indexDesc));

  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record)
{
  // insert code here
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id)
{
  // insert code here
  return HT_OK;
}
