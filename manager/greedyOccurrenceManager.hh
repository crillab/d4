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
#ifndef MANAGER_GREEDY_OCCURRENCE_MANAGER
#define MANAGER_GREEDY_OCCURRENCE_MANAGER

#include "../manager/CnfOccurrenceManager.hh"

class GreedyOccurrenceManager : public CnfOccurrenceManager
{
  vec<vec<int> > initOccList;

  int counterStampViewClause;
  vec<int> stampViewClause; 

  void initializeFromLiteral(Lit l);
  
public:   
  GreedyOccurrenceManager(vec<vec<Lit> > &clauses, int nbVar); 
  void initialize(vec<Var> &setOfVar, vec<Lit> &units);

  // we cannot use these functions here!
  inline void preUpdate(vec<Lit> &lits){assert(0);}
  inline void postUpdate(vec<Lit> &lits){assert(0);}
};

#endif
