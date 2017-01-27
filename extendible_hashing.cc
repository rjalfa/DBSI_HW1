#include <iostream>
#include <vector>
#include <cstdlib>
#define MAX_BUCKET_SIZE 10
#define MAX_BUCKETS_ON_DISK 1000000
#define MAX_BUCKETS_ON_RAM 1024
#define OVERFLOW_START_INDEX 500000
#define HASH_SHIFT 19

using namespace std;

class Bucket
{
	int* storage;
	int bucketOverflowIndex;
	int bucket_size;
	int numRecords;
	int depth;
	int index;
	public:
		bool setItem(int a, int pos)
		{
			if(pos < 0 || pos >= bucket_size) return false;
			storage[pos] = a;
			return true;
		}
		int getItem(int pos)
		{
			if(pos < 0 || pos >= numRecords) return -1;
			return storage[pos];
		}
		int getDepth()
		{
			return this->depth;
		}
		void setDepth(int x=1)
		{
			this->depth	+= x;	
		}
		bool insertItem(int a)
		{
			if(bucket_size - numRecords <= 0) return false;
			if(setItem(a,numRecords))
			{
				numRecords ++;
				return true; 
			}
			return false;
		}
		
		bool search(int a)
		{
			for(int i = 0; i < numRecords; i ++) if(a == storage[i]) return true;
			return false;
		}

		void setBucketOverflowIndex(int a) { this->bucketOverflowIndex = a; }
		int getBucketOverflowIndex() { return this->bucketOverflowIndex; }
		Bucket() : bucketOverflowIndex(-1), bucket_size(MAX_BUCKET_SIZE), numRecords(0) { storage = new int[MAX_BUCKET_SIZE]; }
		~Bucket() { delete[] storage; }
		int size() { return this->numRecords; }
		bool isFull() { return this->numRecords == bucket_size; }
		void emptyBucket() {numRecords = 0;bucketOverflowIndex = -1;}
		
		Bucket(const Bucket& b) : bucketOverflowIndex(b.bucketOverflowIndex),  bucket_size(MAX_BUCKET_SIZE), numRecords(b.numRecords)
		{
			storage = new int[MAX_BUCKET_SIZE];
			for(int i = 0; i < numRecords; i ++ ) storage[i] = b.storage[i];
		}
};


class Disk
{
	vector<Bucket> storage;
	queue<int> freeBuckets;
	public:
		Disk()
		{
			this->storage = vector<Bucket>(MAX_BUCKETS_ON_DISK);
			for(int i = 0; i < MAX_BUCKETS_ON_DISK ; i ++)
			{
				this->freeBuckets.push(i);
			}
		}
		~Disk()
		{
			this->storage.clear();
		}
		Bucket& getBucket(int bucket_idx)
		{
			return storage[bucket_idx];
		}
		int getNewBucket(int bucket_idx)
		{
			int x = this->freeBuckets.front();
			this->freeBuckets.pop();
			return x;
		}
		bool recycleBucket(int bucket_idx)
		{
			this->freeBuckets.push(bucket_idx);
		}
		unsigned int getStorageSize()
		{
			return (unsigned int)storage.size();
		}
};


class RAM
{
	vector<int> storage;
	int mem_used_index;
	Disk* memory;
	vector<int> overflow_buckets;
	public:
		RAM(Disk* mem)
		{
			this->memory = mem;
			this->mem_used_index = 2;
			this->storage = vector<int>(MAX_BUCKETS_ON_RAM, -1);
		}
		~RAM()
		{
			this->storage.clear();
		}
		int getEntry(int bucket_idx)
		{
			if(bucket_idx >= mem_used_index)
			{
				return -1;
			}
			else{
				if(bucket_idx < storage.size())
				{
					return storage[bucket_idx];		
				}
				else
				{
					bucket_idx -= storage.size();
					int entry_idx = bucket_idx/MAX_BUCKET_SIZE;
					Bucket& b = memory->getBucket(overflow_buckets[entry_idx]);
					return b.getItem(bucket_idx - (entry_idx * MAX_BUCKET_SIZE));
				}
			}
			
		}
		bool setEntry(int entry_idx,int a)
		{
			if(entry_idx >= mem_used_index)
			{
				return false;
			}
			else{
				if(entry_idx < storage.size())
				{
					storage[entry_idx] = a;
					return true;
				}
				else
				{
					entry_idx -= storage.size();
					int bucket_idx = entry_idx/MAX_BUCKET_SIZE;
					Bucket& b = memory->getBucket(overflow_buckets[bucket_idx]);
					return b.setItem(entry_idx - (bucket_idx * MAX_BUCKET_SIZE), a);
				}
			}
		}
		unsigned int getStorageSize()
		{
			return (unsigned int)(storage.size() + overflow_buckets.size()*MAX_BUCKET_SIZE);
		}
		bool doubleDirectory()
		{
			this->mem_used_index *= 2;
			int extra_buckets = (this->mem_used_index - storage.size()) - (this->overflow_buckets.size() * MAX_BUCKET_SIZE);
			while(extra_buckets > 0)
			{
				overflow_buckets.push_back(memory->getNewBucket());
				extra_buckets -= MAX_BUCKET_SIZE;
			}
			vector<int> temp_array(this->mem_used_index);
			for(int i = 0; i ,this->mem_used_index/2; i ++)
			{	
				temp_array[2*i] = this->getEntry(i);
				temp_array[2*i+1] = this->getEntry(i);
			}
			for(int i = 0; i ,this->mem_used_index; i ++)
			{	
				this->setEntry(temp_array[i]);
			}
		}
};


class ExtendibleHash 
{
	unsigned int numRecords;
	unsigned int level;
	unsigned int bucket_size;
	unsigned int numOverflowUsed;
	unsigned int overflow_start_idx;
	vector<bool> overflowBucketsUsed;
	Disk* memory;
	RAM* bucket_list;
	unsigned int hash(const int& x, int level)
	{
		return x >> (HASH_SHIFT - level);
	}
	void split(int bucket_idx);
	bool insert(Bucket&,const int&);
	int getNewOverflowBucket();
	bool addOverflowBucket(int bucket_addr);
	void recycleBucket(int bucket_addr);
	bool getBucketData(vector<int>& v);
	void rehash(int item = -1);
	void getBucketData(int bucket_addr, vector<int>& v);
	
	public:
		
		ExtendibleHash(Disk* mem, RAM* bucket_list) : numRecords(0), level(1), bucket_size(MAX_BUCKET_SIZE), numOverflowUsed(0),overflow_start_idx(OVERFLOW_START_INDEX)
		{
			this->memory = mem;
			this->bucket_list = bucket_list;
			overflowBucketsUsed = vector<bool>(memory->getStorageSize() - overflow_start_idx,false);
		}
		void insert(const int& x);
		bool search(const int& x);
		unsigned int N();
		unsigned int B();
		unsigned int b();
		void display();
};

void ExtendibleHash::insert(const int& x)
{
	unsigned int hash_value = hash(x, this->level);
	unsigned int bucket_addr = RAM->getEntry(hash_value);
	Bucket& bucketToAdd = memory->getBucket(bucket_addr);
	//Try Adding to the bucket
	if(!insert(bucketToAdd,x))
	{
		//Couldn't insert in bucket or its overflow pages. Need to add a new overflow page
		//Need to split next pointer.
		//Add Overflow page and retry insert
		if(bucketToAdd.getDepth() == this->level)
		{
			this->level += 1;
			this->RAM->doubleDirectory();
			this->insert(x);
			return;
		}
		else
		{
			bucketToAdd.setDepth();
			int buck_index = memory->getNewBucket();

		}
	}
	//Inserted Successfully
	numRecords++;
}

bool ExtendibleHash::search(const int& x)
{
	//Get bucket address for lookup table
	unsigned int bucket_addr = RAM->getBucket(hash(x));
	bool found = false;
	//Search in bucket and overflows [if there]
	int overflow_idx = bucket_addr;
	while(!found && overflow_idx != -1)
	{
		Bucket& b = memory->getBucket(overflow_idx);
		found = b.search(x);
		overflow_idx = b.getBucketOverflowIndex();
	}
	if(found) return true;
	return false;
}

//Insert record into bucket
bool ExtendibleHash::insert(Bucket& bucket,const int& x)
{
	// Bucket& bucket = memory->getBucket(bucket_addr);
	bool inserted = bucket.insertItem(x);
	int overflow_idx = bucket.getBucketOverflowIndex();
	while(!inserted && overflow_idx != -1)
	{
		Bucket& bucket1 = memory->getBucket(overflow_idx);
		inserted = bucket1.insertItem(x);
		overflow_idx = bucket1.getBucketOverflowIndex();
	}
	return inserted;
}

//Add Overflow Bucket to bucket_addr
bool ExtendibleHash::addOverflowBucket(int bucket_addr)
{
	Bucket& bucket = memory->getBucket(bucket_addr);
	if(bucket.getBucketOverflowIndex() != -1)
	{
		//Already has overflow bucket
		return addOverflowBucket(bucket.getBucketOverflowIndex());
	}
	int new_bucket_idx = getNewOverflowBucket();
	if(new_bucket_idx == -1) return false;
	overflowBucketsUsed[new_bucket_idx - overflow_start_idx] = true;
	numOverflowUsed++;
	bucket.setBucketOverflowIndex(new_bucket_idx);
	return true;
}

//Recycle and clear bucket and overflow buckets of bucket_addr
void ExtendibleHash::recycleBucket(int bucket_addr)
{
	//Clear and remove all overflow buckets
	if(bucket_addr == -1) return;
	Bucket& bucket = memory->getBucket(bucket_addr);
	int overflow_idx = bucket.getBucketOverflowIndex(); 
	if(overflow_idx != -1)
	{
		recycleBucket(overflow_idx);
		overflowBucketsUsed[overflow_idx-overflow_start_idx] = false;
		numOverflowUsed--;
	}
	bucket.setBucketOverflowIndex(-1);

	//Clear the bucket
	bucket.emptyBucket();
}

//Get a new overflow bucket from the pool
int ExtendibleHash::getNewOverflowBucket()
{
	int new_idx = overflow_start_idx;
	while(overflowBucketsUsed[new_idx-overflow_start_idx] && new_idx < (int)memory->getStorageSize()) 
	{
		new_idx++;
	}
	if(new_idx == (int)memory->getStorageSize()) return -1;
	else return new_idx;
}

void ExtendibleHash::getBucketData(int bucket_addr, vector<int>& v)
{
	if(bucket_addr == -1) return;
	do
	{
		Bucket& b = memory->getBucket(bucket_addr);
		for(int i = 0; i < b.size(); i++) {
			v.push_back(b.getItem(i));
		}
		bucket_addr = b.getBucketOverflowIndex();
	} while(bucket_addr != -1);
}

void ExtendibleHash::split(int bucket_addr)
{
	vector<int> temp;
	// Bucket& bucket = memory->getBucket(bucket_addr);
	getBucketData(bucket_addr,temp);
	recycleBucket(bucket_addr);

	unsigned int bucket2_addr = bucket_addr + (1 << level);
	// Bucket& bucket2 = memory->getBucket(bucket2_addr);

	int temp1 = bucket_addr;
	int temp2 = bucket2_addr;

	for(unsigned int i = 0; i < temp.size() ; i++)
	{
		if(hash(temp[i],level+1) == (unsigned int)bucket_addr) 
		{
			Bucket& bucket = memory->getBucket(temp1);
			//Add to first bucket-overflow
			if(bucket.isFull())
			{
				if(bucket.getBucketOverflowIndex() == -1)
				{
					if(!addOverflowBucket(temp1))
					{
						//No overflow buckets available
						cerr << "[ERROR] No overflow buckets available" << endl;
						return;
					}	
					temp1 = bucket.getBucketOverflowIndex();
				}
				// bucket = memory->getBucket(bucket.getBucketOverflowIndex());
			}
			insert(bucket, temp[i]);
		}
		else
		{
			Bucket& bucket = memory->getBucket(temp2);
			//Add to second bucket-overflow
			if(bucket.isFull())
			{
				if(bucket.getBucketOverflowIndex() == -1)
				{
					if(!addOverflowBucket(temp2))
					{
						//No overflow buckets available
						cerr << "[ERROR] No overflow buckets available" << endl;
						return;
					}	
					temp2 = bucket.getBucketOverflowIndex();
				}
				// bucket = memory->getBucket(bucket.getBucketOverflowIndex());
			}
			insert(bucket, temp[i]);
		}
	}
	//data[nextToSplit].clear();
}

void ExtendibleHash::display()
{
	cout << "Global Depth: " << global_depth << endl;
	// for(unsigned int table_addr = 0; table_addr < RAM->getStorageSize(); table_addr ++)
	// {
	// 	Bucket &b = RAM->getBucket(table_addr);
	// 	cout << "Bucket Label :{ " << RAM->getBucket(table_addr) << "} Depth: " << RAM->getBucket(table_addr)
	// 	cout << "Bucket Label :{" << bucket_addr << "} -- "; 
	// 	int t = bucket_addr;
	// 	do
	// 	{
	// 		Bucket& b = memory->getBucket(t);
	// 		cout << "#(" << t << "): ";
	// 		for(int i = 0; i < b.size(); i++) {
	// 			cout << b.getItem(i) << " ";
	// 		}
	// 		t = b.getBucketOverflowIndex();
	// 	} while(t != -1);
	// 	cout << endl;
	// }
}

unsigned int ExtendibleHash::N()
{
	return numRecords;
}

unsigned int ExtendibleHash::B()
{
	return size + numOverflowUsed;
}

unsigned int ExtendibleHash::b()
{
	return bucket_size;
}

int main()
{
	//Initialize a disk and connect hash to disk
	
	Disk* disk = new Disk();
	cerr << "[INFO] Initialized Disk" << endl;
	
	ExtendibleHash lh(disk);
	
	int n;
	cin >> n;
	int cnt = 0;
	int s = 0;
	for(int it = 0; it < n ; it++)
	{
		int x;
		cin >> x;
		lh.insert(x);
		cnt ++ ;
		if(cnt == 5000) 
		{
			x = rand() % n + 1;
			s += lh.search(x);
			cnt = 0;
		}
		cout << lh.N() / (1.0*lh.B() * lh.b()) << endl;
	}
	//lh.display();
	cerr << "N : " << lh.N() << "\nB: " << lh.B() << "\nb: " << lh.b() << "\ns : "<< s << endl;
	delete disk;
}