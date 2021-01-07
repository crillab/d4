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
#include "../heuristics/ClauseBipartiteGraphPartitioner.hh"

#include "../patoh/patoh.h"
#include "../utils/equiv.hh"

using namespace std;


/**
   Compute cut for the current formula.

   @param[in] component, the current set of variable defining the problem
   @param[out] cutSet, the cutting set
 */
void ClauseBipartiteGraphPartitioner::computePartition(vec<Var> &component, vec<int> &cutSet,
                                                       vec<int> &cutVar, ScoringMethod *sm)
{
  adjustStructureWrTClauses();
  int cut;
  cutVar.clear();

  vec< vec<int> > occMap;
  vec<Var> notPlaced;
  for(int i = 0 ; i<component.size() ; i++)
  {
    mapVar[component[i]] = i;
    inCurrentComponent[component[i]] = true;
    occMap.push();
  }

  vec<Lit> unitEquiv;
  vec< vec<Var> > equivVar;
  if(equivSimp)
  {
    em.searchEquiv(s, component, equivVar);
    for(int i = 0 ; i<s.trail.size() ; i++)
      if(!om->litIsAssigned(s.trail[i]) && om->getNbVariable() > var(s.trail[i]))
        unitEquiv.push(s.trail[i]);
  }
  om->preUpdate(unitEquiv);

  // collect the set of clauses
  om->getCurrentClauses(idxClauses, inCurrentComponent);
  for(int i = 0 ; i<idxClauses.size() ; i++) weightClause[idxClauses[i]] = 1;
  buildOccMap(occMap, idxClauses);

  vec<Var> vUse;
  if(reduceFormula)
  {
    computeUselessVariables(component, occMap, idxClauses);
    computeUselessClauses(idxClauses, occMap);
    clearSetOfVariable(component, occMap, vUse);

    int i, j;
    for(i = j = 0 ; i<idxClauses.size() ; i++) if(idxClauses[i] != -1) idxClauses[j++] = idxClauses[i];

    idxClauses.shrink(i - j);
    buildOccMap(occMap, idxClauses);
  } else component.copyTo(vUse);

  if(equivSimp)
  {
    for(int i = 0 ; i<equivVar.size() ; i++)
    {
      assert(equivVar[i].size());
      Var v = equivVar[i].last();
      if(!inCurrentComponent[v] || useLessVariable[v]) continue;
      for(int j = 0 ; j<occMap[mapVar[v]].size() ; j++) markedClauses[occMap[mapVar[v]][j]] = true;

      vec<Var> &eq = equivVar[i];
      for(int j = 0 ; j<eq.size() - 1 ; j++)
      {
        if(!inCurrentComponent[eq[j]] || useLessVariable[eq[j]]) continue;
        for(int k = 0 ; k<occMap[mapVar[eq[j]]].size() ; k++)
        {
          if(!markedClauses[occMap[mapVar[eq[j]]][k]])
          {
            occMap[mapVar[v]].push(occMap[mapVar[eq[j]]][k]);
            markedClauses[occMap[mapVar[eq[j]]][k]] = true;
          }
        }
        useLessVariable[eq[j]] = true;
      }

      for(int j = 0 ; j<occMap[mapVar[v]].size() ; j++) markedClauses[occMap[mapVar[v]][j]] = false;
    }

    // remove useless variables
    int i, j;
    for(i = j = 0 ; i<vUse.size() ; i++)
      if(!useLessVariable[vUse[i]])
      {
        if(i != j)
        {
          vec<int> tmp;
          occMap[i].copyTo(tmp);
          tmp.copyTo(occMap[j]);
        }
        mapVar[j] = vUse[i];
        vUse[j++] = vUse[i];
      }
    vUse.shrink(i - j);
    occMap.shrink(i - j);
  }

  // graph initialization
  int posPins = 0;
  for(int i = 0 ; i<vUse.size() ; i++)
  {
    xpins[i] = posPins;
    for(int j = 0 ; j<occMap[i].size() ; j++) pins[posPins++] = occMap[i][j];
  }
  xpins[vUse.size()] = posPins;

  // hypergraph partitioner
  PaToH_Parameters args;
  if(vUse.size() < 200) PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_DEFAULT);
  else PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_QUALITY);

  args._k = 2;
  args.seed = 1;

  for(int i = 0 ; i<idxClauses.size() ; i++) cwghts[i] = weightClause[idxClauses[i]];
  PaToH_Alloc(&args, idxClauses.size(), vUse.size(), 1, cwghts, NULL, xpins, pins);
  PaToH_Part(&args, idxClauses.size(), vUse.size(), 1, 0, cwghts, NULL, xpins, pins,
             NULL, partvec, partweights, &cut);

  extractCutFromClauses(occMap, vUse, cutVar);
  for(int i = 0 ; i<component.size() ; i++)
    useLessVariable[component[i]] = inCurrentComponent[component[i]] = false;
  PaToH_Free();

  for(int i = 0 ; i<equivVar.size() ; i++)
  {
    Var v = equivVar[i].last();
    bool isInCut = false;
    for(int j = 0 ; j<cutVar.size() && !isInCut ; j++) isInCut = v == cutVar[j];
    if(!isInCut) continue;
    for(int j = 0 ; j<equivVar[i].size() - 1 ; j++)
      if(inCurrentComponent[equivVar[i][j]]) cutVar.push(equivVar[i][j]);
  }

  om->postUpdate(unitEquiv);
}// computePartition


/**
   Remove useless clauses (a clause is useless if I can find
   another one with exactly the same variable).

   @param[out] idxClauses, the set of index of clauses we want to purge
*/
void ClauseBipartiteGraphPartitioner::computeUselessClauses(vec<int> &idxClauses, vec< vec<int> > &occMap)
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
