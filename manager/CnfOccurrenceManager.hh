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
#ifndef MANAGER_CNF_OCCURRENCE_MANAGER
#define MANAGER_CNF_OCCURRENCE_MANAGER

#include <iostream>

#include "../mtl/Alg.hh"
#include "../mtl/Sort.hh"
#include "../interfaces/OccurrenceManagerInterface.hh"
#include "../interfaces/BucketManagerInterface.hh"

using namespace std;

class CnfOccurrenceManager : public OccurrenceManagerInterface
{
protected:
  vec<vec<Lit> > clauses;
  int nbVar, maxSizeClause;
  vec<lbool> currentValue;
  vec<int> nbUnsat;
  vec<int> nbSat;
  vec<Lit> watcher;

  vec<int> currentIdx;
  int currentSize;
  vec<int> stackSize;
  vec<bool> inCurrentComponent;

  inline void showOccurenceList()
  {
    for(int i = 0 ; i<occList.size() ; i++)
      {
        printf("%s%d: ", (i&1) ? "-" : "", (i>>1) + 1);
        for(int j = 0 ; j<occList[i].size() ; j++) printf("%d ", occList[i][j]);
        printf("\n");
      }
  }

protected:
  // to manage the connected component
  vec<int> mustUnMark;
  vec<Var> tmpVecVar;
  vec<int> idxComponent;
  vec<bool> tmpMark, markView;

  inline void resetUnMark()
  {
    for(int i = 0 ; i<mustUnMark.size() ; i++) markView[mustUnMark[i]] = false;
    mustUnMark.setSize(0);
  }// resetUnMark

  int connectedToLit(Lit l, vec<int> &v, vec<Var> &varComponent, int nbComponent);

public:
  CnfOccurrenceManager(int nbClause, int nbVar, int maxClauseSize);
  CnfOccurrenceManager(vec<vec<Lit> > &clauses, int nbVar);

  int computeConnectedComponent(vec< vec<Var> > &varConnected, vec<Var> &setOfVar, vec<Var> &freeVar,
                                vec<Var> &notFreeVar);

  inline void initFormula(vec<vec<Lit> > &_clauses)
  {
    clauses.clear();
    currentIdx.clear();

    for(int i = 0 ; i<_clauses.size() ; i++)
      {
        clauses.push();
        assert(_clauses[i].size());
        _clauses[i].copyTo(clauses.last());
        currentIdx.push(i);
      }

    currentSize = clauses.size();
    stackSize.clear();
    for(int i = 0 ; i<currentValue.size() ; i++) currentValue[i] = l_Undef;
  }// initFormula

  inline int getNbBinaryClause(Lit l)
  {
    int nbBin = 0;

    for(int i = 0 ; i<occList[toInt(l)].size() ; i++)
      {
        int idx = occList[toInt(l)][i];
        if(clauses[idx].size() - nbUnsat[idx] == 2) nbBin++;
      }

    return nbBin;
  }

  inline void showOccList()
  {
    for(int i = 0 ; i<occList.size() ; i++)
      {
        Lit l = mkLit(i>>1, i&1);
        if(!occList[i].size()) continue;

        printf("%d: ", readableLit(l));
        for(int j = 0 ; j<occList[i].size() ; j++) printf("%d ", occList[i][j]);
        printf("\n");
      }
  }


  inline void showFormula()
  {
    printf("Occurrence Managaer: print formula\n");
    for(int i = 0 ; i<clauses.size() ; i++) showListLit(clauses[i]);
  }// showFormula


  inline int getNbBinaryClause(Var v){return getNbBinaryClause(mkLit(v, false)) + getNbBinaryClause(mkLit(v, true));}
  inline int getNbNotBinaryClause(Lit l){return getNbClause(l) - getNbBinaryClause(l);}
  inline int getNbNotBinaryClause(Var v){return getNbClause(v) - getNbBinaryClause(v);}
  inline int getNbClause(Var v){return getNbClause(mkLit(v, false)) + getNbClause(mkLit(v, true));}
  inline int getNbClause(Lit l){return occList[toInt(l)].size();}
  inline int getNbClause(){return clauses.size();}
  inline vec<int> &getVecIdxClause(Lit l){return occList[toInt(l)];}
  inline vec<Lit> &getClause(int idx){return clauses[idx];}
  inline int getNbUnsat(int idx){return nbUnsat[idx];}
  inline int getNbVariable(){return nbVar;}

  inline bool litIsAssigned(Lit l){return currentValue[var(l)] != l_Undef;}
  inline bool litIsAssignedToTrue(Lit l)
  {
    if(sign(l)) return currentValue[var(l)] == l_False;
    else return currentValue[var(l)] == l_True;
  }

  inline bool varIsAssigned(Var v){return currentValue[v] != l_Undef;}
  inline int getMaxSizeClause(){return maxSizeClause;}

  virtual inline int getSumSizeClauses()
  {
    int sum = 0;
    for(int i = 0 ; i<clauses.size() ; i++) sum += clauses[i].size();
    return sum;
  }// getSumSizeClauses

  bool isSatisfiedClause(int idx);
  bool isSatisfiedClause(vec<Lit> &c);
  bool isNotSatisfiedClauseAndInComponent(int idx, vec<bool> &inCurrentComponent);


  // debug part
  void checkCurrentInterpretation(Solver &s);

  inline bool byPass(int mode, int idx)
  {
    if(mode >= NB && clauses[idx].size() <= 2) return true;
    if(mode == NT && !nbUnsat[idx]) return true;
    return false;
  }

  void getCurrentClauses(vec<int> &idxClauses, vec<bool> &inCurrentComponent);
  void updateCurrentClauseSet(vec<Var> &component);
  void popPreviousClauseSet();
};

#endif
