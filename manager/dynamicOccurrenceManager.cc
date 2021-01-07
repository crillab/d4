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
#include <iostream>
#include "../manager/dynamicOccurrenceManager.hh"

using namespace std;


/**
   OccurrenceManager constructor. This function initialized the
   structures used.

   @param[in] nbC, the maximum number of clauses allowed
   @param[in] nbV, the number of variables in the problem
   @param[in] maxClSz, the largest clause size
 */
DynamicOccurrenceManager::DynamicOccurrenceManager(int nbC, int nbV, int maxClSz) :
    CnfOccurrenceManager(nbC, nbV, maxClSz)
{
}// DynamicOccurrenceManager

/**
   OccurrenceManager constructor. This function initialized the
   structures used.

   @param[in] _clauses, the set of clauses
   @param[in] _nbVar, the number of variables in the problem
 */
DynamicOccurrenceManager::DynamicOccurrenceManager(vec<vec<Lit> > &_clauses, int _nbVar) :
    CnfOccurrenceManager(_clauses, _nbVar)
{
  for(int i = 0 ; i<clauses.size() ; i++)
    {
      vec<Lit> &c = clauses[i];
      for(int j = 0 ; j<c.size() ; j++) occList[toInt(c[j])].push(i);
    }
}// DynamicOccurrenceManager


/**
   Initiliaze the occurrence manager with a new set of clauses.
 */
void DynamicOccurrenceManager::initFormula(vec<vec<Lit> > &_clauses)
{
  CnfOccurrenceManager::initFormula(_clauses);

  // clear the occurrence list
  for(int i = 0 ; i<occList.size() ; i++) occList[i].clear();

  // construct the occurrence list
  for(int i = 0 ; i<clauses.size() ; i++)
    {
      vec<Lit> &c = clauses[i];
      for(int j = 0 ; j<c.size() ; j++) occList[toInt(c[j])].push(i);
      if(c.size() > maxSizeClause) maxSizeClause = c.size();
    }

  mustUnMark.capacity(clauses.size());
  for(int i = 0 ; i<clauses.size() ; i++)
  {
    if(markView.size() > i) markView[i] = false; else markView.push(false);
    if(nbUnsat.size() > i) nbUnsat[i] = 0; else nbUnsat.push(0);
    if(nbSat.size() > i) nbSat[i] = 0; else nbSat.push(0);
    if(watcher.size() > i) watcher[i] = clauses[i][0]; else watcher.push(clauses[i][0]);
  }
}// initFormula

/**
   Update the occurrence list w.r.t. a new set of assigned variables.
   It's important that the order is conserved between the moment where
   we assign and the moment we unassign.

   @param[in] lits, the new assigned variables
 */
void DynamicOccurrenceManager::preUpdate(vec<Lit> &lits)
{
  vec<int> reviewWatcher;

  for(int i = 0 ; i<lits.size() ; i++)
    {
      Lit l = lits[i];
      currentValue[var(l)] = sign(l) ? l_False : l_True;

      vec<int> &op = occList[toInt(l)];
      for(int j = 0 ; j<op.size() ; j++)
        {
          int idxCl = op[j];
          vec<Lit> &c = clauses[idxCl];
          nbSat[idxCl]++;
          for(int k = 0 ; k<c.size() ; k++)
            if(currentValue[var(c[k])] == l_Undef)
              removeIdxFromOccList(occList[toInt(c[k])], occList[toInt(l)][j]);
        }

      vec<int> &on = occList[toInt(~l)];
      for(int j = 0 ; j<on.size() ; j++)
      {
        int idxCl = on[j];
        nbUnsat[idxCl]++;
        if(watcher[idxCl] == ~l) reviewWatcher.push(idxCl);
      }
    }

  // we search another non assigned literal if requiered
  for(int i = 0 ; i<reviewWatcher.size() ; i++)
  {
    int idxCl = reviewWatcher[i];
    if(nbSat[idxCl]) continue;

    vec<Lit> &c = clauses[idxCl];
    for(int k = 0 ; k<c.size() ; k++)
      if(currentValue[var(c[k])] == l_Undef)
      {
        watcher[idxCl] = c[k];
        break;
      }
  }

  stackSize.push(currentSize);
  int i = 0;
  while(i<currentSize)
  {
    if(!nbSat[currentIdx[i]]) i++;
    else
    {
      currentSize--;
      int tmp = currentIdx[currentSize];
      currentIdx[currentSize] = currentIdx[i];
      currentIdx[i] = tmp;
    }
  }
}// assignValue


/**
   Update the occurrence list w.r.t. a new set of unassigned variables.
   It's important that the order is conserved between the moment where
   we assign and the moment we unassign.

   @param[in] lits, the new assigned variables
 */
void DynamicOccurrenceManager::postUpdate(vec<Lit> &lits)
{
  for(int i = lits.size() - 1 ; i >= 0 ; i--)
    {
      Lit l = lits[i];

      vec<int> &o = occList[toInt(l)];
      for(int j = 0 ; j<o.size() ; j++)
        {
          int idxCl = o[j];
          vec<Lit> &c = clauses[idxCl];
          nbSat[idxCl]--;

          for(int k = 0 ; k<c.size() ; k++)
            {
              if(currentValue[var(c[k])] == l_Undef)
                {
                  assert(occList[toInt(c[k])].size() < occList[toInt(c[k])].capacity());
                  occList[toInt(c[k])].push_(occList[toInt(l)][j]);
                }
            }
        }

      for(int j = 0 ; j<occList[toInt(~l)].size() ; j++) nbUnsat[occList[toInt(~l)][j]]--;
      currentValue[var(l)] = l_Undef;
    }

  popPreviousClauseSet();
}// unassignValue


/**
   Remove a value from a vector.
 */
void DynamicOccurrenceManager::removeIdxFromOccList(vec<int> &o, int idx)
{
  assert(o.size());

  for(int i = 0 ; i<o.size() ; i++)
    {
      if(o[i] == idx)
        {
          o[i] = o.last();
          o.pop();
          return;
        }
    }
  assert(0);
}// removeIdxFromOccList


/////////////////////////////////////////////////////////////////////////////////
///////////////////////////      DEBUGGING FUNCTION      ////////////////////////
/////////////////////////////////////////////////////////////////////////////////

/**
   Debugging function.
 */
void DynamicOccurrenceManager::debug(Solver &s)
{
  cerr << "We call the DynamicOccurrenceManager debugging function" << endl;

  for(int i = 0 ; i<clauses.size() ; i++)
    {
      int nbU = 0;
      bool isSAT = false;

      vec<Lit> &c = clauses[i];
      for(int j = 0 ; !isSAT && j<c.size() ; j++)
        if(s.value(c[j]) == l_True) isSAT = true;
        else if(s.value(c[j]) == l_False) nbU++;

      if(!isSAT)
        {
          assert(nbUnsat[i] == nbU);

          for(int j = 0 ; j<c.size() ; j++)
            if(s.value(c[j]) == l_Undef)
              {
                vec<int> &occ = occList[toInt(c[j])];
                int isIn = false;
                for(int k = 0 ; k<occ.size() && !isIn ; k++) isIn = occ[k] == i;
                assert(isIn);
              }
        }
    }

  for(int i = 0 ; i<occList.size() ; i++)
    {
      // verify that we do not have twice the same index in the occurrence list.
      for(int j = 0 ; j<occList[i].size() ; j++)
        for(int k = j + 1 ; k<occList[i].size() ; k++)
          assert(occList[i][j] != occList[i][k]);

      // verify that no clause are satisfied
      Lit l = mkLit(i>>1, i&1);

      if(s.value(l) != l_Undef) continue;
      for(int j = 0 ; j<occList[i].size() ; j++)
        {
          bool isSAT = false;
          for(int k = 0 ; k<clauses[occList[i][j]].size() && isSAT ; k++)
            isSAT = s.value(clauses[occList[i][j]][k]) == l_True;

          assert(!isSAT);
        }
    }
}// debug
