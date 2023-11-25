#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> // for debugging

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

// Hash Function
int hash(int id, int buckets){
    return (id * (id+3)) % buckets;
}
int hash2(int id, int buckets){
    return id % buckets;
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

/*
HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) { // mporoume na ths pername k epipleon orismata ???
  
  char* data;
  BF_Block* block;
  HT_info ht_info;
  HT_block_info ht_block_info;
  BF_Block_Init(&block);
  if ((BF_GetBlock(indexDesc, 0, block)) < 0) {
    printf("Error getting block in HT_InsertEntry\n");
    return -1;
  }

  // get pointer to block's 0 data
  data = BF_Block_GetData(block);
  // get the metadata of this block so that we can access ht_info.fileDesc
  memcpy(&ht_info, data, sizeof(HT_info));


  if ((BF_GetBlock(ht_info.fileDesc, 1, block)) < 0) {
    printf("Error getting block in HT_InsertEntry\n");
    return -1;
  }
  // get pointer to block's 1 data
  data = BF_Block_GetData(block);
  memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));
  // printf(" ht_block_info.local_depth = %d\n", ht_block_info.local_depth);
  // printf(" ht_block_info.next_block = %d\n", ht_block_info.next_block);
  // printf(" ht_block_info.num_records = %d\n", ht_block_info.num_records);
  // printf(" ht_block_info.max_records = %d\n", ht_block_info.max_records);

  // if there is enough space for the record
  // // if (ht_block_info.num_records < ht_info->max_records) { // prepei mallon na balw to "global" max records tou bucket/block edw (an nai prepei na to orisw) -> to size opws to lew an 8umamai kala 
  if (ht_block_info.num_records < ht_block_info.max_records) {
    // insert the record in the last position (for records) in the block
    printf(" record.id = %d\n", record.id);
    printf(" record.name = %s\n", record.name);
    printf(" record.surname = %s\n", record.surname);
    printf(" record.city = %s\n", record.city);
    memcpy(data + sizeof(Record) * ht_block_info.num_records, &record, sizeof(Record));

    // this block's data changed, so we update its hp_block_indo
    // maybe more have to change here
    ht_block_info.num_records++;

    // copy the updated hp_info at the end of the block
    memcpy(data + BF_BLOCK_SIZE - sizeof(HT_block_info), &ht_block_info, sizeof(HT_block_info));

    // write the block back on disc
    BF_Block_SetDirty(block);
    if ((BF_UnpinBlock(block)) < 0) {
      printf("Error unpinning block in HT_InsertEntry\n");
      return -1;
    }
  }
  // else {
  //   // there is not enough space
  //   if ((BF_UnpinBlock(block)) < 0) {
  //     printf("Error unpinning block in HT_InsertEntry\n");
  //     return -1;
  //   }

  //   // we need to allocate a new block, so we get out of this If
  //   // new_block_needed = true;
  // }

  BF_Block_Destroy(&block);

  return HT_OK;
}
*/

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  
  char* data;
  BF_Block* block;
  HT_info ht_info;
  HT_block_info ht_block_info;
  BF_Block_Init(&block);
  if ((BF_GetBlock(indexDesc, 0, block)) < 0) {
    printf("Error getting block in HT_InsertEntry\n");
    return -1;
  }

  // get pointer to block's 0 data
  data = BF_Block_GetData(block);
  // get the metadata of this block so that we can access ht_info.fileDesc
  memcpy(&ht_info, data, sizeof(HT_info));

  // In which bucket to insert
  int bucket_to_insert = hash2(record.id, ht_info.buckets);
  if(bucket_to_insert < 0 || bucket_to_insert >= ht_info.buckets)
      return -1;
  assert(bucket_to_insert >= 0 && bucket_to_insert < ht_info.buckets);
  
  
















  if ((BF_GetBlock(ht_info.fileDesc, 1, block)) < 0) {
    printf("Error getting block in HT_InsertEntry\n");
    return -1;
  }
  // get pointer to block's 1 data
  data = BF_Block_GetData(block);
  memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));


  // if there is enough space for the record
  // // if (ht_block_info.num_records < ht_info->max_records /* den einai orismeno akomh */ ) {
  if (ht_block_info.num_records < ht_block_info.max_records) {
    // insert the record in the last position (for records) in the block
    printf(" record.id = %d\n", record.id);
    printf(" record.name = %s\n", record.name);
    printf(" record.surname = %s\n", record.surname);
    printf(" record.city = %s\n", record.city);
    memcpy(data + sizeof(Record) * ht_block_info.num_records, &record, sizeof(Record));

    // this block's data changed, so we update its hp_block_indo
    // maybe more have to change here
    ht_block_info.num_records++;

    // copy the updated hp_info at the end of the block
    memcpy(data + BF_BLOCK_SIZE - sizeof(HT_block_info), &ht_block_info, sizeof(HT_block_info));

    // write the block back on disc
    BF_Block_SetDirty(block);
    if ((BF_UnpinBlock(block)) < 0) {
      printf("Error unpinning block in HT_InsertEntry\n");
      return -1;
    }
  }
  // else {
  //   // there is not enough space
  //   if ((BF_UnpinBlock(block)) < 0) {
  //     printf("Error unpinning block in HT_InsertEntry\n");
  //     return -1;
  //   }

  //   // we need to allocate a new block, so we get out of this If
  //   // new_block_needed = true;
  // }

  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {

  void* data;
  BF_Block* block;
  BF_Block_Init(&block);
  HT_info ht_info;
  HT_block_info ht_block_info;
  Record record;

  if ((BF_GetBlock(indexDesc, 0, block)) < 0) {
    printf("Error getting block in HT_InsertEntry\n");
    return -1;
  }

  // get pointer to block's 0 data
  data = BF_Block_GetData(block);
  // get the metadata of this block so that we can access ht_info.fileDesc
  memcpy(&ht_info, data, sizeof(HT_info));

  // Firstly get the total num of blocks in heap file
  int total_blocks;
  if ((BF_GetBlockCounter(ht_info.fileDesc, &total_blocks)) < 0) {
    printf("Error getting num of blocks in HT_InsertEntry\n");
    return -1;
  }

  // Unpin block 0 since we dont need it anymore
  if ((BF_UnpinBlock(block)) < 0) {
    printf("Error unpinning block in HT_InsertEntry\n");
    return -1;
  }

  // for (int i=1 ; i<total_blocks ; i++) {
  for (int i=1 ; i<2 ; i++) {
    // Bring this block to memory
    if ((BF_GetBlock(ht_info.fileDesc, i, block)) < 0) {
      return -1;
    }

    // get pointer to this block's data
    data = BF_Block_GetData(block);

    // get the metadata of this last block
    memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));

    // for every record in this block
    for(int k = 0 ; k < ht_block_info.num_records * sizeof(Record) ; k = k + sizeof(Record)) {      
      // copy in our variable record, the contents of the current record we are reading
      memcpy(&record, data + k, sizeof(Record));

      // check if it's the one we are looking for
      // printf("record.id = %d\n", record.id);
      // printf("id = %d\n", id);
      if (id != NULL) {
        if (record.id == *id) {
          // we store as "number of total blocks read" the numbber of the block we just found the last occurance of the id we are searching for
          // num_blocks_read = i;
          // we print it each time it occures
          printf("Record in offset %d in Block %d: id=%d, name=%s, surname=%s, city=%s\n", k, i, record.id, record.name, record.surname, record.city);
        }
      }
      else {
        printf("Record in offset %d in Block %d: id=%d, name=%s, surname=%s, city=%s\n", k, i, record.id, record.name, record.surname, record.city);
      }
    }

    // Write babck on disk the block we just read
    if ((BF_UnpinBlock(block)) < 0) {
      printf("Error unpinning block in HT_GetAllEntries\n");
      return -1;
    }
  }

  BF_Block_Destroy(&block);

  return HT_OK;
}

