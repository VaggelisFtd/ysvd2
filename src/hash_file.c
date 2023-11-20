#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HT_ERROR;        \
  }                         \
}

//static int file_count;
HT_info* HT_table[MAX_OPEN_FILES];      // hash table for open files

// hash function


HT_ErrorCode HT_Init() {
  
  int i;

  CALL_BF(BF_Init(LRU));

  for (i=0; i<MAX_OPEN_FILES; i++) {
    HT_table[i] = NULL;
  }

  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {

  // Open files are at maximum - we can't create more
  //if (file_count == MAX_OPEN_FILES) { return HT_ERROR; } //den yparxei kenh thesh

  HT_info ht_info;
  BF_Block *block;
  void *data;

  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, ht_info.fileDesc));

  // META DATA BLOCK

  // First block has HT_info --> Initialize
  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(ht_info.fileDesc, block));
  data = BF_Block_GetData(block);

  ht_info.is_ht = true;
  ht_info.global_depth = depth; 

  // Copy struct HP_info in the meta-data block (first)
  memcpy(block, &ht_info, sizeof(HT_info));
  
  // Make it dirty and unpin so that it is saved in disc
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  // HASH TABLE BLOCK

  // Determine blocks required for ht

  //allocate first block for ht
  // BF_AllocateBlock


  // Call Destroy after calling Init to free the memory
  BF_Block_Destroy(&block);

  // Close the file
  CALL_BF(BF_CloseFile(ht_info.fileDesc));
  
  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}
