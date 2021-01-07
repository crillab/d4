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

#include "../interfaces/OccurrenceManagerInterface.hh"
#include "../mtl/Vec.hh"
#include "../utils/SolverTypes.hh"

#include "../interfaces/PartitionerInterface.hh"
#include "../heuristics/VarBipartiteGraphPartitioner.hh"

#include "../patoh/patoh.h"
#include "../utils/equiv.hh"

using namespace std;


/**
   Compute cut for the current formula using hmetis.

   @param[in] component, the current set of variable defining the problem
   @param[out] cutSet, the cutting set
 */
void VarBipartiteGraphPartitioner::computePartition(vec<Var> &component, vec<int> &cutSet, vec<int> &cutVar, ScoringMethod *sm)
{
  assert(0);
  
  PaToH_Parameters args;  
  int cut;
  cutVar.clear();

  vec< vec<int> > occMap;
  vec<Var> notPlaced;  
  for(int i = 0 ; i<component.size() ; i++)
    {
      mapVar[component[i]] = i;
      inCurrentComponent[component[i]] = true;
    }
  
  // collect the set of clauses
  vec<int> idxClauses;
  for(int i = 0 ; i<om->getNbClause() ; i++)
    if(om->isNotSatisfiedClauseAndInComponent(i, inCurrentComponent))
      {        
        idxClauses.push(i);
        weightClause[i] = 1;
      }

  // graph initialization  
  int posPins = 0;
  for(int i = 0 ; i<idxClauses.size() ; i++)
    {
      xpins[i] = posPins;
      vec<Lit> &c = om->getClause(idxClauses[i]);
      for(int j = 0 ; j<c.size() ; j++) pins[posPins++] = mapVar[var(c[j])];
    }
  xpins[idxClauses.size()] = posPins;

#if 0
  for(int i = 0 ; i<posPins ; i++) cout << pins[i] << " ";
  cout << endl;
  for(int i = 0 ; i <= idxClauses.size() ; i++) cout << xpins[i] << " ";
  cout << endl;
#endif
      
  // hypergraph partitioner
  if(idxClauses.size() < 200) PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_DEFAULT); 
  else PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_QUALITY); 
  
  args._k = 2;
  args.seed = 0;

  for(int i = 0 ; i<idxClauses.size() ; i++) cwghts[i] = 1;
  PaToH_Alloc(&args, component.size(), idxClauses.size(), 1, cwghts, NULL, xpins, pins);
  PaToH_Part(&args, component.size(), idxClauses.size(), 1, 0, cwghts, NULL, xpins, pins, NULL, partvec, partweights, &cut);

#if 1
  cout << "cut: " << cut << endl;  
  int cpt = 0;
  for(int i = 0 ; i<idxClauses.size() ; i++)
    {
      bool split = false;
      vec<Lit> &c = om->getClause(idxClauses[i]);
      for(int j = 1 ; !split && j<c.size() ; j++)
        split = partvec[mapVar[var(c[j])]] != partvec[mapVar[var(c[0])]];

      if(split)
        {
          for(int j = 0 ; j<c.size() ; j++) cout << readableLit(c[j]) << "(" << partvec[mapVar[var(c[j])]]<< ") ";
          cout << endl;
        }
    }
  cout << "-------------------------------------------" << endl;
  exit(0);
#endif

  for(int i = 0 ; i<component.size() ; i++) inCurrentComponent[component[i]] = false; 
  PaToH_Free();  
}// computePartition


/**
   Remove useless clauses (a clause is useless if I can find
   another one with exactly the same variable).
   
   @param[out] idxClauses, the set of index of clauses we want to purge
*/
void VarBipartiteGraphPartitioner::computeUselessClauses(vec<int> &idxClauses, vec< vec<int> > &occMap)
{
  for(int i = 0 ; i<idxClauses.size() ; i++)
    {
      if(idxClauses[i] == -1) continue;
      vec<Lit> &c = om->getClause(idxClauses[i]);
      for(int j = 0 ; j<c.size() ; j++) markedVar[var(c[j])] = true;          // we mark the clause

      // search a variable in c that minimized the number of occurrences
      Var v = var_Undef;
      int nbAvailableVar = 0;
      for(int j = 0 ; j<c.size() ; j++)
        {
          Var vp = var(c[j]);
          if(!om->varIsAssigned(vp) && !useLessVariable[vp])
            {
              nbAvailableVar++;
              if(v == var_Undef || occMap[mapVar[v]].size() > occMap[mapVar[vp]].size()) v = vp; 
            }
        }
      
      if(v != var_Undef)
        {
          for(int j = 0 ; j<occMap[mapVar[v]].size() ; j++)
            {
              int idxCl = occMap[mapVar[v]][j];                
              if(idxCl == i || idxClauses[idxCl] == -1) continue;
              vec<Lit> &d = om->getClause(idxClauses[idxCl]);

              int cpt = 0;
              for(int k = 0 ; cpt < nbAvailableVar && k<d.size() ; k++)
                if(markedVar[var(d[k])] && !om->litIsAssigned(d[k]) && !useLessVariable[var(d[k])]) cpt++;                        

              if(cpt >= nbAvailableVar) // we keep the largest clause
                {
                  weightClause[idxClauses[idxCl]] += weightClause[idxClauses[i]];
                  idxClauses[i] = -1;
                  break;
                }
            }
        }
          
      for(int j = 0 ; j<c.size() ; j++) markedVar[var(c[j])] = false;         // unmark          
    }
}// computeUselessClauses
