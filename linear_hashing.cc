#include <iostream>
#include <vector>
#define MAX_BUCKET_SIZE 2
#define MAX_BUCKETS_ON_DISK 100
#define OVERFLOW_START_INDEX 50
using namespace std;

class Bucket
{
	int* storage;
	int bucketOverflowIndex;
	int bucket_size;
	int numRecords;
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
		void setBucketOverflowIndex(int a)
		{
			this->bucketOverflowIndex = a;
		}
		int getBucketOverflowIndex()
		{
			return this->bucketOverflowIndex;
		}
		Bucket() : bucketOverflowIndex(-1), bucket_size(MAX_BUCKET_SIZE), numRecords(0)
		{
			storage = new int[MAX_BUCKET_SIZE];
		}
		~Bucket()
		{
			delete[] storage;
		}
		Bucket(const Bucket& b) : bucketOverflowIndex(b.bucketOverflowIndex),  bucket_size(MAX_BUCKET_SIZE), numRecords(b.numRecords)
		{
			storage = new int[MAX_BUCKET_SIZE];
			for(int i = 0; i < numRecords; i ++ ) storage[i] = b.storage[i];
		}
		void emptyBucket()
		{
			numRecords = 0;
			bucketOverflowIndex = -1;
		}
		int size()
		{
			return this->numRecords;
		}
		bool isFull()
		{
			return this->numRecords == bucket_size; 
		}
};


class Disk
{
	vector<Bucket> storage;
	public:
		Disk()
		{
			this->storage = vector<Bucket>(MAX_BUCKETS_ON_DISK);
		}
		~Disk()
		{
			this->storage.clear();
		}
		Bucket& getBucket(int bucket_idx)
		{
			return storage[bucket_idx];
		}
		unsigned int getStorageSize()
		{
			return (unsigned int)storage.size();
		}
};


class LinearHash 
{
	unsigned int numRecords;
	unsigned int nextToSplit;
	unsigned int size;
	unsigned int level;
	unsigned int bucket_size;
	unsigned int overflow_start_idx;
	vector<bool> overflowBucketsUsed;
	Disk* memory;
	unsigned int hash(const int& x, unsigned int l)
	{
		return x % (1 << l);
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
		LinearHash(Disk* mem) : numRecords(0), nextToSplit(0), size(2), level(1), bucket_size(MAX_BUCKET_SIZE), overflow_start_idx(OVERFLOW_START_INDEX)
		{
			this->memory = mem;
			overflowBucketsUsed = vector<bool>(memory->getStorageSize() - overflow_start_idx,false);
		}
		void insert(const int& x);
		void display();
};

void LinearHash::insert(const int& x)
{
	//Get the bucket address to insert
	unsigned int bucket_addr = hash(x,level);
	if(bucket_addr < nextToSplit) bucket_addr = hash(x,level+1);
	Bucket& bucketToAdd = memory->getBucket(bucket_addr);
	cerr << "[INFO] Hash value: " << bucket_addr << endl;
	//Try Adding to the bucket
	if(!insert(bucketToAdd,x))
	{
		//Couldn't insert in bucket or its overflow pages. Need to add a new overflow page
		//Need to split next pointer.
		//Add Overflow page and retry insert
		if(!addOverflowBucket(bucket_addr)) {
			cerr << "[ERROR] No overflow buckets available" << endl;
			return;
		}
		else cerr << "[INFO] Overflow bucket added : " << bucketToAdd.getBucketOverflowIndex()<< endl;
		if(!insert(bucketToAdd,x)) {
			//No More overflow pages. Can't add.
			cerr << "[ERROR] Internal error" << endl;
			return;
		}

		//Split the N bucket and rehash entries.
		split(nextToSplit);

		nextToSplit++;

		if(nextToSplit == (1u << level))
		{
			nextToSplit = 0;
			level++;
		}
	}
}

//Insert record into bucket
bool LinearHash::insert(Bucket& bucket,const int& x)
{
	// Bucket& bucket = memory->getBucket(bucket_addr);
	bool inserted = bucket.insertItem(x);
	bool has_overflow = (bucket.getBucketOverflowIndex() != -1);
	while(!inserted && has_overflow)
	{
		Bucket& bucket1 = memory->getBucket(bucket.getBucketOverflowIndex());
		inserted = bucket1.insertItem(x);
		has_overflow = (bucket1.getBucketOverflowIndex() != -1);
	}
	return inserted;
}

//Add Overflow Bucket to bucket_addr
bool LinearHash::addOverflowBucket(int bucket_addr)
{
	Bucket& bucket = memory->getBucket(bucket_addr);
	int new_bucket_idx = getNewOverflowBucket();
	if(new_bucket_idx == -1) return false;
	if(bucket.getBucketOverflowIndex() != -1)
	{
		//Already has overflow bucket
		return addOverflowBucket(bucket.getBucketOverflowIndex());
	}
	overflowBucketsUsed[new_bucket_idx - overflow_start_idx] = true;
	bucket.setBucketOverflowIndex(new_bucket_idx);
	return true;
}

//Recycle and clear bucket and overflow buckets of bucket_addr
void LinearHash::recycleBucket(int bucket_addr)
{
	//Clear and remove all overflow buckets
	if(bucket_addr == -1) return;
	Bucket& bucket = memory->getBucket(bucket_addr);
	int overflow_idx = bucket.getBucketOverflowIndex(); 
	if(overflow_idx != -1)
	{
		recycleBucket(overflow_idx);
		overflowBucketsUsed[overflow_idx-overflow_start_idx] = false;
	}
	bucket.setBucketOverflowIndex(-1);

	//Clear the bucket
	bucket.emptyBucket();
}

//Get a new overflow bucket from the pool
int LinearHash::getNewOverflowBucket()
{
	int new_idx = overflow_start_idx;
	while(overflowBucketsUsed[new_idx-overflow_start_idx] && new_idx < MAX_BUCKETS_ON_DISK) new_idx++;
	if(new_idx == MAX_BUCKETS_ON_DISK) return -1;
	else return new_idx;
}

void LinearHash::getBucketData(int bucket_addr, vector<int>& v)
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

void LinearHash::split(int bucket_addr)
{
	vector<int> temp;
	// Bucket& bucket = memory->getBucket(bucket_addr);
	getBucketData(bucket_addr,temp);
	recycleBucket(bucket_addr);

	unsigned int bucket2_addr = bucket_addr + (1 << level);
	// Bucket& bucket2 = memory->getBucket(bucket2_addr);
	size ++;

	int temp1 = bucket_addr;
	int temp2 = bucket2_addr;
	cerr << "[INFO] Splitting bucket in " << bucket_addr << " " << bucket2_addr << endl;

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

void LinearHash::display()
{
	cout << "Next to Split: " << nextToSplit << endl;
	cout << "Level: " << level << endl;
	for(unsigned int i = 0; i < size; i ++)
	{
		vector<int> t;
		getBucketData(i,t);
		cout << "Bucket #" << i <<" : ";
		for(unsigned int j = 0; j < t.size(); j++) cout << t[j] << " ";
		cout << endl; 
	}
}

int main()
{
	//Initialize a disk and connect hash to disk
	
	Disk* disk = new Disk();
	cerr << "[INFO] Initialized Disk" << endl;
	
	LinearHash lh(disk);
	
	int n;
	cout << "Enter number of entries: ";
	cin >> n;
	while(n--)
	{
		int x;
		cout << "Enter record: ";
		cin >> x;
		lh.insert(x);
		lh.display();
	}
}