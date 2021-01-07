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
/*******************************************************************************************[Vec.h]
Copyright (c) 2003-2007, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Vec_h
#define Vec_h

#include <stdio.h>
#include <assert.h>
#include <new>

#include "../mtl/IntTypes.hh"
#include "../mtl/XAlloc.hh"

//=================================================================================================
// Automatically resizable arrays
//
// NOTE! Don't use this vector on datatypes that cannot be re-located in memory (with realloc)

template<class T>
class vec {
  T*  data;
  int sz;
  int cap;

  // Don't allow copying (error prone):
  vec<T>&  operator = (vec<T>& other) { assert(0); return *this; }
  vec        (vec<T>& other) { assert(0); }
             
  // Helpers for calculating next capacity:
  static inline int  imax   (int x, int y) { int mask = (y-x) >> (sizeof(int)*8-1); return (x&mask) + (y&(~mask)); }
  //static inline void nextCap(int& cap){ cap += ((cap >> 1) + 2) & ~1; }
  static inline void nextCap(int& cap){ cap += ((cap >> 1) + 2) & ~1; }

public:
  // Constructors:
  vec()                       : data(NULL) , sz(0)   , cap(0)    { }
  explicit vec(int size)      : data(NULL) , sz(0)   , cap(0)    { growTo(size); }
  vec(int size, const T& pad) : data(NULL) , sz(0)   , cap(0)    { growTo(size, pad); }
  ~vec()                                                          {clear(true); }

  // Pointer to first element:
  operator T*       (void)           { return data; }

  // Size operations:
  int      size     (void) const     { return sz; }
  void     setSize  (int n)          { sz = n; }
  void     shrink   (int nelems)     { assert(nelems <= sz); for (int i = 0; i < nelems; i++) sz--, data[sz].~T(); }
  void     shrink_  (int nelems)     { assert(nelems <= sz); sz -= nelems; }
  int      capacity (void) const     { return cap; }
  void     capacity (int min_cap);
  void     growTo   (int size);
  void     growTo   (int size, const T& pad);
  void     clear    (bool dealloc = false);
  bool     removeElt(const T&elem);

  // Stack interface:
  void     push  (void)              { if (sz == cap) capacity(sz+1); new (&data[sz]) T(); sz++; }
  void     push  (const T& elem)     { if (sz == cap) capacity(sz+1); data[sz++] = elem; }
  void     push_ (const T& elem)     { assert(sz < cap); data[sz++] = elem; }
  void     inc(void)     { assert(sz < cap); sz++; }
  void     pop   (void)              { assert(sz > 0); sz--, data[sz].~T(); }
  // NOTE: it seems possible that overflow can happen in the 'sz+1' expression of 'push()', but
  // in fact it can not since it requires that 'cap' is equal to INT_MAX. This in turn can not
  // happen given the way capacities are calculated (below). Essentially, all capacities are
  // even, but INT_MAX is odd.

  const T& last  (void) const        { return data[sz-1]; }
  T&       last  (void)              { return data[sz-1]; }

  // Vector interface:
  const T& operator [] (int index) const { return data[index]; }
  T&       operator [] (int index)       { return data[index]; }

  // Duplicatation (preferred instead):
  void initialize(int size, const T& pad);
  void copyTo(vec<T>& copy) const { copy.clear(); copy.growTo(sz); for (int i = 0; i < sz; i++) copy[i] = data[i]; }
  void moveTo(vec<T>& dest) { dest.clear(true); dest.data = data; dest.sz = sz; dest.cap = cap; data = NULL; sz = 0; cap = 0; }
};


template<class T> void vec<T>::capacity(int min_cap) 
{
  if (cap >= min_cap) return;
  int add = imax((min_cap - cap + 1) & ~1, ((cap >> 1) + 2) & ~1);   // NOTE: grow by approximately 3/2
  if ((add > INT_MAX - cap) || (((data = (T*)::realloc(data, (cap += add) * sizeof(T))) == NULL) && errno == ENOMEM))
    throw OutOfMemoryException();
}


template<class T> void vec<T>::initialize(int size, const T& pad) 
{
  capacity(size);
  for (T* p = data, *end = data + size; p != end ; ) *(p++) = pad;
  sz = size; 
}
 
 
template<class T> void vec<T>::growTo(int size, const T& pad) 
{
  if (sz >= size) return;
  capacity(size);
  for (int i = sz; i < size; i++) data[i] = pad;
  sz = size; 
}
 
 
template<class T> void vec<T>::growTo(int size) 
{
  if (sz >= size) return;
  capacity(size);
  for (int i = sz; i < size; i++) new (&data[i]) T();
  sz = size; 
}
 
 
template<class T> void vec<T>::clear(bool dealloc) 
{     
  if (data != NULL)
    {
      for (int i = 0; i < sz; i++) data[i].~T();
      sz = 0;
      if (dealloc) free(data), data = NULL, cap = 0; 
    } 
}

template<class T> bool vec<T>::removeElt(const T &elem) 
{
  int pos = -1;
  for(int j = 0 ; j<sz && pos == -1 ; j++) if(data[j] == elem) pos = j;
  data[pos] = data[--sz];
  return pos != -1;
}


//=================================================================================================

#endif
