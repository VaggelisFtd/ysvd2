#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "../include/bf.h"
#include "../include/hash_file.h"
// #include "../include/record.h"

int Check(int call)
{
  BF_ErrorCode code = call;
  // printf("%d wwwwww\n", code);   // an den uparxei ena printf edw h mesa sto IF, skaei to programma???
  if (code != BF_OK)
  {
    BF_PrintError(code);
    printf("bf_error_code = %d\n", code); // h edw*
    return -1;
  }
}

#define MAX_RECORDS 1000 // you can change it if you want
#define GLOBAL_DEPTH 2 // you can change it if you want // prepei na bgei sthn telikh main
#define FILE_NAME "data.db"

const char* names[] = {
  "Yannis",
  "Christofos",
  "Sofia",
  "Marianna",
  "Vagelis",
  "Maria",
  "Iosif",
  "Dionisis",
  "Konstantina",
  "Theofilos",
  "Giorgos",
  "Dimitris"
};

const char* surnames[] = {
  "Ioannidis",
  "Svingos",
  "Karvounari",
  "Rezkalla",
  "Nikolopoulos",
  "Berreta",
  "Koronis",
  "Gaitanis",
  "Oikonomou",
  "Mailis",
  "Michas",
  "Halatsis"
};

const char* cities[] = {
  "Athens",
  "San Francisco",
  "Los Angeles",
  "Amsterdam",
  "London",
  "New York",
  "Tokyo",
  "Hong Kong",
  "Munich",
  "Miami"
};

#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }



int** ht_array_global;
int* fd_array;

int main() {
  BF_Init(LRU);
  
  CALL_OR_DIE(HT_Init());

  int indexDesc;
  // CALL_OR_DIE(HT_CreateIndex(FILE_NAME, GLOBAL_DEPTH));
  
  HT_info ht_info;
  HT_block_info ht_block_info;

  BF_Block *block;
  BF_Block_Init(&block);
  char *data;

  // if (Check(BF_CreateFile(FILE_NAME)) < 0) {
  //   printf("Error creating file: %s in HT_CreateFile\n", FILE_NAME);
  //   return -1;
  // }

  // if (Check(BF_OpenFile(FILE_NAME, &ht_info.fileDesc)) < 0) {
  //   printf("Error opening file: %s in HT_CreateFile\n", FILE_NAME);
  //   return -1;
  // }


  // if (Check(BF_AllocateBlock(ht_info.fileDesc, block)) < 0) {
  //   printf("Error allocating block in HT_CreateFile\n");
  //   return -1;
  // }

  // headblock = BF_Block_GetData(block);

  // // ht_array_global = malloc(GLOBAL_DEPTH * sizeof(int));
  // ht_array_global = malloc(2 * sizeof(int));

  // ht_info.is_ht = true;
  // // ht_info.global_depth = GLOBAL_DEPTH;
  // // ht_info.global_depth = 1;                 // xekiname me 2 buckets (kai blocks: 0, 1 i guess) -> ara global_depth = 1
  // ht_info.global_depth = 1;                 // xekiname me 4 buckets (kai blocks: 0, 1, 2, 3) -> ara global_depth = 2
  // // ht_info.ht_array = malloc(GLOBAL_DEPTH * sizeof(int)); // space for 2 blocks/buckets
  // ht_info.ht_array = ht_array_global;       // space for 2 blocks/buckets
  // ht_info.ht_array_head = 0;                // block 0 is the head of the ht_array
  // ht_info.ht_array_length = 1;              // there only 1 block needed to store ht_array (yet)
  // // ht_info.ht_array_size = GLOBAL_DEPTH;     // 2 or GLOBAL_DEPTH pointers of ht_array have been allocated
  // ht_info.ht_array_size = 2;
  // ht_info.num_blocks = 2;                  // total number of buckets/blocks in this ht file 

  // // ===
	// // int step = 2;
  // // int BlockId = 1;
  // // int i = 0;
  // // while (step <= ht_info.ht_array_size) {
	// // 	if (i < step) {
	// // 		ht_info.ht_array[i] = BlockId;
	// // 		i++;
	// // 	}
	// // 	else {
	// // 		step *= 2;
	// // 		BlockId++;
	// // 	}
  // // }

	// // ht_info.ht_array[0] = 0; // => KANENA NA MHN DEIXNEI STO 0
	// ht_info.ht_array[0] = 1;
	// ht_info.ht_array[1] = 2;
	// // ht_info.ht_array[2] = 1;
	// // ht_info.ht_array[3] = 1;
	// // ht_info.ht_array[4] = 2;
	// // ht_info.ht_array[5] = 2;
	// // ht_info.ht_array[6] = 2;
	// // ht_info.ht_array[7] = 2;


	// for(int j=0 ; j < ht_info.ht_array_size ; j++)
	// 	printf("ht_info.ht_array[%d] = %d\n", j, ht_info.ht_array[j]);



  // // CALL_OR_DIE(HT_CreateIndex(FILE_NAME, 3));



	// // exit(0);
  // // ===
	
  // memcpy(headblock, &ht_info, sizeof(HT_info));
  // BF_Block_SetDirty(block);
  // if (Check(BF_UnpinBlock(block)) < 0) {
  //   printf("Error allocating block in HT_CreateFile\n");
  //   return -1;
  // }

  // if (Check(BF_AllocateBlock(ht_info.fileDesc, block)) < 0) {   // allocate 1st bucket/block
  //   printf("Error allocating block in HT_CreateFile\n");
  //   return -1;
  // }
  // data = BF_Block_GetData(block);
  // HT_block_info ht_block_info;
  // ht_block_info.local_depth = 1; // may have to be an expression that hardcoded
  // // ht_block_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
  // ht_block_info.max_records = 8;
  // ht_block_info.next_block = 0;
  // ht_block_info.num_records = 0;
  // ht_block_info.indexes_pointed_by = 1;
  // memcpy(data + BF_BLOCK_SIZE - sizeof(HT_block_info), &ht_block_info, sizeof(HT_block_info));
  // BF_Block_SetDirty(block);
  // if (Check(BF_UnpinBlock(block)) < 0) {
  //   printf("Error allocating block in HT_CreateFile\n");
  //   return -1;
  // }
  
  // BF_GetBlock(ht_info.fileDesc, 1, block);
  // data = BF_Block_GetData(block);
  // memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));

  // if (Check(BF_AllocateBlock(ht_info.fileDesc, block)) < 0) {   // allocate 2nd bucket/block
  //   printf("Error allocating block in HT_CreateFile\n");
  //   return -1;
  // }
  // data = BF_Block_GetData(block);
  // ht_block_info.local_depth = 1; // may have to be an expression that hardcoded
  // // ht_block_info.max_records = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
  // ht_block_info.max_records = 8;
  // ht_block_info.next_block = 0;
  // ht_block_info.num_records = 0;
  // ht_block_info.indexes_pointed_by = 1;
  // memcpy(data + BF_BLOCK_SIZE - sizeof(HT_block_info), &ht_block_info, sizeof(HT_block_info));
  // BF_Block_SetDirty(block);
  // if (Check(BF_UnpinBlock(block)) < 0) {
  //   printf("Error allocating block in HT_CreateFile\n");
  //   return -1;
  // }

// =====================================================================================
  fd_array = malloc(MAX_OPEN_FILES * sizeof(int));
	for (int i=0 ; i<MAX_OPEN_FILES ; i++)
		fd_array[i] = -1;

  ht_array_global = malloc(MAX_OPEN_FILES * sizeof(int*));

  CALL_OR_DIE(HT_CreateIndex(FILE_NAME, 1));

  int curr_fd;
  BF_OpenFile(FILE_NAME, &curr_fd);
  BF_GetBlock(curr_fd, 0, block);
  data = BF_Block_GetData(block);
  memcpy(&ht_info, data, sizeof(HT_info));

  for (int i=0 ; i<ht_info.ht_array_size ; i++) {
    printf("ht_info.ht_array[%d] = %d\n", i, ht_info.ht_array[i]);
  }

  BF_GetBlock(curr_fd, 1, block);
  data = BF_Block_GetData(block);
  memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));
  BF_UnpinBlock(block);

  BF_GetBlock(curr_fd, 2, block);
  data = BF_Block_GetData(block);
  memcpy(&ht_block_info, data + BF_BLOCK_SIZE - sizeof(HT_block_info), sizeof(HT_block_info));
  BF_UnpinBlock(block);

  CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc)); 
  printf("indexDesc = %d\n", indexDesc);
  
  Record record;
  srand(time(NULL));
  int r;
  // printf("Inserting %d Entries\n", MAX_RECORDS);
  printf("Inserting 32 Entries\n");
  // for (int id = 0; id < MAX_RECORDS; ++id) {
  // for (int id = 0; id < 68; ++id) {
  for (int id = 0; id < 33; ++id) {
    // create a record
    record.id = id;
    r = rand() % 12;
    memcpy(record.name, names[r], strlen(names[r]) + 1);
    r = rand() % 12;
    memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
    r = rand() % 10;
    memcpy(record.city, cities[r], strlen(cities[r]) + 1);

    CALL_OR_DIE(HT_InsertEntry(fd_array[indexDesc], record));
    // CALL_OR_DIE(HT_InsertEntry(ht_info.fileDesc, record));
    // CALL_OR_DIE(HT_InsertEntry(indexDesc, randomRecord())); // cant define randomRecord() for some reason...
  }


  // record.id = 26;
  // memcpy(record.name, "Name26", 7);
  // memcpy(record.surname, "Surname26", 10);
  // memcpy(record.city, "City26", 7);
  // CALL_OR_DIE(HT_InsertEntry(ht_info.fileDesc, record));
  // record.id = 14;
  // memcpy(record.name, "Name14", 7);
  // memcpy(record.surname, "Surname14", 10);
  // memcpy(record.city, "City14", 7);
  // CALL_OR_DIE(HT_InsertEntry(ht_info.fileDesc, record));
  // record.id = 16;
  // memcpy(record.name, "Name16", 7);
  // memcpy(record.surname, "Surname16", 10);
  // memcpy(record.city, "City16", 7);
  // CALL_OR_DIE(HT_InsertEntry(ht_info.fileDesc, record));
  // record.id = 12;
  // memcpy(record.name, "Name12", 7);
  // memcpy(record.surname, "Surname12", 10);
  // memcpy(record.city, "City12", 7);
  // CALL_OR_DIE(HT_InsertEntry(ht_info.fileDesc, record));
  // record.id = 10;
  // memcpy(record.name, "Name10", 7);
  // memcpy(record.surname, "Surname10", 10);
  // memcpy(record.city, "City10", 7);
  // CALL_OR_DIE(HT_InsertEntry(ht_info.fileDesc, record));
  // record.id = 21;
  // memcpy(record.name, "Name21", 7);
  // memcpy(record.surname, "Surname21", 10);
  // memcpy(record.city, "City21", 7);
  // CALL_OR_DIE(HT_InsertEntry(ht_info.fileDesc, record));


  // printf("RUN PrintAllEntries\n");
  int target_id = 22;
  // int target_id = rand() % MAX_RECORDS;
  // HT_PrintAllEntries(ht_info.fileDesc, &target_id);                   // print specific id
  CALL_OR_DIE(HT_PrintAllEntries(ht_info.fileDesc, NULL));            // print all

  CALL_OR_DIE(HashStatistics(FILE_NAME));

  // CALL_OR_DIE(HT_CloseFile(ht_info.fileDesc));
  CALL_OR_DIE(HT_CloseFile(indexDesc));
  
  BF_Block_Destroy(&block);

  // for(int i=0; i<ht_info.ht_array_size ; i++) {
  //   free(ht_info.ht_array);
  // }
  free(ht_info.ht_array);

  BF_Close();
  
}