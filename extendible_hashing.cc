#include <iostream>
#include <vector>
#include <queue>
#include <cassert>	
#include <cstdlib>
#include <climits>
#include <ctime>
#include <cstdio>
#include <set>
int MAX_BUCKET_SIZE = 70;
#define MAX_BUCKETS_ON_DISK 1000000
#define MAX_BUCKETS_ON_RAM 1024
#define HASH_SHIFT 20

using namespace std;

template <class T>
ostream& operator<< (ostream &out,const vector<T>& V)
{
	for(int i=0;i < (int)V.size(); i++)
	{
		out<<V[i]<<" ";
	}
	out<<endl;
	return out;
}

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
			if(pos < 0 || pos >= bucket_size) return -1;
			return storage[pos];
		}
		int getDepth()
		{
			return this->depth;
		}
		void setDepth(int x)
		{
			this->depth	= x;	
		}
		int getIndex()
		{
			return this->index;
		}
		void setIndex(int x)
		{
			this->index	= x;	
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
		Bucket() : bucketOverflowIndex(-1), bucket_size(MAX_BUCKET_SIZE), numRecords(0), depth(-1), index(-1) { storage = new int[MAX_BUCKET_SIZE]; }
		~Bucket() { delete[] storage; }
		int size() { return this->numRecords; }
		bool isFull() { return this->numRecords == bucket_size; }
		void emptyBucket() {numRecords = 0;bucketOverflowIndex = -1;}
		
		Bucket(const Bucket& b) : bucketOverflowIndex(b.bucketOverflowIndex),  bucket_size(MAX_BUCKET_SIZE), numRecords(b.numRecords)
		{
			storage = new int[MAX_BUCKET_SIZE];
			for(int i = 0; i < numRecords; i ++ ) storage[i] = b.storage[i];
		}
		friend ostream& operator<<(ostream&,const Bucket&);
};


class Disk
{
	vector<Bucket> storage;
	queue<int> freeBuckets;
	int access_count;
	public:
		Disk()
		{
			access_count = 0;
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
			access_count ++ ;
			return storage[bucket_idx];
		}
		int getAccessCount()
		{
			return access_count;
		}
		int getNewBucket()
		{
			if(this->freeBuckets.empty()) return -1;
			int x = this->freeBuckets.front();
			this->freeBuckets.pop();
			return x;
		}
		void recycleBucket(int bucket_idx)
		{
			if(bucket_idx < 0 || bucket_idx >= (int)storage.size()) return;
			this->storage[bucket_idx].emptyBucket();
			this->freeBuckets.push(bucket_idx);
		}
		unsigned int getStorageSize()
		{
			return (unsigned int)storage.size();
		}
		unsigned int usedBuckets()
		{
			return (unsigned int)(storage.size() - freeBuckets.size());
		}
};


class RAM
{
	vector<int> storage;
	int mem_used_index;
	Disk* memory;
	vector<int> overflow_buckets;
	public:
		int getDirectorySize() {return mem_used_index;}
		RAM(Disk* mem)
		{
			this->memory = mem;
			this->mem_used_index = 2;
			this->storage = vector<int>(MAX_BUCKETS_ON_RAM, -1);
			//Acquire two buckets from memory
			storage[0] = mem->getNewBucket();
			Bucket& b0 = mem->getBucket(storage[0]);
			b0.setDepth(1);
			b0.setIndex(0);
			storage[1] = mem->getNewBucket();
			Bucket& b1 = mem->getBucket(storage[1]);
			b1.setDepth(1);
			b1.setIndex(1);
		}
		~RAM()
		{
			this->storage.clear();
		}
		int getEntry(int entry_idx) const
		{
			if(entry_idx >= mem_used_index)
			{
				return -1;
			}
			else{
				if(entry_idx < (int)storage.size())
				{
					return storage[entry_idx];		
				}
				else
				{
					entry_idx -= (int)storage.size();
					int bucket_idx = entry_idx/MAX_BUCKET_SIZE;
					Bucket& b = memory->getBucket(overflow_buckets[bucket_idx]);
					return b.getItem(entry_idx % MAX_BUCKET_SIZE);
				}
			}
			
		}
		int setEntry(int entry_idx,int a)
		{
			if(entry_idx >= mem_used_index)
			{
				return -1;
			}
			else{
				if(entry_idx < (int)storage.size())
				{
					storage[entry_idx] = a;
					return 0;
				}
				else
				{
					entry_idx -= (int)storage.size();
					int bucket_idx = entry_idx/MAX_BUCKET_SIZE;
					Bucket& b = memory->getBucket(overflow_buckets[bucket_idx]);
					b.setItem(a, entry_idx % MAX_BUCKET_SIZE);
					return overflow_buckets[bucket_idx];
				}
			}
		}
		unsigned int getStorageSize()
		{
			return (unsigned int)(storage.size() + overflow_buckets.size()*MAX_BUCKET_SIZE);
		}
		int doubleDirectory()
		{
			this->mem_used_index *= 2;
			int extra_buckets = (this->mem_used_index - (int)storage.size()) - ((int)(this->overflow_buckets.size()) * MAX_BUCKET_SIZE);
			int ct = 0;
			while(extra_buckets > 0)
			{
				int pt = memory->getNewBucket();
				overflow_buckets.push_back(pt);
				ct ++;
				if(extra_buckets >= MAX_BUCKET_SIZE)
				{
					Bucket& b = memory->getBucket(pt);
					for(int i = 0; i < MAX_BUCKET_SIZE; i ++) b.insertItem(-1); 
				}
				else
				{
					Bucket& b = memory->getBucket(pt);
					for(int i = 0; i < extra_buckets; i ++) b.insertItem(-1); 
				}
				extra_buckets -= MAX_BUCKET_SIZE;
			}
			vector<int> temp_array(this->mem_used_index);
			for(int i = 0; i < this->mem_used_index/2; i ++)
			{	
				temp_array[2*i] = this->getEntry(i);
				temp_array[2*i+1] = this->getEntry(i);
			}
			for(int i = 0; i < this->mem_used_index; i ++) assert(this->setEntry(i,temp_array[i]) != -1);
			return ct;
		}
		unsigned int numOverflowBuckets() { return (unsigned int)overflow_buckets.size(); }
		friend ostream& operator<<(ostream&,const RAM&);
};


class ExtendibleHash 
{
	unsigned int numRecords;
	unsigned int level;
	unsigned int bucket_size;
	Disk* memory;
	RAM* directory;
	unsigned int hash(const int& x, int shift)
	{
		if(HASH_SHIFT < shift) cerr << "OVERFLOW ERROR\n";
		return x >> (HASH_SHIFT - shift);
	}
	bool insert(Bucket&,const int&);
	void recycleBucket(int bucket_addr);
	int getBucketData(int bucket_addr, vector<int>& v);
	bool forceInsert(Bucket&, const int&,int);	
	public:
		
		ExtendibleHash(Disk* mem, RAM* ram) : numRecords(0), level(1), bucket_size(MAX_BUCKET_SIZE)
		{
			this->memory = mem;
			this->directory = ram;
		}
		int insert(const int& x);
		bool search(const int& x);
		unsigned int N();
		unsigned int B();
		unsigned int b();
		void display();
};

int ExtendibleHash::insert(const int& x)
{
	int c = 1;
	unsigned int hash_value = hash(x, this->level);
	unsigned int bucket_addr = directory->getEntry(hash_value);
	Bucket& bucketToAdd = memory->getBucket(bucket_addr);
	//Try Adding to the bucket
	if(bucketToAdd.isFull())
	{
		//Couldn't insert in bucket. Need to split/add overflow
		//Need to split next pointer.
		//Add Overflow page and retry insert
		if(bucketToAdd.getDepth() == (int)this->level)
		{
			this->level += 1;
			c += this->directory->doubleDirectory();
		}
		//Smaller level than global now
		vector<int> data;
		c += 2*getBucketData(bucket_addr,data);
		data.push_back(x);
	
		recycleBucket(bucket_addr);
		bucketToAdd.setDepth(bucketToAdd.getDepth() + 1);
		bucketToAdd.setIndex(bucketToAdd.getIndex() * 2);

		//Get new Bucket to add;
		int buck_index = memory->getNewBucket();
		Bucket& newBucket = memory->getBucket(buck_index);
		newBucket.setDepth(bucketToAdd.getDepth());
		newBucket.setIndex(bucketToAdd.getIndex() + 1);
		
		//Fix Directory pointers
		set<int> s;
		int y = newBucket.getIndex();
		for(int i = 0; i < (1 << (this->level - newBucket.getDepth())); i ++)
		{
			s.insert(directory->setEntry((y << (this->level - newBucket.getDepth())) + i, buck_index));
		}
		c += (int)s.size();
		//Rehash
		for(int i = 0; i < (int)data.size(); i ++)
		{
			unsigned int _hash_value = hash(data[i],this->level);
			unsigned int p_bucket_addr = directory->getEntry(_hash_value);
			if(p_bucket_addr == bucket_addr) forceInsert(bucketToAdd,data[i],p_bucket_addr);
			else forceInsert(newBucket,data[i],p_bucket_addr);
		}
	}
	else bucketToAdd.insertItem(x);
	//Inserted Successfully
	numRecords++;
	return c;
}

bool ExtendibleHash::search(const int& x)
{
	//Get bucket address for lookup table
	unsigned int bucket_addr = directory->getEntry(hash(x,this->level));
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

bool ExtendibleHash::forceInsert(Bucket& bucket, const int& x, int bucket_addr)
{
	bool inserted = bucket.insertItem(x);
	int overflow_idx = bucket.getBucketOverflowIndex();
	int prev_overflow_idx = bucket_addr;
	while(!inserted && overflow_idx != -1)
	{
		Bucket& bucket1 = memory->getBucket(overflow_idx);
		inserted = bucket1.insertItem(x);
		prev_overflow_idx = overflow_idx;
		overflow_idx = bucket1.getBucketOverflowIndex();
	}
	//Add Overflow Bucket if no space
	if(!inserted && overflow_idx == -1)
	{
		int new_bucket_idx = memory->getNewBucket();
		if(new_bucket_idx == -1) return false;
		if(prev_overflow_idx != bucket_addr)
		{
			Bucket& bucket0 = memory->getBucket(prev_overflow_idx);
			Bucket& bucket1 = memory->getBucket(new_bucket_idx);
			bucket1.setIndex(bucket0.getIndex());
			bucket1.setDepth(bucket0.getDepth());
			bucket0.setBucketOverflowIndex(new_bucket_idx);
			inserted = bucket1.insertItem(x);
		}
		else
		{
			Bucket& bucket1 = memory->getBucket(new_bucket_idx);
			bucket.setBucketOverflowIndex(new_bucket_idx);
			bucket1.setIndex(bucket.getIndex());
			bucket1.setDepth(bucket.getDepth());
			inserted = bucket1.insertItem(x);	
		}
	}
	return inserted;	
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
		memory->recycleBucket(overflow_idx);
	}
	bucket.setBucketOverflowIndex(-1);

	//Clear the bucket
	bucket.emptyBucket();
}

int ExtendibleHash::getBucketData(int bucket_addr, vector<int>& v)
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

void ExtendibleHash::display()
{
	cout << "Global Depth: " << this->level << endl;
	cout << "Data: \n" << *directory << endl;
}

ostream& operator<<(ostream& out,const Bucket& b)
{
	out << "{LD:"<<b.depth<<";idx:"<<b.index<<";ovf:"<<b.bucketOverflowIndex<<" [ ";
	for(int i = 0; i < b.numRecords; i++) out << b.storage[i] << " ";
	out << "] }";
	return out;
}

ostream& operator<<(ostream& out, const RAM& ram)
{
	out << "Directory Size: " << ram.mem_used_index << endl;
	for(int i = 0; i < ram.mem_used_index; i ++)
	{
		out << "INDEX " << i << ":= ";
		int t = ram.getEntry(i);
		while(t != -1)
		{
			out << "[" << t << "]";
			Bucket& b = (ram.memory)->getBucket(t);
			out << b << " ";
			t = b.getBucketOverflowIndex();
		}
		out << endl;
	}
	return out;
}

unsigned int ExtendibleHash::N()
{
	return numRecords;
}

unsigned int ExtendibleHash::B()
{
	return (this->memory)->usedBuckets() - (this->directory)->numOverflowBuckets();
}

unsigned int ExtendibleHash::b()
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
	RAM* ram = new RAM(disk);
	cerr << "[INFO] Initialized Disk" << endl;
	
	ExtendibleHash eh(disk,ram);
	
	int n;
	// cout << "Number of records: ";
	cin >> n;
	int cnt = 0;
	int s = 0;
	float till_here;
	long long cumsum = 0;
	for(int it = 0; it < n ; it++)
	{
		int x;
		cin >> x;
		// cout << "Enter Record: ";
		cout << eh.insert(x) << endl;
		data.push_back(x);
		cnt ++ ;
		// eh.display();
		if(cnt == 5000) 
		{
			for(int ii=0;ii<50;ii++){
				x = data[rand() % data.size()];
				int before = disk->getAccessCount();
				int cs = eh.search(x);
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
		// cout << eh.N() / (1.0*eh.B() * eh.b()) << endl;
		// getchar();
	}
	cerr << "N : " << eh.N() << "\nB: " << eh.B() << "\nb: " << eh.b() << "\ns : "<< s << endl;
	delete disk;
	delete ram;
}