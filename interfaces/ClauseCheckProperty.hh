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
#ifndef INTERFACES_CLAUSE_CHECK_PROPERTY
#define INTERFACES_CLAUSE_CHECK_PROPERTY

#include <iostream>
#include "../utils/SolverTypes.hh"

using namespace std;

class ClauseCheckProperty
{
protected:
  vec<bool> markedComponent;
  
public:
  ClauseCheckProperty(int nbVar)
  {
    while(markedComponent.size() < nbVar) markedComponent.push(false);
  }
  
  virtual bool satisfyProperty(vec<Lit> &cl) = 0;
  
  inline void setComponent(vec<Var> &component)
  {
    for(int i = 0 ; i<component.size() ; i++) markedComponent[component[i]] = true; 
  }
  
  inline void unsetComponent(vec<Var> &component)
  {
    for(int i = 0 ; i<component.size() ; i++) markedComponent[component[i]] = false; 
  }

  virtual void printClause(vec<Lit> &cl)
  {
    for(int i = 0 ; i<cl.size() ; i++) cout << readableLit(cl[i]) << " ";
    cout << endl;
  }

  virtual bool extractRemainingClause(vec<Lit> &retCl, vec<Lit> &cl)
  {
    for(int i = 0 ; i<cl.size() ; i++) retCl.push(cl[i]);
    return true;
  }
};

#endif
