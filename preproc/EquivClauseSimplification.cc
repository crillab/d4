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

#include "EquivClauseSimplification.hh"

using namespace std;

/**
   Initialize the solver with the good number of variables.

   @param[in] nbVar
 */
EquivClauseSimplification::EquivClauseSimplification(int nbVar)
{
  while(s.nVars() < nbVar)
    {
      s.newVar();
      occurrences.push();
      occurrences.push();
    }
}// EquivClauseSimplification


/**
   Put a set of clauses given as an input in the solver.

   @param[in] clauses, the set of clauses we want to add in the solver.
 */
void EquivClauseSimplification::loadSolver(vec< vec<Lit> > &clauses)
{
  for(int i = 0 ; i<clauses.size() ; i++)
    {
      bool canAdd = s.addClause_(clauses[i]);
      assert(canAdd);      
    }
}// loadSolver


/**
   Clean the solver by removing all the clause and unit literals.
 */
void EquivClauseSimplification::cleanUpSolver()
{
  // remove the trail
  for (int c = s.trail.size()-1; c >= 0; c--)
    {
      Var x = var(s.trail[c]);      
      s.assigns[x] = l_Undef;      
      s.insertVarOrder(x);
    }
  s.qhead = 0;
  s.trail.clear();
  s.trail_lim.clear();
  
  // remove init clauses
  for(int i = 0 ; i<s.clauses.size() ; i++) s.removeClause(s.clauses[i], true);
  s.clauses.clear();

  // remove learnt clauses
  for(int i = 0 ; i<s.learnts.size() ; i++) s.removeClause(s.learnts[i], true);
  s.learnts.clear();
}// cleanUpSolver


/**
   This function applies vivification on the clauses of the solver.
*/
void EquivClauseSimplification::vivification()
{
  int i, j;  
  int nbRemoveLit = 0;
  int nbRemoveClause = 0;  
  
  for (i = j = 0; i < s.clauses.size(); i++)
    {      
      Clause& c = s.ca[s.clauses[i]];
      s.newDecisionLevel();
      s.detachClause(s.clauses[i], true);
      
      vec<Lit> tmp;
      for(int k = 0 ; k<c.size() ; k++) tmp.push(c[k]);          
      sort(tmp, LitOrderOccurrenceLt(occurrences));
      
      for(int k = 0 ; k<c.size() ; k++) c[k] = tmp[k];          

      bool keepClause = true;
      for(int k = 0 ; k<c.size() ; k++)
        {
          if(s.value(c[k]) == l_True){keepClause = false; break;}          
          if(s.value(c[k]) == l_False)
            {
              nbRemoveLit++;
              c[k--] = c[c.size() - 1];
              c.shrink(1);            
              continue;
            }
              
          s.uncheckedEnqueue(~c[k]);
          CRef confl = s.propagate();              
          if(confl != CRef_Undef){ keepClause = false; break;}
        }
          
      s.cancelUntil(0);
      if(keepClause)
        {          
          if(c.size() > 1)
            {
              s.attachClause(s.clauses[i]);
              s.clauses[j++] = s.clauses[i];
            }
          else
            {
              s.removeNotAttachedClause(s.clauses[i]);
              s.uncheckedEnqueue(c[0]);
            }
        }else
        {
          nbRemoveLit += tmp.size();
          s.removeNotAttachedClause(s.clauses[i]);
          nbRemoveClause++;          
        }
    }
  s.clauses.shrink(i - j);
  s.checkGarbage();

  cerr << "clauses(" << nbRemoveClause << ") -- literals(" << nbRemoveLit << ")" << endl;
}// vivification



/**
   Remove a clause and update the occurrences of this one.
   @param[in] index, the clause index
 */
void EquivClauseSimplification::removeAndUpdateOccurrence(int index, Lit blocked, bool clauseAttached)
{
  if(clauseAttached) s.detachClause(s.clauses[index]);
  Clause &rm = s.ca[s.clauses[index]];

  for(int i = 0 ; i<rm.size() ; i++)
    {
      vec<int> &v = occurrences[toInt(rm[i])];
      int pos = -1;
      for(int j = 0 ; j<v.size() && pos == -1 ; j++) pos = (v[j] == index) ? j : -1;
      assert(pos != -1);
      if(rm[i] == blocked) v[pos] = -1;
      else
        {
          v[pos] = v.last(); 
          v.pop();          
        }
    }

  rm.mark(1); 
  s.ca.free(s.clauses[index]);
      
  if(index < (s.clauses.size() - 1))
    {
      Clause &mv = s.ca[s.clauses.last()];
      for(int i = 0 ; i<mv.size() ; i++)
        {
          vec<int> &v = occurrences[toInt(mv[i])];
          int pos = -1;
          for(int j = 0 ; j<v.size() && pos == -1 ; j++) pos = (v[j] == (s.clauses.size() - 1)) ? j : -1;
          assert(pos != -1);
          if(mv[i] == blocked)
            {
              v[pos] = -1;         
              v.push(index);
            }
          else v[pos] = index;         
        }
          
      s.clauses[index] = s.clauses.last();
      s.clauses.pop();
    }else s.clauses.pop();    
}// removeAndUpdateOccurrence

/**
   Try to remove one literal by vivification.   
*/
int EquivClauseSimplification::tryToRemoveLiteral(Lit l)
{
  int nbLiteralEliminated = 0;

  // look if we can remove l
  vec<int> &v = occurrences[toInt(l)];
  for(int j = 0 ; j<v.size() && s.value(l) == l_Undef ; j++)
    {
      if(v[j] == -1){v[j--] = v.last(); v.pop(); continue;}

      Clause &c = s.ca[s.clauses[v[j]]];
      s.newDecisionLevel();
       
      // creating the 'assumption'
      for(int k = 0 ; k<c.size() ; k++) 
        if(s.value(c[k]) != l_Undef) continue;
        else s.uncheckedEnqueue(c[k] == l ? c[k] : ~c[k]);    
         
      if(s.propagate() != CRef_Undef)
        {
          s.detachClause(s.clauses[v[j]], true);
          for(int k = 0 ; k<c.size() ; k++) if(c[k] == l){c[k] = c[0]; c[0] = l; break;}
                    
          if(c.size() > 2)
            {             
              c[0] = c.last(); c.shrink(1); // remove it         
              s.attachClause(s.clauses[v[j]]);
              v[j--] = v.last(); v.pop();
            }
          else 
            {
              s.cancelUntil(0);
              nbLiteralEliminated++;
              if(s.value(c[1]) == l_Undef) s.uncheckedEnqueue(c[1]);
              removeAndUpdateOccurrence(v[j], l);
              nbLiteralEliminated++;
            }
          nbLiteralEliminated++;
        }
      s.cancelUntil(0);
    }

  for(int j = 0 ; j<v.size() ; j++) if(v[j] == -1){v[j--] = v.last(); v.pop();}
  return nbLiteralEliminated;
}// tryToRemoveLiteral


/**
   This function applies occurrence elimination on the clauses of the
   solver.
 */
void EquivClauseSimplification::occurrenceElimination()
{
  rebuildOccurrence();
  
  vec<Lit> orderedLit;
  int nbElim = 0;  
  
  for(int i = 0 ; i<s.nVars() ; i++) 
    {
      Lit l = mkLit(i, false);
      if(occurrences[toInt(l)].size() == 0 && occurrences[toInt(~l)].size() == 0) continue;      
      
      if(s.value(i) == l_Undef)
        {
          orderedLit.push(~l);
          orderedLit.push(l);
        }
    }
  sort(orderedLit, LitOrderOccurrenceLt(occurrences));
   
  for(int i = 0 ; i<orderedLit.size() ; i++)
    {      
      Lit l = orderedLit[i];
      if(s.value(l) != l_Undef) continue;
      nbElim += tryToRemoveLiteral(l);
    }

  // remove detached clauses
  int i, j;
  for(i = j = 0 ; i<s.clauses.size() ; i++)
    if(s.ca[s.clauses[i]].attached()) s.clauses[j++] = s.clauses[i];
  s.clauses.shrink(i - j);  
  s.checkGarbage();
  
  cerr << "occurrence elimination (" << nbElim << ")" << endl;
}// occurrenceElimination


/**
   Construct the occurrence list.
 */
void EquivClauseSimplification::rebuildOccurrence()
{
  for(int i = 0 ; i<occurrences.size() ; i++) occurrences[i].clear();
  for(int i = 0 ; i<s.clauses.size() ; i++)
    {
      Clause &c = s.ca[s.clauses[i]];
      for(int j = 0 ; j<c.size() ; j++) occurrences[toInt(c[j])].push(i); 
    }   
}// rebuildOccurrence
                     

/**
   Call the preproc on a given formula.
   That means, vivification and occurrence elimination w.r.t. the options.

   @param[out] clauses, the set init clauses (the result is put in this set)
   @param[in] optV, true if we must performed the vivification
   @param[in] optO, true if we must performed occurrence elimination
 */
void EquivClauseSimplification::equivPreproc(vec< vec<Lit> > &clauses, bool optV, bool optO)
{
  if(!optO && !optV) return;
  
  cleanUpSolver();
  assert(s.clauses.size() == 0 && s.learnts.size() == 0 && s.trail.size() == 0);

  loadSolver(clauses); 
  if(optO) occurrenceElimination();
  if(optV) vivification();

  clauses.clear();
  for(int i = 0 ; i<s.clauses.size() ; i++)
    {
      Clause &c = s.ca[s.clauses[i]];
      clauses.push();
      for(int j = 0 ; j<c.size() ; j++) (clauses.last()).push(c[j]); 
    }

  // the unit literals
  for(int i = 0 ; i<s.trail.size() ; i++)
    {
      clauses.push();
      clauses.last().push(s.trail[i]);
    }
}// equivPreproc
