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
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

struct Block
{                            // Contains one block of valid, dirty, tag, index and offset bits
    std::string valid = "0"; // valid bit
    std::string dirty = "0"; // dirty bit
    std::string index;       // index bits - Found a use -> Required to get back the original memory address for recalculation of L2tag and L2index during replacement
    std::string tag;         // tag bits
    // std::vector<bool> offset;    //offset bits - still unclear as of now wheter this is even required

    // indexnum - number of bits for sets in cache = log(cachesize/(blocksize*assoc)), blocknum - number of bits for offset in block = log(Blocksize)
    Block(int indexnum, int offsetnum) : index(indexnum, 0),
                                         tag(64 - indexnum - offsetnum, 0) {}
    // offset(offsetnum, 0) {}
};

struct Set
{ // Contains a aset which is the collection of Blocks (Number of Blocks in a set = Associativity of Cache)
    std::vector<Block> set;
    std::vector<int> LRU; // Keeps track of history of usage of block (eg: [1,2,3,4] => first block recently used, last block least recently used)

    Set(int indexnum, int offsetnum, int assoc) : set(assoc, Block(indexnum, offsetnum)), // initialize the set to contain n number of Blocks where n = Associativity of Cache)
                                                  LRU(assoc)
    {
        for (int i = 0; i < assoc; i++)
        {
            LRU[i] = i; // intialise LRU with [1,2,3,...]
        }
    }
};

struct Cache
{
    std::vector<Set> cache;
    int indexnum;      // a variable to store number of bits for index
    int tagnum;        // a variable to store number of bits for tag
    int setnum;        // a variable to store the number of sets
    int associativity; // a variable to store the set asssociativity of the Cache
    int offsetnum;     // a variable to store number of bits required for offset
    int reads = 0;
    int readmisses = 0;
    int writes = 0;
    int writemisses = 0;
    int missrate = 0;
    int writebacks = 0;

    Cache(int Block_size, int Cache_size, int assoc)
    {
        setnum = static_cast<int>(Cache_size / (assoc * Block_size)); // Number of sets
        associativity = assoc;
        indexnum = static_cast<int>(log2(setnum));      // Number of bits for sets
        offsetnum = static_cast<int>(log2(Block_size)); // Number of bits for offset
        tagnum = 64 - indexnum - offsetnum;
        for (int i = 1; i <= setnum; i++)
        {
            cache.push_back(Set(indexnum, offsetnum, assoc)); // Adding sets to cache vector
        }
    }
};

struct Cache_Simulator
{
    // std::vector<std::vector<std::string,std::bitset<32>>> instructions;
    std::ifstream file;
    Cache L1;
    Cache L2;
    int L2_SIZE;

    Cache_Simulator(const std::string &filename, int BLOCKSIZE, int L1_Size, int L1_Assoc, int L2_Size, int L2_Assoc) : file(filename),
                                                                                                                        L1(Cache(BLOCKSIZE, L1_Size, L1_Assoc)),
                                                                                                                        L2(Cache(BLOCKSIZE, L2_Size, L2_Assoc)),
                                                                                                                        L2_SIZE(L2_Size) {}

    void StartSimulator()
    {
        // function to read file and call MemoryManager
        if (!file.is_open())
        {
            std::cout << "Error in opening file"
                      << "/n";
        }
        else
        {
            std::string line;
            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                std::string word1, word2, address;
                iss >> word1 >> std::ws >> word2;
                long long hex_int = std::stoll(word2, nullptr, 16);
                // convert integer to binary string
                address = std::bitset<64>(hex_int).to_string();
                MemoryManager(word1, address);
                // std::cout << "Line: " << line << "\n";
                // printstate();
                // printfinalans();
                // std::cout << "\n";
            }
            printfinalans();

            file.close();
        }
    }

    void MemoryManager(std::string &type, std::string address)
    {
        // Divide into L1 index, L1 block, L1 tag, L2 index, L2 block, L2 tag
        std::string L1tagbits = address.substr(0, L1.tagnum);
        std::string L1indexbits = address.substr(L1.tagnum, L1.indexnum);
        std::string L2tagbits = address.substr(0, L2.tagnum);
        std::string L2indexbits = address.substr(L2.tagnum, L2.indexnum);

        // if "r" then L1_Read()
        // else L1_Write()
        if (type == "r")
        {
            L1_Read(L1tagbits, L1indexbits, L2tagbits, L2indexbits);
        }
        else
        {
            L1_Write(L1tagbits, L1indexbits, L2tagbits, L2indexbits);
        }
    }

    int binaryStringToDecimal(const std::string &binaryStr)
    {
        // Create a bitset from the binary string
        std::bitset<64> bits(binaryStr);

        // Convert the bitset to an unsigned long long int and return it
        return static_cast<int>(bits.to_ulong());
    }

    void LRU_update(std::vector<int> *LRU, int mostrecent)
    {
        // function to update LRU

        // Logic as proposed by brian -> LRU[0] contains the index for the block that was least recently used,
        // upon using a block place it on the last index of LRU -> Self updating LRU

        // mostrecent is the index of most recently used block in Set

        int mostrecentindex = 0; // the position of the mostrecent in LRU
        for (int i = 0; i < (*LRU).size(); i++)
        {
            if ((*LRU)[i] == mostrecent)
            {
                mostrecentindex = i;
            }
        }
        std::rotate((*LRU).begin() + mostrecentindex, (*LRU).begin() + mostrecentindex + 1, (*LRU).end()); // rotate the vector as prescribed earlier
    }

    void L1_Read(std::string L1tagbits, std::string L1indexbits, std::string L2tagbits, std::string L2indexbits)
    {
        // Go to that index postion
        // Check all valid bits
        // if all valid then check tag (loop for the number of assoc's)
        // if any tag valid then increment L1.reads, update LRU
        // else increment L1.readmisses, go to L2_Read(), go to L1_replace() [Since there may be writebacks], update LRU
        // else if any invalid bit present then set valid bit increment L1.readmisses and go to L2_Read(), replace that tag, update LRU
        // return

        int L1index = binaryStringToDecimal(L1indexbits);
        // Set L1Set = L1.cache[L1index];
        int hit_achieved = 0;
        int valid_count = 0;
        // First loop through valid blocks
        for (int i = 0; i < L1.associativity; i++)
        {
            if (L1.cache[L1index].set[i].valid == "1")
            {
                valid_count++;

                if (L1.cache[L1index].set[i].tag == L1tagbits)
                { // This is the data we need
                    //* L1 hit
                    L1.reads += 1;                         // update L1.reads
                    LRU_update(&L1.cache[L1index].LRU, i); // update LRU

                    hit_achieved = 1;
                    //! instead of setting hit_achieved, we could put return statement here
                    break;
                }
            }
        }
        if (hit_achieved == 0)
        {
            if (valid_count == L1.associativity)
            {
                //* L1 read miss, on valid block. executes replace. independent of i.
                L1.reads += 1;                                                // update the reads (since its trying to be read -> according to testcase result on piazza)
                L1.readmisses += 1;                                           // update L1 read miss since we are going to L2
                L2_Read(L2tagbits, L2indexbits);                              // Read from L2
                L1_Replace(&L1.cache[L1index], L1tagbits, L1indexbits);       // set the tag bits of the least recently used block (done in Replace function to incorporate writebacks)
                LRU_update(&L1.cache[L1index].LRU, L1.cache[L1index].LRU[0]); // update LRU  -> note that the most recently used will now be the LRU[0] because that is where the new tagbits come in
            }
            else
            {
                for (int i = 0; i < L1.associativity; i++)
                {
                    if (L1.cache[L1index].set[i].valid == "0") // traverse through invalid blocks, to find first invalid block
                    {
                        //* L1 read miss, fill on invalid block
                        L1.reads += 1;                                // update the reads (since its trying to be read -> according to testcase result on piazza))
                        L1.readmisses += 1;                           // update L1 read miss since we are going to L2
                        L2_Read(L2tagbits, L2indexbits);              // Read from L2
                        L1.cache[L1index].set[i].tag = L1tagbits;     // set the tag bits as the required one
                        L1.cache[L1index].set[i].index = L1indexbits; // set the index bits as required one
                        L1.cache[L1index].set[i].valid = "1";         // set the valid bit for this block
                        LRU_update(&L1.cache[L1index].LRU, i);        // update LRU
                        break;
                    }
                }
            }
        }
    }

    void L1_Write(std::string L1tagbits, std::string L1indexbits, std::string L2tagbits, std::string L2indexbits)
    {
        // Similar to L1_Read??

        // Go to that index position
        // Check all valid bits
        // if all valid then check tag (loop for number of assoc's)
        // if any tag found then update dirty bit, increment L1.writes, update LRU
        // else increment L1.writemisses, go to L2_Read(), go to L1_replace() [Since there may be writebacks], [update the value in this placed block], update dirty bit, update LRU
        // else if any valid bit present then set valid bit, increment L1.writemissses and go to L2_Read(), replace that tag, update dirty bit, update LRU

        int L1index = binaryStringToDecimal(L1indexbits);
        // Set L1Set = L1.cache[L1index];

        int hit_achieved = 0;
        int valid_count = 0;
        // First loop through valid blocks
        for (int i = 0; i < L1.associativity; i++)
        {

            if (L1.cache[L1index].set[i].valid == "1")
            {
                valid_count++;

                if (L1.cache[L1index].set[i].tag == L1tagbits)
                {
                    //* L1 hit
                    L1.writes += 1;                        // update L1.reads
                    L1.cache[L1index].set[i].dirty = "1";  // data is written so now its dirty -> incase for multi processors we can also make the corresponding L2.valid bit invalid ig
                    LRU_update(&L1.cache[L1index].LRU, i); // update LRU

                    hit_achieved = 1;
                    break;
                }
            }
        }
        if (hit_achieved == 0)
        {
            if (valid_count == L1.associativity)
            {
                L1.writes += 1;                                               // update the writes (since its trying to be written onto -> according to testcase result on piazza)
                L1.writemisses += 1;                                          // update L1 write miss since we are going to L2
                L1_Replace(&L1.cache[L1index], L1tagbits, L1indexbits);       // set the tag bits of the least recently used block (done in Replace function to incorporate writebacks)
                L2_Read(L2tagbits, L2indexbits);                              // Read from L2 (I am guessing that we read from L2, write on L1)
                L1.cache[L1index].set[L1.cache[L1index].LRU[0]].dirty = "1";  // updated dirty bit of this block
                LRU_update(&L1.cache[L1index].LRU, L1.cache[L1index].LRU[0]); // update LRU  -> note that the most recently used will now be the LRU[0] because that is where the new tagbits come in
            }
            else
            {
                for (int i = 0; i < L1.associativity; i++)
                {
                    if (L1.cache[L1index].set[i].valid == "0") // traverse through invalid blocks, to find first invalid block
                    {
                        L1.writes += 1;                               // update the writes (since its trying to be written -> according to testcase result on piazza))
                        L1.writemisses += 1;                          // update L1 read miss since we are going to L2
                        L2_Read(L2tagbits, L2indexbits);              // Read from L2 (I am guessing that we read from L2, write on L1)
                        L1.cache[L1index].set[i].tag = L1tagbits;     // set the tag bits as the required one
                        L1.cache[L1index].set[i].index = L1indexbits; // set the index bits as the required one
                        L1.cache[L1index].set[i].valid = "1";         // set the valid bit for this block
                        L1.cache[L1index].set[i].dirty = "1";         // set the dirty bit since its now differing from other memory data value
                        LRU_update(&L1.cache[L1index].LRU, i);        // update LRU
                        break;
                    }
                }
            }
        }
    }

    void L1_Replace(Set *L1Set, std::string L1tagbits, std::string L1indexbits)
    {
        // function to replace the LRU with incoming tag and index bits
        // Check LRU position:
        // if dirty bit of LRU position is set (ie 1) then writeback this value onto L2 (should I call L2_write? -> should set dirty bit for L2 block ), increment L1.writebacks, replace the tag with new tag, unset dirty bit
        // else replace tag with new tag
        // return

        int LRUindex = (*L1Set).LRU[0];
        // Block B = (*L1Set).set[LRUindex];
        // std::string address = L1tagbits + L1indexbits;                        // got to get back the orginal memory address using Block B's tag and index (dont care about offset)
        std::string address = (*L1Set).set[LRUindex].tag + (*L1Set).set[LRUindex].index;
        std::string Corr_L2tagbit = address.substr(0, L2.tagnum);             // now we can get corresponding tagbits in L2
        std::string Corr_L2indexbit = address.substr(L2.tagnum, L2.indexnum); // similarly corresponding indexbits in L2

        if ((*L1Set).set[LRUindex].dirty == "1")
        {
            (*L1Set).set[LRUindex].dirty = "0";
            L2_Write(Corr_L2tagbit, Corr_L2indexbit);
            L1.writebacks += 1;
        }
        (*L1Set).set[LRUindex].tag = L1tagbits;
        (*L1Set).set[LRUindex].index = L1indexbits;
    }

    void L2_Read(std::string L2tagbits, std::string L2indexbits)
    {
        // Go to that index postion
        // Check all valid bits
        // if all valid then check tag (loop for the number of assoc's)
        // if any tag valid then increment L2.reads, update LRU
        // increment L2.readmisses, go to L2_replace() [Since there may be writebacks],update LRU
        // else if any invalid bit present then set valid bit, increment L2.readmisses, replace that tag, update LRU
        // return

        // Same as L1_Reads?

        int L2index = binaryStringToDecimal(L2indexbits);
        // Set L2Set = L2.cache[L2index];

        int hit_achieved = 0;
        int valid_count = 0;
        for (int i = 0; i < L2.associativity; i++)
        {
            if (L2.cache[L2index].set[i].valid == "1")
            {
                valid_count++;
                if (L2.cache[L2index].set[i].tag == L2tagbits)
                {
                    L2.reads += 1;                         // update L2.reads
                    LRU_update(&L2.cache[L2index].LRU, i); // update LRU

                    hit_achieved = 1;
                    break;
                }
            }
        }
        if (hit_achieved == 0)
        {
            if (valid_count == L2.associativity)
            {
                L2.reads += 1;      // update the reads (since its trying to be read -> according to testcase result on piazza)
                L2.readmisses += 1; // update L2 read miss since we are going to DRAM
                // L2_Read(L1tagbits, L1indexbits, L2tagbits, L2indexbits); //Read from L2
                L2_Replace(&L2.cache[L2index], L2tagbits, L2indexbits);       // set the tag bits of the least recently used block (done in Replace function to incorporate writebacks)
                LRU_update(&L2.cache[L2index].LRU, L2.cache[L2index].LRU[0]); // update LRU
            }
            else
            {
                for (int i = 0; i < L2.associativity; i++)
                {
                    if (L2.cache[L2index].set[i].valid == "0")
                    {                       // if the present block being read has no data then:
                        L2.reads += 1;      // update the reads (since its trying to be read -> according to testcase result on piazza))
                        L2.readmisses += 1; // update L2 read miss since we are going to DRAM
                        // L2_Read(L1tagbits, L1indexbits, L2tagbits, L2indexbits); //Read from DRAM
                        L2.cache[L2index].set[i].tag = L2tagbits;     // set the tag bits as the required one
                        L2.cache[L2index].set[i].index = L2indexbits; // set the index bits as required one
                        L2.cache[L2index].set[i].valid = "1";         // set the valid bit for this block
                        LRU_update(&L2.cache[L2index].LRU, i);        // update LRU
                        break;
                    }
                }
            }
        }
    }

    void L2_Write(std::string L2tagbits, std::string L2indexbits)
    {
        // Again similar to L1_Write?
        // But this function is used only for writebacks???

        // Since this function is only used in writebacks it means that the data was present in L1 and hence should be present in L2 according to specifications of the cache
        // So we dont have to deal with the valid bit things as we did in L1_read/write etc
        // But its possible that the writeback is happening to an already dirty block - What to do here? writeback to DRAM or just update without writeback to DRAM?
        // For now im assuming just update value (Makes most sense acc to me)

        // So we just search for the tag in the set and then update L2.writes? -> This is literally the only case?
        // This is the only case because its not possible that the tag is not there in set since by the structure rules of our cache since data was present in L1 it has to be present in L2

        // Huh this means that an L2 write miss is never possible?? ---> Aight that seems to be the case in the output uploaded by Ma'am too

        int L2index = binaryStringToDecimal(L2indexbits);
        // Set L2Set = L2.cache[L2index];
        int hit_achieved = 0;
        int valid_count = 0;

        for (int i = 0; i < L2.associativity; i++)
        {
            if (L2.cache[L2index].set[i].valid == "1")
            {
                valid_count++;
                if (L2.cache[L2index].set[i].tag == L2tagbits)
                {
                    L2.writes += 1;                        // update L2.writes
                    L2.cache[L2index].set[i].dirty = "1";  // data is written so now its dirty -> incase for multi processors we can also make the corresponding L2.valid bit invalid ig
                    LRU_update(&L2.cache[L2index].LRU, i); // update LRU

                    hit_achieved = 1;
                }
            }
        }

        if (hit_achieved == 0)
        {
            if (valid_count == L2.associativity)
            {
                L2.writes += 1;      // update the writes (since its trying to be written onto -> according to testcase result on piazza)
                L2.writemisses += 1; // update L2 write miss since we are going to L2
                // L2_Read(L2tagbits, L2indexbits);           // Read from L2 (I am guessing that we read from L2, write on L2)
                L2_Replace(&L2.cache[L2index], L2tagbits, L2indexbits);       // set the tag bits of the least recently used block (done in Replace function to incorporate writebacks)
                L2.cache[L2index].set[L2.cache[L2index].LRU[0]].dirty = "1";  // updated dirty bit of this block
                LRU_update(&L2.cache[L2index].LRU, L2.cache[L2index].LRU[0]); // update LRU  -> note that the most recently used will now be the LRU[0] because that is where the new tagbits come in
            }
            else
            {
                for (int i = 0; i < L2.associativity; i++)
                {
                    if (L2.cache[L2index].set[i].valid == "0")
                    {
                        L2.writes += 1;      // update the writes (since its trying to be written -> according to testcase result on piazza))
                        L2.writemisses += 1; // update L1 read miss since we are going to L2
                        // L2_Read(L2tagbits, L2indexbits); // Read from L2 (I am guessing that we read from L2, write on L1)
                        L2.cache[L2index].set[i].tag = L2tagbits;     // set the tag bits as the required one
                        L2.cache[L2index].set[i].index = L2indexbits; // set the index bits as the required one
                        L2.cache[L2index].set[i].valid = "1";         // set the valid bit for this block
                        L2.cache[L2index].set[i].dirty = "1";         // set the dirty bit since its now differing from other memory data value
                        LRU_update(&L2.cache[L2index].LRU, i);        // update LRU
                        break;
                    }
                }
            }
        }
    }

    void L2_Replace(Set *L2Set, std::string L2tagbits, std::string L2indexbits)
    {
        // function to replace the LRU with incoming tag and index bits
        // Check LRU position:
        // if dirty bit of LRU position is set (ie 1) then writeback this value onto Mem (what to do here?), increment L2.writebacks, replace the tag with new tag
        // else replace tag with new tag
        // return

        int LRUindex = (*L2Set).LRU[0];
        // Block B = (*L2Set).set[LRUindex];

        //This new address calculation is for inclusivity - We are going to delete from both L1 and L2 incase it is replaced from L2
        // std::string address = (*L2Set).set[LRUindex].tag + (*L2Set).set[LRUindex].index;
        // std::string Corr_L1tagbit = address.substr(0, L1.tagnum);             // now we can get corresponding tagbits in L
        // std::string Corr_L1indexbit = address.substr(L1.tagnum, L1.indexnum); // similarly corresponding indexbits in L1

        if ((*L2Set).set[LRUindex].dirty == "1")
        {
            (*L2Set).set[LRUindex].dirty = "0";
            L2.writebacks += 1;
        }
        (*L2Set).set[LRUindex].tag = L2tagbits;
        (*L2Set).set[LRUindex].index = L2indexbits;
    }

    void printfinalans()
    {
        // std::cout << "\n\n===== Simulation Results =====";
        // std::cout << "\ni. number of L1 reads:\t\t\t\t" << std::dec << L1.reads;
        // std::cout << "\nii. number of L1 read misses:\t\t\t" << std::dec << L1.readmisses;
        // std::cout << "\niii. number of L1 writes:\t\t\t" << std::dec << L1.writes;
        // std::cout << "\niv. number of L1 write misses:\t\t\t" << std::dec << L1.writemisses;
        // std::cout << "\nv. L1 miss rate:\t\t\t\t" << std::fixed << std::setprecision(4) << ((float)L1.readmisses + (float)L1.writemisses) / (L1.reads + L1.writes);
        // std::cout << "\nvi. number of writebacks from L1 memory:\t" << std::dec << L1.writebacks;

        if (L2_SIZE != 0)
        {
            // std::cout << "\nvii. number of L2 reads:\t\t\t" << std::dec << L2.reads;
            // std::cout << "\nviii. number of L2 read misses:\t\t\t" << std::dec << L2.readmisses;
            // std::cout << "\nix. number of L2 writes:\t\t\t" << std::dec << L2.writes;
            // std::cout << "\nx. number of L2 write misses:\t\t\t" << std::dec << L2.writemisses;
            // std::cout << "\nxi. L2 miss rate:\t\t\t\t" << std::fixed << std::setprecision(4) << ((float)L2.readmisses + (float)L2.writemisses) / (L2.reads + L2.writes);
            // std::cout << "\nxii. number of writebacks from L2 memory:\t" << std::dec << L2.writebacks;
            std::cout << "\nxiii. The total access time is:\t\t\t" << std::dec << ((float)1*(L1.reads + L1.writes) + (float)20*(L2.reads + L2.writes) + (float)200*(L2.readmisses + L2.writemisses + L2.writebacks))/1000 << " ns\n\n";
        }
    }

    void printstate()
    {

        // printing L1
        std::cout << "L1\n\n";
        for (int i = 0; i < L1.setnum; i++)
        { // for each set in an index
            std::cout << "set no:" << i << "\n";
            for (int j = 0; j < L1.associativity; j++)
            { // for each block in a set
                Block block = L1.cache[i].set[j];
                if (block.valid == "0")
                {
                    std::cout << "Empty\t";
                }
                else
                {
                    std::cout << block.tag << " " << block.index << " - " << block.dirty << "\t";
                }
            }
            std::cout << "\n LRU\t";
            for (int j = 0; j < L1.associativity; j++)
            { // for each block in a set

                std::cout << L1.cache[i].LRU[j] << "\t";
            }
            std::cout << "\n";
        }
        std::cout << "\n";

        std::cout << "L2\n\n";
        for (int i = 0; i < L2.setnum; i++)
        { // for each set in an index
            std::cout << "set no:" << i << "\n";
            for (int j = 0; j < L2.associativity; j++)
            { // for each block in a set
                Block block = L2.cache[i].set[j];
                if (block.valid == "0")
                {
                    std::cout << "Empty\t";
                }
                else
                {
                    std::cout << block.tag << " " << block.index << " - " << block.dirty << "\t";
                }
            }
            std::cout << "\n LRU\t";
            for (int j = 0; j < L2.associativity; j++)
            { // for each block in a set

                std::cout << L2.cache[i].LRU[j] << "\t";
            }
            std::cout << "\n";
        }

        std::cout << "\n\n";
    }
};

#endif