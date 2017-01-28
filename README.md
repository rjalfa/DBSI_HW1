# DBSI_HW1

The code is organized in 2 files (linear_hashing.cc and extendible_hashing.cc) . The major classes for the project are:
1. Bucket [Represents buckets on the disk]  
2. Disk [Represents Disk of Buckets]  
3. RAM [In Extendible Hashing, represents the RAM directory]  
4. ExtendibleHash [The Extendible hash class and functions]  
5. LinearHash [The LinearHash Class and functions]  

Bucket:
1. setItem(int a, int pos) - Sets the data entry pos of bucket to a
2. getItem(int pos) - Returns the data entry pos
3. insertItem(int a) - Inserts the data 

Disk:
The disk stores all the buckets and acts as a secondary memory interface for the program. Main functions are as follows:
1. getBucket(int addr) - Get the bucket in the disk at bucket address addr.
2. getNewBucket() - Returns the address of a new unused bucket in the disk
3. recycleBucket(int addr) - Makes the bucket at address addr free for use in future.
4. getAccessCount() - returns the value of access_count, which is a variable that is incremented whenever getBucket() is called.
The disk uses a queue for keeping all the buckets that are free to serve getNewBucket() requests. The size of disk is predefined in the MAX_BUCKETS_ON_DISK macro.

RAM: (only in extendible hashing)
The RAM acts as a directory for extendible hashing. Its size is fixed to MAX_BUCKETS_ON_RAM macro.
1. setEntry(int pos, int a) : Sets RAM entry pos (either in main or secondary memory) to value a
2. getEntry(int pos): Returns the RAM entry pos (either in main memory or secondary memory).
3. doubleDirectory(): Doubles the current directory and copies value of position i to positions 2*i and 2*i+1

Common Functions to Hash(s):
1. insert(int a): Inserts the value a in the hash table
2. display(): Displays the whole hash table
3. search(int a): Checks whether the value a  is in the hash table or not

Specifics:
LinearHash:
The Linear Hash has the following features.
1. insert(Bucket, int a): Tries to insert the value a in the Bucket or some overflow bucket of Bucket.
2. get# DBSI_HW1

The code is organized in 2 files. The major classes for the project are:
1. Bucket [Represents buckets on the disk]  
2. Disk [Represents Disk of Buckets]  
3. RAM [In Extendible Hashing, represents the RAM directory]  
4. ExtendibleHash [The Extendible hash class and functions]  
5. LinearHash [The LinearHash Class and functions]  

Bucket:
1. setItem(int a, int pos) - Sets the data entry pos of bucket to a
2. getItem(int pos) - Returns the data entry pos
3. insertItem(int a) - Inserts the data 
