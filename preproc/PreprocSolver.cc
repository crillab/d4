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
#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"

#include "PreprocSolver.hh"

/**
   Take as input a solver and initialize the occurrence data
   structure.

   @param[in] s, the solver
 */
PreprocSolver::PreprocSolver(Solver &s, vec<bool> &pVar): solver(s)
{
  pVar.copyTo(isProtectedVar);
  initOccList(solver.clauses);
    
  for(int i = 0 ; i<(solver.clauses).size() ; i++)
    {
      Clause &c = getClause(i);
      hashKeyInit.push(hashClause(c));
      stampSubsum.push(0);
      c.markIdx(i);
    }

  s.remove_satisfied = false;
}// PreprocSolver
  

/**
   Initialize the occurrence list with the set of clauses given in
   parameter.

   @param[in] refClauses, the set of reference on clauses
 */
void PreprocSolver::initOccList(vec<CRef> &refClauses)
{
  occurrences.clear();
  for(int i = 0 ; i<solver.nVars() ; i++)
    {
      // for both phases of a variable (use toInt)
      occurrences.push(); 
      occurrences.push();
    }

  for(int i = 0 ; i<refClauses.size() ; i++)
    {
      Clause &c =(solver.ca)[refClauses[i]];

      for(int j = 0 ; j<c.size() ; j++)
        if(isProtectedVar[var(c[j])]) occurrences[toInt(c[j])].push(i);
    }  
}// initOccList


/**
   Search in a given list if an element occurs or not. In the positive
   case this element is removed.
   
   @param[out] setOfIdx, the set of idx where we search
   @param[in] idx, the element we search to remove
 */
void PreprocSolver::searchAndRemoveOcc(vec<int> &setOfIdx, int idx)
{
  int pos = -1;
  for(int j = 0 ; j<setOfIdx.size() && pos == -1 ; j++) if(setOfIdx[j] == idx) pos = j;

  assert(pos != -1);
  setOfIdx[pos] = setOfIdx.last();
  setOfIdx.pop();
}// searchAndRemoveOcc


/**
   Remove a clause and uptdate the occurrences.

   @param[in] idx, the index of the clause
   @param[in] refClauses, the set of clauses
 */
void PreprocSolver::removeClauseOcc(int idx)
{
  vec<CRef> &refClauses = solver.clauses;
  Clause &c = (solver.ca)[refClauses[idx]];
  if(c.attached()) solver.detachClause(refClauses[idx], true);

  for(int i = 0 ; i<c.size() ; i++)
    {
      if(!isProtectedVar[var(c[i])]) continue;
      searchAndRemoveOcc(occurrences[toInt(c[i])], idx);
    }

  hashKeyInit[idx] = 0;
  freePositionInClauses.push(idx);
  c.mark(1); 
  (solver.ca).free(refClauses[idx]);
}// removeClauseOcc


/**
   Add a clause and uptdate the occurrence list.

   @param[in] lits, the clause which is added
   @param[in] refClauses, the set of clauses
 */
void PreprocSolver::addClauseOcc(vec<Lit> &lits)  
{
  vec<CRef> &refClauses = solver.clauses;
  assert(lits.size());
  
  if(lits.size() == 1)
    {
      if(solver.value(lits[0]) == l_False) killUnsat();
      if(solver.value(lits[0]) == l_Undef) solver.uncheckedEnqueue(lits[0]);
      assert(solver.value(lits[0]) == l_True);      
    }
  else
    {            
      int pos = (freePositionInClauses.size()) ? freePositionInClauses.last() : refClauses.size();
      if(pos < refClauses.size()) freePositionInClauses.pop();
      else
        {
          refClauses.push();
          hashKeyInit.push();
          stampSubsum.push(0);
        }

      //  printf("addClauseOcc (%d): ", pos); showListLit(lits);
  
      refClauses[pos] = (solver.ca).alloc(lits, false);
      hashKeyInit[pos] = hashClause((solver.ca)[refClauses[pos]]);
      for(int i = 0 ; i<lits.size() ; i++)
        {
          if(isProtectedVar[var(lits[i])])
            {
              // printf("add %d =< %d\n", readableLit(lits[i]), pos);
              occurrences[toInt(lits[i])].push(pos);
            }
        }

      solver.attachClause(refClauses[pos]);
      solver.ca[refClauses[pos]].markIdx(pos);
    }
}// addClauseOcc


/**
   Remove the learnt clauses.
 */
void PreprocSolver::removeLearnt()
{
  for(int i = 0 ; i<solver.learnts.size() ; i++) solver.removeClause((solver.learnts)[i], true);
  (solver.learnts).clear();  
}// removeLearnt


/**
   Remove the clause that are in the free set.
 */
void PreprocSolver::removeAndCompact()
{
  vec<bool> mustBeRm;
  mustBeRm.initialize((solver.clauses).size(), false);
  for(int i = 0 ; i<freePositionInClauses.size() ; i++) mustBeRm[freePositionInClauses[i]] = true;

  // clean the occurrence list to reconstruct it
  for(int i = 0 ; i<occurrences.size() ; i++) occurrences[i].clear();
  
  int i, j;
  for(i = j = 0 ; i<(solver.clauses).size() ; i++)
    if(!mustBeRm[i])
      {
        Clause &c = solver.ca[(solver.clauses)[i]];
        assert(c.attached());

        for(int k = 0 ; k<c.size() ; k++) occurrences[toInt(c[k])].push(j); 
        (solver.clauses)[j++] = (solver.clauses)[i];
      }
  (solver.clauses).shrink(i - j);  

  freePositionInClauses.clear();  
}// removeAndCompact


/**
   Remove all clauses from the formula for a given literal.

   @param[in] l, the litral we search for removing the clauses where it appears.
 */
void PreprocSolver::removeClauseFromLit(Lit l)
{
  vec<int> idxRm;      
  for(int j = 0 ; j<occurrences[toInt(l)].size() ; j++) idxRm.push(occurrences[toInt(l)][j]);
  for(int j = 0 ; j<occurrences[toInt(~l)].size() ; j++) idxRm.push(occurrences[toInt(~l)][j]);
  
  for(int j = 0 ; j<idxRm.size() ; j++) removeClauseOcc(idxRm[j]);
}// removeClauseFromLit

/**
   Print the occurrence list.
 */
void PreprocSolver::printOccList()
{
  printf("--------------------------\n");
  for(int i = 0 ; i<occurrences.size() ; i++)
    {
      printf("%d => ", readableLit(mkLit(i>>1, i&1)));
      showListInt(occurrences[i]);          
    }
}// printOccList


/**
   Check if the structures are correctly filled
 */
void PreprocSolver::debug()
{
  printf("DEBUG FUNCTION\n");
            
  for(int i = 0 ; i<solver.clauses.size() ; i++)
    {
      Clause &c = solver.ca[solver.clauses[i]];
      printf("debug %d: ", c.attached()); c.showClause();
      if(!c.attached()) continue;

      for(int j = 0 ; j<c.size() ; j++)
        {
          if(!isProtectedVar[var(c[j])]) continue;
              
          vec<int> &v = occurrences[toInt(c[j])];              
          bool in = false;
          for(int k = 0 ; !in && k<v.size() ; k++) in = v[k] == i;
          assert(in);
        }
    }

  for(int i = 0 ; i<solver.nVars() ; i++)
    {
      if(!isProtectedVar[i]) continue;
      Lit l = mkLit(i, false);

      for(int j = 0 ; j<occurrences[toInt(l)].size() ; j++)
        {
          Clause &c = solver.ca[solver.clauses[occurrences[toInt(l)][j]]];
              
          printf("&&& %d: ", readableLit(l)); c.showClause();
          assert(c.attached());

          bool in = false;
          for(int k = 0 ; !in && k<c.size() ; k++) in = c[k] == l;                          
          assert(in);              
        }

      l = ~l;
      for(int j = 0 ; j<occurrences[toInt(l)].size() ; j++)
        {
          Clause &c = solver.ca[solver.clauses[occurrences[toInt(l)][j]]];
          printf("&&& %d: ", readableLit(l)); c.showClause();
          assert(c.attached());

          bool in = false;
          for(int k = 0 ; !in && k<c.size() ; k++) in = c[k] == l;
          assert(in);              
        }

    }
}// debug



