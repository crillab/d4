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
#ifndef CORE_SHARE_STRUCTURE
#define CORE_SHARE_STRUCTURE

struct VarOrderPrior
{
  ScoringMethod *sm;
  bool operator () (Var x, Var y) const { return sm->computeScore(x) > sm->computeScore(y); }
  VarOrderPrior(ScoringMethod *_sm) : sm(_sm) { }
};

inline void prepareVecClauses(vec<vec<Lit> > &clauses, Solver &s)
{
  vec<bool> markedClauseLit;
  for(int i = 0 ; i<s.nVars()<<1 ; i++) markedClauseLit.push(false); 

  int i = 0, pj = 0;
    
  // copy the simplified formula    
  for( ; i<s.clauses.size() ; i++)
    {      
      Clause &c = s.ca[s.clauses[i]];
      if(s.satisfied(c)) continue;

      clauses.push();
      for(int j = 0 ; j<c.size() ; j++)
        if(s.value(c[j]) == l_Undef) (clauses.last()).push(c[j]);
        
      vec<Lit> &cl = clauses.last();
      for(int j = 0 ; j<cl.size() ; j++) markedClauseLit[toInt(cl[j])] = true;        
      
      bool alreadyPresent = false;
      for(int j = 0 ; j<(clauses.size() - 1) && !alreadyPresent ; j++)
        {
          if(cl.size() != clauses[j].size()) continue;
          alreadyPresent = true;
          for(int k = 0 ; alreadyPresent && k<clauses[j].size() ; k++)
            alreadyPresent = markedClauseLit[toInt(clauses[j][k])];
        }

      for(int j = 0 ; j<cl.size() ; j++) markedClauseLit[toInt(cl[j])] = false;
      if(alreadyPresent) clauses.pop();
      s.clauses[pj++] = s.clauses[i];
    }

  s.clauses.shrink(i - pj);
}// prepareVecClauses

#endif
