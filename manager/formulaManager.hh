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
#ifndef MANAGER_FORMULA_MANAGER
#define MANAGER_FORMULA_MANAGER

class FormulaManager
{
private:
  vec<vec<Lit> > clauses;
  vec<int> nbSat, nbUnsat;
  vec<vec<int> > occurrence;
  int nbVar;
  
public:
  FormulaManager(vec<vec<Lit> > &cls, int nbVar);
  void assignValue(vec<Lit> &lits);
  void unassignValue(vec<Lit> &lits);
  void debug(vec<lbool> &currentValue);

  inline int getNbVar(){return nbVar;}
  inline vec<vec<Lit> > &getClauses(){return clauses;}
  inline int nbLitSatInClause(int idx){return nbSat[idx];}
  inline int nbLitUnsatInClause(int idx){return nbUnsat[idx];}  
};

#endif
