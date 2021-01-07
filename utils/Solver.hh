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
/****************************************************************************************[Solver.h]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Solver_h
#define Solver_h

#include <iostream>
#include <fstream>

#include "../mtl/Heap.hh"

class ScoringMethod; // forward declaration
  
struct Watcher {
  CRef cref;
  Lit  blocker;
  Watcher(CRef cr, Lit p) : cref(cr), blocker(p) {}
  bool operator==(const Watcher& w) const { return cref == w.cref; }
  bool operator!=(const Watcher& w) const { return cref != w.cref; }
};

struct VarOrderLt 
{
  const vec<double>&  activity;
  bool operator () (Var x, Var y) const { return activity[x] > activity[y]; }
  VarOrderLt(const vec<double>&  act) : activity(act) { }
};

  
//=================================================================================================
// Solver -- the main class:
  
class Solver 
{
public:
    
  // Constructor/Destructor:
  //
  Solver(std::ostream *certif = NULL);
  virtual ~Solver();

  //Certificate:
  std::ostream *cert; 
  
  // Problem specification:
  //
  Var     newVar    (bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.
    
  bool    addClause (const vec<Lit>& ps);                     // Add a clause to the solver. 
  bool    addEmptyClause();                                   // Add the empty clause, making the solver contradictory.
  bool    addClause (Lit p);                                  // Add a unit clause to the solver. 
  bool    addClause (Lit p, Lit q);                           // Add a binary clause to the solver. 
  bool    addClause (Lit p, Lit q, Lit r);                    // Add a ternary clause to the solver. 
  bool    addClause_(      vec<Lit>& ps);                     // Add a clause to the solver without making superflous internal copy. Will
  // change the passed vector 'ps'.

  // Solving:
  bool    simplify     ();                                    // Removes already satisfied clauses.
  bool    solveWithAssumptions ();                            // Search for a model that respects assumptions in attribute 'assumptions'.
  bool    solveWithAssumptions (vec<Lit> &assums, bool model = false); // Search for a model that respects assumptions in attribute 'assumptions'.
  bool    solveWithAssumptions (vec<Lit> &assums, vec<Lit> &unit, bool model = false); // idem, but we collect the unit literals under assumptions
  
  bool    solve        (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions.
  lbool   solveLimited (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
  lbool   solveLimited (const vec<Lit>& assumps, int nbConflict); // Search for a model that respects a given set of assumptions (With resource constraints).
  bool    solve        ();                        // Search without assumptions.
  bool    solve        (Lit p);                   // Search for a model that respects a single assumption.
  bool    solve        (Lit p, Lit q);            // Search for a model that respects two assumptions.
  bool    solve        (Lit p, Lit q, Lit r);     // Search for a model that respects three assumptions.
  bool    okay         () const;                  // FALSE means solver is in a conflicting state

  void    toDimacs     (FILE* f, const vec<Lit>& assumps);            // Write CNF to file in DIMACS-format.
  void    toDimacs     (const char *file, const vec<Lit>& assumps);
  void    toDimacs     (FILE* f, Clause& c, vec<Var>& map, Var& max);

  // Convenience versions of 'toDimacs()':
  void    toDimacs     (const char* file);
  void    toDimacs     (const char* file, Lit p);
  void    toDimacs     (const char* file, Lit p, Lit q);
  void    toDimacs     (const char* file, Lit p, Lit q, Lit r);
    
  // Variable mode:
  // 
  void    setPolarity    (Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
  void    setDecisionVar (Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.

  // Read state:
  //
  lbool           value              (Var x) const;       // The current value of a variable.
  lbool           value              (Lit p) const;       // The current value of a literal.
  lbool           modelValue         (Var x) const;       // The value of a variable in the last model. The last call to solve must have been satisfiable.
  lbool           modelValue         (Lit p) const;       // The value of a literal in the last model. The last call to solve must have been satisfiable.
  int             nAssigns           ()      const;       // The current number of assigned literals.
  int             nClauses           ()      const;       // The current number of original clauses.
  const Clause &  getClause          (int i) const;       // The set of clauses in the solver
  int             nLearnts           ()      const;       // The current number of learnt clauses.
  int             nVars              ()      const;       // The current number of variables.
  int             nFreeVars          ()      const;

  // Resource contraints:
  //
  void    setConfBudget(int64_t x);
  void    setPropBudget(int64_t x);
  void    budgetOff();
  void    interrupt();          // Trigger a (potentially asynchronous) interruption of the solver.
  void    clearInterrupt();     // Clear interrupt indicator flag.

  // Memory managment:
  //
  virtual void garbageCollect();
  void    checkGarbage(double gf);
  void    checkGarbage();

  // Extra results: (read-only member variable)
  //
  vec<lbool> model;             // If problem is satisfiable, this vector contains the model (if any).
  vec<Lit>   conflict;          // If problem is unsatisfiable (possibly under assumptions),
  // this vector represent the final conflict clause expressed in the assumptions.

  // Mode of operation:
  //
  int       verbosity;
  double    var_decay;
  double    clause_decay;
  double    random_var_freq;
  double    random_seed;
  bool      luby_restart;
  int       ccmin_mode;         // Controls conflict clause minimization (0=none, 1=basic, 2=deep).
  int       phase_saving;       // Controls the level of phase saving (0=none, 1=limited, 2=full).
  bool      rnd_pol;            // Use random polarities for branching heuristics.
  bool      rnd_init_act;       // Initialize variable activities with a small random value.
  double    garbage_frac;       // The fraction of wasted memory allowed before a garbage collection is triggered.

  int       restart_first;      // The initial restart limit.                                                                (default 100)
  double    restart_inc;        // The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
  double    learntsize_factor;  // The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
  double    learntsize_inc;     // The limit for learnt clauses is multiplied with this factor each restart.                 (default 1.1)

  int       learntsize_adjust_start_confl;
  double    learntsize_adjust_inc;

  // Statistics: (read-only member variable)
  //
  uint64_t solves, starts, decisions, rnd_decisions, propagations, conflicts;
  uint64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;

public:

  // Helper structures:
  //
  struct VarData { CRef reason; int level; };
  static inline VarData mkVarData(CRef cr, int l){ VarData d = {cr, l}; return d; }

  struct WatcherDeleted
  {
    const ClauseAllocator& ca;
    WatcherDeleted(const ClauseAllocator& _ca) : ca(_ca) {}
    bool operator()(const Watcher& w) const { return ca[w.cref].mark() == 1; }
  };

  struct LitOrderLt 
  {
    const vec<double>&  activity;
    bool operator () (Lit x, Lit y) const { return activity[var(x)] > activity[var(y)]; }
    LitOrderLt(const vec<double>&  act) : activity(act) { }
  };
    
  // Solver state:
  bool                ok;               // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
  vec<CRef>           clauses;          // List of problem clauses.
  vec<CRef>           learnts;          // List of learnt clauses.
  vec<CRef>           phantoms;          // List of learnt clauses.

  vec<double>         scoreActivity;    // count the number of time a variable occurs in a conflict
  double              cla_inc;          // Amount to bump next clause with.
  vec<double>         activity;         // A heuristic measurement of the activity of a variable.
  double              var_inc;          // Amount to bump next variable with.
  OccLists<Lit, vec<Watcher>, WatcherDeleted>
  watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
  vec<lbool>          assigns;          // The current assignments.
  vec<char>           polarity;         // The preferred polarity of each variable.
  vec<char>           decision;         // Declares if a variable is eligible for selection in the decision heuristic.
  vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
  vec<int>            trail_lim;        // Separator indices for different decision levels in 'trail'.
  vec<VarData>        vardata;          // Stores reason and level for each variable.
  int                 qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
  int                 simpDB_assigns;   // Number of top-level assignments since last execution of 'simplify()'.
  int64_t             simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplify()'.
  vec<Lit>            assumptions;      // Current set of assumptions provided to solve by the user.
  Heap<VarOrderLt>    order_heap;       // A priority queue of variables ordered with respect to the variable activity.
  double              progress_estimate;// Set by 'search()'.
  bool                remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.

  ClauseAllocator     ca;

  // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
  // used, exept 'seen' wich is used in several places.
  //
  vec<char>           seen;
  vec<Lit>            analyze_stack;
  vec<Lit>            analyze_toclear;
  vec<Lit>            add_tmp;

  double              max_learnts;
  double              learntsize_adjust_confl;
  int                 learntsize_adjust_cnt;
  int idxReasonFinal;
  int idxClausesCpt;
  
  // Resource contraints:
  //
  int64_t             conflict_budget;    // -1 means no budget.
  int64_t             propagation_budget; // -1 means no budget.
  bool                asynch_interrupt;

  // Main internal methods:
  //
  void     insertVarOrder   (Var x);                                                 // Insert a variable in the decision order priority queue.
  Lit      pickBranchLit    ();                                                      // Return the next decision variable.
  void     newDecisionLevel ();                                                      // Begins a new decision level.
  void     uncheckedEnqueue (Lit p, CRef from = CRef_Undef);                         // Enqueue a literal. Assumes value of literal is undefined.
  bool     enqueue          (Lit p, CRef from = CRef_Undef);                         // Test if fact 'p' contradicts current state, enqueue otherwise.
  CRef     propagate        ();                                                      // Perform unit propagation. Returns possibly conflicting clause.
  void     cancelUntil      (int level);                                             // Backtrack until a certain level.
  void     analyze          (CRef confl, vec<Lit>& out_learnt, int& out_btlevel);    // (bt = backtrack)
  void     analyzeLastUIP   (CRef confl, vec<Lit>& out_learnt, int& out_btlevel);    // (bt = backtrack)
  void     analyzeFinal     (Lit p, vec<Lit>& out_conflict);                         // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
  bool     litRedundant     (Lit p, uint32_t abstract_levels);                       // (helper method for 'analyze()')
  lbool    search           (int nof_conflicts);                                     // Search for a given number of conflicts.
  lbool    solve_           (bool rebuildHeap = true, int nbConflict = 0);           // Main solve method (assumptions given in 'assumptions').
  void     reduceDB         ();                                                      // Reduce the set of learnt clauses.
  void     removeSatisfied  (vec<CRef>& cs);                                         // Shrink 'cs' to contain only non-satisfied clauses.
  void     rebuildOrderHeap ();

  // Maintaining Variable/Clause activity:
  //
  void     varDecayActivity ();                      // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
  void     varBumpActivity  (Var v, double inc);     // Increase a variable with the current 'bump' value.
  void     varBumpActivity  (Var v);                 // Increase a variable with the current 'bump' value.
  void     claDecayActivity ();                      // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
  void     claBumpActivity  (Clause& c);             // Increase a clause with the current 'bump' value.

  // Operations on clauses:
  //
  void     attachClause     (CRef cr);               // Attach a clause to watcher lists.
  void     detachClause     (CRef cr, bool strict = false); // Detach a clause to watcher lists.
  void     removeClause     (CRef cr, bool strict = false); // Detach and free a clause.
  void     removeNotAttachedClause (CRef cr); // Detach and free a clause.
  bool     locked           (const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
  bool     satisfied        (const Clause& c) const; // Returns TRUE if a clause is satisfied in the current state.

  void     relocAll         (ClauseAllocator& to);

  // Misc:
  //
  int      decisionLevel    ()      const; // Gives the current decisionlevel.
  uint32_t abstractLevel    (Var x) const; // Used to represent an abstraction of sets of decision levels.
  CRef     reason           (Var x) const;
  int      level            (Var x) const;
  double   progressEstimate ()      const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...
  bool     withinBudget     ()      const;

  // Static helpers:
  //

  // Returns a random float 0 <= x < 1. Seed must never be 0.
  static inline double drand(double& seed) 
  {
    seed *= 1389796;
    int q = (int)(seed / 2147483647);
    seed -= (double)q * 2147483647;
    return seed / 2147483647; 
  }
    
  // Returns a random integer 0 <= x < size. Seed must never be 0.
  static inline int irand(double& seed, int size){return (int)(drand(seed) * size);}
  vec<char> kindOfVariable;
    
private:    
  vec<int> flags;
  vec<Lit> litFlags;
  vec<Lit> priorityList;    
  vec<Lit> lastUnit;
  vec<bool> defined;
  vec<bool> insistTruePolarity;
  
public:
  bool phantomMode;
  Lit phantomLit;

  
  // deal with the occurrence list which keep information about the
  // number of SAT/UNS lit in clause with a size greater than 2
  int limTrailNbSatUns;
  vec< vec<int> > occGtThree;
  bool occGtThreeInit;

  // this variables are used to keep the integrity of the occurence list 
  vec<bool> alreadyConsidered;
  vec<Lit> mustBeConsidered;
  vec<bool> toUnConsidered;
  vec<Lit> heapConsideredOcc;
  vec<bool> wasConsideredOcc;
    
private:

  vec<int> occHeapSz;    
  int currentTotalVar;
    
  vec<Var> topVariables;
  vec<Var> noSymVariables;

  bool needModel;
  vec< vec<int> > occDomainConstraints;
  vec< vec<Lit> > listOfDomainConstraints;    
public:
  inline void setInsistTruePolarity(Var v, bool value){insistTruePolarity[v] = value;}
  inline bool getInsistTruePolarity(Var v){return insistTruePolarity[v];}
  
  inline void startPhantomMode()
  {
    if(phantomMode) return;
    phantomMode = true;
    Var v = newVar();
    phantomLit = mkLit(v, false);
  }// startPhantomMode


  inline void refillAssums()
  {
    while(decisionLevel() < assumptions.size())
      {
	Lit p = assumptions[decisionLevel()];
	assert(value(p) != l_False);
	
	if (value(p) == l_True) newDecisionLevel();
	else
	  {
	    newDecisionLevel();
	    uncheckedEnqueue(p);
	    CRef r = propagate();
	    assert(r == CRef_Undef);
	  }
      }
  }// refillAssums
  
  inline void setNeedModel(bool b){needModel = b;}


  /**
     Create the equivalence classes
  */
  inline void createEquivClass(vec< vec<Lit> > &equivSet, vec< vec<Lit> > &classSet, vec<Lit> &equivLit)
  {
    for(int i = 0 ; i<equivSet.size() ; i++)
      {          
        if(!equivSet[i].size()) continue;
        Lit l = equivSet[i][0];
        if(sign(l)) l = ~l;
        if(var(equivLit[var(l)]) != var(l)) continue;

        classSet.push();
        vec<Lit> &c = classSet.last();
          
        vec<Var> stack;
        stack.push(var(l));
        while(stack.size())
          {
            Var v = stack.last();
            c.push(mkLit(v, sign(equivLit[v])));
            stack.pop();
                  
            for(int j = i ; j<equivSet.size() ; j++)
              {
                if(!equivSet[j].size()) continue;
                if(v != var(equivSet[j][0]) && v != var(equivSet[j][1])) continue;
                int pos = v == var(equivSet[j][0]) ? 0 : 1;

                Lit other = equivSet[j][1 - pos], cl = equivSet[j][pos];
                if(sign(other)) {other = ~other; cl = ~cl;}

                if(var(equivLit[var(other)]) != var(other)) continue;                  
                equivLit[var(other)] = (sign(cl)) ? ~equivLit[var(cl)] : equivLit[var(cl)];
                assert(var(other) != var(l));
                stack.push(var(other));
                equivSet[j].clear();
              }
          }
      }

  }// createEquivClass
    
  /**
     Replace equivalence variables.
  */
  inline void replaceEquiv(vec< vec<Lit> > &equivSet)
  {
    if(!equivSet.size()) return;
      
    vec<Lit> equivLit, visited;
    for(int i = 0 ; i<nVars() ; i++)
      {
        equivLit.push(mkLit(i, false)); // l <-> l
        visited.push(lit_Undef);
      }
          
    setNeedModel(true);      
    solve();
    removeSatisfied(clauses);
          
    // built equiv
    for(int i = 0 ; i<equivSet.size() ; i++)
      {
        if(model[var(equivSet[i][0])] == l_False) equivSet[i][0] = ~equivSet[i][0];
        if(model[var(equivSet[i][1])] == l_False) equivSet[i][1] = ~equivSet[i][1];
      }

    vec< vec<Lit> > classSet;
    createEquivClass(equivSet, classSet, equivLit);
      
    // detach all clauses
    for(int i = 0 ; i<clauses.size() ; i++) detachClause(clauses[i], true);

    // printf("equivLit: "); showListLit(equivLit);
      
    // replace lit by equiv element
    int i, j;
    for(i = j = 0 ; i<clauses.size() ; i++)
      {
        Clause &c = ca[clauses[i]];
          
        for(int k = 0 ; k<c.size() ; k++)
          if(var(equivLit[var(c[k])]) != var(c[k]))
            c[k] = (sign(c[k])) ? ~equivLit[var(c[k])] : equivLit[var(c[k])];
          
        // check if we have to remove or reduce the clause
        // c.showClause();
        bool isTaut = false;
        int ic, jc;
        for(ic = jc = 0 ; !isTaut && ic<c.size() ; ic++)
          {
            if(value(c[ic]) != l_Undef){isTaut = value(c[ic]) == l_True; continue;}
            if(visited[var(c[ic])] == lit_Undef)
              {
                visited[var(c[ic])] = c[ic];
                c[jc++] = c[ic];                  
              }
            else isTaut = visited[var(c[ic])] == ~c[ic];
          }

                    
        for(int k = 0 ; k<c.size() ; k++) visited[var(c[k])] = lit_Undef;
        if(!isTaut)
          {
            c.shrink(ic - jc);
              
            if(c.size() > 1) clauses[j++] = clauses[i];
            else
              {
                assert(0);
                if(value(c[0]) == l_Undef) uncheckedEnqueue(c[0]);
                c.mark(1);
                ca.free(clauses[i]);
              }
              
            // printf("%d: ", isTaut);
            // c.showClause();
            // printf("\n");
          }
        else
          {
            c.mark(1);
            ca.free(clauses[i]);
          }          
      }
    clauses.shrink(i - j);
      
    // attach all clauses
    for(int i = 0 ; i<clauses.size() ; i++) attachClause(clauses[i]);

    // add equivalence
    for(int i = 0 ; i<classSet.size() ; i++)
      {
        for(int j = 1 ; j<classSet[i].size() ; j++)
          {
            addClause(~classSet[i][0], classSet[i][j]);
            addClause(classSet[i][0], ~classSet[i][j]);
          }
      }
      
    setNeedModel(false);
#if 0
    printf("Print formula\n");
    for(int i = 0 ; i<clauses.size() ; i++) ca[clauses[i]].showClause();
    printf("END\n");
#endif
  }// replaceEquiv

    
  /**
     Insert a clause and check if we have to propagate something.       
  */
  inline void insertClauseAndPropagate(vec<Lit> &cl)
  {
    // printf("clause: "); showListLit(cl);
    if (cert != nullptr)
      {
	std::ostream &cval = *cert;
	for (int i = 0; i < cl.size(); i++)
	  cval << (var(cl[i]) + 1)*(-2 * sign(cl[i]) + 1) << " ";
	cval << "0\n";

#if 0
	printf("insert idx %d : ", idxClausesCpt);
	for(int j = 0 ; j<cl.size() ; j++)
	  printf("%s%d ", sign(cl[j]) ? "-" : "", var(cl[j]) + 1);
	printf("\n");
#endif
      }

    idxClausesCpt++;
    if(cl.size() > 1)
      {
        int posHigherLevel = 1;
        for(int j = 2 ; j<cl.size() ; j++) if(level(var(cl[j])) > level(var(cl[posHigherLevel]))) posHigherLevel = j;
        Lit tmp = cl[posHigherLevel];
        cl[posHigherLevel] = cl[1];
        cl[1] = tmp;
          
        cancelUntil(level(var(cl[1])));
        CRef cr = ca.alloc(cl, true);
        learnts.push(cr);
        attachClause(cr);
        uncheckedEnqueue(cl[0], cr);
	ca[cr].idxReason(idxClausesCpt);
      } else
      {
        cancelUntil(0);      
        uncheckedEnqueue(cl[0]);
      }
      
    propagate();

    while(decisionLevel() < assumptions.size())
      {
        Lit p = assumptions[decisionLevel()];
        newDecisionLevel();          
        if (value(p) == l_Undef) uncheckedEnqueue(p);
        propagate();          
      }
  }// insertClauseAndPropagate
   
  inline vec<char> &getPolarity(){return polarity;}

  void searchAtMostOne(vec<Lit> &vc, vec<Lit> &canBeTrue);
  inline char getKindOfVariable(int idx){return kindOfVariable[idx];}
  inline void backTrack(){cancelUntil(decisionLevel() - 1);}
  inline void computeLitPropagate(Lit l, vec<Lit> &vp)
  {
    vp.clear();
    newDecisionLevel();
    uncheckedEnqueue(l);
      
    if(propagate() == CRef_Undef) 
      {
        for(int j = trail_lim[decisionLevel() - 1] ; j<trail.size() ; j++) vp.push(trail[j]);      
        assert(vp[0] == l);
      }
    backTrack();
  }    


  /**
     Return true if one of the literal is true.
       
     @param[in] vc, the at most one constraint
  */
  inline bool oneIsTrue(vec<Lit> &vc)
  {     
    for(int i = 0 ; i<vc.size() ; i++) if(value(vc[i]) == l_True) return true;
    return false;
  }// oneIsTrue    
    
  inline void putListTop(vec<Lit> &v)
  {
    for(int i = 0 ; i<v.size() ; i++)
      {
        kindOfVariable[var(v[i])] = 't'; 
        topVariables.push(var(v[i]));
      }
  }// putListTop  

  inline void putListNoSym(vec<Lit> &v)
  {
    for(int i = 0 ; i<v.size() ; i++)
      {
        kindOfVariable[var(v[i])] = 'n'; 
        noSymVariables.push(var(v[i]));
      }
  }// putListNoSym


  inline void rebuildTrail(vec<Lit> &areUnit)
  {
    for(int i = 0 ; i<areUnit.size() ; i++) if(value(areUnit[i]) == l_Undef) uncheckedEnqueue(areUnit[i]);
    propagate();
  }
  
  inline bool clauseIsSAT(Clause &c)
  {
    for(int i = 0 ; i<c.size() ; i++) if(value(c[i]) == l_True) return true;
    return false;
  }
   
        
  /**
     Collect the unit literal present in the set of variable
     setOfVar.
       
     @param[in] setOfVar, the set of var
     @param[out] unitsLit, the unit literals collected
  */
  void collectUnit(vec<Var> &setOfVar, vec<Lit> &unitsLit, Lit dec = lit_Undef);

  
  vec<int> saveFree;
  vec<lbool> currentModel;
  void computeBackBone();
  void computeBackBone(vec<Var> &v);
       

  ////////////////////////// Connected component ///////////////////////////////////////////
  vec<int> inTheHeap;
  int stampInTheHeap;
  inline bool isInTheHeap(Var v){return inTheHeap[v] == stampInTheHeap;}

  vec<int> mustUnMark;
  inline void resetUnMark()
  {
    for(int i = 0 ; i<mustUnMark.size() ; i++) ca[clauses[mustUnMark[i]]].markView(0); 
    mustUnMark.setSize(0);      
  }// resetUnMark
    
  inline void rebuildWithConnectedComponent(vec<Var> &v)
  {
    stampInTheHeap++;
    for(int j = 0 ; j<v.size() ; j++) inTheHeap[v[j]] = stampInTheHeap;
    order_heap.build(v);
  }// rebuidWithConnectedComponent

  void connectedToLit(Lit l, vec<int> &v, vec<Var> &varComponent, int nbComponent);
    
    
  int isTautologie;
  inline bool getIsTautologie(){return isTautologie;}

  ////////////////////////// Show information part /////////////////////////////////////
  bool showDebug;

  inline void removePhantomTrace()
  {
    removePhantomTrail();
    removePhantomClausesFromLearnt();
    removePhantom();    
  }

  inline void removeLearnt()
  {      
    int i, j;
    for (i = j = 0 ; i < learnts.size() ; i++)
      {
        Clause& c = ca[learnts[i]];
        if (!locked(c)) removeClause(learnts[i], true);
        else learnts[j++] = learnts[i];
      }
      
    learnts.shrink(i - j);
    checkGarbage();
  }// removeLearnt


  inline void removePhantomClausesFromLearnt()
  {
    int i, j;
    for (i = j = 0 ; i < learnts.size() ; i++)
      {
        Clause& c = ca[learnts[i]];

        // check if c contains a phantomLit
        bool phantomClause = false;
        for(int k = 0 ; !phantomClause && k<c.size() ; k++) phantomClause = c[k] == ~phantomLit; 
        
        if (phantomClause)
          {
            assert(!locked(c));
            removeClause(learnts[i], true);
          }
        else learnts[j++] = learnts[i];
      }
      
    learnts.shrink(i - j);
    checkGarbage();
  }// removePhantomClausesFromLearnt

  inline void removePhantomTrail()
  {
    cancelUntil(0);

    int i, j;
    for (i = j = 0; i<trail.size() ; i++)
      {
        Var x = var(trail[i]);
        if(x == var(phantomLit))
          {            
            assigns[x] = l_Undef;
            insertVarOrder(x);
          }
        else trail[j++] = trail[i];
      }
            
    trail.shrink(i - j);      
    if(trail_lim.size()) trail_lim[0] = trail.size();
  }// removePhantomTrail
    
  inline void removePhantom()
  {
    for (int i = 0 ; i < phantoms.size() ; i++)
      {
        assert(!locked(ca[phantoms[i]]));
        removeClause(phantoms[i], true);
      }
      
    phantoms.clear();
    checkGarbage();
  }// removePhantom

    
  /**
     Cancel until zero and remove the additional unit literal such
     that trail[i] with i >= szt

     @param[in] szt, the 'nex' trail size
  */
  inline void cancelUntilOldZeroLevelTrail(int szt)
  {
    cancelUntil(0);
    assert(szt <= trail.size());

    for (int c = trail.size()-1; c >= szt ; c--)
      {
        Var x = var(trail[c]);         
        assigns[x] = l_Undef;
        insertVarOrder(x); 
      }
            
    qhead = szt;
    trail.shrink(trail.size() - szt);
      
    if(trail_lim.size()) trail_lim[0] = szt;
  }// cancelUntilOldZeroLevelTrail


  /**
     Add a phantom clause
       
     @param[in] ps, a set of literals.

     \return true if the clause has been added, false otherwise.
  */
  inline bool addPhantomClause(vec<Lit>& ps)
  {
    assert(decisionLevel() == 0);    
    ps.push(~phantomLit);
    assert(ps.size() > 1);
      
    // Check if clause is satisfied and remove false/duplicate literals:
    Lit p; int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
      if (value(ps[i]) == l_True || ps[i] == ~p) return true;
      else if (value(ps[i]) != l_False && ps[i] != p) ps[j++] = p = ps[i];
    ps.shrink(i - j);
  
    if (ps.size() == 0) return ok = false;
    else if (ps.size() == 1)
      {
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == CRef_Undef);
      }else
      {
        CRef cr = ca.alloc(ps, false);
        phantoms.push(cr);
        attachClause(cr);
      }
      
    return true;
  }// addPhantomClause



  inline void showTrail()
  {
    printf("--> %d: ", decisionLevel());
    for(int i = 0 ; i<trail.size() ; i++)
      {
        printf("%d(%d) ", readableLit(trail[i]), level(var(trail[i])));
	assert(level(var(trail[i])) <= decisionLevel());
        if(0 && reason(var(trail[i])) != CRef_Undef)
          {
            printf("%d ", ca[reason(var(trail[i]))].learnt());
            ca[reason(var(trail[i]))].showClause();
          }
        //else printf("\n");
      }
    printf("\n");
  }
       
  inline void showSimplifiedClause(Clause &c)
  {
    if(clauseIsSAT(c)){printf("0\n"); return;}
    for(int i = 0 ; i<c.size() ; i++)
      if(value(c[i]) == l_Undef) printf("%d ", readableLit(c[i]));
    printf("0\n");
  }// showSimplifiedClause

  inline void showAttachedFormula()
  {
    printf("-> showAttachedFormula\n");
    printf("clauses: \n");
    for(int i = 0 ; i<clauses.size() ; i++)
      if(ca[clauses[i]].attached() && !clauseIsSAT(ca[clauses[i]]))
        {
          // printf("%d(%d): ", i, ca[clauses[i]].size());
          showSimplifiedClause(ca[clauses[i]]);
        }

    printf("phantoms: \n");
    for(int i = 0 ; i<phantoms.size() ; i++)
      if(ca[phantoms[i]].attached() && !clauseIsSAT(ca[phantoms[i]]))
        {
          printf("%d(%d): ", i, ca[phantoms[i]].size()); showSimplifiedClause(ca[phantoms[i]]);
        }
    printf("---\n");

    printf("learnts: \n");
    for(int i = 0 ; i<learnts.size() ; i++)
      if(ca[learnts[i]].attached() && !clauseIsSAT(ca[learnts[i]]))
        {
          printf("%d(%d): ", i, ca[learnts[i]].size()); showSimplifiedClause(ca[learnts[i]]);
        }
    printf("---\n");
  }

  inline void showLearntClauses()
  {
    printf("learnt clauses\n");
    for(int i = 0 ; i<learnts.size() ; i++)
      showSimplifiedClause(ca[learnts[i]]);      
    printf("---\n");
  }

  inline void showSimplifiedFormula()
  {
    for(int i = 0 ; i<clauses.size() ; i++)
      {
        Clause &c = ca[clauses[i]];          
        if(clauseIsSAT(c)) continue;
        showSimplifiedClause(c);          
      }       
  }// showSimplifiedFormula

  inline void showCurrentConnectedFormula()
  {
    //printf("showCurrentConnectedFormula %d\n", stampInTheHeap);
    //for(int i = 0 ; i<inTheHeap.size() ; i++) printf("%d => %d\n", i + 1, inTheHeap[i]);
      
    for(int i = 0 ; i<clauses.size() ; i++)
      {
        Clause &c = ca[clauses[i]];
        if(clauseIsSAT(c) || !c.attached() || (inTheHeap[var(c[0])] != stampInTheHeap)) continue;
        printf("%d(%d): ", i, c.size());
        showSimplifiedClause(c);
      } 
  }// showCurrentConnectedFormula


  inline void showAttachedClause()
  {
    for(int i = 0 ; i<nVars() ; i++)
      {
        for(int phase = 0 ; phase < 2 ; phase++)
          { 
            Lit p = mkLit(i, phase);
            printf("clause attached to %d\n", readableLit(p));

            vec<Watcher>&  ws  = watches[p];
            Watcher *i, *end;
      
            for (i = (Watcher*) ws, end = i + ws.size();  i != end; i++)
              {
                Clause &c = ca[i->cref];
                c.showClause();
              }
            printf("--------------------\n");
          }
      }
  }// showAttachedClause


  void showFormulaDotDual(vec<Var> &setOfVar, vec<int> &cutSet);
  void showFormulaDotPrimal(vec<Var> &setOfVar, vec<Var> &priority, ScoringMethod *sm);
  void showFormulaDotIncidence(vec<Var> &setOfVar, vec<Var> &priority, ScoringMethod *sm);
  void showHyperGraph(vec<Var> &setOfVar);
    
  ////////////////////////// Stuff part /////////////////////////////////////    
    
  inline void showDiff(vec<Lit> &v1, vec<Lit> &v2)
  {
    if(v1.size() != v2.size()) return;
    bool showPrintf = false;
    for(int i = 0 ; i<v1.size() ; i++) 
      if(v1[i] != v2[i]){printf("(%d -- %d)", readableLit(v1[i]), readableLit(v2[i]));showPrintf = true;}
    if(showPrintf) printf("\n---------\n");
  }// showDiff

        
  inline void definedBlockedSymVariable(vec<int> &c)
  {
    for(int i = 0 ; i<c.size() ; i++)
      {
        Var x = (c[i] > 0) ? c[i] - 1 : -(c[i] + 1);
        kindOfVariable[x] = 'n';
      }
  }// definedBlockedSymVariable


  // additional stuff
  inline void intToLit(vec<int> &c, vec<Lit> &ls)
  {
    ls.clear();
    for(int i = 0 ; i<c.size() ; i++)
      {
        Var x = (c[i] > 0) ? c[i] - 1 : -(c[i] + 1);
        ls.push(mkLit(x, c[i] < 0));
      }
  }// intToLit

 };


//=================================================================================================
// Implementation of inline methods:


inline CRef Solver::reason(Var x) const { return vardata[x].reason; }
inline int  Solver::level (Var x) const { return vardata[x].level; }

inline void Solver::insertVarOrder(Var x) {
  if (!order_heap.inHeap(x) && decision[x] && isInTheHeap(x)) order_heap.insert(x); }

inline void Solver::varDecayActivity() { var_inc *= (1 / var_decay); }
inline void Solver::varBumpActivity(Var v) { varBumpActivity(v, var_inc); }
inline void Solver::varBumpActivity(Var v, double inc) {
  scoreActivity[v] += 1;
  if ( (activity[v] += inc) > 1e100 ) {
    // Rescale:
    for (int i = 0; i < nVars(); i++)
      activity[i] *= 1e-100;
    var_inc *= 1e-100; }

  // Update order_heap with respect to new activity:
  if (order_heap.inHeap(v))
    order_heap.decrease(v); }

inline void Solver::claDecayActivity() { cla_inc *= (1 / clause_decay); }
inline void Solver::claBumpActivity (Clause& c) {
  if ( (c.activity() += cla_inc) > 1e20 ) {
    // Rescale:
    for (int i = 0; i < learnts.size(); i++)
      ca[learnts[i]].activity() *= 1e-20;
    cla_inc *= 1e-20; } }

inline void Solver::checkGarbage(void){ return checkGarbage(garbage_frac); }
inline void Solver::checkGarbage(double gf){
  if (ca.wasted() > ca.size() * gf)
    garbageCollect(); }

// NOTE: enqueue does not set the ok flag! (only public methods do)
inline bool     Solver::enqueue         (Lit p, CRef from)      { return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true); }
inline bool     Solver::addClause       (const vec<Lit>& ps)    { ps.copyTo(add_tmp); return addClause_(add_tmp); }
inline bool     Solver::addEmptyClause  ()                      { add_tmp.clear(); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p)                 { add_tmp.clear(); add_tmp.push(p); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q)          { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q, Lit r)   { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); add_tmp.push(r); return addClause_(add_tmp); }
inline bool     Solver::locked          (const Clause& c) const { return value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c; }
inline void     Solver::newDecisionLevel()                      { trail_lim.push(trail.size()); }

inline int      Solver::decisionLevel ()      const   { return trail_lim.size(); }
inline uint32_t Solver::abstractLevel (Var x) const   { return 1 << (level(x) & 31); }
    
inline lbool          Solver::value          (Var x)  const  { return assigns[x]; }
inline lbool          Solver::value          (Lit p)  const  { return assigns[var(p)] ^ sign(p); }
inline lbool          Solver::modelValue     (Var x)  const  { return model[x]; }
inline lbool          Solver::modelValue     (Lit p)  const  { return model[var(p)] ^ sign(p); }
inline int            Solver::nAssigns       ()       const  { return trail.size(); }
inline int            Solver::nClauses       ()       const  { return clauses.size(); }
inline const Clause&  Solver::getClause      (int i)  const  { return ca[clauses[i]]; }
inline int            Solver::nLearnts       ()       const  { return learnts.size(); }
inline int            Solver::nVars          ()       const  { return vardata.size(); }
inline int            Solver::nFreeVars      ()       const  { return (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]); }
inline void           Solver::setPolarity    (Var v, bool b) { polarity[v] = b; }
inline void           Solver::setDecisionVar (Var v, bool b)
{ 
  if      ( b && !decision[v]) dec_vars++;
  else if (!b &&  decision[v]) dec_vars--;

  decision[v] = b;
  insertVarOrder(v);
}
inline void     Solver::setConfBudget(int64_t x){ conflict_budget    = conflicts    + x; }
inline void     Solver::setPropBudget(int64_t x){ propagation_budget = propagations + x; }
inline void     Solver::interrupt(){ asynch_interrupt = true; }
inline void     Solver::clearInterrupt(){ asynch_interrupt = false; }
inline void     Solver::budgetOff(){ conflict_budget = propagation_budget = -1; }
inline bool     Solver::withinBudget() const {
  return !asynch_interrupt &&
    (conflict_budget    < 0 || conflicts < (uint64_t)conflict_budget) &&
                          (propagation_budget < 0 || propagations < (uint64_t)propagation_budget); }

// FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
// pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
// all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
inline bool Solver::solveWithAssumptions ()
{
  budgetOff();
  bool ret = solve_(false) == l_True;
  return ret;
}// solveWithAssumptions


inline bool Solver::solveWithAssumptions (vec<Lit> &assums, bool model)
{
  // prepare the assumption
  cancelUntil(0);
  assumptions.clear();
  assums.copyTo(assumptions);

  // solve the problem and return the solution (maybe save it w.r.t. the value of model)
  bool saveNeedModel = needModel;
  setNeedModel(model);
  bool ret = solveWithAssumptions();
  setNeedModel(saveNeedModel);
  cancelUntil(0);
  return ret;
}// solveWithAssumptions


inline bool Solver::solveWithAssumptions (vec<Lit> &assums, vec<Lit> &units, bool model)
{
  // prepare the assumption
  cancelUntil(0);
  assumptions.clear();
  assums.copyTo(assumptions);

  // solve the problem and return the solution (maybe save it w.r.t. the value of model)
  bool saveNeedModel = needModel;
  setNeedModel(model);
  bool ret = solveWithAssumptions();
  setNeedModel(saveNeedModel);

  // collect the unit literals
  units.clear();
  int limitTrail = (assums.size() && assums.size() < trail_lim.size()) ? trail_lim[assums.size()] : trail.size();  
  for(int i = 0 ; ret && i<limitTrail ; i++) units.push(trail[i]);
    
  cancelUntil(0);
  return ret;
}// solveWithAssumptions


inline bool     Solver::solve   ()   { budgetOff(); assumptions.clear(); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p)               { budgetOff(); assumptions.clear(); assumptions.push(p); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q)        { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q, Lit r) { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); assumptions.push(r); return solve_() == l_True; }
inline bool     Solver::solve         (const vec<Lit>& assumps){ budgetOff(); assumps.copyTo(assumptions); return solve_() == l_True; }
inline lbool    Solver::solveLimited  (const vec<Lit>& assumps){ assumps.copyTo(assumptions); return solve_(); }
inline lbool    Solver::solveLimited  (const vec<Lit>& assumps, int nbConflict)
{ assumps.copyTo(assumptions); return solve_(false, nbConflict); }
 
inline bool     Solver::okay          ()      const   { return ok; }
 
inline void     Solver::toDimacs     (const char* file){ vec<Lit> as; toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p){ vec<Lit> as; as.push(p); toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p, Lit q){ vec<Lit> as; as.push(p); as.push(q); toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p, Lit q, Lit r){ vec<Lit> as; as.push(p); as.push(q); as.push(r); toDimacs(file, as); }
 

//=================================================================================================
// Debug etc:
 
#endif 
