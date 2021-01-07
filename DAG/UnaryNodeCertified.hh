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
#ifndef Minisat_DAG_UnaryNodeCertified_h
#define Minisat_DAG_UnaryNodeCertified_h

#include "DeterministicOrNode.hh"
#include "DAG.hh"

template<class T> class DAG;
template<class T> class UnaryNodeCertified : public DAG<T>
{
  using DAG<T>::nbEdges;
  using DAG<T>::globalStamp;
  using DAG<T>::idxOutputStruct;
  using DAG<T>::stamp;

public:
  T nbModels;
  bool saveDecision;
  Branch<T> branch;
  vec<int> reasonForUnits;
  bool fromCache;

  UnaryNodeCertified(DAG<T> *l)
  {
    if(!l->creator) l->creator = this;
    branch.initBranch(l);
    fromCache = false;
  }

  UnaryNodeCertified(DAG<T> *l, vec<Lit> &unitLit, bool fromCache_, vec<int> &idxReason, vec<Var> &freeVar)
  {
    branch.initBranch(unitLit, l, freeVar);
    nbEdges++;
    idxReason.copyTo(reasonForUnits);
    fromCache = fromCache_;
  }

  inline int getSize_()
  {
    if(stamp == globalStamp) return 0;
    stamp = globalStamp;
    return branch.d->getSize_();
  }


  inline void printNNF(std::ostream& out, bool certif)
  {
    if(stamp >= globalStamp) return;
    stamp = globalStamp + idxOutputStruct + 1;
    int idxCurrent = ++idxOutputStruct;

    out << "o " << idxCurrent << " 1 ";
    for(int i = 0 ; i<reasonForUnits.size() ; i++) out << reasonForUnits[i] << " ";
    out << "0" << endl;

    branch.printNNF(out, certif);
    out << idxCurrent << " " << (branch.d)->getIdx() << (fromCache ? " 1 " : " 2 ");
    Lit *pUnit = &DAG<T>::unitLits[branch.idxUnitLit];
    for( ; *pUnit != lit_Undef ; pUnit++) out << readableLit(*pUnit) << " " ;
    out << "0" << endl;
  }// printNNF


  inline bool isSAT()
  {
    if(stamp == globalStamp) return saveDecision;
    saveDecision = branch.isSAT();
    stamp = globalStamp;
    return saveDecision;
  }// isSAT


  inline bool isSAT(vec<Lit> &unitsLitBranches)
  {
    if(stamp == globalStamp) return saveDecision;
    saveDecision = branch.isSAT(unitsLitBranches);
    stamp = globalStamp;
    return saveDecision;
  }// isSAT


  inline T computeNbModels()
  {
    if(stamp == globalStamp) return nbModels;
    nbModels = branch.computeNbModels();
    stamp = globalStamp;
    return nbModels;
  }// computeNbModels

private:
  // Used when generating d-DNNF: branches are translated as ImplicitAnd nodes.
  // ImplicitAnd<T>* newAnd0;
  // ImplicitAnd<T>* newAnd1;

  // findLitFromBranches finds a var that is common to two branches.
  // It must be positive in one branch, negative in the other.
  // If it is positive in the first one, negative in the second, a positive value will be returned.
  // Else it will be negative.
  Lit findLitFromBranches() {
    assert(false);
    return lit_Undef;
  }

  void printBranch(Branch<T>& b, int index, std::ostream& out) {
    int nbUnitLit = b.nbUnit();
    for (int i = 0; i < nbUnitLit ; i++) {
      out << "L " << readableLit((&DAG<T>::unitLits[b.idxUnitLit])[i]) << std::endl;
    }
    out << "A " << (nbUnitLit + 1) << " ";
    for (int i = 0; i < nbUnitLit; i++) {
      out << (index - i - 1) << " ";
    }
    out << DAG<T>::nodeToIndex[b.d] << std::endl;
  }
};
#endif
