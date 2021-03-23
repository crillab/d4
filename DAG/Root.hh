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
#ifndef Minisat_DAG_Root_h
#define Minisat_DAG_Root_h

#include "DAG.hh"

template<class T> class DAG;

template<class T> class rootNode : public DAG<T>
{
public:
  using DAG<T>::globalStamp;
  using DAG<T>::fixedValue;
  using DAG<T>::idxOutputStruct;
  using DAG<T>::stamp;


  Branch<T> b;
  int nbVariable;
  vec<int> reasonForUnits;
  bool fromCache;

  rootNode(int nbVar)
  {
    nbVariable = nbVar;
    this->unitLits = (Lit *) malloc(sizeof(Lit));
    this->szUnitLits = this->capUnitLits = 1;
    this->unitLits[0] = lit_Undef;

    this->freeVariables = (Var *) malloc(sizeof(Var));
    this->freeVariables[0] = var_Undef;
    this->szFreeVariables = this->capFreeVariables = 1;

    this->nbNodes = this->nbEdges = 0;
    this->globalStamp = 1;
    this->fixedValue.clear();
    this->newAnd = nullptr;
  }

  ~rootNode() {
    if (newAnd != nullptr) {
      delete newAnd;
    }
  }

  int getSize_(){return 1 + b.d->getSize_();}

  inline void assignRootNode(vec<Lit> &units, DAG<T> *d, bool fromCache_,
                             int nbVar, vec<Var> &fVar, vec<int> &idxReason)
  {
    b.initBranch(units, d, fVar);
    idxReason.copyTo(reasonForUnits);
    fromCache = fromCache_;
  }

  inline void printNNF(std::ostream& out, bool certif)
  {
    stamp = globalStamp + 1;
    int idxCurrent = ++idxOutputStruct;

    if(certif)
    {
      out << "o " << idxCurrent << " 1 ";
      for(int i = 0 ; i<reasonForUnits.size() ; i++) out << reasonForUnits[i] << " ";
      out << "0" << endl;
    }
    else out << "o " << idxCurrent << " 0" << endl;

    b.printNNF(out, certif);
    out << idxCurrent << " " << (b.d)->getIdx() << " ";
    if(certif) out << (fromCache ? "1" : "2") << " ";

    Lit *pUnit = &DAG<T>::unitLits[b.idxUnitLit];
    for( ; *pUnit != lit_Undef ; pUnit++) out << readableLit(*pUnit) << " ";
    out << "0" << endl;

    globalStamp += idxOutputStruct;
  }// printNNF


  inline bool isSAT()
  {
    globalStamp++;
    return b.isSAT();
  }

  inline bool isSAT(vec<Lit> &units)
  {
    globalStamp++;
    return b.isSAT(units);
  }


  inline T computeNbModels()
  {
    globalStamp++;
    return b.computeNbModels();
  }

private:
  ImplicitAnd<T>* newAnd;
};

#endif
