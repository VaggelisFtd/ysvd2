#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "../include/bf.h"
#include "../include/hash_file.h"
// #include "../include/record.h"

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

int main() {
  BF_Init(LRU);
  
  CALL_OR_DIE(HT_Init());

  int indexDesc;
  // CALL_OR_DIE(HT_CreateIndex(FILE_NAME, GLOBAL_DEPTH));
  // CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc)); 

  Record record;
  srand(12569874);
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













    // ====================================
    HT_info ht_info;
    BF_Block *block;
    void *headblock; // was named data
    void *data;

    if (Check(BF_CreateFile(FILE_NAME)) < 0) {
    printf("Error creating file: %s in HP_CreateFile\n", FILE_NAME);
    return -1;
    }

    if (Check(BF_OpenFile(FILE_NAME, &ht_info.fileDesc)) < 0) {
      printf("Error opening file: %s in HP_CreateFile\n", FILE_NAME);
      return -1;
    }

    BF_Block_Init(&block);

    if (Check(BF_AllocateBlock(hp_info.fileDesc, block)) < 0) {
      printf("Error allocating block in HP_CreateFile\n");
      return -1;
    }

    headblock = BF_Block_GetData(block);
    ht_info.is_ht = true;
    ht_info.global_depth = GLOBAL_DEPTH; // xekiname me 2 buckets (00, 01 i guess)
    ht_info.ht_array = malloc(GLOBAL_DEPTH * sizeof(int)); // space for 2 blocks/buckets

    memcpy(headblock, &ht_info, sizeof(HT_info));
     BF_Block_SetDirty(block);
    if (Check(BF_UnpinBlock(block)) < 0) {
      printf("Error allocating block in HP_CreateFile\n");
      return -1;
    }

    
    // ====================================
















    CALL_OR_DIE(HT_InsertEntry(indexDesc, record));
    // CALL_OR_DIE(HT_InsertEntry(indexDesc, randomRecord())); // cant define randomRecord() for some reason...

  }

  // printf("RUN PrintAllEntries\n");
  // int id = rand() % MAX_RECORDS;
  // CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
  //CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));


  // CALL_OR_DIE(HT_CloseFile(indexDesc));
  BF_Close();
}
