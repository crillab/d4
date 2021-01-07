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
#include "OccurrenceLitElimination.hh"


/**
   Try to remove a maximum number of the occurrences clauses of the
   literal l given in parameter.

   @param[in] l, the literal we want to remove
   @param[in] refClauses, the set of clause 
 */
void OccurrenceLitElimination::run(Lit l)
{
  assert(!(os.getLearnts()).size());
   
  // the clause index
  vec<int> v;
  os.copyOccLitInVec(l, v);
  
  for(int i = 0 ; i<v.size() && os.value(l) == l_Undef ; i++)
    {
      CRef &cr = os.getCRef(v[i]);
      Clause &c = os.getClause(v[i]);
      
      os.newDecisionLevel();
       
      // we create an 'assumption'
      bool isSAT = false;
      for(int j = 0 ; j<c.size() && !isSAT ; j++)
        {
          isSAT = os.value(c[j]) == l_True;
          if(os.value(c[j]) != l_Undef) continue;
          else os.uncheckedEnqueue(c[j] == l ? c[j] : ~c[j]);
        }

      if(isSAT) os.removeClauseOcc(v[i]); // we can remove the (satisfiable) clause
      else if(os.propagate() != CRef_Undef)
        {
          nbRemovedLitOccRm++;
          os.detachClause(cr, true);

          for(int j = 0 ; j<c.size() ; j++) if(c[j] == l){c[j] = c[0]; c[0] = l; break;} // push at the front
                    
          if(c.size() > 2)
            {
              os.searchAndRemoveOccFromLit(c[0], v[i]);
              c[0] = c.last(); c.shrink(1); // remove it              
              os.attachClause(cr);
              os.setHashKeyInit(v[i]);
            }
          else 
            {
              os.cancelUntil(0);
              if(os.value(c[1]) == l_Undef) os.uncheckedEnqueue(c[1]);
              os.removeClauseOcc(v[i]);
            }
        }
      
      os.cancelUntil(0);
    }
}// occurrenceElimination


/**
   Run the occurrence elimination process.
*/
void OccurrenceLitElimination::run()
{
  double currTime = cpuTime(); 
  vec<Lit> orderedLit;

  for(int i = 0 ; i<os.nVars() ; i++)
    {      
      if(os.value(i) != l_Undef) continue;
      orderedLit.push(mkLit(i, false));
      orderedLit.push(mkLit(i, true));
    }

  sort(orderedLit, LitOrderOccurrenceLt(os));

  for(int i = 0 ; i<orderedLit.size() ; i++)
    {
      Lit l = orderedLit[i];
      if(os.value(l) != l_Undef || !os.getNbOccLit(l)) continue;
      run(l);
    }

  os.removeAndCompact();
  timeOccRm += cpuTime() - currTime;
}// run
