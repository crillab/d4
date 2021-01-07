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
#ifndef UTILS_RESIDUAL_NOT_PC
#define UTILS_RESIDUAL_NOT_PC

#include <iostream>

#include "../interfaces/ClauseCheckProperty.hh"
#include "../utils/SolverTypes.hh"

using namespace std;

class ResidualNotPC
{
private:
  ClauseCheckProperty *cck;
  vec< vec<Lit> > residue;
  vec<int> indexes;
  
  vec<int> stack;
  int currentStart;

public:

  ResidualNotPC(ClauseCheckProperty *_cck) : cck(_cck)
  {
    currentStart = 0;
  }

  inline vec<Lit> &getClause(int i){return residue[i];}
  inline vec<Lit> &getRemainingClause(int i){return residue[indexes[currentStart + i]];}
  
  inline void pushSize(){stack.push(currentStart);}
  inline void popSize(){currentStart = stack.last(); stack.pop();}
  inline int getCurrentSize(){return residue.size() - currentStart;}
  inline void setCurrentSize(int csz){currentStart = csz;}
  inline int topSize(){assert(stack.size()); return stack.last();}

  inline void setComponent(vec<Var> &component){cck->setComponent(component);}
  inline void unsetComponent(vec<Var> &component){cck->unsetComponent(component);}
  
  inline void addClause(vec<Lit> &cl)
  {
    residue.push();
    cl.copyTo(residue.last());
    indexes.push(residue.size() - 1);
  }// addClause

  void updateResidualFormula(vec<Var> &component);
  void extractRemainingClauses(vec<vec<Lit> > &rcl);
  void printRemainingClauses();  
};

#endif
