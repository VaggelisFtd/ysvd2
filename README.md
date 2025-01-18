# Extendible-Hashing-System

A system implementing extendible hashing for efficient data management using dynamic hash tables.

## Getting Started

### Building the Project
To build the project, run:
```bash
make clean   # Clean the environment if previously built
make hp      # Compile the hash file system
make run     # Execute the program
```

### Running the Program
The main executable initializes the LRU replacement strategy and invokes the functions implemented in `hash_file.c`. Functions are tested step-by-step, with print statements before each call.

Example commands:
```bash
make clean
make hp
make run
```

### Cleaning Up
To reset the environment, run:
```bash
make clean
```

## Program Details

### Components
1. **hash_file.h:**
   - Defines data structures for the hash file system, including:
     - `HT_info`: Metadata for the hash file, such as global depth, file descriptor, and hash table.
     - `HT_block_info`: Metadata for individual hash table blocks, including local depth and record count.

2. **hash_file.c:**
   - Implements functions for creating, managing, and querying extendible hash tables. Key functions include:
     - `HT_CreateIndex`: Initializes a new hash table file.
     - `HT_OpenIndex`: Opens an existing hash table file.
     - `HT_CloseFile`: Closes an open hash table file.
     - `HT_InsertEntry`: Inserts a new record, splitting blocks if necessary.
     - `HT_PrintAllEntries`: Prints all records in the hash table.
     - `HT_HashStatistics`: Displays statistics such as the number of records per block.

3. **main.c:**
   - Demonstrates the use of hash table functions by initializing global arrays, invoking each function, and printing outputs for verification.

### Execution Workflow
- **Hash Table Initialization:**
  - `HT_CreateIndex` initializes the file and sets up metadata.
  - Two blocks are created by default, each pointing to an extendible hash table bucket.

- **Insertion Process:**
  - A hashed value determines the target bucket.
  - If the bucket is full:
    - Splitting occurs, with local and global depths adjusted.
    - New metadata is written to the disk.

- **Querying and Printing:**
  - All entries or specific entries (based on ID) can be printed.
  - `HT_HashStatistics` provides insights into block usage and distribution.


## License
This project is shared for educational purposes. No license is granted for commercial or non-educational use without explicit permission.

## Authors
- [Theodosia Papadima](https://github.com/sulpap)
- [Vaggelis Papakostas](https://github.com/VaggelisPapako)
- [Vaggelis Fotiadis](https://github.com/VaggelisFtd)
