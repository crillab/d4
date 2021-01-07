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
#ifndef MANAGER_CACHEBUCKET
#define MANAGER_CACHEBUCKET

#include <stdio.h>
#include <cstdint>
#include <math.h>

class DataInfo
{
 private:
  uint64_t info1;
  uint32_t info2;

  struct
  {
    unsigned count:30;
    unsigned dirty:2;
  } stats;


 public:
  DataInfo()
  {
    info1 = info2 = 0;
  }

  DataInfo(unsigned szData,           // :20;
           unsigned nbVar,            // :18;
           unsigned nbLit,            // :19;
           unsigned nbClause,         // :18;
           // unsigned nbDiffSize,    // :15; -> can be computed from the others
           unsigned nbOctetsData,     // :2;
           unsigned nbOctetsVar,      // :2;
           unsigned nbOctetsDistrib   // :2
           )
  {
    info1 = ((uint64_t) nbVar) | ((uint64_t) nbLit << 24) | ((uint64_t) nbClause << 48);
    info2 = ((uint32_t) nbOctetsData) | ((uint32_t) nbOctetsVar << 2) |
            ((uint32_t) nbOctetsDistrib << 4) | ((uint32_t) szData << 6);
    stats = {(unsigned) nbVar, 0};
  }

  unsigned szData(){return info2 >> 6;}
  void szData(unsigned sz){info2 = (info2&((1<<6) - 1)) | ((uint32_t) sz << 6);}
  unsigned nbOctetsData(){return info2 & ((1<<2) - 1);}
  unsigned nbOctetsVar(){return (info2>>2) & ((1<<2) - 1);}
  unsigned nbOctetsDistrib(){return (info2>>4) & ((1<<2) - 1);}
  unsigned nbClause(){return info1>>48 & ((1<<24) - 1);}
  unsigned nbLit(){return info1>>24 & ((1<<24) - 1);}
  unsigned nbVar(){return info1 & ((1<<24) - 1);}

  // need to be computed
  unsigned nbDiffSize()
  {
    if(!nbOctetsDistrib()) return 0;
    return (szData() - nbLit() * nbOctetsData() - nbVar() * nbOctetsVar()) / nbOctetsDistrib();
  }

  inline void reinitCount(int v = 0) {stats.count = v;}
  inline void incCount(int v = 1){stats.count += v;}
  inline void divCount(){stats.count >>= 1;}
  inline void decCount(int v = 1){if(v > stats.count) stats.count = 0; else stats.count -= v;}
  inline int count() {return stats.count;}

  inline int dirty() {return stats.dirty;}
  inline void reinitDirty() {stats.dirty = 0;}
  inline void incDirty() { if(stats.dirty < 3) stats.dirty++;}
  inline void decDirty() { if(stats.dirty > 0) stats.dirty--;}
  inline void setTrueDirty(){stats.dirty = 1;}
  inline void setFalseDirty(){stats.dirty = 0;}

  inline bool operator==(const DataInfo& d)
  {
    return info1 == d.info1 && info2 == d.info2;
  } // operator ==
};

template <class T> class CacheBucket
{
public:
  char *data;
  DataInfo header;
  T fc;

  CacheBucket()
  {
    data = NULL;
    header.szData(0);
  }


  inline void set(char *d, unsigned sz, unsigned nbVar, unsigned nbLit, unsigned nbClause,
                  unsigned nbDiffSize, unsigned nbOctetsData, unsigned nbOctetsVar, unsigned nbOctetsDistrib)
  {
    data = d;
    header = DataInfo(sz, nbVar, nbLit, nbClause, nbOctetsData, nbOctetsVar, nbOctetsDistrib);
#if 0
    assert(sz == header.szData());
    assert(nbVar == header.nbVar());
    assert(nbLit == header.nbLit());
    assert(nbClause == header.nbClause());
    assert(nbDiffSize == header.nbDiffSize());
    assert(nbOctetsVar == header.nbOctetsVar());
    assert(nbOctetsData == header.nbOctetsData());
    assert(nbOctetsDistrib == header.nbOctetsDistrib());
#endif
  }

  inline void lockedBucket(T v)
  {
    header.reinitCount();
    fc = v;
  }

  inline void reinitCount(int v = 0) {header.reinitCount(v);}
  inline void incCount(int v){header.incCount(v);}
  inline void divCount(){header.divCount();}
  inline void decCount(int v){header.decCount(v);}
  inline int count() {return header.count();}

  inline void reinitDirty() {header.reinitDirty();}
  inline void setTrueDirty(){header.setTrueDirty();}
  inline void incDirty(){header.incDirty();}
  inline void decDirty(){header.decDirty();}
  inline void setFalseDirty(){header.setFalseDirty();}
  inline int dirty() {return header.dirty();}

  inline void szData(int s) {header.szData(s);}
  inline int szData() {return header.szData();}
  inline int nbVar(){return header.nbVar();}


  template <typename U> void printData(void *data, int sz)
  {
    U *p = static_cast<U *>(data);
    for(int i = 0 ; i<sz ; i++)
    {
      printf("%d ", *p);
      p++;
    }
    printf("\n");
  }// printdata

  inline void print()
  {
    printf("Bucket size(%d) nbVar(%d) nbClause(%d) nbLit(%d) nbDiff(%d) count(%d) dirty(%d)\n",
           header.szData(), header.nbVar(), header.nbClause(), header.nbLit(),
           header.nbDiffSize(), header.count(), header.dirty());

    // print the variable
    printf("Var: %d(%d)\n", header.nbVar(), header.nbOctetsVar());
    switch(header.nbOctetsVar())
    {
      case 1 : printData<char>(data, header.nbVar()); break;
      case 2 : printData<char16_t>(data, header.nbVar()); break;
      default : printData<char32_t>(data, header.nbVar()); break;
    }

    // distribution
    char *dataDistrib = &data[header.nbVar() * header.nbOctetsVar()];
    printf("Distribution: %d(%d)\n", header.nbDiffSize(), header.nbOctetsDistrib());
    switch(header.nbOctetsDistrib())
    {
      case 1 : printData<char>(dataDistrib, header.nbDiffSize()); break;
      case 2 : printData<char16_t>(dataDistrib, header.nbDiffSize()); break;
      default : printData<char32_t>(dataDistrib, header.nbDiffSize()); break;
    }

    char *dataClause = &dataDistrib[header.nbDiffSize() * header.nbOctetsDistrib()];

    printf("Clause: %d(%d)\n", header.nbClause(), header.nbOctetsData());
    switch(header.nbOctetsData())
    {
      case 1 : printData<char>(dataClause, &data[header.szData()] - dataClause); break;
      case 2 : printData<char16_t>(dataClause, &data[header.szData()] - dataClause); break;
      default : printData<char32_t>(dataClause, &data[header.szData()] - dataClause); break;
    }

    printf("All data: ");
    for(int i = 0 ; i<header.szData() ; i++) printf("%hhX ", data[i]);
    printf("\n");
    printf("------------------------------------------\n");
  }

  inline bool sameHeader(CacheBucket<T> &b)
  {
    return header == b.header;
  }
};
#endif
