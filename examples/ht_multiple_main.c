
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "../include/bf.h"
#include "../include/hash_file.h"
#include "../include/record.h"

#define BUCKETS_NUM 13 // you can change it if you want
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK)        \
    {                         \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }
const char *names[] = {
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
    "Dimitris"};

const char *surnames[] = {
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
    "Halatsis"};

const char *cities[] = {
    "Athens",
    "San Francisco",
    "Los Angeles",
    "Amsterdam",
    "London",
    "New York",
    "Tokyo",
    "Hong Kong",
    "Munich",
    "Miami"};

int main()
{
  HT_Init(); // Initialize the library

  // Create and open the first index file
  int indexDesc1;
  if (HT_CreateIndex("test_hash_file1.db", 100) == HT_OK &&
      HT_OpenIndex("test_hash_file1.db", &indexDesc1) == HT_OK)
  {
    printf("Index 1 opened successfully!\n");

    // Insert some sample records into the first file
    Record record1 = {1, "John", "Doe", "New York"};
    HT_InsertEntry(indexDesc1, record1);

    printf("\nPrinting all entries for index 1:\n");
    HT_PrintAllEntries(indexDesc1, NULL);

    // Close the first index file
    if (HT_CloseFile(indexDesc1) == HT_OK)
    {
      printf("Index 1 closed successfully!\n");
    }
    else
    {
      fprintf(stderr, "Error closing index file 1.\n");
    }
  }
  else
  {
    fprintf(stderr, "Error opening index file 1.\n");
  }

  // Create and open the second index file
  int indexDesc2;
  if (HT_CreateIndex("test_hash_file2.db", 50) == HT_OK &&
      HT_OpenIndex("test_hash_file2.db", &indexDesc2) == HT_OK)
  {
    printf("\nIndex 2 opened successfully!\n");

    // Insert some sample records into the second file
    Record record2 = {2, "Jane", "Smith", "San Francisco"};
    HT_InsertEntry(indexDesc2, record2);

    printf("\nPrinting all entries for index 2:\n");
    HT_PrintAllEntries(indexDesc2, NULL);

    // Additional testing or function calls for the second index file can be added here

    // Close the second index file
    if (HT_CloseFile(indexDesc2) == HT_OK)
    {
      printf("Index 2 closed successfully!\n");
    }
    else
    {
      fprintf(stderr, "Error closing index file 2.\n");
    }
  }
  else
  {
    fprintf(stderr, "Error opening index file 2.\n");
  }

  return 0;
}
