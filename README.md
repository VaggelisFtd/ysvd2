Άσκηση 2 ΥΣΒΔ

Παπαδήμα Θεοδοσία 1115202000162
Παπακώστας Ευάγγελος 1115201800152
Φωτιάδης Ευάγγελος 1115201900301

Implementation:
hash_file.h

Προστέθηκαν 2 δομές(structs):
HT_info για τις πληροφορίες του HT αρχείου
HT_block_info για τις πληροφορίες του block

hash_file.c

Custom functions:

hash,hash2 για το hashing με ορίσματα id,buckets 
checkOpenFiles να ελέγχουμε τα ανοιχτά αρχεία
dirtyUnpin για unpin dirty block με όρισμα ένα block

HT_CreateIndex