#ifndef __CACHE_HPP__
#define __CACHE_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <bitset>
#include <cmath>

struct Block{ //Contains one block of valid, dirty, tag, index and offset bits
    std::vector<bool> valid={0}; //valid bit 
    std::vector<bool> dirty={0}; //dirty bit
    std::vector<bool> index;     //index bits
    std::vector<bool> tag;       //tag bits
    // std::vector<bool> offset;    //offset bits - still unclear as of now wheter this is even required

    //indexnum - number of bits for sets in cache = log(cachesize/(blocksize*assoc)), blocknum - number of bits for offset in block = log(Blocksize)
    Block(int indexnum, int offsetnum) : 
        index(indexnum, 0),
        tag(32 - indexnum - offsetnum, 0) {}
        // offset(offsetnum, 0) {} 
};

struct Set{ //Contains a aset which is the collection of Blocks (Number of Blocks in a set = Associativity of Cache)
    std::vector<Block> set; 
    std::vector<int> LRU; //Keeps track of history of usage of block (eg: [1,2,3,4] => first block recently used, last block least recently used)

    Set(int indexnum, int offsetnum, int assoc) :
        set(assoc,Block(indexnum,offsetnum)),  //initialize the set to contain n number of Blocks where n = Associativity of Cache)
        LRU(assoc,0) {}                        //initialize LRU to 0
};

struct Cache{
    std::vector<Set> cache;
    int setnum; //a variable to store the number of sets
    int associativity; //a variable to store the set asssociativity of the Cache
    int reads;
    int readmisses;
    int writes;
    int writemisses;
    int missrate;
    int writebacks;

    Cache(int Block_size, int Cache_size, int assoc) {
        int setnum = static_cast<int>(Cache_size/(assoc*Block_size)); //Number of sets
        int associativity = assoc;
        int indexnum = static_cast<int>(log2(setnum));                //Number of bits for sets
        int offsetnum = static_cast<int>(log2(Block_size));           //Number of bits for offset
        for (int i=1;i<=setnum;i++){
            cache.push_back(Set(indexnum,offsetnum,assoc));           //Adding sets to cache vector
        }
    }
};

struct Cache_Simulator{
    std::vector<std::vector<std::string,int>> instructions;

    Cache_Simulator(std::ifstream &file, int BLOCKSIZE, int L1_Size, int L1_Assoc, int L2_Size, int L2_Assoc){
        Cache L1 = Cache(BLOCKSIZE,L1_Size,L1_Assoc);
        Cache L2 = Cache(BLOCKSIZE,L2_Size,L2_Assoc);

    }

    void constructInstructions(){
        //function to read file and append into instructions in the format of [["r/w",address1],["r/w",address2],so on]
    }

    void MemoryManager(){
        //Divide into L1 index, L1 block, L1 tag, L2 index, L2 block, L2 tag
        //if "r" then L1_Read()
        //else L1_Write()
    }

    void convert(){
        //dunno if required but just to convert vector to number or string format if necessary
    }

    void LRU_update(){
        //function to update LRU
    }

    void L1_Read(){
        //Go to that index postion
        //Check all valid bits 
        //if all valid then check tag (loop for the number of assoc's)
            //if any tag valid then increment L1.reads, update LRU
            //else increment L1.readmisses, go to L2_Read(), go to L1_replace() [Since there may be writebacks], update LRU
        //else if any invalid bit present then set valid bit increment L1.readmisses and go to L2_Read(), replace that tag, update LRU
        //return
    }

    void L1_Write(){
        // Similar to L1_Read??

        //Go to that index position
        //Check all valid bits
        //if all valid then check tag (loop for number of assoc's)
            //if any tag found then update dirty bit, increment L1.writes, update LRU
            //else increment L1.writemisses, go to L2_Read(), go to L1_replace() [Since there may be writebacks], [update the value in this placed block], update dirty bit, update LRU
        //else if any valid bit present then set valid bit, increment L1.writemissses and go to L2_Read(), replace that tag, update dirty bit, update LRU
    }

    void L1_replace(){
        //function to replace the LRU with incoming tag and index bits
        //Check LRU position:
        //if dirty bit of LRU position is set (ie 1) then writeback this value onto L2 (should I call L2_write? -> should set dirty bit for L2 block ), increment L1.writebacks, replace the tag with new tag, unset dirty bit
        //else replace tag with new tag
        //return
    }

    void L2_Read(){
        //Go to that index postion
        //Check all valid bits 
        //if all valid then check tag (loop for the number of assoc's)
            //if any tag valid then increment L2.reads, update LRU
            //increment L2.readmisses, go to L2_replace() [Since there may be writebacks],update LRU
        //else if any invalid bit present then set valid bit, increment L2.readmisses, replace that tag, update LRU
        //return
    }

    void L2_Write(){

    }

    void L2_replace(){
        //function to replace the LRU with incoming tag and index bits
        //Check LRU position:
        //if dirty bit of LRU position is set (ie 1) then writeback this value onto Mem (what to do here?), increment L2.writebacks, replace the tag with new tag
        //else replace tag with new tag
        //return
    }

};

#endif