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
#include "../utils/HittingSet.hh"

using namespace std;

/**
   Constructor.

   @param[in] nbVar, the number of variables in the problem
 */
HittingSet::HittingSet(int nbVar, ScoringMethod *_sm)
{
  sm = _sm;
  
  for(int i = 0 ; i<nbVar ; i++)
    {
      selectedLit.push(false);
      selectedLit.push(false);
      occ.push();
      occ.push();
    }
}// HittingSet


/**
   Compute a hitting set on a given set of clauses represented as set
   of literals.

   @param[in] cls, the set of clauses
   @param[out] hs, the hitting computed
 */
void HittingSet::computeHittingSet(vec<vec<Lit> > &cls, vec<Lit> &hs)
{
  // construct the occurence list
  vec<Lit> occSet;
  for(int i = 0 ; i<cls.size() ; i++)
    {
      vec<Lit> &cl = cls[i];
      for(int j = 0 ; j<cl.size() ; j++)
	{
	  Lit l = cl[j];
	  if(!selectedLit[toInt(l)])
	    {
	      selectedLit[toInt(l)] = true;
	      occSet.push(l);		
	    }
	  occ[toInt(l)].push(i);
	}
    }
  for(int i = 0 ; i<occSet.size() ; i++) selectedLit[toInt(occSet[i])] = false; 
  
  // select a literal in each clause
  for(int i = 0 ; i<cls.size() ; i++)
    {
      vec<Lit> &cl = cls[i];

      // take the best literal w.r.t. vs
      Lit best = cl[0];
      double bestScore = sm->computeScore(var(best));
      for(int j = 1 ; j<cl.size() ; j++)
	if(sm->computeScore(var(cl[j])) > bestScore)
	  {
	    best = cl[j];
	    bestScore = sm->computeScore(var(best));	    
	  }

      if(!selectedLit[toInt(best)])
	{
	  hs.push(best);
	  selectedLit[toInt(best)] = true;
	}
    }

  // remove useless literals from the hitting set
  sort(hs, LitOrderScoringMethod(sm));  

  vec<int> nbSelected;
  for(int i = 0 ; i<cls.size() ; i++)
    {
      int cpt = 0;
      vec<Lit> &cl = cls[i];
      for(int j = 0 ; j<cl.size() ; j++)
	if(selectedLit[toInt(cl[j])]) cpt++;
      nbSelected.push(cpt);
    }
  
  for(int i = hs.size() - 1 ; i >= 0 ; i--)
    {
      bool mustKept = false;
      Lit l = hs[i];
      vec<int> &ol = occ[toInt(l)]; 
      
      for(int j = 0 ; !mustKept && j<ol.size() ; j++) mustKept = nbSelected[ol[j]] == 1; 
      
      if(!mustKept)
	{
	  hs[i] = hs.last();
	  hs.pop();
	  selectedLit[toInt(l)] = false;

	  for(int j = 0 ; j<ol.size() ; j++)
	    nbSelected[ol[j]]--;
	}
    }
  
  for(int i = 0 ; i<occSet.size() ; i++) occ[toInt(occSet[i])].clear();
  for(int i = 0 ; i<hs.size() ; i++) selectedLit[toInt(hs[i])] = false;

  // debug(cls, hs);
}// computeHittingSet


void HittingSet::debug(vec<vec<Lit> > &cls, vec<Lit> &hs)
{
  for(int i = 0 ; i<selectedLit.size() ; i++) assert(!selectedLit[i]);
  for(int i = 0 ; i<occ.size() ; i++) assert(!occ[i].size());

  for(int i = 0 ; i<cls.size() ; i++)
    {
      vec<Lit> &cl = cls[i];
      bool inHs = false;
      for(int j = 0 ; j<cl.size() && !inHs; j++)
	for(int k = 0 ; k<hs.size() && !inHs ; k++) inHs = hs[k] == cl[j];
      assert(inHs);
    }
}// debug
