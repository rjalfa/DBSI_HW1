#include <iostream>
#include <vector>
#include <queue>
#include <cstdlib>
#include <climits>
#define MAX_BUCKET_SIZE 2
#define MAX_BUCKETS_ON_DISK 1000000
#define MAX_BUCKETS_ON_RAM 1024
#define OVERFLOW_START_INDEX 500000
#define HASH_SHIFT 19

using namespace std;

template <class T>
ostream& operator<< (ostream &out, vector<T> V)
{
	for(int i=0;i < V.size(); i++)
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
			if(pos < 0 || pos >= numRecords) return -1;
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

		friend ostream& operator<<(ostream&,const Bucket&);
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
		int getEntry(int bucket_idx) const
		{
			if(bucket_idx >= mem_used_index)
			{
				return -1;
			}
			else{
				if(bucket_idx < (int)storage.size())
				{
					return storage[bucket_idx];		
				}
				else
				{
					bucket_idx -= (int)storage.size();
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
				if(entry_idx < (int)storage.size())
				{
					storage[entry_idx] = a;
					return true;
				}
				else
				{
					entry_idx -= (int)storage.size();
					int bucket_idx = entry_idx/MAX_BUCKET_SIZE;
					Bucket& b = memory->getBucket(overflow_buckets[bucket_idx]);
					bool pp = b.insertItem(a);
					return pp;
				}
			}
		}
		unsigned int getStorageSize()
		{
			return (unsigned int)(storage.size() + overflow_buckets.size()*MAX_BUCKET_SIZE);
		}
		void doubleDirectory()
		{
			this->mem_used_index *= 2;
			int extra_buckets = (this->mem_used_index - (int)storage.size()) - ((int)(this->overflow_buckets.size()) * MAX_BUCKET_SIZE);
			while(extra_buckets > 0)
			{
				overflow_buckets.push_back(memory->getNewBucket());
				extra_buckets -= MAX_BUCKET_SIZE;
			}
			vector<int> temp_array(this->mem_used_index);
			for(int i = 0; i < this->mem_used_index/2; i ++)
			{	
				temp_array[2*i] = this->getEntry(i);
				temp_array[2*i+1] = this->getEntry(i);
			}
			for(int i = 0; i < this->mem_used_index; i ++) this->setEntry(i,temp_array[i]);
		}

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
		return x >> (HASH_SHIFT - shift + 1);
	}
	bool insert(Bucket&,const int&);
	void recycleBucket(int bucket_addr);
	void getBucketData(int bucket_addr, vector<int>& v);
	bool forceInsert(Bucket&, const int&,int);	
	public:
		
		ExtendibleHash(Disk* mem, RAM* ram) : numRecords(0), level(1), bucket_size(MAX_BUCKET_SIZE)
		{
			this->memory = mem;
			this->directory = ram;
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
	unsigned int bucket_addr = directory->getEntry(hash_value);
	Bucket& bucketToAdd = memory->getBucket(bucket_addr);
	//Try Adding to the bucket
	if(!insert(bucketToAdd,x))
	{
		//Couldn't insert in bucket or its overflow pages. Need to split/add overflow
		//Need to split next pointer.
		//Add Overflow page and retry insert
		if(bucketToAdd.getDepth() == (int)this->level)
		{
			this->level += 1;
			this->directory->doubleDirectory();
		}
		//Smaller level than global now
		bucketToAdd.setDepth(bucketToAdd.getDepth() + 1);
		bucketToAdd.setIndex(bucketToAdd.getIndex() * 2);
		vector<int> data;
		getBucketData(bucket_addr,data);
		data.push_back(x);
	
		recycleBucket(bucket_addr);
		
		//Get new Bucket to add;
		int buck_index = memory->getNewBucket();
		Bucket& newBucket = memory->getBucket(buck_index);
		newBucket.setDepth(bucketToAdd.getDepth());
		newBucket.setIndex(bucketToAdd.getIndex() * 2 + 1);
		
		//Fix Directory pointers
		int y = newBucket.getIndex();
		for(int i = 0; i < (1 << (this->level - newBucket.getDepth())); i ++)
		{
			directory->setEntry((y << (this->level - newBucket.getDepth())) + i, buck_index);
		}
		
		// display();

		//Rehash
		for(int i = 0; i < (int)data.size(); i ++)
		{
			unsigned int _hash_value = hash(data[i],this->level);
			unsigned int p_bucket_addr = directory->getEntry(_hash_value);
			if(p_bucket_addr == bucket_addr) forceInsert(bucketToAdd,data[i],p_bucket_addr);
			else forceInsert(newBucket,data[i],p_bucket_addr);
		}
	}
	//Inserted Successfully
	numRecords++;
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

void ExtendibleHash::display()
{
	cout << "Global Depth: " << this->level << endl;
	cout << "Data: \n" << *directory << endl;
	// for(unsigned int table_addr = 0; table_addr < directory->getStorageSize(); table_addr ++)
	// {
	// 	Bucket &b = directory->getBucket(table_addr);
	// 	cout << "Bucket Label :{ " << directory->getBucket(table_addr) << "} Depth: " << directory->getBucket(table_addr)
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
	return (this->memory)->usedBuckets();
}

unsigned int ExtendibleHash::b()
{
	return bucket_size;
}

int main()
{
	//Initialize a disk and connect hash to disk
	
	Disk* disk = new Disk();
	RAM* ram = new RAM(disk);
	cerr << "[INFO] Initialized Disk" << endl;
	
	ExtendibleHash eh(disk,ram);
	
	int n;
	// cout << "Number of records: ";
	cin >> n;
	int cnt = 0;
	int s = 0;
	for(int it = 0; it < n ; it++)
	{
		int x;
		cin >> x;
		// cout << "Enter Record: ";
		eh.insert(x);
		cnt ++ ;
		// eh.display();
		if(cnt == 5000) 
		{
			x = rand() % n + 1;
			s += eh.search(x);
			cnt = 0;
		}
		cout << eh.N() / (1.0*eh.B() * eh.b()) << endl;
	}
	cerr << "N : " << eh.N() << "\nB: " << eh.B() << "\nb: " << eh.b() << "\ns : "<< s << endl;
	delete disk;
	delete ram;
}