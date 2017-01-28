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
4. hash(int x, int level) : Returns the hash value of x, which in case of LH is level LSB bits and in case of EH is level MSB bits of x (x is padded to 20 bits in this case)

Specifics:

ExtendibleHash:
The Linear Hash has the following features.
bool insert(Bucket&,const int&) : Try to insert the value a in the bucket or ay overflow bucket of Bucket.
void recycleBucket(int bucket_addr) : Recycles the bucket.
int getBucketData(int bucket_addr, vector<int>& v): Gets data of bucket and overflows.
bool forceInsert(Bucket&, const int&,int): Inserts the value in bucket or overflow. May insert a new overflow.

LinearHash:
* int split(int bucket_idx) : Split the bucket at bucket_idx according to the rules of linear hashing.
* bool insert(Bucket&,const int&) : Try to insert the value a in the Bucket or any overflow bucket of Bucket.
* int getNewOverflowBucket() : Return a new unused overflow bucket
* bool addOverflowBucket(int bucket_addr) : Add a new overflow bucket to bucket_addr
* void recycleBucket(int bucket_addr) : Recycle the bucket by removing the data and overflow buckets of the bucket_addr. 
* int getBucketData(int bucket_addr, vector<int>& v): Get the bucket data (from the bucket and the overflows).

Some salient features and assumptions:
1. The code has been tested on datasets that have been provided in the submission. The limits and constants set in the code are for comfortably fitting the data of the order same as those in the datasets. But due to the certain characteristics of the individual hash functions, there can be cases where the code may run out of memory or crash. These cases are far from naturally generated ones, and it is assumed that such caes won't be input in the program.
2. The Linear hash has half of the hard disk 
