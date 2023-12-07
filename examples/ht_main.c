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
  if (code != BF_OK)
  {
    BF_PrintError(code);
    printf("bf_error_code = %d\n", code);
    return -1;
  }
}

#define MAX_RECORDS 40 // you can change it if you want
#define GLOBAL_DEPTH 1 // you can change it if you want // prepei na bgei sthn telikh main
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
  
  HT_info ht_info;
  HT_block_info ht_block_info;

  BF_Block *block;
  BF_Block_Init(&block);
  char *data;

  fd_array = malloc(MAX_OPEN_FILES * sizeof(int));
	for (int i=0 ; i<MAX_OPEN_FILES ; i++)
		fd_array[i] = -1;

  ht_array_global = malloc(MAX_OPEN_FILES * sizeof(int*));

  printf("Creating Index...\n");
  CALL_OR_DIE(HT_CreateIndex(FILE_NAME, 3));

  int curr_fd;

  BF_OpenFile(FILE_NAME, &curr_fd);
  BF_GetBlock(curr_fd, 0, block);
  data = BF_Block_GetData(block);
  memcpy(&ht_info, data, sizeof(HT_info));
  BF_UnpinBlock(block);

  printf("Openning Index...\n");
  CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc)); 
  
  Record record;
  srand(time(NULL));
  int r;
  printf("Inserting %d Entries\n", MAX_RECORDS);
  for (int id = 0; id < MAX_RECORDS; ++id) {
    // create a record
    record.id = id;
    r = rand() % 12;
    memcpy(record.name, names[r], strlen(names[r]) + 1);
    r = rand() % 12;
    memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
    r = rand() % 10;
    memcpy(record.city, cities[r], strlen(cities[r]) + 1);

    CALL_OR_DIE(HT_InsertEntry(fd_array[indexDesc], record));
  }

  printf("\n");
  printf("Running HT_PrintAllEntries passing argument NULL:\n");
  CALL_OR_DIE(HT_PrintAllEntries(ht_info.fileDesc, NULL));            // print all
  int target_id = rand() % MAX_RECORDS;
  printf("\n");
  printf("Running HT_PrintAllEntries with id: %d\n", target_id);
  HT_PrintAllEntries(ht_info.fileDesc, &target_id);                   // print specific id

  CALL_OR_DIE(HashStatistics(FILE_NAME));

  printf("\n");
  printf("Closing File...\n");
  CALL_OR_DIE(HT_CloseFile(indexDesc));
  
  BF_Block_Destroy(&block);

  BF_Close();  
}