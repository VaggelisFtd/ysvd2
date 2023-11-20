#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "../include/hash_file.h"

#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

HT_ErrorCode HT_Init() {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  //insert code here
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

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record/*, HT_info *ht_info */) { // mporoume na ths pername k epipleon orismata
  
  char* data;
  BF_Block* block;
  HT_block_info ht_block_info;
  // printf(" =========== \n");
  // if ((BF_GetBlock(indexDesc, 1, block)) < 0) {
  //   printf("Error getting block in HT_InsertEntry\n");
  //   return -1;
  // }
  // // get pointer to block's 1 data
  // printf(" =========== \n");
  // data = BF_Block_GetData(block);
  // // get the metadata of this block
  // printf(" =========== \n");
  // memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));
  // printf(" =========== \n");
  // // if there is enough space for the record
  // // if (ht_block_info.num_records < ht_info->max_records) { /*prepei mallon na balw to max records tou bucket/block edw -> to size opws to lew an 8umamai kala */
  // if (ht_block_info.num_records < ht_block_info.max_records) {
  //   // insert the record in the last position (for records) in the block
  //   memcpy(data + sizeof(Record) * ht_block_info.num_records, &record, sizeof(Record));

  //   // this block's data changed, so we update its hp_block_indo
  //   ht_block_info.num_records++;

  //   // copy the updated hp_info at the end of the block
  //   memcpy(data + BF_BLOCK_SIZE - sizeof(HT_block_info), &ht_block_info, sizeof(HT_block_info));

  //   // write the block back on disc
  //   BF_Block_SetDirty(block);
  //   if ((BF_UnpinBlock(block)) < 0) {
  //     printf("Error unpinning block in HT_InsertEntry\n");
  //     return -1;
  //   }
  // }
  // else {
  //   // there is not enough space
  //   if ((BF_UnpinBlock(block)) < 0) {
  //     printf("Error unpinning block in HT_InsertEntry\n");
  //     return -1;
  //   }

  //   // we need to allocate a new block, so we get out of this If
  //   // new_block_needed = true;
  // }

  // BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

