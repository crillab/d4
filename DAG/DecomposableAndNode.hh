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
#ifndef Minisat_DAG_DecompAndNode_h
#define Minisat_DAG_DecompAndNode_h

#include <string>
#include "DAG.hh"

#define BLOCK_ALLOC_ALL_CHILDREN 1<<20

template<class T> class DAG;
template<class T> class DecomposableAndNode : public DAG<T>
{
public:
  using DAG<T>::nbEdges;
  using DAG<T>::saveFreeVar;
  using DAG<T>::globalStamp;
  using DAG<T>::idxOutputStruct;
  using DAG<T>::stamp;

  static DAG<T> **allChildren;
  static int capSzAllChildren, szAllChildren;

  struct
  {
    unsigned szChildren:22;
    unsigned posInAllChildren:32;
  } header;

  DecomposableAndNode(vec<DAG<T> *> &sons)
  {
    header.szChildren = sons.size();
    header.posInAllChildren = giveMeEmplacementChildren(sons.size());

    DAG<T> **children = &allChildren[header.posInAllChildren];
    for(int i = 0 ; i<header.szChildren ; i++) children[i] = sons[i];

    nbEdges += header.szChildren;
    szAllChildren += sons.size();
  }

  ~DecomposableAndNode()
  {
    szAllChildren -= header.szChildren;
    if(!szAllChildren)
      {
        free(allChildren);
        allChildren = NULL;
      }
  }

  inline unsigned int giveMeEmplacementChildren(int nbChildren)
  {
    while(capSzAllChildren < (szAllChildren + nbChildren))
    {
      capSzAllChildren += BLOCK_ALLOC_ALL_CHILDREN;
      allChildren = (DAG<T> **) realloc(allChildren, capSzAllChildren * sizeof(DAG<T>*));
      if(!allChildren)
      {
        printf("Memory out bufferInfo %d\n",(int) (capSzAllChildren * sizeof(DAG<T>)));
        exit(30);
      }
    }
    return szAllChildren;
  }// giveMeEmplacementChildren


  inline int getSize_()
  {
    int cpt = 0;
    DAG<T> **children = &allChildren[header.posInAllChildren];
    for(int i = 0 ; i<header.szChildren ; i++) cpt += children[i]->getSize_();
    return 1 + cpt;
  }


  inline void printNNF(std::ostream& out, bool certif)
  {
    if(stamp >= globalStamp) return;
    stamp = globalStamp + idxOutputStruct + 1;
    int idxCurrent = ++idxOutputStruct;

    // out << "a " << idxCurrent << " " << header.szChildren << " 0" << endl;
    out << "a " << idxCurrent << " 0" << endl;
    
    DAG<T> **children = &allChildren[header.posInAllChildren];
    for(int i = 0 ; i<header.szChildren ; i++)
      {
        children[i]->printNNF(out, certif);
        out << idxCurrent << " " << children[i]->getIdx() << " 0" << endl;
      }
  }// printNNF


  inline bool isSAT(vec<Lit> &unitsLitBranches)
  {
    DAG<T> **children = &allChildren[header.posInAllChildren];
    for(int i = 0 ; i < header.szChildren ; i++) if(!children[i]->isSAT(unitsLitBranches)) return false;
    return true;
  }


  inline T computeNbModels()
  {
    T nbModels = 1;
    DAG<T> **children = &allChildren[header.posInAllChildren];
    for(int i = 0 ; i < header.szChildren ; i++) nbModels *= children[i]->computeNbModels();
    return nbModels;
  }// computeNbModels
};

// initialize the static attributs
template<typename T> int DecomposableAndNode<T>::capSzAllChildren{0};
template<typename T> int DecomposableAndNode<T>::szAllChildren{0};
template<typename T> DAG<T> **DecomposableAndNode<T>::allChildren{NULL};
#endif
