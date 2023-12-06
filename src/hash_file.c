#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>	// for log2()
#include <assert.h> // for debugging

#include "bf.h"
#include "../include/hash_file.h"

#define BITS sizeof(int) * 8

#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
	BF_PrintError(code);    \
	return HT_ERROR;        \
  }                         \
}

int *hash_table[MAX_OPEN_FILES]; 

//HT_info *hash_table[MAX_OPEN_FILES]; // hash table for open files

void intToBinary(HT_info ht_info, int* binary, int var) {	
	// Loop through each bit
	int* result = malloc(BITS * sizeof(int));

	for (int i = ht_info.global_depth ; i >= 0 ; i--) {
		result[i] = var & 1; // Get the least significant bit
		
		// Right shift to check the next bit
		var = var >> 1;
	}

	// for (int i = 0; i < ht_info.global_depth+1 ; i++) {
	// 	printf(" %d", result[i]);
	// }
	// printf("\n");

	for (int i = 0; i < ht_info.global_depth ; i++) {
        binary[i] = result[i];
    }
	
	free(result);
}

int binaryToInt(int* binary, int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
        result = result * 2 + binary[i];
    }
    return result;
}

// Hash Function
int hash(int id, int buckets){
	return (id * (id+3)) % buckets;
}
int hash2(int id, int buckets){
	// printf(" id = %d \n", id);
	// printf(" buckets = %d \n", buckets);
	// printf(" id mod buckets = %d\n", id % buckets);
	return id % buckets;
}

int DirtyUnpin(BF_Block *block)
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

HT_ErrorCode HT_Init()
{
  for (int i = 0; i < MAX_OPEN_FILES; i++)
    hash_table[i] = NULL;

  return HT_OK;
}

/*we don't check for max open files beacause we can create, as many as we want, but we can only have 20 open*/
HT_ErrorCode HT_CreateIndex(const char *filename, int depth)
{
  HT_info ht_info;
  HT_block_info ht_block_info;
  BF_Block* block;
  void* data;
  int fd;
  int N;
  int required_blocks;
  int i;

  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &ht_info.fileDesc));

  // META DATA BLOCK --> first

  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(ht_info.fileDesc, block));

  data = BF_Block_GetData(block);

  ht_info.is_ht = true;
  ht_info.global_depth = depth;

  N = pow(2, ht_info.global_depth); // 2^depth --> number of entries
  
  ht_info.ht_array_head = -1;
  ht_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
  ht_info.ht_array_size = N;
  ht_info.ht_array = malloc(ht_info.ht_array_size*sizeof(int));
  ht_info.ht_array_length = 1;
  ht_info.num_blocks = required_blocks+1;
  for(i=0; i<ht_info.ht_array_size; i++){
    ht_info.ht_array[i] = -1;
  }
  //ht_info.max_ht = (BF_BLOCK_SIZE - sizeof(int))/sizeof(int);   //! size of ht info?

  memcpy(block, &ht_info, sizeof(HT_info));

  // HASH TABLE BLOCK --> second

  required_blocks = ceil(N / ht_info.max_records); // number of blocks we need for hash table

  //printf("required blocks for hash table: %d\n", required_blocks);

  // for (i = 0; i < required_blocks; i++)
  // {
  //   BF_Block *next_block;
  //   BF_Block_Init(&next_block);
  //   CALL_BF(BF_AllocateBlock(fd, next_block));
  //   if (i==0) // save hash table's first block id
  //   {
  //     CALL_BF(BF_GetBlockCounter(ht_info.fileDesc, &ht_info.ht_array_head)); // this gives 1
  //     ht_info.ht_array_head--;  // but id is 0
  //   }
  //   data = BF_Block_GetData(next_block);
  //   memcpy(data, &ht_info, sizeof(HT_info));

  //   DirtyUnpin(next_block);

  //   BF_Block_Destroy(&next_block);
  // }

  //last initializations in HT_info
  //ht_info.ht_array_size = i;
  //ht_info.ht_array_length = required_blocks;


  DirtyUnpin(block);

  for(i=0; i<2; i++) 
  {
    CALL_BF(BF_AllocateBlock(fd, block));
    data = BF_Block_GetData(block);
    ht_block_info.num_records = 0;
    ht_block_info.local_depth = 1;
    ht_block_info.max_records = MAX_RECORDS;  //!
    ht_block_info.next_block = 0;
    ht_block_info.indexes_pointed_by = 0;
    memcpy(data, &ht_block_info, sizeof(HT_block_info));

    DirtyUnpin(block);
  }

  //print
  // for(i=0; i<N/2; i++)
  // {
    
  // }
  //print misa index sto block 1 misa 2

  BF_Block_Destroy(&block);

  //CALL_BF(BF_CloseFile(ht_info.fileDesc)); den doulevei-->giati? afou kaloume open

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc)
  {
    /* Open files are at maximum - we can't open more */
    if (checkOpenFiles() == HT_ERROR)
      return HT_ERROR;

    HT_info *ht_info;
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
        //hash_table[i] = (openedIndex *)malloc(sizeof(openedIndex));
        hash_table[i] = (int *)malloc(sizeof(int));
        if (hash_table[i] == NULL) 
        {
          perror("malloc");
          return -1;
        }
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
  free(hash_table[indexDesc]);
  hash_table[indexDesc] = NULL; 
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

	// BF_Init(LRU);
  
	int step = 2; // used to determing how many and which pointers to redirect after ht_array doubling

	char* head_data;
	char* target_data;
	HT_info ht_info;
	HT_block_info ht_block_info;
	BF_Block* head_block;
	BF_Block* target_block;
	BF_Block_Init(&head_block);
	BF_Block_Init(&target_block);
	if ((BF_GetBlock(indexDesc, 0, head_block)) < 0) {
		printf("Error getting block in HT_InsertEntry\n");
		return -1;
	}

	// get pointer to block's 0 data
	head_data = BF_Block_GetData(head_block);
	// get the metadata of this head_block so that we can access ht_info.fileDesc
	memcpy(&ht_info, head_data, sizeof(HT_info));
	for(int j=0 ; j < ht_info.ht_array_size ; j++)
		printf("ht_info.ht_array[%d] = %d\n", j, ht_info.ht_array[j]);

	// We are done with info from block 0, so we can unpin it now?
	// here?
	// check in IF if we unpin it more than once -> makes Seg!!!

	// In which bucket to insert
	int bucket_to_insert = hash2(record.id, ht_info.ht_array_size);
	if(bucket_to_insert < 0 || bucket_to_insert >= ht_info.ht_array_size) {
		return -1;
	}
	assert(bucket_to_insert >= 0 && bucket_to_insert < ht_info.ht_array_size);
	
	// Check if bucket has enough space for the Record
	int target_block_id = ht_info.ht_array[bucket_to_insert];
	printf("\n Trying to insert record id: %d in target_block [%d] -> %d\n", record.id, bucket_to_insert, target_block_id);
	// Pin target_block
	if (BF_GetBlock(indexDesc, target_block_id, target_block) < 0) {
		printf("Error getting target_block in HT_InsertEntry\n");
		return -1;
	}
	// get pointer to target_block's data
	target_data = BF_Block_GetData(target_block);
	// copy its data to our local var
	memcpy(&ht_block_info, target_data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));
	// Check if there is room to add the record
	printf(" ht_block_info.num_records = %d, ht_block_info.max_records %d\n", ht_block_info.num_records, ht_block_info.max_records);
	if (ht_block_info.num_records < ht_block_info.max_records) {
		
		printf(" writing record id: %d on block:%d\n", record.id, target_block_id);
		// insert the record in the last position of records in the block
		memcpy(target_data + sizeof(Record) * ht_block_info.num_records, &record, sizeof(Record));

		// this block's data changed, so we update its ht_block_info
		ht_block_info.num_records++;
		printf(" ht_block_info.num_records new: %d\n", ht_block_info.num_records);

		// ht_array is updated - already checked with prints

		// copy the updated ht_block_info at the end of the block
		memcpy(target_data + BF_BLOCK_SIZE - sizeof(HT_block_info), &ht_block_info, sizeof(HT_block_info));

		// write the block back on disc
		BF_Block_SetDirty(target_block);
		BF_UnpinBlock(target_block);

		// target_data = NULL; // xreiazetai?
		BF_Block_Destroy(&head_block);
		BF_Block_Destroy(&target_block);

		return HT_OK;
	}
	else {
		// if there is not enough space in tthe target block
		printf(" =====PLEON DEN XWRANE RECORDS STO BLOCK %d======== \n", target_block_id);
		
		// Do we need the headblock anymore???
		// if ((BF_UnpinBlock(head_block)) < 0) {
		//   printf("Error unpinning block in HT_InsertEntry\n");
		//   return -1;
		// }

		// Till this line, target_block points to hashed target_block
		// So, now we create a new block* so that we can track both old-target-block and new-inserted-block

		// char* new_data;
		// BF_Block* new_block;
		// BF_Block_Init(&new_block);
		// int new_block_id;

		// HT_block_info new_ht_block_info;

		// Array of 8 records to store the old ones i have to re-insert
		Record copy_records[8];
		// Up to 8 records will be stored, so i init IDs as -1 at first to indicate the end of the valid records, when id = -1 is reached
		// We will use ht_block_info.num_records anyway, but -1 is an extra safety
		for(int i=0 ; i<8 ; i++) {
			copy_records[i].id = -1;
		}


		// // We allocate a new block
		// if (BF_AllocateBlock(ht_info.fileDesc, new_block) < 0) {
		// printf("Error allocating block in HT_InsertEntry\n");
		// return -1;
		// }

		// // Here we have: target_block_id and new_block_id
		// // We get new blocks id
		// if ((BF_GetBlockCounter(ht_info.fileDesc, &new_block_id)) < 0) {
		// printf("Error getting num of blocks in HT_InsertEntry\n");
		// return -1;
		// }
		// new_block_id--; // we "subtract" block with id: 0, containing ht_info
		// printf(" new_block_id = %d \n", new_block_id);

		// new_ht_block_info.num_records = 0;
		// new_ht_block_info.local_depth = ht_block_info.local_depth;
		// new_ht_block_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
		// new_ht_block_info.next_block = 0;
		// new_ht_block_info.indexes_pointed_by = 0; // will be updated/increase later (otan kanw ++)
		// // new_ht_block_info.indexes_pointed_by = ht_block_info.indexes_pointed_by; // auto mporei na ginei updated later an kanw ++ h 8a einai la8os???

		// new_data = BF_Block_GetData(new_block);

		// memcpy(new_data + BF_BLOCK_SIZE - sizeof(HT_block_info), &new_ht_block_info, sizeof(HT_block_info));
		// BF_Block_SetDirty(new_block);


		// Here we have: target_block_id, new_block_id

		// update ht_info's total block/buckets number (+2 allocated blocks -1 full block we will destroy (to re-insert its Records with new hash-func))
		ht_info.num_blocks++;
		// paizei na mhn xreiazetai kan - afou exoume thn GetBlockCounter()
		printf(" ht_info.num_blocks = %d \n", ht_info.num_blocks);
		// memcpy(data, &ht_info, sizeof(HT_info)); // ginetai mesa sto if + (na sumplhrwsw an ginetai allou wste na xerw
		// BF_Block_SetDirty(head_block);			// oti den xreiazetai se ka8e kladi twn ifs)

		// At this point ht_block_info contains target_block's info
		// Check if more than 1 ht_array indexes point to the FULL BLOCK
		printf("ht_block_info.local_depth = %d\n", ht_block_info.local_depth);
		printf("ht_info.global_depth = %d\n", ht_info.global_depth);
		if((ht_block_info.local_depth < ht_info.global_depth) || (ht_block_info.indexes_pointed_by > 1)) {
		// if(ht_block_info.local_depth < ht_info.global_depth) {



			char* new_data;
      BF_Block* new_block;
      BF_Block_Init(&new_block);
      int new_block_id;

      HT_block_info new_ht_block_info;

      // We allocate a new block
      if (BF_AllocateBlock(ht_info.fileDesc, new_block) < 0) {
      printf("Error allocating block in HT_InsertEntry\n");
      return -1;
      }

      // Here we have: target_block_id and new_block_id
      // We get new blocks id
      if ((BF_GetBlockCounter(ht_info.fileDesc, &new_block_id)) < 0) {
      printf("Error getting num of blocks in HT_InsertEntry\n");
      return -1;
      }
      new_block_id--; // we "subtract" block with id: 0, containing ht_info
      printf(" new_block_id = %d \n", new_block_id);
      
      new_ht_block_info.num_records = 0;
      new_ht_block_info.local_depth = ht_block_info.local_depth;
      new_ht_block_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
      new_ht_block_info.next_block = 0;
      new_ht_block_info.indexes_pointed_by = 0; // will be updated/increase later (otan kanw ++)
      // new_ht_block_info.indexes_pointed_by = ht_block_info.indexes_pointed_by; // auto mporei na ginei updated later an kanw ++ h 8a einai la8os???

      new_data = BF_Block_GetData(new_block);

      memcpy(new_data + BF_BLOCK_SIZE - sizeof(HT_block_info), &new_ht_block_info, sizeof(HT_block_info));
      BF_Block_SetDirty(new_block);




      ht_block_info.local_depth++;
			new_ht_block_info.local_depth++;

			printf("MPHKA\n");
			// And make all other indexes pointing to FULL target block, point to its (empty) replacement block
			int i = 0;
			int count = 0;
      		int old_num_indexes = ht_block_info.indexes_pointed_by;
			// int full_block_insex_count = 0; // logika den xreiazetai afou ebala to indexes--;
			printf("ht_info.ht_array_size = %d\n", ht_info.ht_array_size);
			printf("\n");
			for(int j=0 ; j < ht_info.ht_array_size ; j++)
				printf("ht_info.ht_array[%d] = %d\n", j, ht_info.ht_array[j]);
      		printf("old_num_indexes = %d\n", old_num_indexes);
			// Iterate through ht_array
			do {
				// For each index pointing to full target block
				if (ht_info.ht_array[i] == target_block_id) {
					// In order to equally distribute the indexes pointing to full block and the new block
					if (count >= old_num_indexes/2) {
						ht_info.ht_array[i] = new_block_id;
						new_ht_block_info.indexes_pointed_by++;
						ht_block_info.indexes_pointed_by--;
          			}
          			count++;
				}
				i++;
			} while (i < ht_info.ht_array_size); // minus the above target re-routed one
			printf("HT ARRAY AFTER\n");
			printf("ht_info.ht_array_size = %d\n", ht_info.ht_array_size);
			printf("\n");
			for(int j=0 ; j < ht_info.ht_array_size ; j++)
				printf("ht_info.ht_array[%d] = %d\n", j, ht_info.ht_array[j]);

			// SOOOOOOOOOOOOOOOS - NA KANW TO IDIO GIA TO FULL BLOCK - oxi ++; alla na dw pou 8a to enhmerwnw k 8a to kanw unpin
			// kai an xreiazetai kan na ginei auto?

			// Re-write new_ht_block_info to new block (changes indexes_pointed_by from inside while)
			memcpy(new_data + BF_BLOCK_SIZE - sizeof(HT_block_info), &new_ht_block_info, sizeof(HT_block_info));
			BF_Block_SetDirty(new_block);
			// we would also memcpy/setDirty ht_block_info here, but we need to changes its num_records to 0 later on, after we use it in ForLoop below

			// Re-write it to Block 0 (changes ht_array + num_blocks from outside IF above)
			memcpy(head_data, &ht_info, sizeof(HT_info));
			// Update ht_info by overwriting it (as always) and store it
			// so that everything is ready to re-call HT_InsertEntry()
			BF_Block_SetDirty(head_block);
			BF_UnpinBlock(head_block);

			// BF_GetBlock(ht_info.fileDesc, 0, head_block);
			// head_data = BF_Block_GetData(head_block);
			// memcpy(&ht_info, head_data, sizeof(HT_info));
			// printf("HT ARRAY AFTER AFTER TESTING\n");
			// printf("ht_info.ht_array_size = %d\n", ht_info.ht_array_size);
			// printf("\n");
			// for(int j=0 ; j < ht_info.ht_array_size ; j++)
			// 	printf("ht_info.ht_array[%d] = %d\n", j, ht_info.ht_array[j]);
			printf("MID WAY THROUGH\n");
			i = 0;
			// For each already existing record in FULL BLOCK (target_block) + Record_to_insert -> HT_InsertEntry(...) (hashing with 1 more bucket this time)
      		for(int k = 0 ; k < ht_block_info.num_records * sizeof(Record) ; k = k + sizeof(Record)) {
				// copy in our Record copy_array, the contents of the current record we are reading, in order to re-insert it
        		memcpy(&copy_records[i++], target_data + k, sizeof(Record));
			}

			// swsto print
			// for(int j=0 ; j < 8 ; j++)
			// 	printf("copy_records[%d].id = %d\n", j, copy_records[j].id);

			// !Important - this move marks total records of FULL_BLOCK as 0, making it appear as EMPTY while it is indeed STILL FULL
			int old_num_records = ht_block_info.num_records;
			ht_block_info.num_records = 0;
			// !Important - we do that so we can re-insert with the new hash_func + buckets the old records and the new one
			// by overwriting this full block, like it was empty
			// we trust that we wont read old remaining records by using ht_block_info.num_records as seilling! ( new_recs < olds_recs )
			memcpy(target_data + BF_BLOCK_SIZE - sizeof(HT_block_info), &ht_block_info, sizeof(HT_block_info));
			BF_Block_SetDirty(target_block);

			// Finally store all changes
			BF_UnpinBlock(new_block);
			BF_UnpinBlock(target_block);







			// Now that everything is set, we are ready to re-insert all the old records + the new one
			i = 0;
			printf("old record num: %d\n", old_num_records);

			// For each already existing record in FULL BLOCK + Record_to_insert -> HT_InsertEntry(...) (hashing with 1 more bucket this time)
			for(int k = 0 ; k < old_num_records * sizeof(Record) ; k = k + sizeof(Record)) {
				// We use our record array with the copies
				Record temp_record = copy_records[i++];
				printf("old record copy being re-inserted is: id=%d, name=%s, surname=%s, city=%s\n", temp_record.id, temp_record.name, temp_record.surname, temp_record.city);
				// if (!HT_InsertEntry(indexDesc, temp_record))
				if (HT_InsertEntry(ht_info.fileDesc, temp_record))
					return HT_ERROR;
			}
			
			// Now once more for the new Record to insert as we said above
			// if (!HT_InsertEntry(indexDesc, record))
			if (HT_InsertEntry(ht_info.fileDesc, record))
				return HT_ERROR;

			// If we get here, all insertions (old + new) were executed properly, thus
      BF_Block_Destroy(&new_block);
			return HT_OK;

			// NA DW TI 8A KANW ME TA IDS TWN BLOCKS - kinda solved me .num_records = 0;

		}
		else {
			// At this point ht_block_info contains target_block's info
			// Check if more than 1 ht_array indexes point to the FULL BLOCK
			// exit(5);


			// APO TIS DIAFANEIES BLEPW OTI DEN KANOUME INSERT EDW, MONO STIS 2 PARAPANW PERIPTWSEIS
			// EDW APLA AUXANOUME PINAKA K GLOBAL DEPTH ==> WSTE NA PESOUME SE MIA APTIS PERIPTWSEIS APO PANW
			// SWSTO?


			// Firstly, update the global depth
			// ht_info.global_depth++;
      		
			// Double ht_array's size
			int old_ht_array_size = ht_info.ht_array_size;
			int new_ht_array_size = old_ht_array_size * 2;
			// int new_ht_array_size_bin = new_ht_array_size;
			ht_info.ht_array_size = new_ht_array_size;
			ht_info.ht_array = realloc(ht_info.ht_array, new_ht_array_size * sizeof(int));
			assert(ht_info.ht_array != NULL);
			
			// // Temporary array to hold binary bits
			// int binary[BITS];
			int* binary = malloc(BITS * sizeof(int));
			
			// intToBinary(ht_info, binary, new_ht_array_size_bin);

			// printf("new_ht_array_size = %d\n", new_ht_array_size);
			// printf("Binary representation of new_ht_array_size: ");
			// for (int i = 0; i < ht_info.global_depth + 1 ; i++) {
			// 	printf(" %d", binary[i]);
			// }
			// printf("\n");

			// Create a temporary copy of the old hash table
			int* old_ht_array = malloc(old_ht_array_size * sizeof(int));
			for(int j=0 ; j < old_ht_array_size ; j++) {
				// memcpy(old_ht_array, ht_info.ht_array, old_ht_array_size * sizeof(int));
				old_ht_array[j] = ht_info.ht_array[j];
			}
			
			for(int j=0 ; j < old_ht_array_size ; j++)
				printf("old ht array[%d] = %d\n", j, old_ht_array[j]);


			// For each entry in the old hash table
			for (int i = 0; i < new_ht_array_size; i++) {
			// for (int i = 0; i < old_ht_array_size; i++) {
				// Extract the first d bits of the index
				// int old_index = i & ((1 << ht_info.global_depth) - 1);
				// int old_index = (i >> (BITS - ht_info.global_depth)) & ((1 << ht_info.global_depth) - 1);
				// int K = (i >> (BITS - ht_info.global_depth));



				// intToBinary(ht_info, binary, ht_info.global_depth);
				intToBinary(ht_info, binary, i);
				for (int i = 0; i < ht_info.global_depth ; i++) {
					printf(" %d", binary[i]);
				}
				printf("\n");

				int old_index = binaryToInt(binary, ht_info.global_depth);
				// printf("old_index ==== %d\n", old_index);

				// Copy the bucket pointer from the old hash table to the new hash table
				ht_info.ht_array[i] = old_ht_array[old_index];
				printf("ht_info.ht_array[%d] = %d\n", i, ht_info.ht_array[i]);
			}


			// Free the temporary copy of the old hash table
			free(old_ht_array);

			// Finally, update the global depth
			ht_info.global_depth++;

			// exit(0);

      // SOOOOOOOOOOS - DEN FTIAXNW KAINOURIO BLOCK EDW, NA TO METAFERW APO EXW STO FOR, MESA STA 2 CASES APO PANW

			// memcpy(new_data + BF_BLOCK_SIZE - sizeof(HT_block_info), &new_ht_block_info, sizeof(HT_block_info));
			// BF_Block_SetDirty(new_block);
			// BF_UnpinBlock(new_block);

			// Store updated global depth and th_array
			memcpy(head_data, &ht_info, sizeof(HT_info));
			BF_Block_SetDirty(head_block);
			BF_UnpinBlock(head_block);
			
			int i = 0;
			// For each already existing record in FULL BLOCK (target_block) + Record_to_insert -> HT_InsertEntry(...) (hashing with 1 more bucket this time)
      		for(int k = 0 ; k < ht_block_info.num_records * sizeof(Record) ; k = k + sizeof(Record)) {
				// copy in our Record copy_array, the contents of the current record we are reading, in order to re-insert it
        		memcpy(&copy_records[i++], target_data + k, sizeof(Record));
			}
			
			int old_num_records = ht_block_info.num_records;
			// Now that everything is set, we are ready to re-insert all the old records + the new one
			i = 0;
			printf("old record num: %d\n", old_num_records);

			// For each already existing record in FULL BLOCK + Record_to_insert -> HT_InsertEntry(...) (hashing with 1 more bucket this time)
			for(int k = 0 ; k < old_num_records * sizeof(Record) ; k = k + sizeof(Record)) {
				// We use our record array with the copies
				Record temp_record = copy_records[i++];
				printf("old record copy being re-inserted is: id=%d, name=%s, surname=%s, city=%s\n", temp_record.id, temp_record.name, temp_record.surname, temp_record.city);
				// if (!HT_InsertEntry(indexDesc, temp_record))
				if (HT_InsertEntry(ht_info.fileDesc, temp_record))
					return HT_ERROR;
			}
			
			// Now once more for the new Record to insert as we said above
			// if (!HT_InsertEntry(indexDesc, record))
			if (HT_InsertEntry(ht_info.fileDesc, record))
				return HT_ERROR;

			// If we get here, all insertions (old + new) were executed properly, thus
			return HT_OK;
		}




		// Is there anything else to update ???
	}

	// head_data = NULL;	//xreiazontai auta?
	// target_data = NULL;
	// sto telos (h k sthn arxh, na tto dokimasw) na kanw unpin k to block 0 pou xrhsimopoioume se olh th sunarthsh
	BF_Block_Destroy(&head_block);
	BF_Block_Destroy(&target_block);
	// BF_Block_Destroy(&new_block); // mallon oxi edw, giati orizetai mono mesa sto for??

	// FInally, if every record was inserted properly, we return code successful
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
	printf("TOTAL BLOCKS: %d\n", total_blocks);

	// Unpin block 0 since we dont need it anymore
	if ((BF_UnpinBlock(block)) < 0) {
		printf("Error unpinning block in HT_InsertEntry\n");
		return -1;
	}

  	for (int i=1 ; i<total_blocks ; i++) {
  	// for (int i=1 ; i<2 ; i++) {
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