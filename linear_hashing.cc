#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
int MAX_BUCKET_SIZE = 70;
#define MAX_BUCKETS_ON_DISK 1000000
#define OVERFLOW_START_INDEX 500000
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
	int access_count;
	public:
		int getAccessCount()
		{
			return access_count;
		}
		Disk()
		{
			access_count = 0;
			this->storage = vector<Bucket>(MAX_BUCKETS_ON_DISK);
		}
		~Disk()
		{
			this->storage.clear();
		}
		Bucket& getBucket(int bucket_idx)
		{
			access_count ++;
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
	unsigned int numOverflowUsed;
	unsigned int overflow_start_idx;
	vector<bool> overflowBucketsUsed;
	Disk* memory;
	unsigned int hash(const int& x, unsigned int l)
	{
		return x % (1 << l);
	}
	int split(int bucket_idx);
	bool insert(Bucket&,const int&);
	int getNewOverflowBucket();
	bool addOverflowBucket(int bucket_addr);
	void recycleBucket(int bucket_addr);
	void getBucketData(vector<int>& v);
	void rehash(int item = -1);
	int getBucketData(int bucket_addr, vector<int>& v);
	
	public:
		
		LinearHash(Disk* mem) : numRecords(0), nextToSplit(0), size(2), level(1), bucket_size(MAX_BUCKET_SIZE), numOverflowUsed(0),overflow_start_idx(OVERFLOW_START_INDEX)
		{
			this->memory = mem;
			overflowBucketsUsed = vector<bool>(memory->getStorageSize() - overflow_start_idx,false);
		}
		
		int insert(const int& x);
		bool search(const int& x);
		unsigned int N();
		unsigned int B();
		unsigned int b();
		void display();
};

int LinearHash::insert(const int& x)
{
	//Get the bucket address to insert
	int c = 0;
	unsigned int bucket_addr = hash(x,level);
	if(bucket_addr < nextToSplit) bucket_addr = hash(x,level+1);
	Bucket& bucketToAdd = memory->getBucket(bucket_addr);
	//Try Adding to the bucket
	if(bucketToAdd.isFull())
	{
		//Couldn't insert in bucket or its overflow pages. Need to add a new overflow page
		//Need to split next pointer.
		//Add Overflow page and retry insert
		if(!addOverflowBucket(bucket_addr)) {
			cerr << "[ERROR] No overflow buckets available" << endl;
			return 0;
		}
		if(!insert(bucketToAdd,x)) {
			//No More overflow pages. Can't add.
			cerr << "[ERROR] Internal error" << endl;
			return 0;
		}

		//Split the N bucket and rehash entries.
		c = split(nextToSplit);

		nextToSplit++;

		if(nextToSplit == (1u << level))
		{
			nextToSplit = 0;
			level++;
		}
	}
	else
	{
		this->insert(bucketToAdd, x);
	}
	//Inserted Successfully
	numRecords++;
	return c;
}

bool LinearHash::search(const int& x)
{
	unsigned int bucket_addr = hash(x,level);
	if(bucket_addr < nextToSplit) bucket_addr = hash(x,level+1);
	//Search in bucket and overflows [if there]
	bool found = false;
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
bool LinearHash::insert(Bucket& bucket,const int& x)
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
bool LinearHash::addOverflowBucket(int bucket_addr)
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
		numOverflowUsed--;
	}
	bucket.setBucketOverflowIndex(-1);

	//Clear the bucket
	bucket.emptyBucket();
}

//Get a new overflow bucket from the pool
int LinearHash::getNewOverflowBucket()
{
	int new_idx = overflow_start_idx;
	while(overflowBucketsUsed[new_idx-overflow_start_idx] && new_idx < (int)memory->getStorageSize()) 
	{
		new_idx++;
	}
	if(new_idx == (int)memory->getStorageSize()) return -1;
	else return new_idx;
}

int LinearHash::getBucketData(int bucket_addr, vector<int>& v)
{
	int a = 0;
	if(bucket_addr == -1) return 0;
	do
	{
		Bucket& b = memory->getBucket(bucket_addr);
		for(int i = 0; i < b.size(); i++) {
			v.push_back(b.getItem(i));
		}
		a ++ ;
		bucket_addr = b.getBucketOverflowIndex();
	} while(bucket_addr != -1);
	return a;
}

int LinearHash::split(int bucket_addr)
{
	vector<int> temp;
	int cost = 0;
	// Bucket& bucket = memory->getBucket(bucket_addr);
	cost += getBucketData(bucket_addr,temp);
	recycleBucket(bucket_addr);

	unsigned int bucket2_addr = bucket_addr + (1 << level);
	// Bucket& bucket2 = memory->getBucket(bucket2_addr);
	size ++;

	int temp1 = bucket_addr;
	int temp2 = bucket2_addr;
	cost += 2;
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
						return cost;
					}	
					temp1 = bucket.getBucketOverflowIndex();
				}
				cost ++;
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
						return cost;
					}	
					temp2 = bucket.getBucketOverflowIndex();
				}
				cost++;
				// bucket = memory->getBucket(bucket.getBucketOverflowIndex());
			}
			insert(bucket, temp[i]);
		}
	}
	return cost;
	//data[nextToSplit].clear();
}

void LinearHash::display()
{
	cout << "Next to Split: " << nextToSplit << endl;
	cout << "Level: " << level << endl;
	for(unsigned int bucket_addr = 0; bucket_addr < size; bucket_addr ++)
	{
		cout << "Bucket Label :{" << bucket_addr << "} -- "; 
		int t = bucket_addr;
		do
		{
			Bucket& b = memory->getBucket(t);
			cout << "#(" << t << "): ";
			for(int i = 0; i < b.size(); i++) {
				cout << b.getItem(i) << " ";
			}
			t = b.getBucketOverflowIndex();
		} while(t != -1);
		cout << endl;
	}
}

unsigned int LinearHash::N()
{
	return numRecords;
}

unsigned int LinearHash::B()
{
	return size + numOverflowUsed;
}

unsigned int LinearHash::b()
{
	return bucket_size;
}

vector<int> data;

int main(int argc, char** argv)
{
	srand((unsigned int)time(0));
	//Initialize a disk and connect hash to disk
	if(argc < 2) exit(1);
	MAX_BUCKET_SIZE = atoi(argv[1]);
	Disk* disk = new Disk();
	cerr << "[INFO] Initialized Disk" << endl;
	
	LinearHash lh(disk);
	
	int n;
	cin >> n;
	int cnt = 0;
	int s = 0;
	float till_here;
	long long cumsum = 0;
	for(int it = 0; it < n ; it++)
	{
		int x;
		cin >> x;
		cout << lh.insert(x) << endl;
		data.push_back(x);
		// cout << "Access Count: " << disk->getAccessCount() - i << endl;
		cnt ++ ;
		if(cnt == 5000) 
		{
			for(int ii=0;ii<50;ii++){
				x = data[rand() % data.size()];
				int before = disk->getAccessCount();
				int cs = lh.search(x);
				s += cs;	
				int after = disk->getAccessCount();
				if(cs)
				{
					cumsum += (after - before);
					till_here = (float) (cumsum) / (1.0f * (float)s);
				}
				//cout << till_here << endl;
			}
			cnt = 0;
		}
		
		// cout << lh.N() / (1.0*lh.B() * lh.b()) << endl;

		// lh.display();
	}
	cerr << "N : " << lh.N() << "\nB: " << lh.B() << "\nb: " << lh.b() << "\ns : "<< s << endl;
	delete disk;
}