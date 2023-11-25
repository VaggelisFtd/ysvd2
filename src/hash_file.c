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

  // We are done with info from block 0, so we can unpin it now?
  // here?
  // check in IF if we unpin it more than once -> makes Seg!!!

  // In which bucket to insert
  int bucket_to_insert = hash2(record.id, ht_info.num_buckets);
  if(bucket_to_insert < 0 || bucket_to_insert >= ht_info.num_buckets)
      return -1;
  assert(bucket_to_insert >= 0 && bucket_to_insert < ht_info.num_buckets);
  
  // Check if bucket has enough space for the Record
  int target_block_id = ht_info.ht_array[bucket_to_insert];
  // Pin target block
  if (BF_GetBlock(indexDesc, target_block_id, block) < 0) {
    printf("Error getting block in HT_InsertEntry\n");
    return -1;
  }
  // get pointer to target blocks data
  data = BF_Block_GetData(block);
  // copy its data to our local var
  memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));
  // Check if there is room to add the record
  // if (ht_block_info.local_depth < ht_info.global_depth) {    // maybe we MUST use this one - or are depths just for pointers and not space in a block?
  if (ht_block_info.num_records < ht_block_info.max_records) {
    
    printf(" writing id:%d on block:%d\n", record.id, target_block_id);
    // insert the record in the last position of records in the block
    memcpy(data + sizeof(Record) * ht_block_info.num_records, &record, sizeof(Record));

    // this block's data changed, so we update its ht_block_info
    ht_block_info.num_records++;

    // copy the updated ht_info at the end of the block
    memcpy(data + BF_BLOCK_SIZE - sizeof(HT_block_info), &ht_block_info, sizeof(HT_block_info));

    // write the block back on disc
    if (Check(DirtyUnpin(block)) < 0) {
      printf("Error unpinning block in HT_InsertEntry\n");
      return -1;
    }

    return HT_OK;
  }
  else {
    // if there is not enough space in tthe target block
    
    // kapou edw na kanw k unpin to gemato target block
    // prepei na ginei edw H pio katw? (h ka8olou, pou de nomizw)
    // Firstly we write back in memory the full target_block
    if ((BF_UnpinBlock(block)) < 0) {
      printf("Error unpinning block in HT_InsertEntry\n");
      return -1;
    }

    // We allocate a new block
    if (Check(BF_AllocateBlock(ht_info.fileDesc, block)) < 0) {
      printf("Error allocating block in HT_InsertEntry\n");
      return -1;
    }

    // Here we have: target_block_id and new_block_id
    // We get new blocks id
    int new_block_id;
    if ((BF_GetBlockCounter(ht_info.fileDesc, &new_block_id)) < 0) {
      printf("Error getting num of blocks in HT_InsertEntry\n");
      return -1;
    }
    new_block_id--; // we subtract block with id: 0, containing ht_info

    // update ht_info's total block/buckets number
    ht_info.num_buckets++;
    
    // Check if more than 1 ht_array indexes point to the FULL BLOCK
    // if(ht_block_info.local_depth < ht_info.global_depth) {
    if((ht_block_info.local_depth < ht_info.global_depth) && (ht_block_info.indexes_pointed_by > 1)) { // xreiazetai o extra elegxos h einai akrivws to idio pragma???
      // Make the CURRENT POINTER point to the newly allocated block (keeping all other "indexes_pointed_by" pointers pointing to same block as before)
      ht_info.ht_array[bucket_to_insert] = new_block_id;

      // For each already existing record in FULL BLOCK + Record_to_insert -> HT_InsertEntry(...) (hashing with 1 more bucket this time)
      for(int k = 0 ; k < ht_block_info.num_records * sizeof(Record) ; k = k + sizeof(Record)) {
        Record temp_record;      
        // copy in our variable temp_record, the contents of the current record we are reading
        memcpy(&temp_record, data + k, sizeof(Record));
        if (!HT_InsertEntry(ht_info.fileDesc, temp_record))
          return HT_ERROR;
      }
      
      // Now once more for the Record_to_insert as we said above
      if (!HT_InsertEntry(ht_info.fileDesc, record))
        return HT_ERROR;
      
      // mhpws ola ta parakatw prepei na ginoun prin ta Insert() ???????? ======================== SOS =============================

      // Increase FULL_BLOCKs local_depth 
      ht_block_info.local_depth++; 

      HT_block_info new_block_ht_block_info;
      new_block_ht_block_info.indexes_pointed_by = 1;                   // just this one pointer we just re-routed to point to this new block
      new_block_ht_block_info.local_depth = ht_block_info.local_depth;  // the increased local depth just above
      new_block_ht_block_info.max_records = ht_block_info.max_records;  // same const value
      new_block_ht_block_info.next_block = 0;                           // we should not handle overflows anyway
      new_block_ht_block_info.num_records = 0;                          // still 0 records inside, it will be increased after calling HT_Insert below 
      
      // get pointer to new blocks data
      data = BF_Block_GetData(block);
      
      // copy the updated ht_info at the end of the newly allocated block
      memcpy(data + BF_BLOCK_SIZE - sizeof(HT_block_info), &new_block_ht_block_info, sizeof(HT_block_info));
      
    }
    // If exactly 1 ht_array index points to the FULL BLOCK
    // else if ((ht_block_info.local_depth == ht_info.global_depth) && (ht_block_info.indexes_pointed_by == 1)) { // xreiazetai o extra elegxos h einai akrivws to idio pragma???
    else if (ht_block_info.local_depth == ht_info.global_depth) {
      int old_ht_array_size = ht_info.num_buckets;
      int new_ht_array_size = 2 * old_ht_array_size;

      // Make sure it is stored correctly updating blocks containing the ht_array using ht_info (next_block ect.)
      // ...

      // explaining comment
      for (int i=old_ht_array_size ; i < new_ht_array_size ; i++) {
        // ara o 1os kainourios pointer deixnei ekei pou edeixne o pointer 0, o 2os ekei pou edeixne o 1 klp, opws k to deixnei stis diafaneies
        ht_info.ht_array[i] = ht_info.ht_array[i - old_ht_array_size];

        char* temp_data;
        HT_block_info temp_ht_block_info;
        BF_Block* temp_block;
        BF_Block_Init(temp_block);
        if (BF_GetBlock(indexDesc, ht_info.ht_array[i], temp_block) < 0) {
          printf("Error getting temp_block in HT_InsertEntry\n");
          return -1;
        }
        temp_data = BF_Block_GetData(temp_block);
        memcpy(&temp_ht_block_info, temp_data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));
        // update each time the ht_block_info.indexes_pointed_by of this temp_block
        temp_ht_block_info.indexes_pointed_by++;
        memcpy(temp_data + BF_BLOCK_SIZE - sizeof(HT_block_info), &temp_ht_block_info, sizeof(HT_block_info));

        if ((BF_UnpinBlock(temp_block)) < 0) {
          printf("Error unpinning temp_block in HT_InsertEntry\n");
          return -1;
        }

        BF_Block_Destroy(&temp_block);
        temp_data = NULL; // xreiazetai?
      }

      // Update global depth
      ht_info.global_depth++;

      // Is there anything else to update ???

      // For each already existing record in FULL BLOCK + Record_to_insert -> HT_InsertEntry(...) (hashing with 1 more bucket this time)
      for(int k = 0 ; k < ht_block_info.num_records * sizeof(Record) ; k = k + sizeof(Record)) {
        Record temp_record;      
        // copy in our variable temp_record, the contents of the current record we are reading
        memcpy(&temp_record, data + k, sizeof(Record));
        if (!HT_InsertEntry(ht_info.fileDesc, temp_record))
          return HT_ERROR;
      }
      
      // Now once more for the Record_to_insert as we said above
      if (!HT_InsertEntry(ht_info.fileDesc, record))
        return HT_ERROR;

    }

    // Is there anything else to update ???
  }















  // if ((BF_GetBlock(ht_info.fileDesc, 1, block)) < 0) {
  //   printf("Error getting block in HT_InsertEntry\n");
  //   return -1;
  // }
  // // get pointer to block's 1 data
  // data = BF_Block_GetData(block);
  // memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));


  // // if there is enough space for the record
  // // // if (ht_block_info.num_records < ht_info->max_records /* den einai orismeno akomh */ ) {
  // if (ht_block_info.num_records < ht_block_info.max_records) {
  //   // insert the record in the last position (for records) in the block
  //   printf(" record.id = %d\n", record.id);
  //   printf(" record.name = %s\n", record.name);
  //   printf(" record.surname = %s\n", record.surname);
  //   printf(" record.city = %s\n", record.city);
  //   memcpy(data + sizeof(Record) * ht_block_info.num_records, &record, sizeof(Record));

  //   // this block's data changed, so we update its hp_block_indo
  //   // maybe more have to change here
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

  // sto telos (h k sthn arxh, na tto dokimasw) na kanw unpin k to block 0 pou xrhsimopoioume se olh th sunarthsh
  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {

  // void* data;
  char* data;
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

