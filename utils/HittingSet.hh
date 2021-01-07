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
#ifndef UTILS_HITTING_SET
#define UTILS_HITTING_SET

#include "../mtl/Sort.hh"
#include "../mtl/Vec.hh"
#include "../utils/SolverTypes.hh"
#include "../heuristics/ScoringMethod.hh"


class HittingSet
{  
  vec<bool> selectedLit;
  ScoringMethod *sm;

  vec<vec< int> > occ;

  void debug(vec<vec<Lit> > &cls, vec<Lit> &hs);
  
public:
  struct LitOrderScoringMethod
  {
    ScoringMethod *sm;
    bool operator () (Lit x, Lit y) const {return sm->computeScore(var(x)) > sm->computeScore(var(y)); }
    LitOrderScoringMethod(ScoringMethod *_sm) : sm(_sm) { }
  };

  
  HittingSet(int nbVar, ScoringMethod *_sm);
  void computeHittingSet(vec<vec<Lit> > &cls, vec<Lit> &hs);
};

#endif
