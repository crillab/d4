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
#ifndef INTERFACE_OCC_MANAGER_INTERFACE
#define INTERFACE_OCC_MANAGER_INTERFACE

#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"

class OccurrenceManagerInterface
{
public:
  vec< vec<int> > occList;
  virtual ~OccurrenceManagerInterface(){}

  virtual int getNbBinaryClause(Var v) = 0;
  virtual int getNbNotBinaryClause(Var v) = 0;
  virtual int getNbClause(Var v) = 0;
  virtual int getNbBinaryClause(Lit l) = 0;
  virtual int getNbNotBinaryClause(Lit l) = 0;
  virtual int getNbClause(Lit l) = 0;
  virtual int getNbClause() = 0;

  virtual vec<Lit> &getClause(int idx) = 0;
  virtual int getSizeClause(int idx){return getClause(idx).size();}
  virtual vec<int> &getVecIdxClause(Lit l) = 0;
  virtual int getNbUnsat(int idx) = 0;
  virtual int getNbVariable() = 0;
  virtual int getSumSizeClauses() = 0;

  virtual bool litIsAssigned(Lit l) = 0;
  virtual bool litIsAssignedToTrue(Lit l) = 0;

  virtual bool varIsAssigned(Var v) = 0;
  virtual int getMaxSizeClause() = 0;
  virtual bool isSatisfiedClause(int idx) = 0;
  virtual bool isSatisfiedClause(vec<Lit> &c) = 0;
  virtual bool isNotSatisfiedClauseAndInComponent(int idx, vec<bool> &inCurrentComponent) = 0;

  inline const vec< vec<int> > &getOccurrenceList(){return occList;}

  virtual int computeConnectedComponent(vec<vec<Var> > &varConnected, vec<Var> &setOfVar,
                                        vec<Var> &freeVar, vec<Var> &notFreeVar) = 0;
  virtual void preUpdate(vec<Lit> &lits) = 0;
  virtual void postUpdate(vec<Lit> &lits) = 0;
  virtual void initialize(vec<Var> &setOfVar, vec<Lit> &units) = 0;
  virtual void showOccurenceList() {}
  virtual void showFormula(){}
  virtual void initFormula(vec<vec<Lit> > &_clauses) = 0;
  virtual void debug(Solver &s){}

  virtual bool byPass(int mode, int idx){return false;}

  virtual void getCurrentClauses(vec<int> &idxClauses, vec<bool> &inCurrentComponent )
  {
    idxClauses.setSize(0);
    for(int i = 0 ; i<getNbClause() ; i++)
      if(isNotSatisfiedClauseAndInComponent(i, inCurrentComponent)) idxClauses.push(i);
  }


  virtual void getCurrentClauses2(vec<int> &idxClauses, vec<bool> &inCurrentComponent )
  {
    idxClauses.setSize(0);
    for(int i = 0 ; i<getNbClause() ; i++)
      if(isNotSatisfiedClauseAndInComponent(i, inCurrentComponent)) idxClauses.push(i);
  }


  virtual void updateCurrentClauseSet(vec<Var> &component){return;}
  virtual void popPreviousClauseSet(){return;}
};
#endif
