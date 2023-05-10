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
                int hex_int = std::stoi(word2, nullptr, 16);
                // convert integer to binary string
                address = std::bitset<64>(hex_int).to_string();
                MemoryManager(word1, address);
            }
            file.close();
            printfinalans();
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

    void LRU_update(std::vector<int> LRU, int mostrecent)
    {
        // function to update LRU

        // Logic as proposed by brian -> LRU[0] contains the index for the block that was least recently used,
        // upon using a block place it on the last index of LRU -> Self updating LRU

        // mostrecent is the index of most recently used block in Set

        int mostrecentindex = 0; // the position of the mostrecent in LRU
        for (int i = 0; i < LRU.size(); i++)
        {
            if (LRU[i] == mostrecent)
            {
                mostrecentindex = i;
            }
        }
        std::rotate(LRU.begin() + mostrecentindex, LRU.begin() + mostrecentindex + 1, LRU.end()); // rotate the vector as prescribed earlier
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
        Set L1Set = L1.cache[L1index];
        for (int i = 0; i < L1.associativity; i++)
        {                           // number of blocks = set associativity of cache -> check whether any tag matches along with valid bit
            Block B = L1Set.set[i]; // Getting ith block of set

            // if invalid then use this block for read
            if (B.valid == "0")
            {                                    // if the present block being read has no data then:
                L1.reads += 1;                   // update the reads (since its trying to be read -> according to testcase result on piazza))
                L1.readmisses += 1;              // update L1 read miss since we are going to L2
                L2_Read(L2tagbits, L2indexbits); // Read from L2
                B.tag = L1tagbits;               // set the tag bits as the required one
                B.index = L1indexbits;           // set the index bits as required one
                B.valid = "1";                   // set the valid bit for this block
                LRU_update(L1Set.LRU, i);        // update LRU
                break;
            }

            // else if its valid then:
            else
            {
                if (i != L1.associativity - 1)
                { // if its not the last block then its possible that either this contains data or the future blocks have an invalid bit
                    if (B.tag == L1tagbits)
                    {                             // This is the data we need
                        L1.reads += 1;            // update L1.reads
                        LRU_update(L1Set.LRU, i); // update LRU
                        break;
                    }
                    continue; // in both cases continue to next iteration
                }
                else
                { // This means that now we are in the last block so if tag doesnt match with this then we have to replace the least recently used (LRU) block
                    if (B.tag == L1tagbits)
                    {                             // This is the data we need
                        L1.reads += 1;            // update L1.reads
                        LRU_update(L1Set.LRU, i); // update LRU
                        break;
                    }
                    else
                    {
                        L1.reads += 1;                             // update the reads (since its trying to be read -> according to testcase result on piazza)
                        L1.readmisses += 1;                        // update L1 read miss since we are going to L2
                        L2_Read(L2tagbits, L2indexbits);           // Read from L2
                        L1_Replace(L1Set, L1tagbits, L1indexbits); // set the tag bits of the least recently used block (done in Replace function to incorporate writebacks)
                        LRU_update(L1Set.LRU, L1Set.LRU[0]);       // update LRU  -> note that the most recently used will now be the LRU[0] because that is where the new tagbits come in
                        break;
                    }
                }
            }
            // LRU_update and L1.reads is kinda done everywere so can be taken as common outside the ifs
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
        Set L1Set = L1.cache[L1index];
        for (int i = 0; i < L1.associativity; i++)
        {                           // number of blocks = set associativity of cache -> check whether any tag matches along with valid bit
            Block B = L1Set.set[i]; // Getting ith block of set

            // if invalid then use this block for write
            if (B.valid == "0")
            {                                    // if the present block being written has no data then:
                L1.writes += 1;                  // update the writes (since its trying to be written -> according to testcase result on piazza))
                L1.writemisses += 1;             // update L1 read miss since we are going to L2
                L2_Read(L2tagbits, L2indexbits); // Read from L2 (I am guessing that we read from L2, write on L1)
                B.tag = L1tagbits;               // set the tag bits as the required one
                B.index = L1indexbits;           // set the index bits as the required one
                B.valid = "1";                   // set the valid bit for this block
                B.dirty = "1";                   // set the dirty bit since its now differing from other memory data value
                LRU_update(L1Set.LRU, i);        // update LRU
                break;
            }

            // else if its valid then:
            else
            {
                if (i != L1.associativity - 1)
                { // if its not the last block then its possible that either this contains data or the future blocks have an invalid bit
                    if (B.tag == L1tagbits)
                    {                             // This is the data we need
                        L1.writes += 1;           // update L1.writes
                        B.dirty = "1";            // data is written so now its dirty -> incase for multi processors we can also make the corresponding L2.valid bit invalid ig
                        LRU_update(L1Set.LRU, i); // update LRU
                        break;
                    }
                    continue; // in both cases continue to next iteration
                }
                else
                { // This means that now we are in the last block so if tag doesnt match with this then we have to replace the least recently used (LRU) block
                    if (B.tag == L1tagbits)
                    {                             // This is the data we need
                        L1.writes += 1;           // update L1.reads
                        B.dirty = "1";            // data is written so now its dirty -> incase for multi processors we can also make the corresponding L2.valid bit invalid ig
                        LRU_update(L1Set.LRU, i); // update LRU
                        break;
                    }
                    else
                    {
                        L1.writes += 1;                            // update the writes (since its trying to be written onto -> according to testcase result on piazza)
                        L1.writemisses += 1;                       // update L1 write miss since we are going to L2
                        L2_Read(L2tagbits, L2indexbits);           // Read from L2 (I am guessing that we read from L2, write on L1)
                        L1_Replace(L1Set, L1tagbits, L1indexbits); // set the tag bits of the least recently used block (done in Replace function to incorporate writebacks)
                        L1Set.set[L1Set.LRU[0]].dirty = "1";       // updated dirty bit of this block
                        LRU_update(L1Set.LRU, L1Set.LRU[0]);       // update LRU  -> note that the most recently used will now be the LRU[0] because that is where the new tagbits come in
                        break;
                    }
                }
            }
            // LRU_update and L1.writes is kinda done everywere so can be taken as common outside the ifs
        }
    }

    void L1_Replace(Set L1Set, std::string L1tagbits, std::string L1indexbits)
    {
        // function to replace the LRU with incoming tag and index bits
        // Check LRU position:
        // if dirty bit of LRU position is set (ie 1) then writeback this value onto L2 (should I call L2_write? -> should set dirty bit for L2 block ), increment L1.writebacks, replace the tag with new tag, unset dirty bit
        // else replace tag with new tag
        // return

        int LRUindex = L1Set.LRU[0];
        Block B = L1Set.set[LRUindex];
        std::string address = L1tagbits + L1indexbits;                        // got to get back the orginal memory address using Block B's tag and index (dont care about offset)
        std::string Corr_L2tagbit = address.substr(0, L2.tagnum);             // now we can get corresponding tagbits in L2
        std::string Corr_L2indexbit = address.substr(L2.tagnum, L2.indexnum); // similarly corresponding indexbits in L2

        if (B.dirty == "1")
        {
            B.dirty = "0";
            L2_Write(Corr_L2tagbit, Corr_L2indexbit);
            L1.writebacks += 1;
        }
        B.tag = L1tagbits;
        B.index = L1indexbits;
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
        Set L2Set = L2.cache[L2index];
        for (int i = 0; i < L2.associativity; i++)
        {                           // number of blocks = set associativity of cache -> check whether any tag matches along with valid bit
            Block B = L2Set.set[i]; // Getting ith block of set

            // if invalid then use this block for read
            if (B.valid == "0")
            {                       // if the present block being read has no data then:
                L2.reads += 1;      // update the reads (since its trying to be read -> according to testcase result on piazza))
                L2.readmisses += 1; // update L2 read miss since we are going to DRAM
                // L2_Read(L1tagbits, L1indexbits, L2tagbits, L2indexbits); //Read from DRAM
                B.tag = L2tagbits;        // set the tag bits as the required one
                B.index = L2indexbits;    // set the index bits as required one
                B.valid = "1";            // set the valid bit for this block
                LRU_update(L2Set.LRU, i); // update LRU
                break;
            }

            // else if its valid then:
            else
            {
                if (i != L2.associativity - 1)
                { // if its not the last block then its possible that either this contains data or the future blocks have an invalid bit
                    if (B.tag == L2tagbits)
                    {                             // This is the data we need
                        L2.reads += 1;            // update L2.reads
                        LRU_update(L2Set.LRU, i); // update LRU
                        break;
                    }
                    continue; // in both cases continue to next iteration
                }
                else
                { // This means that now we are in the last block so if tag doesnt match with this then we have to replace the least recently used (LRU) block
                    if (B.tag == L2tagbits)
                    {                             // This is the data we need
                        L2.reads += 1;            // update L2.reads
                        LRU_update(L2Set.LRU, i); // update LRU
                        break;
                    }
                    else
                    {
                        L2.reads += 1;      // update the reads (since its trying to be read -> according to testcase result on piazza)
                        L2.readmisses += 1; // update L2 read miss since we are going to DRAM
                        // L2_Read(L1tagbits, L1indexbits, L2tagbits, L2indexbits); //Read from L2
                        L2_Replace(L2Set, L2tagbits, L2indexbits); // set the tag bits of the least recently used block (done in Replace function to incorporate writebacks)
                        LRU_update(L2Set.LRU, L2Set.LRU[0]);       // update LRU
                        break;
                    }
                }
            }
            // LRU_update and L2.reads is kinda done everywere so can be taken as common outside the ifs
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
        Set L2Set = L2.cache[L2index];

        for (int i = 0; i < L2.associativity; i++)
        {                           // number of blocks = set associativity of cache -> check whether any tag matches along with valid bit
            Block B = L2Set.set[i]; // Getting ith block of set
            if (B.tag == L2tagbits)
            {                             // This is the data we need
                L2.writes += 1;           // update L2.writes
                B.dirty = "1";            // set dirty bit as value is now modified
                LRU_update(L2Set.LRU, i); // update LRU
                break;
            }
        }
    }

    void L2_Replace(Set L2Set, std::string L2tagbits, std::string L2indexbits)
    {
        // function to replace the LRU with incoming tag and index bits
        // Check LRU position:
        // if dirty bit of LRU position is set (ie 1) then writeback this value onto Mem (what to do here?), increment L2.writebacks, replace the tag with new tag
        // else replace tag with new tag
        // return

        int LRUindex = L2Set.LRU[0];
        Block B = L2Set.set[LRUindex];

        if (B.dirty == "1")
        {
            B.dirty = "0";
            L2.writebacks += 1;
        }
        B.tag = L2tagbits;
        B.index = L2indexbits;
    }

    void printfinalans()
    {
        std::cout << "\n\n===== Simulation Results =====";
        std::cout << "\ni. number of L1 reads:\t\t\t\t" << std::dec << L1.reads;
        std::cout << "\nii. number of L1 read misses:\t\t\t" << std::dec << L1.readmisses;
        std::cout << "\niii. number of L1 writes:\t\t\t" << std::dec << L1.writes;
        std::cout << "\niv. number of L1 write misses:\t\t\t" << std::dec << L1.writemisses;
        std::cout << "\nv. L1 miss rate:\t\t\t\t" << std::fixed << std::setprecision(4) << ((float)L1.readmisses + (float)L1.writemisses) / (L1.reads + L1.writes);
        std::cout << "\nvi. number of writebacks from L1 memory:\t" << std::dec << L1.writebacks;

        if (L2_SIZE != 0)
        {
            std::cout << "\nvii. number of L2 reads:\t\t\t" << std::dec << L2.reads;
            std::cout << "\nviii. number of L2 read misses:\t\t\t" << std::dec << L2.readmisses;
            std::cout << "\nix. number of L2 writes:\t\t\t" << std::dec << L2.writes;
            std::cout << "\nx. number of L2 write misses:\t\t\t" << std::dec << L2.writemisses;
            std::cout << "\nxi. L2 miss rate:\t\t\t\t" << std::fixed << std::setprecision(4) << ((float)L2.readmisses + (float)L2.writemisses) / (L2.reads + L2.writes);
            std::cout << "\nxii. number of writebacks from L2 memory:\t" << std::dec << L2.writebacks << "\n";
        }
    }
};

#endif