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
#include "../manager/CnfOccurrenceManager.hh"

using namespace std;

/**
   Constructor.
 */
CnfOccurrenceManager::CnfOccurrenceManager(int nbClause, int _nbVar, int _maxSizeClause) :
    nbVar(_nbVar), maxSizeClause(_maxSizeClause)
{
  for(int i = 0 ; i<nbVar ; i++)
    {
      inCurrentComponent.push(false);
      currentValue.push(l_Undef);
      idxComponent.push(0);
      tmpVecVar.push();
      occList.push(); occList.push();
      tmpMark.push(false);
    }

  mustUnMark.capacity(nbClause);
  for(int i = 0 ; i<nbClause ; i++)
    {
      markView.push(false);
      nbUnsat.push(0);
      nbSat.push(0);
      watcher.push(lit_Undef);
    }
}// construtor


/**
   Constructor.
 */
CnfOccurrenceManager::CnfOccurrenceManager(vec<vec<Lit> > &_clauses, int _nbVar) : nbVar(_nbVar)
{
  initFormula(_clauses);

  for(int i = 0 ; i<nbVar ; i++)
    {
      currentValue.push(l_Undef);
      idxComponent.push(0);
      tmpVecVar.push();
      occList.push(); occList.push();
      tmpMark.push(false);
      inCurrentComponent.push(false);
    }

  mustUnMark.capacity(clauses.size());

  if(!clauses.size()) return;
  maxSizeClause = clauses[0].size();
  for(int i = 0 ; i<clauses.size() ; i++)
    {
      if(clauses[i].size() > maxSizeClause) maxSizeClause = clauses[i].size();
      markView.push(false);
      nbUnsat.push(0);
      nbSat.push(0);
      watcher.push(clauses[i][0]);
    }
}// construtor


/**
   @param[in] l, the considered literal
   WARNING: varConnected references tmpVecVar. Thus, it is already allocate.
*/
int CnfOccurrenceManager::connectedToLit(Lit l, vec<int> &v, vec<Var> &varComponent, int nbComponent)
{
  int cpt = 0;

  vec<int> &occp = occList[toInt(l)];
  for(int i = 0 ; i<occp.size() ; i++)
    {
      vec<Lit> &c = clauses[occp[i]];
      if(markView[occp[i]]) continue;

      cpt++;
      markView[occp[i]] = true;
      mustUnMark.push_(occp[i]);

      // compute component
      for(int j = 0 ; j<c.size() ; j++)
        {
          if(currentValue[var(c[j])] != l_Undef) continue;

          Var vTmp = var(c[j]);
          if(!v[vTmp])
            {
              varComponent.push_(vTmp);
              v[vTmp] = nbComponent;
            }
        }
    }
  return cpt;
}// connectedToLit


/**
   Look all the formula in order to compute the connected component
   of the formula.

   @param[out] varCo, the different connected components found
   @param[in] setOfVar, the current set of variables
   @param[out] freeVar, the set of variables that are present in setOfVar but not in the problem anymore
   @param[out] notFreeVar, the difference between setOfVar and freeVar

   \return the number of component found
 */
int CnfOccurrenceManager::computeConnectedComponent(vec< vec<Var> > &varCo, vec<Var> &setOfVar,
                                                    vec<Var> &freeVar, vec<Var> &notFreeVar)
{
  freeVar.clear();
  int nbComponent = 0;

  for(int i = 0 ; i<setOfVar.size() ; i++)
    {
      Var v = setOfVar[i];
      if(currentValue[v] != l_Undef || idxComponent[v]) continue;

      // index a new composant
      nbComponent++;
      idxComponent[v] = nbComponent;

      // save the variables of connected component
      tmpVecVar.setSize(0);
      tmpVecVar.push_(v);

      int nbClausesInComponent = 0;
      for(int pos = 0 ; pos < tmpVecVar.size() ; pos++)
        {
          Lit l = mkLit(tmpVecVar[pos], false);
          nbClausesInComponent += connectedToLit(l, idxComponent, tmpVecVar, nbComponent);
          nbClausesInComponent += connectedToLit(~l, idxComponent, tmpVecVar, nbComponent);
        }

      if(tmpVecVar.size() <= 1)
        {
          idxComponent[v] = 0;
          nbComponent--; // it is alone ...
        }
    }

  resetUnMark();

  for(int i = 0 ; i<nbComponent ; i++) varCo.push();
  for(int i = 0 ; i<setOfVar.size() ; i++)
    {
      if(idxComponent[setOfVar[i]])
        {
          assert(nbComponent);
          varCo[idxComponent[setOfVar[i]] - 1].push(setOfVar[i]);
          notFreeVar.push(setOfVar[i]);
        }
      if(!idxComponent[setOfVar[i]] && currentValue[setOfVar[i]] == l_Undef) freeVar.push(setOfVar[i]);

      idxComponent[setOfVar[i]] = 0;
    }

  return nbComponent;
}// computeConnectedComponent


/**
   Test if a given clause is actually satisfied under the current
   interpretation.

   @param[in] idx, the clause index.

   \return true if the clause is satisfied, false otherwise.
 */
inline bool CnfOccurrenceManager::isSatisfiedClause(int idx)
{
  assert(idx < clauses.size());
  return nbSat[idx];
}// isSatisfiedClause


/**
   Test if a given clause is actually satisfied under the current
   interpretation.

   @param[in] idx, the clause index.

   \return true if the clause is satisfied, false otherwise.
 */
inline bool CnfOccurrenceManager::isSatisfiedClause(vec<Lit> &c)
{
  for(int i = 0 ; i<c.size() ; i++)
    {
      if(!litIsAssigned(c[i])) continue;
      if(sign(c[i]) && currentValue[var(c[i])] == l_False) return true;
      if(!sign(c[i]) && currentValue[var(c[i])] == l_True) return true;
    }

  return false;
}// isSatisfiedClause



/**
   Test at the same time if a given clause is actually satisfied under
   the current interpretation and if its set of variables belong to the
   current component that is represented as a boolean map given in
   parameter.

   @param[in] idx, the clause index.
   @param[in] currentComponent, currentComponent[var] is true when var is in the
   current component, false otherwise.

   \return true if the clause is satisfied, false otherwise.
 */
inline bool CnfOccurrenceManager::isNotSatisfiedClauseAndInComponent(int idx, vec<bool> &inCurrentComponent)
{
  if(nbSat[idx]) return false;
  assert(watcher[idx] != lit_Undef);
  assert(!litIsAssigned(watcher[idx]));
  return inCurrentComponent[var(watcher[idx])];
}// isSatisfiedClause



void CnfOccurrenceManager::checkCurrentInterpretation(Solver &s)
{
  for(int i = 0 ; i<s.nVars() ; i++) assert(s.value(i) == currentValue[i]);
}// checkCurrentInterpretation



void CnfOccurrenceManager::getCurrentClauses(vec<int> &idxClauses, vec<bool> &inComponent)
{
  idxClauses.setSize(0);
  for(int i = 0 ; i<currentSize ; i++)
  {
    idxClauses.push(currentIdx[i]);
    assert(!nbSat[currentIdx[i]]);
    assert(isNotSatisfiedClauseAndInComponent(currentIdx[i], inComponent));
  }

  sort(idxClauses);
}// getCurrentclauses


void CnfOccurrenceManager::updateCurrentClauseSet(vec<Var> &component)
{
  for(int i = 0 ; i<component.size() ; i++) inCurrentComponent[component[i]] = true;

  stackSize.push(currentSize);
  int i = 0;
  while(i < currentSize)
  {
    if(isNotSatisfiedClauseAndInComponent(currentIdx[i], inCurrentComponent)) i++;
    else
    {
      currentSize--;
      int tmp = currentIdx[currentSize];
      currentIdx[currentSize] = currentIdx[i];
      currentIdx[i] = tmp;
    }
  }

  for(int i = 0 ; i<currentSize ; i++) assert(!nbSat[currentIdx[i]]);
  for(int i = 0 ; i<component.size() ; i++) inCurrentComponent[component[i]] = false;
}// updatecurrentclauseset


void CnfOccurrenceManager::popPreviousClauseSet()
{
  assert(stackSize.size());
  currentSize = stackSize.last();
  stackSize.pop();
}// poppreviousclauseset
