#ifndef HASH_FILE_H
#define HASH_FILE_H
#include <stdbool.h>
#include "../include/bf.h"
#include "../include/record.h"

#define MAX_RECORDS 1000 // you can change it if you want

typedef enum HT_ErrorCode {
  HT_OK,
  HT_ERROR
} HT_ErrorCode;

/* HT_info struct holds metadata associated with the hash file */

//  typedef struct {
// 	// bool is_ht;					// TRUE is ht file
//     int fileDesc;              	// identifier number for opening file from block
// 	int global_depth;
// 	int* ht_array;				// hash table array - contains int-ids of blocks/buckets
// 	int ht_array_length;  	// new: number of blocks needed to store ht_array - na dw an xreiazetai
// 	int ht_array_head;		// new: id of first block used to store ht_array - na dw an xreiazetai
// 	int ht_array_size;		// new: number of elements(representing hash values) allocated for ht_array
// 	int num_buckets; // mallon prepei na uparxei, mhpws k 8elei na xekinaei me allo ari8mo buckets kapoio arxeio (ara dn ginetai me define gt 8a einai idio se ola)
// } HT_info;

typedef struct {
	bool is_ht;					// TRUE is ht file
    //int fileDesc;              	// identifier number for opening file from block
	int global_depth;			// 
	int ht_id;					// block id of fisrt ht block
    int max_records;            // max number of records that can be stored
	int max_ht;					// max number of entries in each ht block
} HT_info;


typedef struct {
    int num_records;                // number of records in this block
	int local_depth;
	int max_records;				// was (block/bucket)_size
	int next_block;       			// pointer to the next block // SOOOOS - mhpws prepei na ginei int* ???
	int indexes_pointed_by;			// mhpws na uparxei auto wste na briskoume grhgora posa ht_array indexes deixnoun auto to block
									// kai na mporoume grhgora na xwrizoume auto ton ari8mo se 2 isa~ uposunola
									// Sthn ousia dld to 8eloume apla gia na exoume to X apo to X/2 -> new block kai X%2 ->old block, pou 8a kanoume
} HT_block_info;

/*
 * Η συνάρτηση HT_Init χρησιμοποιείται για την αρχικοποίηση κάποιον δομών που μπορεί να χρειαστείτε. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς, επιστρέφεται HT_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_Init();

/*
 * Η συνάρτηση HT_CreateIndex χρησιμοποιείται για τη δημιουργία και κατάλληλη αρχικοποίηση ενός άδειου αρχείου κατακερματισμού με όνομα fileName. 
 * Στην περίπτωση που το αρχείο υπάρχει ήδη, τότε επιστρέφεται ένας κωδικός λάθους. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HΤ_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_CreateIndex(
	const char *fileName,		/* όνομααρχείου */
	int depth
);


/*
 * Η ρουτίνα αυτή ανοίγει το αρχείο με όνομα fileName. 
 * Εάν το αρχείο ανοιχτεί κανονικά, η ρουτίνα επιστρέφει HT_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_OpenIndex(
	const char *fileName, 		/* όνομα αρχείου */
  	int *indexDesc            	/* θέση στον πίνακα με τα ανοιχτά αρχεία  που επιστρέφεται */
);

/*
 * Η ρουτίνα αυτή κλείνει το αρχείο του οποίου οι πληροφορίες βρίσκονται στην θέση indexDesc του πίνακα ανοιχτών αρχείων.
 * Επίσης σβήνει την καταχώρηση που αντιστοιχεί στο αρχείο αυτό στον πίνακα ανοιχτών αρχείων. 
 * Η συνάρτηση επιστρέφει ΗΤ_OK εάν το αρχείο κλείσει επιτυχώς, ενώ σε διαφορετική σε περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_CloseFile(
	int indexDesc 		/* θέση στον πίνακα με τα ανοιχτά αρχεία */
);

/*
 * Η συνάρτηση HT_InsertEntry χρησιμοποιείται για την εισαγωγή μίας εγγραφής στο αρχείο κατακερματισμού. 
 * Οι πληροφορίες που αφορούν το αρχείο βρίσκονται στον πίνακα ανοιχτών αρχείων, ενώ η εγγραφή προς εισαγωγή προσδιορίζεται από τη δομή record. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HT_OK, ενώ σε διαφορετική περίπτωση κάποιος κωδικός λάθους.
 */
HT_ErrorCode HT_InsertEntry(
	int indexDesc,		/* θέση στον πίνακα με τα ανοιχτά αρχεία */
	Record record		/* δομή που προσδιορίζει την εγγραφή */
	/*, HT_info *ht_info */
);

/*
 * Η συνάρτηση HΤ_PrintAllEntries χρησιμοποιείται για την εκτύπωση όλων των εγγραφών που το record.id έχει τιμή id. 
 * Αν το id είναι NULL τότε θα εκτυπώνει όλες τις εγγραφές του αρχείου κατακερματισμού. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HP_OK, ενώ σε διαφορετική περίπτωση κάποιος κωδικός λάθους.
 */
HT_ErrorCode HT_PrintAllEntries(
	int indexDesc,			/* θέση στον πίνακα με τα ανοιχτά αρχεία */
	int *id 				/* τιμή του πεδίου κλειδιού προς αναζήτηση */
);


#endif // HASH_FILE_H
