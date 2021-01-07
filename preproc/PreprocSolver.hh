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
#ifndef PREPROC_PREPROC_SOLVER
#define PREPROC_PREPROC_SOLVER

#include <iostream>

#include "../mtl/Sort.hh"
#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"


using namespace std;

class PreprocSolver
{
protected:
  struct LitOrderLt
  {
    const vec<double>&  activity;
    const vec<bool> &isProtectedVar;
    bool operator () (Lit x, Lit y) const
    {
      if(isProtectedVar[var(x)] && !isProtectedVar[var(y)]) return true;
      if(!isProtectedVar[var(x)] && isProtectedVar[var(y)]) return false;
      return activity[var(x)] > activity[var(y)];
    }
    LitOrderLt(const vec<double>&  act, const vec<bool> &m) : activity(act), isProtectedVar(m) { }
  };

  
public:
  PreprocSolver(Solver &s, vec<bool> &_isProtectedVar);
  void initOccList(vec<CRef> &refClauses);
  void printOccList();
  void printFormula()
  {
    for(int i = 0 ; i<solver.clauses.size() ; i++)
      {
        cout << i << ": ";
        solver.ca[solver.clauses[i]].showClause();
      }
  }// printFormula
  
  void searchAndRemoveOcc(vec<int> &setOfIdx, int idx);
  void searchAndRemoveOccFromLit(Lit l, int idx){searchAndRemoveOcc(occurrences[toInt(l)], idx);}

  void removeAndCompact();
  void removeClauseOcc(int idx);
  void addClauseOcc(vec<Lit> &lits);
  void removeClauseFromLit(Lit l);

  inline Solver &getSolver(){return solver;}
  inline void setNeedModel(bool b){solver.setNeedModel(b);}

  inline void setHashKeyInit(int idx){hashKeyInit[idx] = hashClause(solver.ca[solver.clauses[idx]]);}
  inline uint64_t getHashKeyInit(int idx){return hashKeyInit[idx];}
  
  inline void killUnsat()
  {
    printf("c We detected that the problem is unsat during the forgetting process\ns UNSATISFIABLE\n");
    exit(20);
  }
  
  inline uint64_t hashClause(Clause &c)
  {
    uint64_t ret = 0;
    for(int i = 0 ; i<c.size() ; i++) ret |= 1<<(toInt(c[i]) & 63); // = % 64
    return ret;
  }
    
  inline uint64_t hashSetOfLit(vec<Lit> &c)
  {
    uint64_t ret = 0;
    for(int i = 0 ; i<c.size() ; i++) ret |= 1<<(toInt(c[i]) & 63); // = % 64      
    return ret;
  }

  inline bool litIsMarked(Lit l){return isProtectedVar[var(l)];}

  // occurrence interface
  inline void copyOccLitInVec(Lit l, vec<int> &v){occurrences[toInt(l)].copyTo(v);}
  inline int productOccLit(Lit l){return occurrences[toInt(l)].size() * occurrences[toInt(~l)].size();}
  inline int sumOccLit(Lit l){return occurrences[toInt(l)].size() + occurrences[toInt(~l)].size();}
  inline vec< vec<int> > &getOccurrences(){return occurrences;}  
  inline vec<int> &getOccurrenceLit(Lit l){return occurrences[toInt(l)];}
  inline int getNbOccLit(Lit l){return occurrences[toInt(l)].size();}

  
  // solver interface
  inline bool solve(Lit l){return solver.solve(l);}
  inline bool solve(){return solver.solve();}
  inline vec<lbool> &model(){return solver.model;}

  inline vec<Lit> &getTrail(){return solver.trail;}
  inline int decisionLevel(){return solver.decisionLevel();}  
  inline void newDecisionLevel(){solver.newDecisionLevel();}
  inline lbool value(Lit l){return solver.value(l);}
  inline lbool value(Var v){return solver.value(v);}
  inline void uncheckedEnqueue(Lit l){solver.uncheckedEnqueue(l);}
  inline CRef propagate(){return solver.propagate();}
  inline void attachClause(CRef cr){solver.attachClause(cr);}
  inline void detachClause(CRef cr, bool strict = false){solver.detachClause(cr, strict);}
  inline void detachClause(int idx, bool strict = false){detachClause(solver.clauses[idx], strict);}
  inline void cancelUntil(int lev){solver.cancelUntil(lev);}
  inline vec<CRef> &getClauses(){return solver.clauses;}
  inline vec<CRef> &getLearnts(){return solver.learnts;}
  inline Clause &getClause(int idx){return solver.ca[getCRef(idx)];}
  inline Clause &getClause(CRef cr){return solver.ca[cr];}  
  inline CRef &getCRef(int idx){return solver.clauses[idx];}
  inline vec<Watcher> &getWatcher(Lit l){return solver.watches[l];}
  
  inline int getNbClause(){return (solver.clauses).size();}
  inline int getNbVar(){return solver.nVars();}
  inline int nVars(){return solver.nVars();}
  
  void removeLearnt();

  inline void sortClause(Clause &c)
  {   
    vec<Lit> tmp;
    for(int k = 0 ; k<c.size() ; k++) tmp.push(c[k]);          
    sort(tmp, LitOrderLt(solver.activity, isProtectedVar));
    for(int k = 0 ; k<c.size() ; k++) c[k] = tmp[k];
  }// sortClause


  /**
     Collect the formula and put it in the given vector.
   */
  inline void collectInitFormula(vec<vec<Lit> > &clauses)
  {
    for(int i = 0 ; i<solver.trail.size() ; i++)
      {
        clauses.push();
        clauses.last().push(solver.trail[i]);
      }
    
    for(int i = 0 ; i<solver.clauses.size() ; i++) 
      {
        Clause &c = solver.ca[solver.clauses[i]];
        if(solver.satisfied(c)) continue;

        clauses.push();
        for(int j = 0 ; j<c.size() ; j++)
          {
            assert(solver.value(c[j]) != l_True);
            if(solver.value(c[j]) == l_Undef) clauses.last().push(c[j]);
          }
      }
  }// collectInitFormula
  
  
private:
  Solver &solver;
  vec< vec<int> > occurrences;
  vec<bool> isProtectedVar;
  vec<uint64_t> hashKeyInit;
  vec<int> freePositionInClauses;
  vec<unsigned int> stampSubsum;

protected:
  void debug();
};


#endif
