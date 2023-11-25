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

int check_open_files() 
{
  int i;

  for (i = 0; i < MAX_OPEN_FILES; i++) {
    if (HT_table[i] == NULL) break;
  }
  if (i == MAX_OPEN_FILES){
    printf("Open files are at maximum - more files can't be opened");
    return HT_ERROR;
  }
  return HT_OK;
}

int DirtyUnpin(BF_Block* block) {
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
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

HT_ErrorCode HT_CreateIndex(const char *filename, int depth)    //we don't check for max open files beacause we can create
{                                                               //as many as we want, but we can only have 20 open
  HT_info ht_info;
  BF_Block* block;
  BF_Block* ht_block;
  BF_Block* next_ht_block;
  void* data;
  int file_desc, N, required_blocks, i, curr_id;

  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &ht_info.fileDesc));

  // META DATA BLOCK --> first

  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(ht_info.fileDesc, block));

  data = BF_Block_GetData(block);

  // na doume analoga me ta structs 
  ht_info.is_ht = true;
  ht_info.global_depth = depth;
  ht_info.ht_id = -1;
  ht_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record); // floor? --> bfr
  ht_info.max_ht = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(HT_block_info); // mallon????

  // HASH TABLE BLOCK --> second
  /*----------------------------------------------------------------------------------------------------------------------------*/

  BF_Block_Init(&ht_block);
  CALL_BF(BF_AllocateBlock(ht_info.fileDesc, ht_block));

  // number 2 will be saved in ht_id (ht block is 2nd block, ht_id is the id of this block)
  CALL_BF(BF_GetBlockCounter(ht_info.fileDesc, &ht_info.ht_id));

  // initialize ht block
  data = BF_Block_GetData(ht_block);
  memcpy(data, &ht_info.ht_id, sizeof(int)); //save ht_id in data
  // ---

  // Hash Table can be stored in multiple blocks --> Create & Initialize more if needed

  N = pow(2, ht_info.global_depth); // 2^depth --> number of entries

  required_blocks = ceil(N / ht_info.max_records); // number of blocks we need for hash table

  if (required_blocks > 1){     //  if we need more blocks (we already have 1 ht block)
    BF_Block_Init(&next_ht_block);
    for(i=1; i<required_blocks; i++){
      CALL_BF(BF_AllocateBlock(file_desc, next_ht_block));

      // Get number (id) of the new block
      CALL_BF(BF_GetBlockCounter(ht_info.fileDesc, &curr_id));
      memcpy(data, &curr_id, sizeof(int));

      //initialize ht block
      data = BF_Block_GetData(next_ht_block);
      //memcpy gia max_ht --> loop apo 0 ews max_ht ++ alla ti mpainei sthn memcpy???

      DirtyUnpin(next_ht_block);

      ht_block = next_ht_block; // h next_ht_block = ht_block???
    }
  }

  /*----------------------------------------------------------------------------------------------------------------------------*/

  // Write the meta-data to the first block
  memcpy(data, &ht_info, sizeof(HT_info));

  // Set blocks as dirty & unpin them, so that they are saved in the disc
  DirtyUnpin(block);
  DirtyUnpin(ht_block);

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
  if (check_open_files()== HT_ERROR) return HT_ERROR;

  HT_info *ht_info;
  BF_Block *block;
  void* data;

  CALL_BF(BF_OpenFile(fileName, indexDesc));

  BF_Block_Init(&block);

  CALL_BF(BF_GetBlock(*indexDesc, 0, block));

  // Find empty place and write the file's data
  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (HT_table[i] == NULL)
    {
      HT_table[i] = (HT_info*)malloc(sizeof(HT_info));
      data = BF_Block_GetData(block);
      ht_info = data;
      break;
    }
  }

  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc)
{

  CALL_BF(BF_CloseFile(indexDesc));

  free(HT_table[indexDesc]);

  HT_table[indexDesc] = NULL; 

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
