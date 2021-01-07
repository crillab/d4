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
#ifndef HEURISTICS_CLAUSE_BIPARTITE_GRAPH_PARTITIONER
#define HEURISTICS_CLAUSE_BIPARTITE_GRAPH_PARTITIONER

#include "../utils/SolverTypes.hh"
#include "../utils/equiv.hh"
#include "../interfaces/PartitionerInterface.hh"
#include "../interfaces/OccurrenceManagerInterface.hh"

class ClauseBipartiteGraphPartitioner : public PartitionerInterface
{
  // private:
public:
  Solver &s;
  EquivManager em;
  vec<bool> inCurrentComponent, markedVar, useLessVariable, isInput, markedClauses;
  vec<int> mapVar, weightClause;
  OccurrenceManagerInterface *om;
  vec<int> idxClauses;

  int *xpins, *pins, *cwghts, *vwghts, *partvec, *partweights;
  int sumSize;

  /**
     Compute the sets of clauses and variables which cut the formula
     into two component.

     /!\ We suppose that the variable partVec is set to the current partition.
  */
  inline void extractCutFromClauses(vec< vec<int> > &occMap, vec<Var> &component, vec<int> &cutVar)
  {
    cutVar.clear();
    for(int i = 0 ; i<occMap.size() ; i++)
      {
        vec<int> &c = occMap[i];

        bool split = false;
        for(int j = 1 ; !split && j<c.size() ; j++) split = partvec[c[j]] != partvec[c[0]];

        if(split) cutVar.push(component[i]);
      }
  }// extractCutVarFromClauses

  /**
     Reduce the set of variable present in a cut when we already
     know the variable present.
  */
  inline void reduceCutSet(vec<bool> &present, vec< vec<int> > &occVar, vec<Var> &vars, vec<int> &cutVar)
  {
    assert(om);
    while(vars.size())
      {
        // select variable
        int pos = 0;
        for(int i = 1 ; i<vars.size() ; i++) if(om->getNbClause(vars[i]) < om->getNbClause(vars[pos])) pos = i;
        Var v = vars[pos];
        vars[pos] = vars.last();
        vars.pop();

        int grp = 1 - partvec[mapVar[v]];
        bool cover = true;
        for(int i = 0 ; cover && i<occVar[v].size() ; i++)
          {
            vec<Lit> &c = om->getClause(occVar[v][i]);
            for(int j = 0 ; cover && j<c.size() ; j++)
              cover = partvec[mapVar[var(c[j])]] != grp || present[var(c[j])];
          }

        if(!cover) cutVar.push(v); else present[v] = false;
      }
  }// reduceCutSet


  /**
     Try to reduce the number of variables present in the cut set.
     /!\ We suppose that the variable partVec is set.
  */
  inline void refineCutSet(vec<int> &cutSet, vec<int> &cutVar)
  {
    vec< vec<int> > occVar;
    for(int i = 0 ; i<cutSet.size() ; i++)
      {
        vec<Lit> &c = om->getClause(cutSet[i]);
        for(int j = 0 ; j<c.size() ; j++)
          {
            while(occVar.size() <= var(c[j])) occVar.push();
            occVar[var(c[j])].push(cutSet[i]);
          }
      }

    vec<bool> present;
    present.initialize(om->getNbVariable(), false);

    vec<Var> inVar, outVar;
    for(int i = 0 ; i<cutVar.size() ; i++)
      {
        present[cutVar[i]] = true;
        if(isInput[cutVar[i]]) inVar.push(cutVar[i]); else outVar.push(cutVar[i]);
      }

    cutVar.clear();
    reduceCutSet(present, occVar, outVar, cutVar);
    reduceCutSet(present, occVar, inVar, cutVar);
  }// refineCutSet


  /**
     Take a set of index of clause and build the occurrence map.

     @param[out] occMap, the occurrence map
     @param[in] idxClauses, the set of index of clauses
  */
  inline void buildOccMap(vec< vec<int> > &occMap, vec<int> &idxClauses)
  {
    for(int i = 0 ; i<occMap.size() ; i++) occMap[i].clear();
    for(int i = 0 ; i<idxClauses.size() ; i++)
    {
      vec<Lit> &c = om->getClause(idxClauses[i]);
      for(int j = 0 ; j<c.size() ; j++)
        if(!om->litIsAssigned(c[j]) && !useLessVariable[var(c[j])]) occMap[mapVar[var(c[j])]].push(i);
    }
  }// buildOccMap


  /**
     Remove useless variables and ajust the occMap, mapVar and save
     the new set of variables.

     @param[in] component, the initial set of variables
     @param[out] occMap, the occurrence map
     @param[out] useFulVariable, the set of kept variables
  */
  void clearSetOfVariable(vec<Var> &component, vec< vec<int> > &occMap, vec<Var> &useFulVariable)
  {
    occMap.clear();
    for(int i = 0 ; i<component.size() ; i++)
      if(!om->varIsAssigned(component[i]) && !useLessVariable[component[i]])
        {
          mapVar[component[i]] = useFulVariable.size();
          useFulVariable.push(component[i]);
          occMap.push();
        }
  }// clearSetOfVariable


  /**
     Search if some variable can be drop from the graph.

     @param[in] component, the set of variables
     @param[in] occMap, occurrence lists
     @param[in] idxClauses, correspondance between the occurrence lists and the clauses
  */
  inline void computeUselessVariables(vec<Var> &component, vec< vec<int> > &occMap, vec<int> &idxClauses)
  {
    for(int i = 0 ; i<component.size() ; i++)
      {
        if(!occMap[i].size()) continue;

        // init with the first clause
        int nbLit = 0;
        vec<Lit> &c = om->getClause(idxClauses[occMap[i][0]]);
        for(int j = 0 ; j<c.size() ; j++) if(!om->litIsAssigned(c[j])){markedVar[var(c[j])] = true; nbLit++;}

        bool notUseFul = true;
        int cpt = 0;
        for(int j = 1 ; notUseFul && j<occMap[i].size() ; j++)
          {
            vec<Lit> &d = om->getClause(idxClauses[occMap[i][j]]);
            for(int k = 0 ; notUseFul && k<d.size() ; k++)
              if(!om->litIsAssigned(d[k])){notUseFul = markedVar[var(d[k])]; cpt++;}
          }
        if(notUseFul) useLessVariable[component[i]] = true;
        for(int j = 0 ; j<c.size() ; j++) markedVar[var(c[j])] = false;
      }
  }// computeUselessVariables


  inline void adjustStructureWrTClauses()
  {
    if(om->getNbClause() > markedClauses.size())
      {
        while(om->getNbClause() > markedClauses.size()) markedClauses.push(false);
        while(om->getNbClause() > weightClause.size()) weightClause.push(false);

        partvec = (int *) realloc(partvec, (om->getNbClause() + 1) * sizeof(int));
        cwghts = (int *) realloc(cwghts, (om->getNbClause() + 1) * sizeof(int));
        for(int i = 0 ; i<(om->getNbClause() + 1) ; i++) cwghts[i] = 1;
      }

    if(sumSize < om->getSumSizeClauses())
      {
        sumSize = om->getSumSizeClauses();
        pins = (int *) realloc(pins, sumSize * sizeof(int));
      }
  }// adjustStructureWrTClauses

public:
  ClauseBipartiteGraphPartitioner(Solver &_s, OccurrenceManagerInterface *_om): s(_s), om(_om)
  {
    assert(om);
    em.initEquivManager(om->getNbVariable());
    inCurrentComponent.initialize(om->getNbVariable(), false);
    mapVar.initialize(om->getNbVariable(), 0);
    markedVar.initialize(om->getNbVariable(), false);
    useLessVariable.initialize(om->getNbVariable(), false);
    xpins = (int *) malloc((om->getNbVariable() + 1) * sizeof(int));
    vwghts = (int *) malloc((om->getNbVariable() + 1) * sizeof(int));

    markedClauses.initialize(om->getNbClause(), false);
    weightClause.initialize(om->getNbClause(), 1);
    partvec = (int *) malloc((om->getNbClause() + 1) * sizeof(int));
    cwghts = (int *) malloc((om->getNbClause() + 1) * sizeof(int));
    for(int i = 0 ; i<(om->getNbClause() + 1) ; i++) cwghts[i] = 1;

    sumSize = om->getSumSizeClauses();
    pins = (int *) malloc(sumSize * sizeof(int));

    partweights = (int *) malloc(2 * sizeof(int));
    reduceFormula = equivSimp = false;
  }// constructor

  ~ClauseBipartiteGraphPartitioner()
           {free(xpins); free(pins); free(cwghts); free(vwghts); free(partvec); free(partweights);}
  void computeUselessClauses(vec<int> &idxClauses, vec< vec<int> > &occMap);
  void computePartition(vec<Var> &component, vec<int> &cutSet, vec<int> &cutVar, ScoringMethod *sm);
};



#endif
