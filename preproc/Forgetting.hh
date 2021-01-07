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
#ifndef PREPROC_FORGETTING
#define PREPROC_FORGETTING

#include <iostream>

#include "../mtl/Sort.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Heap.hh"
#include "../mtl/Alg.hh"

#include "../utils/System.hh"
#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"

#include "PreprocSolver.hh"
#include "OccurrenceLitElimination.hh"
#include "Vivification.hh"


using namespace std;

class Forgetting
{    
private:
  PreprocSolver &os;
  OccurrenceLitElimination occElim;
  Vivification vivifier;

  double timeForget;
  int nbForget;
  int nbIteration;
  
  vec<uint64_t> hashKeyInit;
  vec<bool> markedOutputVars;
  vec<bool> markedAsForget;
  vec<char> markedLit;  // must be 'always' false
  vec<unsigned int> stampSubsum;
  vec< vec<int> > watchNewCls;
  unsigned int stamp;
      
  // functions
  void generateAllResolution(Var v, vec< vec<Lit> > &cls);
  void removeSubsum(vec< vec<Lit> > &newCls);
    
  void constructWatchList(vec< vec<Lit> > &newCls);
      
  inline Var selectVarAndPop(vec<Var> &outputVars)
  {
    int best = 0, score = os.productOccLit(mkLit(outputVars[0], false));
      
    for(int i = 1 ; i<outputVars.size() ; i++)
      {
        if(os.value(outputVars[i]) != l_Undef) continue;
          
        Lit l = mkLit(outputVars[i], false);
        if(!os.getNbOccLit(l) || !os.getNbOccLit(~l)){best = i; break;}

        int tmp = os.productOccLit(l);
        if(tmp < score){ score = tmp; best = i;}
      }

    Var tmp = outputVars[best];
    outputVars[best] = outputVars.last();
    outputVars.pop();
    return tmp;
  }// selectVarFromPos

    
public:
  Forgetting(PreprocSolver &_os);
  void run(vec<Var> &outputVars, vec<Var> &forgetVar, int lim_occ);

  void displayStat()
  {
    printf("c Number of variables forgotten: %d\n", nbForget);
    printf("c Number of iterations: %d\n", nbIteration);
    vivifier.displayStat();
    occElim.displayStat();  
    printf("c Time used for the preprocessing step: %lf\nc\n", timeForget);
  }// displayStat
};

#endif
