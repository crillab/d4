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
#include "../utils/ResidualNotPC.hh"


/**
   Update the residual formula.
 */
void ResidualNotPC::updateResidualFormula(vec<Var> &component)
{
  cck->setComponent(component);
  
  for(int i = currentStart ; i<indexes.size() ; i++)
    {
      assert(currentStart <= i);
      vec<Lit> &cl = residue[indexes[i]];
      if(cck->satisfyProperty(cl))
        {          
          int tmp = indexes[i];
          indexes[i] = indexes[currentStart];
          indexes[currentStart] = tmp;
          currentStart++;
        }
    }
  
  cck->unsetComponent(component);
}// updateResidualFormula


/**
   Print the remaining set of clauses that are stored in the residual
   part.
 */
void ResidualNotPC::printRemainingClauses()
{
  cout << "printRemainingClauses " << currentStart << "/" << indexes.size() << endl;
  for(int i = currentStart ; i<indexes.size() ; i++)      
    cck->printClause(residue[indexes[i]]);
}// printRemainingClause


/**
   Extract the residual set of clauses.
 */
void ResidualNotPC::extractRemainingClauses(vec<vec<Lit> > &rcl)
{
  for(int i = currentStart ; i<indexes.size() ; i++)
    {
      rcl.push();
      if(!cck->extractRemainingClause(rcl.last(), residue[indexes[i]]))
	rcl.pop();
    }
}// extractExtractRemainingClauses
