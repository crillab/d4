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
#include "../mtl/Vec.hh"
#include "../utils/SolverTypes.hh"
#include "../manager/formulaManager.hh"

/**
   Constructor. We initialize the structure.

   @param[in] cls, the set of clauses.
 */
FormulaManager::FormulaManager(vec<vec<Lit> > &cls, int _nbVar) : nbVar(_nbVar)
{
  
  for(int i = 0 ; i<nbVar ; i++){occurrence.push(); occurrence.push();}
  
  for(int i = 0 ; i<cls.size() ; i++)
    {
      nbSat.push(0);
      nbUnsat.push(0);
      clauses.push();
      cls[i].copyTo(clauses.last());

      for(int j = 0 ; j<cls[i].size() ; j++)
        {
          assert(toInt(cls[i][j]) < occurrence.size());
          occurrence[toInt(cls[i][j])].push(i);
        }
    }
}// constructor



/**
   Adjust the number of satisfied/unsatisfied literal present in the
   clauses w.r.t. the new current interpretation.
   
   @param[in] newValue, the new current interpretation
 */
void FormulaManager::assignValue(vec<Lit> &lits)
{
  for(int i = 0 ; i<lits.size() ; i++)
    {
      Lit l = lits[i];
      for(int j = 0 ; j<occurrence[toInt(l)].size() ; j++) nbSat[occurrence[toInt(l)][j]]++;
      for(int j = 0 ; j<occurrence[toInt(~l)].size() ; j++) nbUnsat[occurrence[toInt(~l)][j]]++;          
    }
}// assignValue


/**
   Adjust the number of satisfied/unsatisfied literal present in the
   clauses w.r.t. the new current interpretation.
   
   @param[in] newValue, the new current interpretation
 */
void FormulaManager::unassignValue(vec<Lit> &lits)
{
  for(int i = 0 ; i<lits.size() ; i++)
    {
      Lit l = lits[i];
      for(int j = 0 ; j<occurrence[toInt(l)].size() ; j++) nbSat[occurrence[toInt(l)][j]]--;
      for(int j = 0 ; j<occurrence[toInt(~l)].size() ; j++) nbUnsat[occurrence[toInt(~l)][j]]--;
    }  
}// unassignValue


/**
   Debug function that verifies that the data structures are well
   instantiated.
 */
void FormulaManager::debug(vec<lbool> &currentValue)
{
  // verif the number of satisfied and falsified literals in the clauses
  for(int i = 0 ; i<clauses.size() ; i++)
    {
      int sat = 0, unsat = 0;
      for(int j = 0 ; j<clauses[i].size() ; j++)
        {
          if(currentValue[var(clauses[i][j])] == l_Undef) continue;
          
          int sval = currentValue[var(clauses[i][j])] == l_True ? 0 : 1;
          if(sval == sign(clauses[i][j])) sat++; else unsat++;
        }
      assert(nbSat[i] == sat);
      assert(nbUnsat[i] == unsat);      
    }
}// debug
