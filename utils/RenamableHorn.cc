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
#include "RenamableHorn.hh"


/**
   Add a clause in the CNF formula attribute.

   @param[in] cl, the clause we want to add
 */
void RenamableHorn::addClause(vec<Lit> &cl)
{
  cnf.push();
  countPositive.push(0);
  whereFalse.push(0);
  cl.copyTo(cnf.last());

  for(int i = 0 ; i<cl.size() ; i++) occurrences[toInt(cl[i])].push(cnf.size() - 1);
}// addClause


/**
   Initialize the data structure with the information contained in the
   given model (that means we initialize the vector nbTrueLit).

   @param[in] assums, the current assumption we work with
   @param[in] model, the given model
 */
void RenamableHorn::init(bool initRandom)
{
  // generate a random renaming
  if(!initRandom) heuristicInterpretation(); 
  else randomInterpretation();

  assert(countPositive.size() == cnf.size());
  for(int i = 0 ; i<countPositive.size() ; i++) countPositive[i] = 0;
  for(int i = 0 ; i<makeCount.size() ; i++) makeCount[i] = 0;
  for(int i = 0 ; i<breakCount.size() ; i++) breakCount[i] = 0;
  notHornClauses.clear();

  for(int i = 0 ; i<cnf.size() ; i++)
    {
      vec<Lit> &cl = cnf[i];
      
      for(int j = 0 ; j<cl.size() ; j++)
        if(isPositive(cl[j])) countPositive[i]++;

      if(countPositive[i] > 1)
        {
          notHornClauses.push(i);
          whereFalse[i] = notHornClauses.size() - 1;
        }

      if(countPositive[i] == 1) // breakCount
        {
          for(int j = 0 ; j<cl.size() ; j++)
            if(!isPositive(cl[j])) breakCount[var(cl[j])]++;
        }

      if(countPositive[i] == 2) // makeCount
        {
          for(int j = 0 ; j<cl.size() ; j++)
            if(isPositive(cl[j])) makeCount[var(cl[j])]++;
        }
    }
}// init


/**
   Flip the value of the renamed table.
   
   @param[in] v, the variable we want to rename or unrename
 */
void RenamableHorn::flip(Var v)
{ 
  // positive lit
  Lit l = mkLit(v, renamed[v]);
  renamed[v] = !renamed[v];

  //  cout << "we flip " << v + 1 << endl;
  // cout << "lit = " << readableLit(l) << endl; 

  for(int i = 0 ; i<occurrences[toInt(l)].size() ; i++)
    {
      int idxCl = occurrences[toInt(l)][i];
      // cout << "---> " << idxCl << endl;
      
      countPositive[idxCl]--;

      if(countPositive[idxCl] == 0) // update the breakCount
        {
          for(int j = 0 ; j<cnf[idxCl].size() ; j++)
            if(var(cnf[idxCl][j]) != v && !isPositive(cnf[idxCl][j])) breakCount[var(cnf[idxCl][j])]--;
        }
      if(countPositive[idxCl] == 1) // the clause is now horn 
        {
          removeNotHornClause(idxCl);
          
          // update the breakCount
          makeCount[v]--;
          for(int j = 0 ; j<cnf[idxCl].size() ; j++)
            if(isPositive(cnf[idxCl][j])) makeCount[var(cnf[idxCl][j])]--;
            else breakCount[var(cnf[idxCl][j])]++;
              
        }
      if(countPositive[idxCl] == 2)
        {
          // update the makeCount
          for(int j = 0 ; j<cnf[idxCl].size() ; j++)
            if(isPositive(cnf[idxCl][j])) makeCount[var(cnf[idxCl][j])]++;          
        }
    }

  // negative lit
  for(int i = 0 ; i<occurrences[toInt(~l)].size() ; i++)
    {
      int idxCl = occurrences[toInt(~l)][i];
      // cout << "<--- " << idxCl << endl;
      
      countPositive[idxCl]++;

      if(countPositive[idxCl] == 1)
        {
          for(int j = 0 ; j<cnf[idxCl].size() ; j++)
            if(!isPositive(cnf[idxCl][j])) breakCount[var(cnf[idxCl][j])]++;
        }
      if(countPositive[idxCl] == 2)
        {
          breakCount[v]--;
          for(int j = 0 ; j<cnf[idxCl].size() ; j++)
            {
              if(isPositive(cnf[idxCl][j])) makeCount[var(cnf[idxCl][j])]++;
              else breakCount[var(cnf[idxCl][j])]--;
            }

          addNotHornClause(idxCl);
        }
      if(countPositive[idxCl] == 3)
        {
          for(int j = 0 ; j<cnf[idxCl].size() ; j++)
            if(var(cnf[idxCl][j]) != v && isPositive(cnf[idxCl][j])) makeCount[var(cnf[idxCl][j])]--;
        }
    }
}// flip


/**
   Make the CNF formula empty.
 */
void RenamableHorn::reinitDataStructure()
{
  cnf.clear();
  countPositive.clear();
  for(int i = 0 ; i<occurrences.size() ; i++) occurrences[i].clear();
}// reinitDataStructure


/**
   Try to improve the renaming by realizing flips.

   @param[in] nbRuns, the number of tries
   @param[in] nbFlips, the number of allowed flips

   \return the number of non-horn clauses obtained by the best
   interpretation. The result is solved in a general data structure
 */
int RenamableHorn::run(int nbRuns, int nbFlips)
{
  int bestScoreObtained = cnf.size();
  
  for(int runNumber = 0 ; runNumber < nbRuns ; runNumber++)
    {        
      init(false);
      for(int i = 0 ; i<changed.size() ; i++) changed[i] = 0; 
      int bestScoreRenaming = notHornClauses.size();
  
      for(int i = 0 ; i<nbFlips && notHornClauses.size() ; i++)
        {
          int idx = notHornClauses[rand() % notHornClauses.size()];
          vec<Lit> &cl = cnf[idx];
          
          Var vToFlip = var_Undef;

#if 0
          int bestScore = -cnf.size();              
              
          for(int j = 0 ; j<cl.size() ; j++)
            if(isPositive(cl[j]))
              {
                Var v = var(cl[j]);
                int score = makeCount[v] - breakCount[v];

                if(score > bestScore)
                  {
                    vToFlip = v;
                    bestScore = score;
                  }
              }
              
          assert(vToFlip != var_Undef);
          if(bestScore <= 0) vToFlip = selectPositiveRandom(cl);
#else
          Var best = var_Undef, second_best = var_Undef;
          Var youngest = var_Undef;
          int youngest_birthdate = -cnf.size();
          int best_diff = -cnf.size();
          int second_best_diff = -cnf.size();

          for(int i = 0 ; i < cl.size() ; i++)
            {
              if(!isPositive(cl[i])) continue;
                  
              Var v = var(cl[i]);
              int diff = makeCount[v] - breakCount[v];
              int birthdate = changed[v];

              if (birthdate > youngest_birthdate)
                {
                  youngest_birthdate = birthdate;
                  youngest = v;
                }
                  
              if (diff > best_diff || (diff == best_diff && changed[v] < changed[best]))
                {
                  /* found new best, demote best to 2nd best */
                  second_best = best;
                  second_best_diff = best_diff;
                  best = v;
                  best_diff = diff;
                }
              else if (diff > second_best_diff || (diff == second_best_diff && changed[v] < changed[second_best]))
                {
                  /* found new second best */
                  second_best = v;
                  second_best_diff = diff;
                }
            }

          assert(best != var_Undef);
          if (second_best == var_Undef || best != youngest) vToFlip = best;
          else
            {
              if (rand() & 1) vToFlip = second_best;
              else vToFlip = best;
            }
#endif
          
          flip(vToFlip);
          changed[vToFlip] = i;
      
          if(notHornClauses.size() < bestScoreRenaming) bestScoreRenaming = notHornClauses.size();

          if(bestScoreRenaming < bestScoreObtained)
            {
              bestScoreObtained = bestScoreRenaming;
              renamed.copyTo(bestRenaming);
              notHornClauses.copyTo(bestNotHornClauses);
            }          
        }
    }

  return bestScoreObtained;
}// run



/**
   Serach if there is a problem in the data structures.
 */
void RenamableHorn::debug()
{
  // cout << "DEBUG" << endl;
  // printInfoDateStructure();
    
  vec<int> tmpCountPositive;
  vec<int> tmpMakeCount;
  vec<int> tmpBreakCount;
  vec<int> tmpNotHornClauses;
  
  for(int i = 0 ; i<countPositive.size() ; i++) tmpCountPositive.push(0);
  for(int i = 0 ; i<makeCount.size() ; i++) tmpMakeCount.push(0);
  for(int i = 0 ; i<breakCount.size() ; i++) tmpBreakCount.push(0);

  for(int i = 0 ; i<cnf.size() ; i++)
    {
      vec<Lit> &cl = cnf[i];
      
      for(int j = 0 ; j<cl.size() ; j++)
        if(isPositive(cl[j])) tmpCountPositive[i]++;

      if(tmpCountPositive[i] > 1)tmpNotHornClauses.push(i);

      if(tmpCountPositive[i] == 1) // breakCount
        {
          for(int j = 0 ; j<cl.size() ; j++)
            if(!isPositive(cl[j])) tmpBreakCount[var(cl[j])]++;
        }

      if(tmpCountPositive[i] == 2) // makeCount
        {
          for(int j = 0 ; j<cl.size() ; j++)
            if(isPositive(cl[j])) tmpMakeCount[var(cl[j])]++;
        }
    }

  for(int i = 0 ; i<countPositive.size() ; i++) assert(countPositive[i] == tmpCountPositive[i]);
  for(int i = 0 ; i<makeCount.size() ; i++) assert(makeCount[i] == tmpMakeCount[i]);
  for(int i = 0 ; i<breakCount.size() ; i++) assert(breakCount[i] == tmpBreakCount[i]);

  assert(tmpNotHornClauses.size() == notHornClauses.size());
  for(int i = 0 ; i<tmpNotHornClauses.size() ; i++)
    {
      bool isIn = false;
      for(int j = 0 ; !isIn && j<notHornClauses.size() ; j++) isIn = notHornClauses[j] == tmpNotHornClauses[i];
      assert(isIn);
    }
}// debug
