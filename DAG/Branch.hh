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
#ifndef Minisat_DAG_Branch_h
#define Minisat_DAG_Branch_h

#include <iostream>
using namespace std;

#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"
#include "DAG.hh"

template<class T> class DAG;
template<class T> class Branch
{
public:
  int idxUnitLit, idxFreeVar;
  DAG<T> *d;
  static Solver *s;

  inline void initBranch(vec<Lit> &units, DAG<T> *_d, vec<Var> &fVar)
  {
    assert(_d);
    d = _d;
    idxUnitLit = d->saveUnitLit(units);
    idxFreeVar = d->saveFreeVar(fVar);
  }// initBranch

  inline void initBranch(DAG<T> *_d)
  {
    d = _d;
    idxUnitLit = idxFreeVar = 0;
  }// initBranch

  inline int nbUnit()
  {
    int cpt = 0;
    Lit *pUnit = &DAG<T>::unitLits[idxUnitLit];
    for( ; pUnit[cpt] != lit_Undef ; cpt++);
    return cpt;
  }

  inline int nbFree()
  {
    int cpt = 0;
    Var *vf = &DAG<T>::freeVariables[idxFreeVar];
    for( ; vf[cpt] != var_Undef ; cpt++);
    return cpt;
  }

  inline void printNNF(std::ostream& out, bool certif)
  {
    d->printNNF(out, certif);
  }// printNNF

  inline int computeState_(int nbAssums)
  {
    Lit *pUnit = &DAG<T>::unitLits[idxUnitLit];
    for(int i = 0 ; pUnit[i] != lit_Undef ; i++)
      {
        if(!DAG<T>::assumsValue[var(pUnit[i])]) continue;
        nbAssums--;
        if(DAG<T>::assumsValue[var(pUnit[i])] != sign(pUnit[i]) + 1) return TOUCH_UNSAT;
      }

    bool saveTouch = false;
    Var *vf = &DAG<T>::freeVariables[idxFreeVar];
    for(int i = 0 ; vf[i] != var_Undef ; i++)
      {
        if(DAG<T>::assumsValue[vf[i]]) nbAssums--;
        saveTouch = saveTouch || DAG<T>::assumsValue[vf[i]];
      }

    assert(d);
    d->markedNode_(nbAssums);
    if(d->getState() != NOT_TOUCH) return d->getState();
    return saveTouch ? TOUCH : NOT_TOUCH;
  }// computeState_


  inline bool isSAT(vec<Lit> &unitsLitBranches)
  {
    Lit *pUnit = &DAG<T>::unitLits[idxUnitLit];
    for(int i = 0 ; pUnit[i] != lit_Undef ; i++)
      if(DAG<T>::fixedValue[var(pUnit[i])] && (sign(pUnit[i]) + 1) != DAG<T>::fixedValue[var(pUnit[i])]) return 0;

    bool ret = true;
    if(s)
      {
	s->newDecisionLevel();

	for(int i = 0 ; ret && pUnit[i] != lit_Undef ; i++)
	  if(!DAG<T>::fixedValue[var(pUnit[i])])
	    {
	      if(s->value(pUnit[i]) == l_Undef)
                {
                  // cout << "uncheckedEnqueue: " << readableLit(pUnit[i]) << endl;
                  s->uncheckedEnqueue(pUnit[i]);
                }
	      else if(s->value(pUnit[i]) == l_False)
                {
                  // cout << "want to assign to false " << readableLit(pUnit[i]) << endl;
                  ret = false;
                }
	    }

	ret = ret && (s->propagate() == CRef_Undef);
        // cout << "res = " << ret << " --> "; showListLit(s->trail);
      }

    // Solver *saveS = s;
    // if(!ret) s = NULL;

    // bool tmpsat = ret;
    // ret = true;

    if(ret)
      {
	int saveSizeUnit = unitsLitBranches.size();

	for(int i = 0 ; pUnit[i] != lit_Undef ; i++) if(!DAG<T>::fixedValue[var(pUnit[i])]) unitsLitBranches.push(pUnit[i]);
	ret = d->isSAT(unitsLitBranches);
	unitsLitBranches.shrink(unitsLitBranches.size() - saveSizeUnit);
      }

    // if(!tmpsat) s = saveS;
    if(s) s->cancelUntil(s->decisionLevel() - 1);

    return ret;
  }// isSAT


  inline T computeNbModels()
  {
    T computeWeight = 1;

    Lit *pUnit = &DAG<T>::unitLits[idxUnitLit];
    for(int i = 0 ; pUnit[i] != lit_Undef ; i++)
      {
        if(!DAG<T>::varProjected[var(pUnit[i])]) continue;
        if(DAG<T>::fixedValue[var(pUnit[i])] &&
           (sign(pUnit[i]) + 1) != DAG<T>::fixedValue[var(pUnit[i])]) return 0;

        computeWeight *= T(DAG<T>::weights[toInt(pUnit[i])]);
      }
    T c = d->computeNbModels();

    Var *vf = &DAG<T>::freeVariables[idxFreeVar];
    for(int i = 0 ; vf[i] != var_Undef ; i++)
      {
        if(!DAG<T>::varProjected[vf[i]]) continue;
        switch(DAG<T>::fixedValue[vf[i]])
          {
          case IS_FALSE :
            computeWeight *= T(DAG<T>::weights[(vf[i]<<1) | 1]);
            break;
          case IS_TRUE :
            computeWeight *= T(DAG<T>::weights[vf[i]<<1]);
            break;
          default :
            computeWeight *= T(DAG<T>::weightsVar[vf[i]]);
          }
      }

    return c * computeWeight;
  }


  inline int nbReachableAssums(int nbAssums, int deep)
  {
    int ret = 0;
    Lit *pUnit = &DAG<T>::unitLits[idxUnitLit];
    for(int i = 0 ; pUnit[i] != lit_Undef ; i++)
      {
        if(!DAG<T>::assumsValue[var(pUnit[i])]) continue;
        if(DAG<T>::assumsValue[var(pUnit[i])] != sign(pUnit[i]) + 1) return nbAssums;
        ret++;
      }

    Var *vf = &DAG<T>::freeVariables[idxFreeVar];
    for(int i = 0 ; vf[i] != var_Undef ; i++) if(DAG<T>::assumsValue[vf[i]]) ret++;
    return ret + d->nbReachableAssums(nbAssums - ret, deep);
  }
};

template<class T> Solver *Branch<T>::s = NULL;

#endif
