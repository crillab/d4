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
#ifndef PREPROC_EQUIVSIMPLI
#define PREPROC_EQUIVSIMPLI

#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"

#include "../mtl/Sort.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Alg.hh"


class EquivClauseSimplification
{
private:
  Solver s;
  vec< vec<int> > occurrences;

  void loadSolver(vec< vec<Lit> > &clauses);
  void cleanUpSolver();
  void vivification();
  void occurrenceElimination();
  void rebuildOccurrence();

  int tryToRemoveLiteral(Lit l);
  void removeAndUpdateOccurrence(int index, Lit blocked = lit_Undef, bool clauseAttached = false);

protected:
  struct LitOrderOccurrenceLt {
    const vec< vec<int> > &occurrences;
    bool operator () (Lit x, Lit y) const { return occurrences[toInt(x)].size() < occurrences[toInt(y)].size();}
    LitOrderOccurrenceLt(const vec< vec<int> > &occ) : occurrences(occ) { }
  };
  
public:
  EquivClauseSimplification(int nbVar);
  void equivPreproc(vec< vec<Lit> > &clauses, bool vivification = true, bool occurrenceELim = true);  
};

#endif
