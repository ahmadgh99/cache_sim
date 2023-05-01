#ifndef COMPARCH_2_CACHE_H
#define COMPARCH_2_CACHE_H

#include "map"
#include "list"

using namespace std;

//Class simulates Block contains dirty simulating dirty block
class DataBlock{
public:
    bool Dirty;
};


// class for Set, each set contains a map of Ways according to the associativity level of the cache it sits in.
class Set{
public:

    list<unsigned> LRU; // list of unsigned tags to determine which block is the LRU in every set.
    map<unsigned,DataBlock> Ways; // map of <unsigned,DataBlock> for different Ways in a set, where unsigned is the set num.
    int SetSize; // num of bits for overall Ways in a set.
    int BlockSize; // num of bits for each Block that sits in the set.
    int level; // 1 for L1, 2 for L2.
    int SetBit; // num of bits for Set in the address.

    Set(int setSize, int blockSize,int lvl,int SetBit);
    /* used to update data in a set for each write cmd, updates LRU accordingly and updates dirty bit.
     in addition it makes space in cache if neccesary according to the LRU.*/
    void UpdateData(unsigned Address,unsigned data); 
    /*used to read data in a set for each read cmd, makes space in set (Cache) if neccesary according to the LRU and updates it  */
    void ReadData(unsigned Address);
    /* searchs if data is found in set and returns true. otherwise false.*/
    bool DoesDataExist(unsigned Address);
    /*check is all ways in set are filled.*/
    bool isFull() const;
    /*updates list of LRU blocks in set.*/
    void UpdateLRU(unsigned Address);
    /*returns Tag of address according to how many bits are offset and set bits*/
    unsigned GetTag(unsigned Address) const;
    /*removes block of address "Address" from set*/
    void RemoveBlock(unsigned Address);
    /*thrown when evacuating a block with dirty bit == True.*/
    class BlockDirtyException{};
    class ThrowTest{};
};

class Cache {
public:
    unsigned MemAccessTime;
    unsigned BlockSize;
    bool WriteAlloc;
    unsigned L1Size;
    unsigned L1Assoc;
    unsigned L2Size;
    unsigned L2Assoc;
    unsigned L1Cyc;
    unsigned L2Cyc;

    map<unsigned,Set> L1;
    map<unsigned,Set> L2;

    Cache(unsigned MemTime,unsigned SizeofBlock,unsigned WriteState,unsigned L1_size,
          unsigned L1_assoc,unsigned L2_size,unsigned L2_assoc, unsigned L1_cycle, unsigned L2_cycle);
    /* used by different Cache levels in each Write cmd. calls UpdateData method from Set and handles the write operation according to where a specific set is located.
    calculates #Accesses. #Misses for each cache level. and total MemCycles.*/
    void WriteData(unsigned address);
    /* similar to WriteData but used for Read operations.*/
    void ReadData(unsigned address);
    /*returns Set num according to Cache level and given Address.*/
    unsigned GetSet(int Level, unsigned Address) const;
    /*puts the MissRate calculation for each Cache level in returned parameter. */
    static void GetMissRate(double* L1MissRate,double* L2MissRate);
    /*calculates average access time for all commands in total*/
    static double GetAvgAccTime();
};



#endif //COMPARCH_2_CACHE_H
