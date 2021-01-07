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
#ifndef UTILS_INTERPRETATION_REFINER
#define UTILS_INTERPRETATION_REFINER

#include "../utils/SolverTypes.hh"

class InterpretationRefiner
{
private:
  vec<vec<Lit> > cnf;
  vec<bool> isProtected;

  vec<vec<int> > occurrences;

  vec<bool> marked;
  vec<bool> isSelector;
  vec<bool> satisfiedByAssums; 
  
  vec<int> selectorToClauseIdx;
  vec<int> nbTrueLit;          // nbTrueLit[i]: the number of literal that are existantially quatified and that satisfy the ith clause 
  vec<int> nbExistVariables;
  vec<int> idxClausesWithExist;
  
public:
  InterpretationRefiner(vec<vec<Lit> > &formula, vec<bool> &pvar);
  InterpretationRefiner(vec<bool> &pvar);

  void initIdxClausesWithExist(int nbVar, vec<int> &idxVec, vec<Lit> &selectors);
  
  void init(vec<Lit> &assums, vec<lbool> &model);
  void transferPureLiteral(vec<Lit> &assums, vec<lbool> &model);
  void refine(vec<Lit> &assums, vec<lbool> &model);
  void shouldBeRelaxed(vec<Var> &subsetOfEVar);    
  void addClause(vec<Lit> &cl);
};

#endif
