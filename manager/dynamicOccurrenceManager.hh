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
#ifndef MODELCOUNTER_DYNAMIC_OCCURRENCE_MANAGER
#define MODELCOUNTER_DYNAMIC_OCCURRENCE_MANAGER

#include "../utils/System.hh"
#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"
#include "../mtl/Sort.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Heap.hh"
#include "../mtl/Alg.hh"
#include "../DAG/DAG.hh"

#include "../manager/CnfOccurrenceManager.hh"

class DynamicOccurrenceManager : public CnfOccurrenceManager
{
private:
  void initClauses(vec<vec<Lit> > &clauses);

public:
  DynamicOccurrenceManager(int nbClause, int nbVar, int maxClauseSize);
  DynamicOccurrenceManager(vec<vec<Lit> > &clauses, int nbVar);

  void preUpdate(vec<Lit> &lits);
  void postUpdate(vec<Lit> &lits);
  void debug(Solver &s);
  void removeIdxFromOccList(vec<int> &o, int idx);

  // we cannot use this function here
  inline void initialize(vec<Var> &setOfVar, vec<Lit> &units){assert(0);}
  void initFormula(vec<vec<Lit> > &_clauses);
};

#endif
