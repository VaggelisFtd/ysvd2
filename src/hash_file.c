#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "bf.h"
#include "hash_file.h"
// #define MAX_OPEN_FILES 20

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }

// HT_info *Hash_table[MAX_OPEN_FILES]; // hash table for open files

// Hash Function
int hash(int id, int buckets)
{
  return (id * (id + 3)) % buckets; // ask vaggelis
}
int hash2(int id, int buckets)
{
  return id % buckets;
}

int check_open_files()
{
  int i;

  for (i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (Hash_table[i] == NULL)
      break;
  }
  if (i == MAX_OPEN_FILES)
  {
    printf("Open files are at maximum - more files can't be opened");
    return HT_ERROR;
  }
  return HT_OK;
}

int DirtyUnpin(BF_Block *block)
{
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
}

HT_ErrorCode HT_Init()
{
  // maybe fileCount = 0; ?
  CALL_BF(BF_Init(LRU));
  for (int i = 0; i < MAX_OPEN_FILES; i++)
    Hash_table[i] = NULL;

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

  /*initialization*/
  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &ht_info.fileDesc));

  /* 1st block: metadata */
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
    for (i = 1; i < required_blocks; i++)
    {
      CALL_BF(BF_AllocateBlock(file_desc, next_ht_block));
      CALL_BF(BF_GetBlockCounter(ht_info.fileDesc, &curr_id)); // Get number (id) of the new block

      memcpy(data, &curr_id, sizeof(int));
      data = BF_Block_GetData(next_ht_block); // initialize ht block

      // memcpy(ht_info.max_ht)    // memcpy gia max_ht --> loop apo 0 ews max_ht ++ alla ti mpainei sthn memcpy???
      DirtyUnpin(next_ht_block);
      ht_block = next_ht_block; // h next_ht_block = ht_block???
    }
  }
  memcpy(data, &ht_info, sizeof(HT_info)); // Write the meta-data to the first block

  /* Set blocks as dirty & unpin them, so that they are saved in the disc */
  DirtyUnpin(block);
  DirtyUnpin(ht_block);

  /* Call Destroy to free the memory */
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&ht_block);

  CALL_BF(BF_CloseFile(ht_info.fileDesc)); // Close the file

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc)
{
  /* Open files are at maximum - we can't open more */
  if (check_open_files() == HT_ERROR)
    return HT_ERROR;

  HT_info *ht_info;
  BF_Block *block;
  void *data;

  CALL_BF(BF_OpenFile(fileName, indexDesc));
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(*indexDesc, 0, block));

  /* Find empty place and write the file's data */
  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (Hash_table[i] == NULL)
    {
      Hash_table[i] = (HT_info *)malloc(sizeof(HT_info));
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
  /*Closing open files and free them*/
  CALL_BF(BF_CloseFile(indexDesc));
  free(Hash_table[indexDesc]);
  Hash_table[indexDesc] = NULL;

  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record)
{
  /*V1
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
  // sto telos (h k sthn arxh, na tto dokimasw) na kanw unpin k to block 0 pou xrhsimopoioume se olh th sunarthsh
  BF_Block_Destroy(&block);
  return HT_OK;
}
*/

  /*V2 */
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id)
{
  // insert code here
  return HT_OK;
}
