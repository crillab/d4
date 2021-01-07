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
#include "Vivification.hh"

/**
   Realized the vivication [Piette2008] procedure on a set of clauses
   given in parameter.

   WARNING: this function does not work if the set of clauses is not contiguous.

   [Piette2008] Cédric Piette, Youssef Hamadi, Lakhdar Sais:
   Vivifying Propositional Clausal Formulae. ECAI 2008: 525-529
 */
void Vivification::run()
{
  os.removeLearnt();
  assert(!(os.getLearnts()).size());  
  
  double currTime = cpuTime();  
  for(int i = 0 ; i<os.getNbClause() ; i++)
    {
      CRef &cr = os.getCRef(i);
      Clause& c = os.getClause(i); 
      
      if(!c.attached()) continue;

      os.newDecisionLevel();
      os.detachClause(i, true);      
      os.sortClause(c);

      bool keepClause = true, shorted = false;
      for(int k = 0 ; k<c.size() ; k++)
        {
          if(os.value(c[k]) == l_True){keepClause = false; break;}          
          if(os.value(c[k]) == l_False)
            {
              if(os.litIsMarked(c[k])) os.searchAndRemoveOccFromLit(c[k], i);

              nbRemovedLitVivi++;
              c[k--] = c[c.size() - 1];
              c.shrink(1);
              shorted = true;
              continue;
            }
              
          os.uncheckedEnqueue(~c[k]);
          CRef confl = os.propagate();
              
          if(confl != CRef_Undef){ keepClause = false; break;}
        }
      
      os.cancelUntil(0);
      if(!keepClause || c.size() <= 1)
        {
          nbRemovedClauseVivi++;
          os.removeClauseOcc(i);
        }
      else
        {
          os.attachClause(cr);
          if(shorted) os.setHashKeyInit(i);
        }
      
      if(c.size() == 1 && os.value(c[0]) == l_Undef) os.uncheckedEnqueue(c[0]);

    }

  os.removeAndCompact();
  timeVivi += cpuTime() - currTime;
}// run
