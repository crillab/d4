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
#ifndef MODELCOUNTER_EQUIV
#define MODELCOUNTER_EQUIV

#include "../utils/Solver.hh"

class EquivManager
{
private:
  vec<bool> markedVar;
    
public:

  inline void initEquivManager(int nbVar)
  {
    markedVar.initialize(nbVar, false);
  }


  /**
     Take a conflict reference, compute the last UIP, add a learnt
     clause, backtrack and enqueue the unit literal.
  */
  inline void resolveUnit(Solver &s, CRef confl, Lit l)
  {
    int bt;
    vec<Lit> learnt_clause;
    s.analyzeLastUIP(confl, learnt_clause, bt);
    s.cancelUntil(s.decisionLevel() - 1);
    assert(learnt_clause[0] == ~l);
    s.insertClauseAndPropagate(learnt_clause);
  }// resolveUnit
    
    
  /**
     For a given literal, collect the set of literal obtained by
     unit propagation.
       
     @param[in] l, the literal we want to collect the unit list
     @param[out] listVarPU, the vector where is strore the result
  */
  inline bool interCollectUnit(Solver &s, Lit l, vec<Var> &listVarPU)
  {
    listVarPU.clear();
    int posTrail = (s.trail).size();
    s.newDecisionLevel();
    s.uncheckedEnqueue(l);
    CRef confl = s.propagate();

    if(confl != CRef_Undef) // unit literal
      {
        resolveUnit(s, confl, l);
        return false;
      }
      
    for(int j = posTrail + 1; j<s.trail.size() ; j++) listVarPU.push(var(s.trail[j]));      
    s.cancelUntil(s.decisionLevel() - 1);
      
    s.newDecisionLevel();
    s.uncheckedEnqueue(~l);
    confl = s.propagate();

    if(confl != CRef_Undef) // unit literal
      {
        resolveUnit(s, confl, ~l);
        return false;
      }
      
    int i, j;
    for(i = j = 0 ; i<listVarPU.size() ; i++)
      if(s.value(listVarPU[i]) == l_Undef || markedVar[listVarPU[i]]) continue;
      else listVarPU[j++] = listVarPU[i];
      
    listVarPU.shrink(i - j);
    s.cancelUntil(s.decisionLevel() - 1);      
    return true;
  }// collectUnit

    
  /**
     Research equivalence in the set of variable v.
  */
  inline void searchEquiv(Solver &s, vec<Var> &v, vec< vec<Var> > &equivVar)
  {
    vec<Var> reinit;
      
    for(int i = 0 ; i<v.size() ; i++)
      {
        if(s.value(v[i]) != l_Undef || markedVar[v[i]]) continue;
          
        equivVar.push();
        vec<Var> &eql = equivVar.last();
        if(interCollectUnit(s, mkLit(v[i], false), eql))
          {
            for(int j = 0 ; j<(equivVar.last()).size() ; j++)
              if(!markedVar[eql[j]])
                {
                  markedVar[eql[j]] = true;
                  reinit.push(eql[j]);
                }
              
            if(eql.size()) eql.push(v[i]); else equivVar.pop();
          } else equivVar.pop();
      }
    for(int i = 0 ; i<reinit.size() ; i++) markedVar[reinit[i]] = false; 
  }
    
};

#endif
