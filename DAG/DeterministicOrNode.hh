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
#ifndef Minisat_DAG_DetermOrNode_h
#define Minisat_DAG_DetermOrNode_h

#include "DAG.hh"

template<class T> class DAG;
template<class T> class DeterministicOrNode : public DAG<T>
{
  using DAG<T>::nbEdges;
  using DAG<T>::globalStamp;
  using DAG<T>::idxOutputStruct;
  using DAG<T>::stamp;

public:
  T nbModels;
  bool saveDecision;
  DAG<T> *alreadyCompute;
  vec< Branch<T> > sons;

  vec<Lit> unitSaveLit;

  inline void setUnitSaveLit(vec<Lit> &u)
  {
    u.copyTo(unitSaveLit);
  }

  DeterministicOrNode(){stamp = 0;}

  inline void addBranch(vec<Lit> &units, DAG<T> *d, vec<Var> &fVar)
  {
    sons.push();
    (sons.last()).initBranch(units, d, fVar);
    nbEdges++;
  }// addBranch

  inline void addBranch(vec<Lit> &units, DAG<T> *d)
  {
    sons.push();
    vec<Var> fVar;
    (sons.last()).initBranch(units, d, fVar);
    nbEdges++;
  }// addBranch

  inline void addBranch(DAG<T> *d)
  {
    sons.push();
    (sons.last()).initBranch(d);
    nbEdges++;
  }// addBranch

  inline int getSize_()
  {
    if(stamp == globalStamp) return 0;
    stamp = globalStamp;
    int cpt = 0;
    for(int i = 0 ; i<sons.size() ; i++) cpt += sons[i].d->getSize_();
    return 1 + cpt;
  }


  inline void printNNF(std::ostream& out, bool certif)
  {
    if(stamp >= globalStamp) return;
    stamp = globalStamp + idxOutputStruct + 1;
    int idxCurrent = ++idxOutputStruct;

    //out << "o " << idxCurrent << " " << sons.size() << " 0" << endl;
    out << "o " << idxCurrent << endl;

    for(int i = 0 ; i<sons.size() ; i++)
      {
        sons[i].printNNF(out, certif);

        out << idxCurrent << " " << (sons[i].d)->getIdx() << " ";
        Lit *pUnit = &DAG<T>::unitLits[sons[i].idxUnitLit];
        for( ; *pUnit != lit_Undef ; pUnit++) out << readableLit(*pUnit) << " ";
        out << "0" << endl;
      }
  }// printNNF

  inline bool isSAT(vec<Lit> &unitsLitBranches)
  {
    static int cpt = 0;
    int curr = cpt++;

    // printNNF();
    // cout << "call " << curr << " -> " << this << endl;
    // cout << "curr: " << curr << " -> " ; showListLit(unitsLitBranches);

    bool stampOK = stamp == globalStamp;
    bool sava = saveDecision;
    if(0 && stamp == globalStamp)
      {
        // cout << "caching" << endl;
        return saveDecision;
      }

    saveDecision = false;
    for(int i = 0 ; !saveDecision && i<sons.size() ; i++)
      {
        // cout << curr << " => test: " << i << "/" << sons.size() << endl;
	saveDecision = sons[i].isSAT(unitsLitBranches);
        // cout << curr << " = " << i << " => ret: " << " = " << saveDecision << endl;
      }
    stamp = globalStamp;

    if(stampOK) assert(saveDecision == sava);

    return saveDecision;
  }// computeNbModels


  inline T computeNbModels()
  {
    if(stamp == globalStamp) return nbModels;

    nbModels = 0;
    for(int i = 0 ; i<sons.size() ; i++) nbModels += sons[i].computeNbModels();
    stamp = globalStamp;
    return nbModels;
  }// computeNbModels


  void debug(vec<Lit> &trail)
  {
    cout << "Hello: " << this << endl;
    cout << "trail: "; showListLit(trail);
    cout << "save: "; showListLit(unitSaveLit);
  }
};

#endif
