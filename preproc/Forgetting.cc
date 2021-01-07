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
#include "Forgetting.hh"

using namespace std;

/**
   Constructor.

   @param[in] s, the solver (that means the problem)
 */
Forgetting::Forgetting(PreprocSolver &_os) : os(_os), occElim(os), vivifier(os)
{
  markedLit.initialize(os.getNbVar() << 1, false);
  nbIteration = nbForget = timeForget = stamp = 0;
  
  for(int i = 0 ; i<os.getNbVar() ; i++)
    {
      markedAsForget.push(false);
      watchNewCls.push();
      watchNewCls.push();
    }
}// constructor


/**
   Construct the list of watch for a set of new clauses (vec< vec<Lit> >).

   @param[out] watchNewCls, the list of watch on the new clauses
   @param[out] newCls, the new clauses
 */
void Forgetting::constructWatchList(vec< vec<Lit> > &newCls)
{
  for(int i = 0 ; i<watchNewCls.size() ; i++) watchNewCls[i].setSize(0);
  
  for(int i = 0 ; i<newCls.size() ; i++)
    {
      // we want to watch the first literal (but we need to change to avoid collisions)
      int pos = i % newCls[i].size();
      Lit tmp = newCls[i][pos];
      newCls[i][pos] = newCls[i][0];
      newCls[i][0] = tmp;
      
      watchNewCls[toInt(tmp)].push(i);
    }
}// constructWatchList


/**
   Remove the subsumed clauses.

   @param[out] newCls, the new clauses which contains only non
   subsubmed clauses at the end of the process
 */
void Forgetting::removeSubsum(vec< vec<Lit> > &newCls)
{
  // create the hash tables
  vec<uint64_t> hashKeyNew; 
  for(int i = 0 ; i<newCls.size() ; i++) hashKeyNew.push(os.hashSetOfLit(newCls[i]));
  
  constructWatchList(newCls); // Construct a watch list for the clauses given in parameter
 
  for(int i = 0 ; i<newCls.size() ; i++)
    {
      stamp++;
      vec<Lit> &cnew = newCls[i];
      for(int j = 0 ; j<cnew.size() ; j++) markedLit[toInt(cnew[j])] = true; // mark

      bool isSubSum = false;
      for(int j = 0 ; j<cnew.size() && !isSubSum ; j++)
        {
          // look for the watches for the new clauses
          vec<int> &idxs = watchNewCls[toInt(cnew[j])];
          for(int k = 0 ; k<idxs.size() && !isSubSum ; k++)
            {
              if(idxs[k] == i || (hashKeyNew[i] & hashKeyNew[idxs[k]]) != hashKeyNew[idxs[k]]) continue;
              vec<Lit> &cl = newCls[idxs[k]];
              
              if(cl.size() > cnew.size()) continue;              
              
              isSubSum = true;
              for(int l = 0 ; l<cl.size() && isSubSum ; l++) isSubSum = markedLit[toInt(cl[l])];
            }
          
          // look for the watches for the init clauses
          vec<Watcher> &ws = os.getWatcher(~cnew[j]);
          for(int k = 0 ; k<ws.size() && !isSubSum ; k++)
            {
              Clause &cl = os.getClause(ws[k].cref);
              if(stampSubsum[cl.markIdx()] == stamp) continue; else stampSubsum[cl.markIdx()] = stamp;
              if((hashKeyNew[i] & hashKeyInit[cl.markIdx()]) != hashKeyInit[cl.markIdx()]) continue;

              if(cl.size() > cnew.size()) continue;              
              isSubSum = true;
              for(int l = 0 ; l<cl.size() && isSubSum ; l++) isSubSum = markedLit[toInt(cl[l])];
            }
        }
      
      for(int j = 0 ; j<cnew.size() ; j++) markedLit[toInt(cnew[j])] = false; // unmark
      
      if(isSubSum) // this clause is useless
        {
          watchNewCls[toInt(cnew[0])].removeElt(i);
          cnew.clear();
        }
    }

  int i, j;
  for(i = j = 0 ; i<newCls.size() ; i++) if(newCls[i].size()){if(i != j) newCls[i].copyTo(newCls[j++]); else j++;}
  newCls.shrink(i - j);
}// removeSubsum


/**
   From a given variable, the set of resolution obtained from a set of
   clauses is generated and returned.

   @param[in] v, the variable which is used as pivot
   @param[out] cls, the result of the operation
   @param[in] refClauses, the set of clauses

   \return false if the the number of clauses generated is greater
   than limitNbRes, true otherwise
 */
void Forgetting::generateAllResolution(Var v, vec< vec<Lit> > &cls)
{
  for(int i = 0 ; i<markedLit.size() ; i++) assert(!markedLit[i]); 
  
  Lit lPos = mkLit(v, false), lNeg = ~lPos;
  
  vec<int> &idxPosLit = os.getOccurrenceLit(lPos);
  vec<int> &idxNegLit = os.getOccurrenceLit(lNeg);
  
  for(int i = 0 ; i<idxPosLit.size() ; i++)
    {
      Clause &cp = os.getClause(idxPosLit[i]);
      
      vec<Lit> base;
      for(int j = 0 ; j<cp.size() ; j++)
        {
          markedLit[toInt(cp[j])] = true;
          if(cp[j] != lPos && os.value(cp[j]) != l_False) base.push(cp[j]);
        }
      
      for(int j = 0 ; j<idxNegLit.size() ; j++)
        {
          vec<Lit> currCl;
          bool taut = false;
          
          Clause &cn = os.getClause(idxNegLit[j]);
          base.copyTo(currCl);

          // resolution
          for(int k = 0 ; k<cn.size() && ! taut ; k++)
            {
              if(cn[k] == lNeg || os.value(cn[k]) == l_False) continue;
              taut = markedLit[toInt(~cn[k])] || os.value(cn[k]) == l_True;
              
              if(!markedLit[toInt(cn[k])]) currCl.push(cn[k]);
            }
          
          if(!taut)
            {
              cls.push();
              currCl.copyTo(cls.last());
            }
        }
      
      for(int j = 0 ; j<cp.size() ; j++) markedLit[toInt(cp[j])] = false;
    }
}// generateAllResolution


/**
   Realize the forgetting on a set of variables given in parameter.

   @param[in] outputVars, the set of variables we want to forget
   @param[out] forgetVars, the set of variables forget
   
   \return the number of variable removed during all the process
 */
void Forgetting::run(vec<Var> &outputVars, vec<Var> &forgetVars, int lim_occ)
{
  double initTime = cpuTime();
  bool forgetApplied = true;
  
  os.removeLearnt();
  vivifier.run();     
    
  while(forgetApplied)
    {
      nbIteration++;
      forgetApplied = false;
      vec<Var> notAlreadyForget;
      
      // apply the forgetting on all the variables of outputVars
      while(outputVars.size())
        {
          Var varSelected = selectVarAndPop(outputVars);
          if(os.value(varSelected) != l_Undef) continue;

          Lit lp = mkLit(varSelected, false);
          occElim.run(lp);
          occElim.run(~lp);
          
          if(os.productOccLit(lp) > lim_occ){notAlreadyForget.push(varSelected); continue;}
          
          vec< vec<Lit> > genResolution;
          generateAllResolution(varSelected, genResolution);
          removeSubsum(genResolution);
      
          if(genResolution.size() <= os.sumOccLit(lp))
            {
              markedAsForget[varSelected] = true;
              forgetVars.push(varSelected);              
              
              forgetApplied = true;
              os.removeClauseFromLit(lp);                                                         // remove the old clauses
              for(int j = 0 ; j<genResolution.size() ; j++) os.addClauseOcc(genResolution[j]);    // add the new one
            } else notAlreadyForget.push(varSelected);
        }
      
      vivifier.run();
      notAlreadyForget.copyTo(outputVars);
    }
  
  os.removeAndCompact();// shrink
  timeForget += cpuTime() - initTime;
  nbForget += forgetVars.size();
}// runForgetting
  
