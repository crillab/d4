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
#ifndef DAG_PCNODE_h
#define DAG_PCNODE_h

#include "DAG.hh"

#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"


template<class T> class DAG;
template<class T> class PcNode : public DAG<T>
{
  using DAG<T>::globalStamp;
  vec<Lit> units;

public:
  Solver s;

  PcNode(vec<vec<Lit> > &cnf, int nbVar)
  {
    for(int i = 0 ; i<nbVar ; i++) s.newVar();
    for(int i = 0 ; i<cnf.size() ; i++) s.addClause_(cnf[i]);
  }// constructor

  inline void printNNF(std::ostream& out, bool certif){printf("PC");}// printNNF

  inline bool isSAT(vec<Lit> &unitsLitBranches)
  {
    assert(Branch<T>::s);
    // cout << "return SAT: "; showListLit(unitsLitBranches);
    return true; // we can return SAT because we call unit propagation around the path

    cout << "call the SAT solver oO: "; showListLit(unitsLitBranches);
    s.cancelUntil(0);
    s.newDecisionLevel();
    for(int i = 0 ; i<unitsLitBranches.size() ; i++)
      if(s.value(unitsLitBranches[i]) == l_Undef) s.uncheckedEnqueue(unitsLitBranches[i]);

    bool saveResult = s.propagate() == CRef_Undef;
    cout << "trail: "; showListLit(s.trail);
    s.cancelUntil(0);

    cout << "PCNode return: " << saveResult << endl;
    return saveResult;
  }

  inline T computeNbModels() { return 1; }
};

#endif
