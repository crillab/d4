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
#ifndef UTILS_RENAMABLE_HORN
#define UTILS_RENAMABLE_HORN

#include <iostream>

#include "../utils/SolverTypes.hh"

using namespace std;

class RenamableHorn
{
private:
  vec<vec<Lit> > cnf;
  vec<vec<int> > occurrences;
  vec<bool> renamed;
  vec<int> countPositive;
  vec<int> whereFalse;
  vec<int> notHornClauses;
  vec<int> makeCount;
  vec<int> breakCount;
  vec<int> changed;
  
  vec<bool> bestRenaming;
  vec<int> bestNotHornClauses;
  
  void debug();  
public:
  RenamableHorn(int nbVar)
  {
    for(int i = 0 ; i<nbVar ; i++)
      {
        // for each literals
        occurrences.push();
        occurrences.push();

        renamed.push(false);
        makeCount.push(0);
        breakCount.push(0);
        changed.push(0);
      }
  }
  
  void init(bool initRandom = true);
  void reinitDataStructure();
  void flip(Var v);
  void addClause(vec<Lit> &cl);
  int run(int nbRuns, int nbFlips);

  inline vec<bool> &getBestRenaming(){return bestRenaming;}
  inline vec<int> &getBestNotHornClauses(){return bestNotHornClauses;}
  inline vec<Lit> &getClauseFromIndex(int i){return cnf[i];}

  inline void heuristicInterpretation()
  {
    for(int i = 0 ; i<cnf.size()>>5 ; i++)
      {
        int idxCl = rand() % cnf.size();
        vec<Lit> &cl = cnf[idxCl];

        for(int j = 0 ; j<cl.size() ; j++)
          renamed[var(cl[j])] = !sign(cl[j]);
        
        if(rand() & 1)
          {
            int posPositive = rand() % cl.size();
            renamed[var(cl[posPositive])] = sign(cl[posPositive]);
          }
      }
  }// heuristicInterpretation
  
  inline void randomInterpretation()
  {
    for(int i = 0 ; i<renamed.size() ; i++) renamed[i] = rand() & 1;
  }// randomInterpretation

  inline bool isPositive(Lit l){return !(sign(l) ^ renamed[var(l)]);}
  inline void removeNotHornClause(int idx)
  {
    int last = notHornClauses.last();
    int pos = whereFalse[idx];

    notHornClauses[pos] = last;
    whereFalse[last] = pos;
    notHornClauses.pop();
  }


  inline Var selectPositiveRandom(vec<Lit> &cl)
  {
    int pos = rand() % cl.size();

    for(int i = pos ; i<cl.size() ; i++)
      if(isPositive(cl[i])) return var(cl[i]);

    for(int i = 0 ; i<pos ; i++)
      if(isPositive(cl[i])) return var(cl[i]);

    assert(0);
    return var_Undef;
  }// selectPositiveRandom

  inline void addNotHornClause(int idx)
  {
    notHornClauses.push(idx);
    whereFalse[idx] = notHornClauses.size() - 1;
  }

  inline void printInfoDateStructure()
  {
    cout << "countPositive:" << endl;
    for(int i = 0 ; i<cnf.size() ; i++)
      cout << i << " " << countPositive[i] << endl;

    cout << "notHornClauses:" << endl;
    for(int i = 0 ; i<notHornClauses.size() ; i++)
      cout << notHornClauses[i] << " ";
    cout << endl;

    cout << "renamed: " << endl;
    for(int i = 0 ; i<renamed.size() ; i++) cout << renamed[i] << " ";
    cout << endl;

    cout << "makeCount: " << endl;
    for(int i = 0 ; i<makeCount.size() ; i++) cout << makeCount[i] << " ";
    cout << endl;

    cout << "breakCount:" << endl;
    for(int i = 0 ; i<breakCount.size() ; i++) cout << breakCount[i] << " ";
    cout << endl; 
  }// printInfoDateStructure
};

#endif
