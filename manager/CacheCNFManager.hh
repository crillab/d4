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
/**
   Author: Lagniez Jean-Marie
   Creation: 2014/11/10
   Class to implemente the cache data structure.
 */

#ifndef MODELCOUNTER_CACHECNF_h
#define MODELCOUNTER_CACHECNF_h

#include <string.h>
#include <vector>

#include "../manager/BucketManager.hh"
#include "../manager/CacheBucket.hh"
#include "../hashing/HashCnf.hh"

#define GET_NB_CL(x)  ((((unsigned long int) x)>>44) & 1048575) // | 20 |    |    |   |
#define GET_NB_LIT(x) ((((unsigned long int) x)>>24) & 1048575) // |    | 20 |    |   |
#define GET_NB_VAR(x) ((((unsigned long int) x)>>4) & 1048575)  // |    |    | 20 |   |
#define GET_TYPE_VAR(x) (((unsigned long int) x) & 15)          // |    |    |    | 4 |
#define GET_INFO_VAR(x) (((unsigned long int) x) & 16777215)
#define NB_INFO 1

#define BLOCK_BUCKET (1<<4)
#define DEC_SIZE_DIST 12
#define DEC_SIZE_OCC 16

#define BLOCK_BUFFER_INFO (1<<20)
#define BLOCK_BUFFER_FORM (1<<20)

#define SIZE_HASH 999331

#define BUDGET 7

template<class T> class TmpEntry
{
 public:
  CacheBucket<T> e;
  unsigned int hashValue;
  bool defined;

  TmpEntry() { defined = false; }

  TmpEntry(CacheBucket<T> e_, unsigned int hashValue_, bool defined_)
  {
    e = e_;
    hashValue = hashValue_;
    defined = defined_;
  }

  inline T getValue() {return e.fc;}
};

template<class T> class CacheCNF
{
public:
  bool verb;
  std::vector< std::vector< CacheBucket<T> > > hashTable;
  unsigned nbEntry;

  // statistics
  int nbPositiveHit, nbNegativeHit;
  double sumAffectedHitCache;
  int minAffectedHitCache, nbReduceCall;

  // data info
  int nbInitVar;
  unsigned int maxBlockClause;
  unsigned int nbClauses;
  vec<int> sizeVarCacheHit, nbCacheWithSizeVar, nbTestCache;
  unsigned int nbFailedInCache, nbRemoveEntry;
  unsigned long int nbCreationBucket;

  int maxSize;
  HashCnf hashMethod;
  int callReduceCache, strategyRedCache;
  vec<bool> deadSize;
  unsigned long int sumDataSize;

public:
  CacheCNF(int rdCache, int strCache)
  {
    sumDataSize = nbEntry = nbCreationBucket = 0;
    strategyRedCache = strCache;
    callReduceCache = rdCache;
    nbPositiveHit = nbNegativeHit = 0;
    nbFailedInCache = 1;
    nbRemoveEntry = nbReduceCall = sumAffectedHitCache = 0;
    verb = 0;
  }// CacheCNF

  ~CacheCNF()
  {
    hashTable.clear();
  }

  inline int getNbPositiveHit(){return nbPositiveHit;}
  inline int getNbNegativeHit(){return nbNegativeHit;}

  inline void printCacheInformation()
  {
    printf("c \033[1m\033[34mCache Information\033[0m\n");
    printf("c Memory used: %.0f MB\n", memUsedPeak());
    printf("c\n");
    printf("c Number of positive hit: %d\n", nbPositiveHit);
    printf("c Number of negative hit: %d\n", nbNegativeHit);
    printf("c Number of reduceCall: %d\n", nbReduceCall);
    printf("c\n");
  }// printCacheInformation


  inline void pushInHashTable(CacheBucket<T> &cb, unsigned int hashValue, T val)
  {
    hashTable[hashValue % SIZE_HASH].push_back(cb);

    CacheBucket<T> &cbIn = (hashTable[hashValue % SIZE_HASH].back());
    cbIn.lockedBucket(val);
    nbCreationBucket++;
    sumDataSize += cb.szData();
    
    switch(strategyRedCache)
    {
      case 0 : cbIn.reinitCount(cb.nbVar()); break;
      // case 0 : cbIn.reinitCount(nbPositiveHit + nbNegativeHit); break;
      case 1 : ;
      case 2 : cbIn.reinitCount(nbPositiveHit + nbNegativeHit);
    }
    assert(cbIn.count());
    nbEntry++;
  }// pushinhashtable


  inline unsigned int computeHash(CacheBucket<T> &bucket)
  {
    return hashMethod.hash(bucket.data, bucket.szData());
  }

  /**
     Research in the set of buckets if the bucket pointed by i already exist.
     @param[in] idx, the index of the researched bucket
     \return the index of the identical bucket if this one exists, -1 otherwise
  */
  CacheBucket<T> *bucketAlreadyExist(CacheBucket<T> &cb, unsigned int hashValue)
  {
    char *refData = cb.data;
    std::vector<CacheBucket<T> > &listCollision = hashTable[hashValue % SIZE_HASH];

    for(int i = 0 ; i<listCollision.size() ; i++)
      {
        CacheBucket<T> &cbi = listCollision[i];
        if(!cb.sameHeader(cbi)) continue;

        if(!memcmp(refData, cbi.data, cbi.szData()))
        {
          if(!cbi.dirty()) sizeVarCacheHit[cbi.nbVar()]++;
          cbi.setTrueDirty();
          nbPositiveHit++;
          return &cbi;
        }
      }
    nbNegativeHit++;
    return NULL;
  }// bucketAlreadyExist


  /**
     Add an entry in the cache.
   */
  void addInCache(TmpEntry<T> &cb, T val)
  {
    pushInHashTable(cb.e, cb.hashValue, val);
  } // addInCache

  
  /**
     Take a bucket manager as well as a set of variables consisting in the
     variables in the current component and search in the cache if the related
     formula is present in the cache, if it is not the case the bucket is
     created and added.

     @param[in] varConnected, the variable
     @param[in] bm, the bucket manager
   */
  TmpEntry<T> searchInCache(vec<Var> &varConnected, BucketManagerInterface<T> *bm)
  {
    CacheBucket<T> *formulaBucket = bm->collectBuckect(varConnected);
    unsigned int hashValue = computeHash(*formulaBucket);
    CacheBucket<T> *cacheBucket = bucketAlreadyExist(*formulaBucket, hashValue);
    assert(nbTestCache.size() > varConnected.size());
    nbTestCache[varConnected.size()]++;

    if(cacheBucket)
    {
      switch(strategyRedCache)
      {
        case 1 : cacheBucket->reinitCount(nbPositiveHit + nbNegativeHit); break;
          // case 0 : cacheBucket->reinitCount(nbPositiveHit + nbNegativeHit); break;
        case 0 : cacheBucket->incCount(1); break;
      }
      bm->releaseMemory(formulaBucket->data, formulaBucket->szData());
      return TmpEntry<T>(*cacheBucket, hashValue, true);
    }
    else
    {
      nbFailedInCache++;
      nbCacheWithSizeVar[varConnected.size()]++;
      return TmpEntry<T>(*formulaBucket, hashValue, false);
    }
  } // searchInCache


  /**
     Create a bucket and store it in the cache.

     @param[in] varConnected, the variable
     @param[in] bm, the bucket manager
     @param[in] c, the value we want to store
   */
  inline void createAndStoreBucket(vec<Var> &varConnected, BucketManagerInterface<T> *bm, T &c)
  {
    CacheBucket<T> *formulaBucket = bm->collectBuckect(varConnected);
    unsigned int hashValue = computeHash(*formulaBucket);
    pushInHashTable(*formulaBucket, hashValue, c); // add the new bucket
    nbCacheWithSizeVar[varConnected.size()]++;
  }// createBucket



  /**
     Set the information concerning the number of clauses, variables
     and the maximum size of the clauses (these information are useful
     to know the size of the memory blocks we have to allocate).

     @param[in] mVar, the number of variables
     @param[in] nbC, the number of clauses
     @param[in] mSize, the size of the biggest clause
   */
  void setInfoFormula(unsigned int mVar, unsigned int nbC, int mSize)
  {
    minAffectedHitCache = mVar;
    maxSize = mSize;
    maxBlockClause = mSize * nbC;
    nbInitVar = mVar;

    for(int i = 0 ; i<nbInitVar ; i++)
    {
      sizeVarCacheHit.push(0);
      nbCacheWithSizeVar.push(0);
      nbTestCache.push(0);
      deadSize.push(false);
    }
  }// setInfoFormula


  /**
     Initialized the hashTable
  */
  void initHashTable(unsigned int mVar, unsigned int nbC, int mSize)
  {
    setInfoFormula(mVar, nbC, mSize);

    // init hash tables
    hashTable.clear();
    for(int i = 0 ; i<SIZE_HASH ; i++) hashTable.push_back(std::vector<CacheBucket<T> >());
  }// initHashTable


  ///////////////////////////////////////////////////////////////////////////
  ////////////////  Show some information about the hash table  /////////////
  ///////////////////////////////////////////////////////////////////////////
  inline void showTabLit(unsigned long int *v, unsigned int sz)
  {
    for(unsigned int i = 0 ; i<sz ; i++) printf("%ld ", v[i]); printf("\n");
  }

  /**
     Function that give us the information about the size of the
     different conflict lists.
  */
  void showDistribution()
  {
    long int allC = 0;
    vec<int> countElt;
    for(int i = 0 ; i<hashTable.size() ; i++)
    {
      for(int j = 0 ; j<hashTable[i].size() ; j++)
      {
        countElt.push(hashTable[i][j].count());
        allC += hashTable[i][j].count();
      }
    }
    sort(countElt);


    int cpt = 1, val = 0, anotherSum = 0;
    for(int i = 0 ; i<countElt.size() ; i++)
    {
      if(countElt[i] == val) cpt++;
      else
      {
        anotherSum += cpt * val;
        printf("%d %d/%ld %d\n", cpt, val, allC, anotherSum);
        val = countElt[i];
        cpt = 1;
      }
    }
    printf("%d %d/%ld %d\n", cpt, val, allC, anotherSum);


    for(int i = 0 ; i<sizeVarCacheHit.size() ; i++)
    {
      if(sizeVarCacheHit[i]) printf("-> %d %d\n", i, sizeVarCacheHit[i]);
    }

    return;

    int biggestIdx = 0;
    vec<int> tabDistrib;
    for(int i = 0 ; i<hashTable.size() ; i++)
      {
        if(!hashTable[i].size()) continue;
        tabDistrib.push(hashTable[i].size());
        if(hashTable[biggestIdx].size() < hashTable[i].size()) biggestIdx = i;
      }

    sort(tabDistrib);
    for(int i = 0 ; i<tabDistrib.size() ; i++) printf("%d ", tabDistrib[i]);
    printf("\n");

    float sum = 0;
    for(int i = 0 ; i<tabDistrib.size() ; i++) sum += tabDistrib[i];

    printf("average size of conflict list %lf %d\n", ((float) sum) / tabDistrib.size(), tabDistrib.size());
    printf("the median size is %d\n", tabDistrib[tabDistrib.size() >> 1]);
  }// showDistribution

};

#endif
