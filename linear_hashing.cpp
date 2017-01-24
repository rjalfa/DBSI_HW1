#include <iostream>
#include <vector>
#define MAX_BUCKET_SIZE 10
#define MAX_BUCKETS_ON_DISK 10000
#define OVERFLOW_START_INDEX 5000
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
			if(pos < 0 || pos >= numRecords) return false;
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
		Bucket() : bucketOverflowIndex(-1), bucket_size(BUCKET_SIZE), numRecords(0)
		{
			storage = new int[MAX_BUCKET_SIZE];
		}
		~Bucket()
		{
			delete[] storage;
		}
		void emptyBucket()
		{
			numRecords = 0;
			bucketOverflowIndex = -1;
		}
}


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
}


class LinearHash 
{
	unsigned int numRecords = 0;
	unsigned int nextToSplit = 0;
	unsigned int size = 1;
	unsigned int level = 1;
	unsigned int bucket_size = 1;
	unsigned int overflow_start_idx = 5000;
	vector<bool> overflowBucketsUsed;
	Disk* memory = nullptr;
	unsigned int hash(const int& x, unsigned int level)
	{
		return x % (1 << level);
	}
	void split(int bucket_idx);
	bool insert(const int& x, int bucket_idx);
	int getNewOverflowBucket();
	bool addOverflowBucket(int bucket_addr);
	void recycleBucket(int bucket_addr);
	public:
		void insert(const int& x);
		void display();
	
}

void LinearHash::insert(const int& x)
{
	//Get the bucket address to insert
	int bucket_addr = hash(x,level);
	if(bucket_addr < nextToSplit) bucket_addr = hash(x,level+1);
	
	//Try Adding to the bucket
	if(!memory->getBucket(bucket_addr).insertItem(x))
	{
		//Couldn't insert in bucket

	}

	data[bucket_addr].push_back(x);
	if(data[bucket_addr].size() > size) {
		//Overflow, so split bucket_addr
		data.push_back(vector<int>());
		vector<int> temp = data[nextToSplit];
		data[nextToSplit].clear();
		for(int i = 0 ; i < temp.size(); i ++)
		{
			int add = hash(temp[i],level+1);
			data[add].push_back(temp[i]);
		}
		nextToSplit ++;	
	}
	if(nextToSplit == (1 << level))
	{
		nextToSplit = 0;
		level++;
	}
}

//Insert record into bucket
bool LinearHash::insert(const int& x,int bucket_addr)
{
	Bucket& bucket = memory->getBucket(bucket_addr);
	bool inserted = bucket.insertItem(x);
	while(!inserted && bucket.getBucketOverflowIndex() != -1)
	{
		bucket = memory->getBucket(bucket.getBucketOverflowIndex());
		inserted = bucket.insertItem(x);
	}
	return inserted;
}

//Add Overflow Bucket to bucket_addr
bool LinearHash::addOverflowBucket(int bucket_addr)
{
	Bucket& bucket = memory->getBucket(bucket_addr);
	int new_bucket_idx = getNewOverflowBucket();
	if(new_bucket_idx == -1) return false;
	if(bucket.getBucketOverflowIndex != -1)
	{
		//Already has overflow bucket
		return addOverflowBucket(bucket.getBucketOverflowIndex);
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

void LinearHash::split(int bucket_addr)
{
	vector<int> temp;
	Bucket& bucket = 
	for(int i = 0; i < )
	data.push_back(vector<int>());
	data[nextToSplit].clear();
}

void LinearHash::rehash()
{
	
}

void LinearHash::display()
{
	cout << "Next to Split: " << nextToSplit << endl;
	cout << "Level: " << level << endl;
	for(int i = 0; i < data.size() ; i++)
	{
		cout << i << ": ";
		for(int j = 0; j < data[i].size(); j ++) cout << data[i][j] << " ";
		cout << endl;
	}
}

int main()
{

	data = vector<vector<int> >(2);
	int n;
	cin >> n;
	while(n--)
	{
		int x;
		cin >>x;
		insert(x);
		print();
	}
}