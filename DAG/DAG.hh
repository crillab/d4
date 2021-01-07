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
#ifndef SolverDAG_h
#define SolverDAG_h

#include <gmpxx.h>
#include <assert.h>
#include <math.h>
#include <string>
#include <iostream>
#include "../utils/SolverTypes.hh"
#include "../mtl/Vec.hh"
#include <vector>
#include <string>
#include <map>

#define IDX_FALSE 0
#define IDX_TRUE 1

#define BLOCK_UNIT_LITS 1<<20
#define BLOCK_FREE_VARS 1<<20

#define CURR_DB 0
#define SIZE_TOUCH 0

#define IS_NOT_ASSIGN 0
#define IS_TRUE 1
#define IS_FALSE 2

#define NOT_TOUCH 0
#define TOUCH 1
#define TOUCH_UNSAT 2

#include "Branch.hh"
#include "ImplicitAnd.hh"
#include "Root.hh"
#include "True.hh"
#include "False.hh"
#include "DeterministicOrNode.hh"
#include "BinaryDeterministicOrNode.hh"
#include "DecomposableAndNode.hh"
#include "KromFormula.hh"
#include "PCNode.hh"

template<class T> class DAG
{
 public:
  static int globalStamp;
  static int nbNodes;
  static int nbEdges;
  static int nbImplicitAnd;
  static int nbMadeUpLits;

  static Var *freeVariables;
  static unsigned int szFreeVariables, capFreeVariables;

  static Lit *unitLits;
  static unsigned int szUnitLits, capUnitLits;

  static vec<char> fixedValue;               // the variable fixed by the user
  static vec<double> weights;                // the table of weight
  static vec<bool> varProjected;             // the set of projected variables
  static vec<double> weightsVar;             // the table of weight for the variable
  static vec<char> assumsValue;

  static int idxOutputStruct;

  int stamp = 0;

  DAG()
  {
    stamp = 0;
    nbNodes++;
  }

  virtual ~DAG(){nbNodes--;}

  virtual int getSize(){globalStamp++;return getSize_();}
  virtual int getSize_(){return 1;}

  virtual int getIdx()
  {
    assert(stamp >= globalStamp);
    return stamp - globalStamp;
  }

  virtual void printNNF(std::ostream& out, bool certif) = 0;
  virtual T computeNbModels();
  virtual void debug(){}
  virtual void debug(vec<Lit> &trail){}
  virtual void saveFreeVariable(vec<Var>&c){}
  virtual bool isSAT(vec<Lit> &unitsLitBranches) {return false;}


  inline T computeNbModelsConditioning(vec<Lit> &v)
  {
    for(int i = 0 ; i<v.size() ; i++) fixedValue[var(v[i])] = (sign(v[i])) ? IS_FALSE : IS_TRUE;

    T tmp = computeNbModels();
    for(int i = 0 ; i<v.size() ; i++) fixedValue[var(v[i])] = IS_NOT_ASSIGN;

    return tmp;
  }// computeNbModelsConditioning

  inline bool isSATConditioning(vec<Lit> &v)
  {
    for(int i = 0 ; i<v.size() ; i++) fixedValue[var(v[i])] = (sign(v[i])) ? IS_FALSE : IS_TRUE;

    vec<Lit> units;
    v.copyTo(units);

    Solver *stmp = Branch<T>::s;
    assert(stmp->decisionLevel() == 0);
    bool tmp = true;
    if(Branch<T>::s)
    {
      stmp->newDecisionLevel();
      for(int i = 0 ; tmp && i<v.size() ; i++)
      {
        if(stmp->value(v[i]) == l_Undef) stmp->uncheckedEnqueue(v[i]);
        else if(stmp->value(v[i]) == l_False) tmp = false;
      }

      tmp = tmp && (stmp->propagate() == CRef_Undef);
    }

    tmp = tmp && isSAT(units);
    for(int i = 0 ; i<v.size() ; i++) fixedValue[var(v[i])] = IS_NOT_ASSIGN;

    if(stmp) Branch<T>::s->cancelUntil(0);
    return tmp;
  }// isSATConditioning


  /**
     Save the unit literals detected.

     @param[in] v, the set of unit literals
     \return the position in unitLits
  */
  inline int saveUnitLit(vec<Lit> &v)
  {
    if(szUnitLits + v.size() + 1 > capUnitLits)
    {
      capUnitLits += BLOCK_UNIT_LITS;
      unitLits = (Lit *) realloc(unitLits, capUnitLits * sizeof(Lit));
      if(!unitLits){printf("Memory out: saveUnitLit\n"); exit(30);}
      printf("c unitLits %u %lu Bytes\n", capUnitLits, capUnitLits * sizeof(Lit));
    }

    for(int i = 0 ; i<v.size() ; i++) unitLits[szUnitLits++] = v[i];
    unitLits[szUnitLits++] = lit_Undef;

    nbNodes += v.size();
    nbEdges += v.size();

    return szUnitLits - v.size() - 1;
  }// saveUnitLit

  /**
     Save the unit literals detected.

     @param[in] v, the set of unit literals
     \return the position in unitLits
  */
  static inline int saveFreeVar(vec<Var> &v)
  {
    if(v.size() == 0) return 0;
    if(szFreeVariables + v.size() + 1 > capFreeVariables)
    {
      capFreeVariables += BLOCK_FREE_VARS;
      freeVariables = (Var *) realloc(freeVariables, capFreeVariables * sizeof(Var));
      if(!freeVariables){printf("Memory out: saveFreeVar\n"); exit(30);}
      printf("c freeVariables %u\n", capFreeVariables);
    }

    for(int i = 0 ; i<v.size() ; i++) freeVariables[szFreeVariables++] = v[i];
    freeVariables[szFreeVariables++] = var_Undef;
    return szFreeVariables - v.size() - 1;
  }// saveUnitLit


  /* Initialization of the conditionning interpretation */
  static inline void initSizeVector(int nbElt){for(int i = 0 ; i<nbElt ; i++) fixedValue.push(IS_NOT_ASSIGN);}
};

// Needed to please compiler since we use templated static members.
template<typename T> int DAG<T>::nbImplicitAnd{0};
template<typename T> int DAG<T>::nbMadeUpLits{0};

template<class T> Lit *DAG<T>::unitLits = NULL;
template<class T> Var *DAG<T>::freeVariables = NULL;

template<class T> unsigned int DAG<T>::szFreeVariables = 0;
template<class T> unsigned int DAG<T>::capFreeVariables = 0;
template<class T> unsigned int DAG<T>::szUnitLits = 0;
template<class T> unsigned int DAG<T>::capUnitLits = 0;
template<class T> int DAG<T>::nbNodes = 0;
template<class T> int DAG<T>::nbEdges = 0;
template<class T> int DAG<T>::globalStamp = 1;
template<class T> vec<char> DAG<T>::fixedValue;
template<class T> vec<char> DAG<T>::assumsValue;
template<class T> vec<double> DAG<T>::weights;
template<class T> vec<double> DAG<T>::weightsVar;
template<class T> vec<bool> DAG<T>::varProjected;

template<class T> int DAG<T>::idxOutputStruct = 0;

#endif
