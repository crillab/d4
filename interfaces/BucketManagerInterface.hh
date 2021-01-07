/*
* d4
* Copyright (C) 2020  Univ. Artois & CNRS
* 
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef INTERFACE_BUCKET_MANAGER
#define INTERFACE_BUCKET_MANAGER

#include <iostream>
#include <cassert>
#include <string.h>

#include "../utils/SolverTypes.hh"
#include "../manager/CacheBucket.hh"

#define ALL 0
#define NB 1
#define NT 2

#define ONE_OCTET 2
#define TWO_OCTET 3
#define FOUR_OCTET 4
#define EIGHT_OCTET 5

#define PAGE (1<<29)

template<class T> class BucketManagerInterface
{
protected:
  int modeStore;
  vec<char *> allocateData;
  char *data;
  unsigned long int sizeData, posInData;
  CacheBucket<T> bucket;

 public:
  vec<vec<char *>> freeSpace;  // freespace[i][j] points to a free memory space of size i
  unsigned long int allMemory;
  unsigned long int freeMemory;
  unsigned long int pageData;

  virtual void storeFormula(vec<Var> &component, CacheBucket<T> &b) = 0;
  inline void setFixeFormula(const char *cacheStore)
  {
    if(!strcmp(cacheStore, "ALL")) modeStore = ALL;
    if(!strcmp(cacheStore, "NB")) modeStore = NB;
    if(!strcmp(cacheStore,"NT")) modeStore = NT;
    std::cout << "c Strategy: " << modeStore << std::endl;
  }

  virtual ~BucketManagerInterface()
  {
    for(int i = 0 ; i<allocateData.size() ; i++) delete[](allocateData[i]);
    allocateData.clear();
  }

  inline int nbOctetToEncodeInt(unsigned int v) // we know that we cannot have more than 1<<32 variables
  {
    if(v < (1<<8)) return 1;
    if(v < (1<<16)) return 2;
    return 4;
  }// nbOctetToEncodeInt


  inline double remainingMemory()
  {
    return ((double) freeMemory + (sizeData - posInData)) / (double) sizeData;
  } // remainingMemory


  /**
     Initialize the data structure regarding the configuration (ie. number of
     variables, maximum number of clauses and the lenght of the largest clause).
   */
  void init(int nbVar, int nbClause, int maxSizeClause, int strategyCache)
  {
    modeStore = NT;
    allMemory = freeMemory = posInData = 0;
    switch(strategyCache)
    {
      case 1 : sizeData = ((unsigned long int)1<<32) * sizeof(char); break;
      case 2 : ;
      case 0 : sizeData = PAGE * sizeof(char); break;
    }

    // we cannot reinit ... at least for the moment
    assert(!allocateData.size());
    data = new char[sizeData];
    allocateData.push(data);
    allMemory += sizeData;
  } // init


  /**
     Get a pointer on an available array where we can store the data we want to
     save into the bucket.
   */
  char *getArray(int size)
  {
    char *ret = NULL;

    if(size < freeSpace.size() && freeSpace[size].size())
    {
      ret = freeSpace[size].last();
      freeSpace[size].pop();
      freeMemory -= size;
      return ret;
    }

    // go futher to see if we cannot split some entry
    if((size << 1) < freeSpace.size())
    {
      int pos = size << 1;
      while(pos < freeSpace.size() && !freeSpace[pos].size()) pos++;

      // split an entry
      if(pos < freeSpace.size())
      {
        ret = freeSpace[pos].last();
        freeSpace[pos].pop();
        freeMemory -= size;
        freeSpace[pos - size].push(&ret[size]); // split and push
        return ret;
      }
    }

    // take a fresh entry
    if(posInData + size > sizeData)
    {
      int rSz = sizeData - posInData;
      while(freeSpace.size() <= rSz) freeSpace.push();
      freeSpace[rSz].push(&data[posInData]);
      freeMemory += rSz;

      printf("c Allocate a new page for the cache %lu\n", freeMemory);
      posInData = 0;
      data = new char[sizeData];
      allocateData.push(data);

      allMemory += sizeData;
    }

    ret = &data[posInData];
    posInData += size;
    return ret;
  } // getArray


  /**
     Release some memory of a given size and store this information in
     freespace.

     @param[in] m, the memory we want to release
     @param[in] size, the size of the memory block
   */
  inline void releaseMemory(char *m, int size)
  {
    while(freeSpace.size() <= size) freeSpace.push();
    freeSpace[size].push(m);
    freeMemory += size;
  }// reverseLastBucket


  /**
     Collect the bucket associtated to the set of variable given in
     parameter.
     @param[in] component, the variable belonging to the connected component
     \return a formula put in a bucket
  */
  CacheBucket<T> *collectBuckect(vec<Var> &component)
  {
    storeFormula(component, bucket);
    return &bucket;
  }// collectBuckect
};

#endif
