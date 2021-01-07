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

#include "InterpretationRefiner.hh"

using namespace std;

/**
   Constructor that take in input a CNF formula as well as a boolean
   vector that identifies the protected variables.

   @param[in] formula, the input CNF formula
   @param[in] pvar, the boolean vector used to identify the protected variables
 */
InterpretationRefiner::InterpretationRefiner(vec<vec<Lit> > &formula, vec<bool> &pvar)
{
  for(int i = 0 ; i<formula.size() ; i++) addClause(formula[i]);
  pvar.copyTo(isProtected);
}// constructor

/**
   Constructor that take in input a boolean
   vector that identifies the protected variables.

   @param[in] pvar, the boolean vector used to identify the protected variables
 */
InterpretationRefiner::InterpretationRefiner(vec<bool> &pvar)
{
  pvar.copyTo(isProtected);
}// constructor

/**
   Add a clause in the CNF formula attribute.

   @param[in] cl, the clause we want to add
 */
void InterpretationRefiner::addClause(vec<Lit> &cl)
{
  nbTrueLit.push();
  satisfiedByAssums.push(false);

  for(int i = 0 ; i<cl.size() ; i++)
    {
      Lit l = cl[i];
      while(((var(l) + 1) << 1) > occurrences.size()) occurrences.push();
      occurrences[toInt(l)].push(cnf.size());
    } 

  cnf.push();
  cl.copyTo(cnf.last());
}// addClause


/**
   Initialize the data structure with the information contained in the
   given model (that means we initialize the vector nbTrueLit).

   @param[in] assums, the current assumption we work with
   @param[in] model, the given model
 */
void InterpretationRefiner::init(vec<Lit> &assums, vec<lbool> &model)
{
  for(int i = 0 ; i<nbTrueLit.size() ; i++) nbTrueLit[i] = 0;

  for(int i = 0 ; i<cnf.size() ; i++)
    {
      vec<Lit> &cl = cnf[i];
      for(int j = 0 ; j<cl.size() ; j++)
	{
	  if(isProtected[var(cl[j])]) continue;
	  if((model[var(cl[j])] ^ sign(cl[j])) == l_True) nbTrueLit[i]++;
	}
    } 

  for(int i = 0 ; i<satisfiedByAssums.size() ; i++) satisfiedByAssums[i] = false;

  for(int i = 0 ; i<assums.size() ; i++) 
    {
      if(isSelector[var(assums[i])] && !sign(assums[i])) satisfiedByAssums[selectorToClauseIdx[var(assums[i])]] = true;
      if(toInt(assums[i]) >= occurrences.size()) continue; // additional variable
      for(int j = 0 ; j<occurrences[toInt(assums[i])].size() ; j++)
	satisfiedByAssums[occurrences[toInt(assums[i])][j]] = true;
    }
}// init


/**
   Transfer the pure literal into the assumption.

   @param[in] assums, the current assumption we work with
   @param[in] model, the given model   
 */
void InterpretationRefiner::transferPureLiteral(vec<Lit> &assums, vec<lbool> &model)
{
  int nbVar = model.size();
  vec<int> nbOccurrence;
  nbOccurrence.initialize(nbVar << 1, 0);

  for(int i = 0 ; i<cnf.size() ; i++)
    {
      if(satisfiedByAssums[i]) continue;

      vec<Lit> &cl = cnf[i];
      for(int j = 0 ; j<cl.size() ; j++) if(!isProtected[var(cl[j])]) nbOccurrence[toInt(cl[j])]++; 
    }

  for(int i = 0 ; i<assums.size() ; i++) marked[var(assums[i])] = true;   
  bool findPureLiteral = false;
  do
    {
      vec<Lit> pureLit;
      for(int i = 0 ; i<nbVar ; i++)
        {
          if(isProtected[i] || marked[i]) continue;
          
          Lit l = mkLit(i, false);
          assert(nbOccurrence[toInt(l)] >= 0 && nbOccurrence[toInt(~l)] >= 0);
          if(!nbOccurrence[toInt(l)] && !nbOccurrence[toInt(~l)]) continue;
          if(!nbOccurrence[toInt(l)]) pureLit.push(~l);
          if(!nbOccurrence[toInt(~l)]) pureLit.push(l);
        }

      
      findPureLiteral = pureLit.size() > 0;
      for(int i = 0 ; i<pureLit.size() ; i++)
        {
          Lit l = pureLit[i];
          assums.push(l);
          marked[var(l)] = true;
          
          for(int j = 0 ; j<occurrences[toInt(l)].size() ; j++)
            {
              if(satisfiedByAssums[occurrences[toInt(l)][j]]) continue;
              
              vec<Lit> &cl = cnf[occurrences[toInt(l)][j]];              
              satisfiedByAssums[occurrences[toInt(l)][j]] = true;

              for(int k = 0 ; k<cl.size() ; k++) if(!isProtected[var(cl[k])]) nbOccurrence[toInt(cl[k])]--; 
            }

          model[var(l)] = sign(l) ? l_False : l_True;
        }
    }
  while(findPureLiteral);
  for(int i = 0 ; i<assums.size() ; i++) marked[var(assums[i])] = false;   
}// transferPureLiteral


/**
   This function take in input an assumption and a complete
   interpretation. It tries to perform flip into the current model to
   produce a new one that increase the number of clauses satisfies by
   variables that are not protected. Even if it possible to flip
   unprotected variabls it is important to ensure that assumption
   literals are always satisfied by the returned model.

   @param[in] assums, the current assumption we work with
   @param[out] model, the model we want to refine
 */
void InterpretationRefiner::refine(vec<Lit> &assums, vec<lbool> &model)
{
  for(int i = 0 ; i<assums.size() ; i++) marked[var(assums[i])] = true;  
  int limit = isProtected.size(), v = 0;  
  while(v != limit)
    {
      if(v == isProtected.size()) v = 0;
      if(v == limit) break;
      
      if(!marked[v] && !isProtected[v] && (v < occurrences.size() >> 1))
	{
	  Lit l = mkLit(v, model[v] != l_True);
	  bool createUnsat = false;
	  for(int i = 0 ; i<occurrences[toInt(l)].size() && !createUnsat ; i++) 
	    {
	      int idx = occurrences[toInt(l)][i];
	      createUnsat = !satisfiedByAssums[idx] && nbTrueLit[idx] == 1;
	    }

	  if(!createUnsat)
	    {
	      bool createAtLeastOneSAT = false;
	      for(int i = 0 ; i<occurrences[toInt(~l)].size() && !createAtLeastOneSAT ; i++) 
		createAtLeastOneSAT = !satisfiedByAssums[occurrences[toInt(~l)][i]] && nbTrueLit[occurrences[toInt(~l)][i]] == 0;

	      if(createAtLeastOneSAT) // we flip the value of v
		{
		  for(int i = 0 ; i<occurrences[toInt(l)].size() ; i++) nbTrueLit[occurrences[toInt(l)][i]]--; 
		  for(int i = 0 ; i<occurrences[toInt(~l)].size() ; i++) nbTrueLit[occurrences[toInt(~l)][i]]++;
		  model[v] = (model[v] == l_True) ? l_False : l_True;
                  limit = v;
		}
	    }
	}      
      
      v++;
    }
  for(int i = 0 ; i<assums.size() ; i++) marked[var(assums[i])] = false;
}// refine


/**
   Initialize the data structure with the information about the
   clauses which contain at least one existential variable.

   @param[in] nbVar, the number of variables in the problem  
   @param[in] idxVec, for all i in idxVec the clause clauses[idx] contains at least existential variable.
   @param[in] selectors, the set of selectors that are added in the cnf from the counter point of view
 */
void InterpretationRefiner::initIdxClausesWithExist(int nbVar, vec<int> &idxVec, vec<Lit> &selectors)
{
  idxVec.copyTo(idxClausesWithExist);
  for(int i = 0 ; i<cnf.size() ; i++) nbExistVariables.push(0);

  for(int i = 0 ; i<idxClausesWithExist.size() ; i++)
    {
      int cpt = 0;
      vec<Lit> &cl = cnf[idxClausesWithExist[i]];
      for(int j = 0 ; j<cl.size() ; j++) if(!isProtected[var(cl[j])]) cpt++;
      nbExistVariables[idxClausesWithExist[i]] = cpt; 
    }

  for(int i = 0 ; i<nbVar ; i++)
    {
      selectorToClauseIdx.push(0);
      isSelector.push(false);
      marked.push(false);
    } 

  for(int i = 0 ; i<selectors.size() ; i++)
    {
      isSelector[var(selectors[i])] = true;
      selectorToClauseIdx[var(selectors[i])] = idxVec[i];
      marked.push(false);
    }


#if 0
  for(int i = 0 ; i<cnf.size() ; i++)
    {
      if(nbExistVariables[i] == 0){cout << "all: "; showListLit(cnf[i]);}
      else if(nbExistVariables[i] == cnf[i].size()) {cout << "nope: "; showListLit(cnf[i]);}
      else
        {
          bool containPure = false;
          for(int j = 0 ; !containPure && j<cnf[i].size() ; j++)
            containPure = !isProtected[var(cnf[i][j])] && occurrences[toInt(~cnf[i][j])].size() == 0;

          if(!containPure) {cout << "mix: "; showListLit(cnf[i]);}
        }
    }
  exit(0);
#endif
}// initIdxClausesWithExist


/**
   Compute a subset of the existantially quatified variables that can
   be considered in the model counting process even if we have to put
   them at the end of the structure.

   @param[out] subsetOfEVar, the candidate variables
 */
void InterpretationRefiner::shouldBeRelaxed(vec<Var> &subsetOfEVar)
{
  vec<int> candidate;
  for(int i = 0 ; i<idxClausesWithExist.size() ; i++)
    {
      int idx = idxClausesWithExist[i];
      if(satisfiedByAssums[idx] || nbTrueLit[idx]) continue;
      candidate.push(idx);
    }
    
  vec<Lit> candidateLit;
  for(int i = 0 ; i<candidate.size() ; i++)
    {
      vec<Lit> &cl = cnf[candidate[i]];
      for(int j = 0 ; j<cl.size() ; j++)
	{
	  if(!isProtected[var(cl[j])] && !marked[var(cl[j])])	
	    {
	      marked[var(cl[j])] = true;
	      candidateLit.push(cl[j]);
	    }
	}
    } 

  vec<int> tmpNbTrueLit;
  nbTrueLit.copyTo(tmpNbTrueLit);
  
  for(int i = 0 ; i<candidateLit.size() ; i++)
    {     
      Lit l = candidateLit[i];
      marked[var(l)] = false; // unmark

      // assigned to false
      bool canBeRelaxed = true;
      for(int j = 0 ; canBeRelaxed && j<occurrences[toInt(l)].size() ; j++) 
        {
          canBeRelaxed = (tmpNbTrueLit[occurrences[toInt(l)][j]]) > 1 || nbExistVariables[occurrences[toInt(l)][j]] == 1;
          
          // cout << tmpNbTrueLit[occurrences[toInt(l)][j]] << ": ";
          // showListLit(cnf[occurrences[toInt(l)][j]]);
        }

      // assigned to true
      for(int j = 0 ; canBeRelaxed && j<occurrences[toInt(~l)].size() ; j++) 
        {
          canBeRelaxed = (tmpNbTrueLit[occurrences[toInt(~l)][j]]) > 1 || nbExistVariables[occurrences[toInt(~l)][j]] == 1;
          
          // cout << tmpNbTrueLit[occurrences[toInt(~l)][j]] << ": ";
          // showListLit(cnf[occurrences[toInt(~l)][j]]);
        }

      if(canBeRelaxed)
        {
          subsetOfEVar.push(var(l));

          // impact on the satisfied clauses
          for(int j = 0 ; j<occurrences[toInt(~l)].size() ; j++) tmpNbTrueLit[occurrences[toInt(~l)][j]]--;
        }
      //  cout << "----> canBeRelaxed : " << canBeRelaxed << endl;
    }
  // exit(0);

  //  showListVar(subsetOfEVar);
  
  // cout << "------------------------------" << endl;
  // exit(0);
}// shouldBeRelaxed
