#include "Cache.h"
#include <cmath>

//Stats Variables
unsigned L1Misses = 0;
unsigned L1Access = 0;
unsigned L2Misses = 0;
unsigned L2Access = 0;
unsigned TotalMemCycles = 0;
unsigned TimesAccessed = 0;


Set::Set(int size,int blockSize,int lvl,int bits){
    SetSize = size;
    BlockSize = blockSize;
    level = lvl;
    SetBit = bits;
}

void Set::UpdateData(unsigned int Address, unsigned int data) {

    if(Ways.find(GetTag(Address)) != Ways.end()){ // checks if block is already in set.
        UpdateLRU(Address);
        if(level == 1)// if block was found, make it MRU.
            Ways.find(GetTag(Address))->second.Dirty = true; // updates data.
        return;
    }
    //block isn't in Set, add it.
    if(isFull()){ // checks if all ways are occupied.
        if(Ways.find(LRU.back())->second.Dirty)
            throw BlockDirtyException(); // thrown whenever evacuated block was Dirty. 
        Ways.erase(Ways.find((LRU.back()))); // make space and evacuate LRU.
    }

    Ways.insert(Ways.begin(),std::pair<unsigned,DataBlock>(GetTag(Address),DataBlock())); // add new Block to set.
    UpdateLRU(Address); // update LRU accordingly.
    if(level == 1) // checks if new inserted block was inserted in L1 and updates dirty bit to True.
        Ways.find(GetTag(Address))->second.Dirty = true;
}

void Set::ReadData(unsigned int Address) { //
    if(Ways.find(GetTag(Address)) != Ways.end()){ // checks if block is already in set.
        UpdateLRU(Address); // if block was found, make it MRU.
        return;
    }
    /*if block wasnt found:*/
    if(isFull()){ // check if all ways are occupied. 
        if(Ways.find(LRU.back())->second.Dirty)
            throw BlockDirtyException(); // thrown if block to be evacuated had dirtyBit == True.
        Ways.erase(Ways.find((LRU.back()))); // evacuate LRU block.
        LRU.pop_back(); // evacuates LRU block from LRU list.
    }

    Ways.insert(Ways.begin(),std::pair<unsigned,DataBlock>(GetTag(Address),DataBlock()));// adds new block.
    UpdateLRU(Address); // updates LRU accordingly.
}

void Set::RemoveBlock(unsigned int Address) {
    list<unsigned int>::iterator it;
    for(it = LRU.begin(); it != LRU.end(); it++){
        if(*it == GetTag(Address)) { // search for Block.
            Ways.erase(Ways.find(*it)); // remove it from set.
            LRU.erase(it); // remove it from LRU list.
            return;
        }
    }
    throw ThrowTest();
}

bool Set::DoesDataExist(unsigned int Address) {
    return Ways.find(GetTag(Address)) != Ways.end(); // returns true when data was found in set.
}

bool Set::isFull() const {
    return LRU.size() == pow(2,SetSize); // returns true when size of LRU list is equal to num of ways.
}

void Set::UpdateLRU(unsigned int Address) { 
    list<unsigned>::iterator it;
    for(it = LRU.begin();it != LRU.end(); it++){
        if(*it == GetTag(Address)) { // checks if new data to LRU list was already there. pops it from its place and inserts at front of list(makes it MRU).
            LRU.push_front(*it);
            LRU.erase(it);
            return;
        }
    }
    /*if new data to LRU wasn't in list from before, add it.*/
    if(LRU.size() == pow(2,SetSize)) // check if LRU list reached max size and removes LRU element to make space.
        LRU.pop_back();

    LRU.push_front(GetTag(Address)); // new element to LRU list can be added now.
}

unsigned int Set::GetTag(unsigned int Address) const {
    return (Address>>(SetBit+BlockSize) & unsigned(pow(2,32-(SetBit+BlockSize))-1)); // calculates Tag according to num of bits for offset and set.
}

Cache::Cache(unsigned int MemTime, unsigned int SizeofBlock, unsigned int WriteState, unsigned int L1_size,
             unsigned int L1_assoc, unsigned int L2_size, unsigned int L2_assoc, unsigned int L1_cycle,
             unsigned int L2_cycle) {
    this->MemAccessTime = MemTime;
    this->BlockSize = SizeofBlock;
    this->WriteAlloc = WriteState;
    this->L1Size = L1_size;
    this->L1Assoc = L1_assoc;
    this->L1Cyc = L1_cycle;
    this->L2Size = L2_size;
    this->L2Assoc = L2_assoc;
    this->L2Cyc = L2_cycle;

    if(L1Size-BlockSize-L1_assoc == 0) // checks if L1 is fully Associative and handles Cache structure accordingly.
        L1.insert(L1.begin(),std::pair<unsigned,Set>(0,Set((int)(L1Size-BlockSize),BlockSize,1,0)));
    else // L1 isn't fully Associative. handle structure accordingly.
        for (int i = 0; i < pow(2,L1Size-BlockSize-L1_assoc); i++)
            L1.insert(L1.begin(),std::pair<unsigned,Set>(i,Set((int)(L1_assoc),BlockSize,1,L1Size-BlockSize-L1_assoc)));
    if(L2Size-BlockSize-L2_assoc == 0) // checks if L2 is fully Associative and handles Cache structure accordingly.
        L2.insert(L2.begin(),std::pair<unsigned,Set>(0, Set((int)(L2Size-BlockSize),BlockSize,2,0)));
    else // L2 isn't fully Associative. handle structure accordingly.
        for (int i = 0; i < pow(2,L2Size-BlockSize-L2_assoc); i++)
            L2.insert(L2.begin(),std::pair<unsigned,Set>(i,Set((int)(L2_assoc),BlockSize,2,L2Size-BlockSize-L2_assoc)));
}

void Cache::WriteData(unsigned int address) {

    Set& L1Set = L1.find(GetSet(1,address))->second; // L1Set is set to be the set in L1 in which data in "address" should be mapped to.
    Set& L2Set = L2.find(GetSet(2,address))->second; // L2Set is set to be the set in L2 in which data in "address" should be mapped to.

    TimesAccessed++; // updates in each new operation (w/r).
    L1Access++;
    TotalMemCycles += L1Cyc;

    if(L1Set.DoesDataExist(address)){ // if data already exists in L1, update it.
        L1Set.UpdateData(address,address);
        return;
    }
    /*otherwise, look for it in L2.*/
    L1Misses++;
    L2Access++;
    TotalMemCycles+=L2Cyc;

    if(L2Set.DoesDataExist(address)){ // data was found in L2.
        if(!WriteAlloc){ // check Write policy.
            L2Set.UpdateData(address,address); // policy was no-write allocate. therefor just update data where it was found.
            return;
        }else{ // policy was write allocate. bring data to L1.
            L2Set.UpdateLRU(address); // update LRU because we read data in L2 and found it.
            try{
                L1Set.UpdateData(address,address); // tries to bring data block from L2 to L1.
            }catch(Set::BlockDirtyException& e){ // caught if we tried to evacuate dirty Block from L1 to make space.
                unsigned L1Address = L1Set.LRU.back()<<(BlockSize+(L1Size-BlockSize-L1Assoc)); // calculate address of block to be thrown from L1 to make space.
                L1Address += GetSet(1,address)<<BlockSize; // calculate address of block to be thrown from L1 to make space.
                L1Set.Ways.find(L1Set.GetTag(L1Address))->second.Dirty = false; // update dirtyBit to be false.
                L1Set.UpdateData(address,address);
                L2.find(GetSet(2,L1Address))->second.UpdateLRU(L1Address);
            }
        }
        return;
    }
    /*block wasn't found in L2.*/
    L2Misses++;
    TotalMemCycles += MemAccessTime;

    if(!WriteAlloc)
        return; // reached memory and no write allocate. finish.

    /* write policy is write allocate, therefore bring block from memory to L2 and L1.*/
    if(!L2Set.isFull()){
        L2Set.ReadData(address);
        try{
            L1Set.UpdateData(address,address);
        }catch(Set::BlockDirtyException& e){
            unsigned L1Address = L1Set.LRU.back()<<(BlockSize+(L1Size-BlockSize-L1Assoc));
            L1Address += GetSet(1,address)<<BlockSize;
            L1Set.Ways.find(L1Set.GetTag(L1Address))->second.Dirty = false;
            L1Set.UpdateData(address,address);
            L2.find(GetSet(2,L1Address))->second.UpdateLRU(L1Address);
        }
        return;
    }
    /* handles snooping and makes space in L2.*/
    unsigned L2ThrowAdd = L2Set.LRU.back()<<(BlockSize+(L2Size-BlockSize-L2Assoc));
    L2ThrowAdd += GetSet(2,address)<<BlockSize;
    Set& L1ToCheck = L1.find(GetSet(1,L2ThrowAdd))->second;
    Set& L2ToThrow = L2.find(GetSet(2,L2ThrowAdd))->second;
    if(L1ToCheck.Ways.find(L1ToCheck.GetTag(L2ThrowAdd)) != L1ToCheck.Ways.end() &&
    L1ToCheck.Ways.find(L1ToCheck.GetTag(L2ThrowAdd))->second.Dirty){
        L1ToCheck.Ways.find(L1ToCheck.GetTag(L2ThrowAdd))->second.Dirty = false;
    }
    if(L1ToCheck.Ways.find(L1ToCheck.GetTag(L2ThrowAdd)) != L1ToCheck.Ways.end())
        L1ToCheck.RemoveBlock(L2ThrowAdd);
    L2Set.UpdateData(address,address);
    try{
        L1Set.UpdateData(address,address);
    }catch(Set::BlockDirtyException& e){
        unsigned L1Address = L1Set.LRU.back()<<(BlockSize+(L1Size-BlockSize-L1Assoc));
        L1Address += GetSet(1,address)<<BlockSize;
        L1Set.Ways.find(L1Set.GetTag(L1Address))->second.Dirty = false;
        L1Set.UpdateData(address,address);
        L2.find(GetSet(2,L1Address))->second.UpdateLRU(L1Address);
    }
}

void Cache::ReadData(unsigned int address) {

    Set& L1Set = L1.find(GetSet(1,address))->second;
    Set& L2Set = L2.find(GetSet(2,address))->second;

    TimesAccessed++;
    L1Access++;
    TotalMemCycles += L1Cyc;

    if(L1Set.DoesDataExist(address)) { // checks if data already exists in L1 and updates LRU accordingly.
        L1Set.UpdateLRU(address);
        return;
    }
    /* data wasn't found in L1.*/
    L1Misses++;
    L2Access++;
    TotalMemCycles += L2Cyc;

    if(L2Set.DoesDataExist(address)){ // checks if data already exists in L2 and updates LRU accordingly.
        L2Set.UpdateLRU(address);
        try{
            L1Set.ReadData(address);
        }catch(Set::BlockDirtyException& e){ // caught if a block with dirtyBit == true was evacuated to make space.
            unsigned L1Addr = L1Set.LRU.back()<<(BlockSize+(L1Size-BlockSize-L1Assoc));
            L1Addr += GetSet(1,address)<<BlockSize;
            L1Set.Ways.find(L1Set.GetTag(L1Addr))->second.Dirty = false; // sets dirtyBit to false.
            L1Set.ReadData(address);
            L2.find(GetSet(2,L1Addr))->second.UpdateLRU(L1Addr);
        }
        return;
    }
    /*block not found in L2.*/
    L2Misses++;
    TotalMemCycles += MemAccessTime;
    /*make space for block to be brought to L2 and L1, check for snooping and handle it accordingly.*/
    if(!L2Set.isFull()){
        L2Set.ReadData(address);
        try{
            L1Set.ReadData(address);
        }catch(Set::BlockDirtyException& e){
            unsigned L1Addr = L1Set.LRU.back()<<(BlockSize+(L1Size-BlockSize-L1Assoc));
            L1Addr += GetSet(1,address)<<BlockSize;
            L1Set.Ways.find(L1Set.GetTag(L1Addr))->second.Dirty = false;
            L1Set.ReadData(address);
            L2.find(GetSet(2,L1Addr))->second.UpdateLRU(L1Addr);
        }
        return;
    }
    /*snooping*/
    unsigned L2ThrowAdd = L2Set.LRU.back()<<(BlockSize+(L2Size-BlockSize-L2Assoc));
    L2ThrowAdd += GetSet(2,address)<<BlockSize;
    Set& L1ToCheck = L1.find(GetSet(1,L2ThrowAdd))->second;
    Set& L2ToThrow = L2.find(GetSet(2,L2ThrowAdd))->second;
    if(L1ToCheck.Ways.find(L1ToCheck.GetTag(L2ThrowAdd)) != L1ToCheck.Ways.end() &&
       L1ToCheck.Ways.find(L1ToCheck.GetTag(L2ThrowAdd))->second.Dirty){
        L1ToCheck.Ways.find(L1ToCheck.GetTag(L2ThrowAdd))->second.Dirty = false;
    }
    if(L1ToCheck.Ways.find(L1ToCheck.GetTag(L2ThrowAdd)) != L1ToCheck.Ways.end())
        L1ToCheck.RemoveBlock(L2ThrowAdd);

    L2Set.ReadData(address);

    try{
        L1Set.ReadData(address);
    }catch(Set::BlockDirtyException& e){
        unsigned L1Addr = L1Set.LRU.back()<<(BlockSize+(L1Size-BlockSize-L1Assoc));
        L1Addr += GetSet(1,address)<<BlockSize;
        L1Set.Ways.find(L1Set.GetTag(L1Addr))->second.Dirty = false;
        L1Set.ReadData(address);
        L2.find(GetSet(2,L1Addr))->second.UpdateLRU(L1Addr);
    }
}

unsigned Cache::GetSet(int Level, unsigned int Address) const {
    if(Level == 1){
        return (Address>>BlockSize & unsigned(pow(2,L1Size-BlockSize-L1Assoc)-1)); // calculate Set for L1.
    }else{
        return (Address>>BlockSize & unsigned(pow(2,L2Size-BlockSize-L2Assoc)-1));// calculate Set for L2.
    }
}

void Cache::GetMissRate(double *L1MissRate, double *L2MissRate) {
 *L1MissRate = (double)L1Misses/(double)L1Access;
 *L2MissRate = (double)L2Misses/(double)L2Access;
}

double Cache::GetAvgAccTime() {
    return (double)TotalMemCycles/(double)TimesAccessed;
}