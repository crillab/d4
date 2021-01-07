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
#include "../manager/greedyOccurrenceManager.hh"

/**
   Constructor.
   
   @param[in] clauses, the set of clauses.
   @param[in] nbVar, the number of variable present in the problem.
 */
GreedyOccurrenceManager::GreedyOccurrenceManager(vec<vec<Lit> > &_clauses, int _nbVar) : CnfOccurrenceManager(_clauses, _nbVar)
{
  for(int i = 0 ; i<(nbVar<<1) ; i++) initOccList.push();
  
  counterStampViewClause = 0;
  for(int i = 0 ; i<clauses.size() ; i++)
    {
      for(int j = 0 ; j<clauses[i].size() ; j++)
        initOccList[toInt(clauses[i][j])].push(i);

      stampViewClause.push(0);
    }
}// constructor


/**
   Visit the set of not already visided clauses where the literal l
   occur and initialize the data structures nbSat, nbUnsat and the
   occList w.r.t. the currentValue table.

   @param[in] l, a literal
 */
void GreedyOccurrenceManager::initializeFromLiteral(Lit l)
{
  vec<int> &occ = initOccList[toInt(l)];

  for(int j = 0 ; j<occ.size() ; j++)
    if(stampViewClause[occ[j]] != counterStampViewClause)
      {
        stampViewClause[occ[j]] = counterStampViewClause;
        nbUnsat[occ[j]] = 0;

        assert(occ[j] < clauses.size());        
        vec<Lit> &cl = clauses[occ[j]];
        bool isSAT = false;
        for(int k = 0 ; k<cl.size() && !isSAT ; k++)
          {
            if(currentValue[var(cl[k])] != l_Undef)
              {
                isSAT = (sign(cl[k]) && currentValue[var(cl[k])] == l_False) || (!sign(cl[k]) && currentValue[var(cl[k])] == l_True);
                if(!isSAT) nbUnsat[occ[j]]++;
              }
          }

        if(isSAT) continue;
        for(int k = 0 ; k<cl.size() ; k++)
          if(currentValue[var(cl[k])] == l_Undef) occList[toInt(cl[k])].push(occ[j]);
      }
}// initializeFromLiteral


/**
   Compute the occurence list w.r.t. a set of unit variables and a set
   of variables the problem is defined on.

   @param[in] setOfVar, the set of variables present in the problem
   @param[in] units, the set of variables already assigned
 */
void GreedyOccurrenceManager::initialize(vec<Var> &setOfVar, vec<Lit> &units)
{
  for(int i = 0 ; i<currentValue.size() ; i++) currentValue[i] = l_Undef; 
  for(int i = 0 ; i<units.size() ; i++) currentValue[var(units[i])] = (sign(units[i])) ? l_False : l_True; 

  counterStampViewClause++;
  for(int i = 0 ; i<setOfVar.size() ; i++)
    {
      Lit l = mkLit(setOfVar[i], false);
      occList[toInt(l)].clear();
      occList[toInt(~l)].clear();
    }
  
  for(int i = 0 ; i<setOfVar.size() ; i++)
    {      
      initializeFromLiteral(mkLit(setOfVar[i], false));
      initializeFromLiteral(mkLit(setOfVar[i], true));
    }   
}// computeOccurenceList
