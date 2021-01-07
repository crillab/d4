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
#ifndef Minisat_DAG_BinaryDetermOrNode_h
#define Minisat_DAG_BinaryDetermOrNode_h

#include "DeterministicOrNode.hh"
#include "DAG.hh"

template<class T> class DAG;
template<class T> class BinaryDeterministicOrNode : public DAG<T>
{
  using DAG<T>::nbEdges;
  using DAG<T>::globalStamp;
  using DAG<T>::idxOutputStruct;
  using DAG<T>::stamp;

public:
  bool saveDecision;
  Branch<T> firstBranch, secondBranch;
  T nbModels;

  inline void assignFirstBranch(DAG<T> *d, vec<Lit> &units, vec<Var> &fVar)
  {
    firstBranch.initBranch(units, d, fVar);
    nbEdges++;
  }

  inline void assignSecondBranch(DAG<T> *d, vec<Lit> &units, vec<Var> &fVar)
  {
    secondBranch.initBranch(units, d, fVar);
    nbEdges++;
  }

  BinaryDeterministicOrNode(){}

  BinaryDeterministicOrNode(DAG<T> *l, DAG<T> *r)
  {
    firstBranch.initBranch(l);
    secondBranch.initBranch(r);
  }

  BinaryDeterministicOrNode(DAG<T> *l, vec<Lit> &unitLitL, vec<Var> &freeVarL, DAG<T> *r, vec<Lit> &unitLitR,
                            vec<Var> &freeVarR)
  {
    assignFirstBranch(l, unitLitL, freeVarL);
    assignSecondBranch(r, unitLitR, freeVarR);
  }

  inline int getSize_()
  {
    if(stamp == globalStamp) return 0;
    stamp = globalStamp;
    return firstBranch.d->getSize_() + secondBranch.d->getSize_();
  }

  inline void printNNF(std::ostream& out, bool certif)
  {
    if(stamp >= globalStamp) return;
    stamp = globalStamp + idxOutputStruct + 1;
    int idxCurrent = ++idxOutputStruct;

    out << "o " << idxCurrent << " 0" << endl;

    firstBranch.printNNF(out, certif);
    secondBranch.printNNF(out, certif);

    out << idxCurrent << " " << (firstBranch.d)->getIdx() << " ";
    Lit *pUnit = &DAG<T>::unitLits[firstBranch.idxUnitLit];
    for( ; *pUnit != lit_Undef ; pUnit++) out << readableLit(*pUnit) << " ";
    out << "0" << endl;

    out << idxCurrent << " " << (secondBranch.d)->getIdx() << " ";
    pUnit = &DAG<T>::unitLits[secondBranch.idxUnitLit];
    for( ; *pUnit != lit_Undef ; pUnit++) out << readableLit(*pUnit) << " ";
    out << "0" << endl;
  }// printNNF


  inline bool isSAT()
  {
    if(stamp == globalStamp) return saveDecision;
    stamp = globalStamp;
    saveDecision = firstBranch.isSAT() || secondBranch.isSAT();

    return saveDecision;
  }// isSAT

  inline bool isSAT(vec<Lit> &unitsLitBranches)
  {
    if(stamp == globalStamp) return saveDecision;
    stamp = globalStamp;
    saveDecision = firstBranch.isSAT(unitsLitBranches) || secondBranch.isSAT(unitsLitBranches);

    return saveDecision;
  }// isSAT


  inline T computeNbModels()
  {
    if(stamp == globalStamp) return nbModels;
    nbModels = firstBranch.computeNbModels() + secondBranch.computeNbModels();
    stamp = globalStamp;
    return nbModels;
  }// computeNbModels

};
#endif
