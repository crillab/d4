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
/***************************************************************************************[Solver.cc]
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

#include <iostream>
#include <math.h>

#include "../utils/System.hh"
#include "../utils/Options.hh"
#include "../utils/SolverTypes.hh"
#include "../mtl/Sort.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Heap.hh"
#include "../mtl/Alg.hh"
#include "../DAG/DAG.hh"

#include "Solver.hh"

using namespace std;

//=================================================================================================
// Options:


static const char* _cat = "CORE";

static DoubleOption opt_var_decay (_cat, "var-decay",   "The variable activity decay factor",            0.95,     DoubleRange(0, false, 1, false));
static DoubleOption  opt_clause_decay (_cat, "cla-decay",   "The clause activity decay factor",              0.999,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_random_var_freq   (_cat, "rnd-freq",    "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption  opt_random_seed       (_cat, "rnd-seed",    "Used by the random variable selection",         91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_ccmin_mode        (_cat, "ccmin-mode",  "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption     opt_phase_saving      (_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption    opt_rnd_init_act      (_cat, "rnd-init",    "Randomize the initial activity", false);
static BoolOption    opt_luby_restart      (_cat, "luby",        "Use the Luby restart sequence", true);
static IntOption     opt_restart_first     (_cat, "rfirst",      "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption  opt_restart_inc       (_cat, "rinc",        "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption  opt_garbage_frac      (_cat, "gc-frac",     "The fraction of wasted memory allowed before a garbage collection is triggered",  0.20, DoubleRange(0, false, HUGE_VAL, false));


//=================================================================================================
// Constructor/Destructor:


Solver::Solver(ostream *certif) :
    // Parameters (user settable):
    //
    cert(certif)
    , verbosity        (0)
    , var_decay        (opt_var_decay)
    , clause_decay     (opt_clause_decay)
    , random_var_freq  (opt_random_var_freq)
    , random_seed      (opt_random_seed)
    , luby_restart     (opt_luby_restart)
    , ccmin_mode       (opt_ccmin_mode)
    , phase_saving     (opt_phase_saving)
    , rnd_pol          (false)
    , rnd_init_act     (opt_rnd_init_act)
    , garbage_frac     (opt_garbage_frac)
    , restart_first    (opt_restart_first)
    , restart_inc      (opt_restart_inc)

    // Parameters (the rest):
    //
    , learntsize_factor((double)1/(double)3), learntsize_inc(1.1)

    // Parameters (experimental):
    , learntsize_adjust_start_confl (100)
    , learntsize_adjust_inc         (1.5)

    // Statistics: (formerly in 'SolverStats')
    , solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
    , dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)

    , ok                 (true)
    , cla_inc            (1)
    , var_inc            (1)
    , watches            (WatcherDeleted(ca))
    , qhead              (0)
    , simpDB_assigns     (-1)
    , simpDB_props       (0)
    , order_heap         (VarOrderLt(activity))
    , progress_estimate  (0)
    , remove_satisfied   (true)

    // Resource constraints:
    //
    , conflict_budget    (-1)
    , propagation_budget (-1)
    , asynch_interrupt   (false)
{
  limTrailNbSatUns = stampInTheHeap = 0;
  occGtThreeInit = needModel = showDebug = false;
  phantomMode = false;
  phantomLit = lit_Undef;
  idxClausesCpt = 0;
}

Solver::~Solver()
{
}


//=================================================================================================
// Minor methods:


/**
   Creates a new SAT variable in the solver. If 'decision' is cleared,
   variable will not be used as a decision variable (NOTE! This has
   effects on the meaning of a SATISFIABLE result).

   @param[in] sign,
   @param[in] dvar,

   \return the new variable
*/
Var Solver::newVar(bool sign, bool dvar)
{
  int v = nVars();

  watches.init(mkLit(v, false));
  watches.init(mkLit(v, true ));

  assigns.push(l_Undef);
  vardata.push(mkVarData(CRef_Undef, 0));
  //activity .push(0);
  scoreActivity.push(0);
  activity.push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
  seen.push(0);
  polarity.push(sign);
  decision.push();
  trail.capacity(v+1);

  occGtThree.push(); occGtThree.push();

  inTheHeap.push(0); // must be added before
  setDecisionVar(v, dvar);
  defined.push(false);
  litFlags.push(lit_Undef);
  flags.push(0);

  alreadyConsidered.push(false);
  toUnConsidered.push(false);
  wasConsideredOcc.push(false);

  kindOfVariable.push('v');
  saveFree.push(0);
  currentModel.push(l_False);
  insistTruePolarity.push(false);

  return v;
}// newVar


bool Solver::addClause_(vec<Lit>& ps)
{
  assert(decisionLevel() == 0);
  if (!ok) return false;

  isTautologie = false;

  // Check if clause is satisfied and remove false/duplicate literals:
  sort(ps);
  vec<Lit> oc;

  Lit p; int i, j, flag = 0;
  for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
  {
    oc.push(ps[i]);
    if (value(ps[i]) == l_True || ps[i] == ~p || value(ps[i]) == l_False) flag = 1;
  }

  for (i = j = 0, p = lit_Undef; i< ps.size(); i++)
  {
    if (value(ps[i]) == l_True || ps[i] == ~p)
    {
      isTautologie = true;
      return true;
    }
    else if (value(ps[i]) != l_False && ps[i] != p) ps[j++] = p = ps[i];
  }

  ps.shrink(i - j);

  if ((cert != nullptr))
  {
    ostream &cval = *cert;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
      cval << (var(ps[i]) + 1)*(-2 * sign(ps[i]) + 1) << " ";
    cval << "0\nd ";
    for (i = j = 0, p = lit_Undef; i< oc.size(); i++)
      cval << (var(oc[i]) + 1)*(-2 * sign(oc[i]) + 1) << " ";
    cval << "0\n";
#if 0
    printf("clause idx %d : ", idxClausesCpt);
    vec<Lit> &cl = ps;
    for(int j = 0 ; j<cl.size() ; j++)
      printf("%s%d ", sign(cl[j]) ? "-" : "", var(cl[j]) + 1);
    printf("\n");
#endif

    idxClausesCpt++;
  }

  if (ps.size() == 0) return ok = false;
  else if (ps.size() == 1)
  {
    uncheckedEnqueue(ps[0]);
    CRef ref = propagate();

    if(cert != nullptr && ref != CRef_Undef)
    {
      ostream &cval = *cert;
      cval << (var(ca[ref][0]) + 1)*(-2 * sign(ca[ref][0]) + 1) << " 0\n";
    }

    return ok = (ref == CRef_Undef);
  }else
  {
    CRef cr = ca.alloc(ps, false);
    clauses.push(cr);
    attachClause(cr);
    ca[cr].idxReason(idxClausesCpt);
  }

  return true;
}// addClause_


void Solver::collectUnit(vec<Var> &setOfVar, vec<Lit> &unitsLit, Lit dec)
{
  static int cpt = 0;
  unitsLit.clear();
  if(dec != lit_Undef) unitsLit.push(dec);

  for(int i = 0 ; i<setOfVar.size() ; i++)
  {
    Var v = setOfVar[i];
    if(dec != lit_Undef && var(dec) == v) continue;
    if(value(v) != l_Undef)
    {
      unitsLit.push(mkLit(v, value(v) == l_False));
    }
  }
}// collectUnit



void Solver::attachClause(CRef cr)
{
  Clause& c = ca[cr];
  assert(!c.attached());
  c.attached(1);
  assert(c.size() > 1);
  watches[~c[0]].push(Watcher(cr, c[1]));
  watches[~c[1]].push(Watcher(cr, c[0]));
  if (c.learnt()) learnts_literals += c.size(); else clauses_literals += c.size();
}// attachClause


void Solver::detachClause(CRef cr, bool strict)
{
  Clause& c = ca[cr];
  assert(c.attached());
  c.attached(0);
  assert(c.size() > 1);

  if (strict)
  {
    remove(watches[~c[0]], Watcher(cr, c[1]));
    remove(watches[~c[1]], Watcher(cr, c[0]));
  }else
  {
    // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
    watches.smudge(~c[0]);
    watches.smudge(~c[1]);
  }

  if (c.learnt()) learnts_literals -= c.size(); else clauses_literals -= c.size();
}


void Solver::removeClause(CRef cr, bool strict)
{
  Clause& c = ca[cr];

  if (cert != nullptr)
  {
    ostream &cval = *cert;
    cval << "d ";
    c.showClause(cval);
  }
  detachClause(cr, strict);

  // Don't leave pointers to free'd memory!
  if (locked(c)) vardata[var(c[0])].reason = CRef_Undef;
  c.mark(1);
  ca.free(cr);
}

void Solver::removeNotAttachedClause(CRef cr)
{
  Clause& c = ca[cr];

  // Don't leave pointers to free'd memory!
  if (locked(c)) vardata[var(c[0])].reason = CRef_Undef;
  c.mark(1);
  ca.free(cr);
}



bool Solver::satisfied(const Clause& c) const
{
  for (int i = 0; i < c.size(); i++) if (value(c[i]) == l_True) return true;
  return false;
}// satisfied


/**
   Revert to the state at given level (keeping all assignment at
   'level' but not beyond).
*/
void Solver::cancelUntil(int lev)
{
  if (decisionLevel() > lev)
  {
    for (int c = trail.size()-1; c >= trail_lim[lev]; c--)
    {
      Var x = var(trail[c]);
      // printf("toUnConsidered[%d] = %d\n", x + 1, wasConsideredOcc[x]);
      toUnConsidered[x] = wasConsideredOcc[x];

      assigns[x] = l_Undef;
      polarity[x] = sign(trail[c]);
      insertVarOrder(x);
    }
    qhead = trail_lim[lev];
    trail.shrink(trail.size() - trail_lim[lev]);
    trail_lim.shrink(trail_lim.size() - lev);
  }
}// cancelUntil


//=================================================================================================
// Major methods:


Lit Solver::pickBranchLit()
{
  Var next = var_Undef;

  // Activity based decision:
  while (next == var_Undef || value(next) != l_Undef || !decision[next] || !isInTheHeap(next))
  {
    if (order_heap.empty())
    {
      next = var_Undef;
      break;
    }else next = order_heap.removeMin();

    assert(next == var_Undef || isInTheHeap(next));
  }

  if(next != var_Undef && insistTruePolarity[next]) return mkLit(next, false);
  return next == var_Undef ? lit_Undef : mkLit(next, polarity[next]);
}// pickBranchLit


/**
   analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]

   Description:
   Analyze conflict and produce a reason clause.

   Pre-conditions:
   * 'out_learnt' is assumed to be cleared.
   * Current decision level must be greater than root level.

   Post-conditions:
   * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
   * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
   rest of literals. There may be others from the same level though.

*/
void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel)
{
  int pathC = 0;
  Lit p = lit_Undef;

  // Generate conflict clause:
  out_learnt.push();      // (leave room for the asserting literal)
  int index   = trail.size() - 1;

  do
  {
    assert(confl != CRef_Undef); // (otherwise should be UIP)
    Clause& c = ca[confl];

    if (c.learnt()) claBumpActivity(c);

    for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++)
    {
      Lit q = c[j];

      if (!seen[var(q)] && level(var(q)) > 0)
      {
        varBumpActivity(var(q));
        seen[var(q)] = 1;
        if (level(var(q)) >= decisionLevel()) pathC++; else out_learnt.push(q);
      }
    }

    // Select next clause to look at:
    while (!seen[var(trail[index--])]);
    p = trail[index+1];
    confl = reason(var(p));
    seen[var(p)] = 0;
    pathC--;
  }while (pathC > 0);
  out_learnt[0] = ~p;

  // Simplify conflict clause:
  //
  int i, j;
  out_learnt.copyTo(analyze_toclear);
  if (ccmin_mode == 2){
    uint32_t abstract_level = 0;
    for (i = 1; i < out_learnt.size(); i++)
      abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

    for (i = j = 1; i < out_learnt.size(); i++)
      if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
        out_learnt[j++] = out_learnt[i];

  }else if (ccmin_mode == 1)
  {
    for (i = j = 1; i < out_learnt.size(); i++){
      Var x = var(out_learnt[i]);

      if (reason(x) == CRef_Undef)
        out_learnt[j++] = out_learnt[i];
      else{
        Clause& c = ca[reason(var(out_learnt[i]))];
        for (int k = 1; k < c.size(); k++)
          if (!seen[var(c[k])] && level(var(c[k])) > 0){
            out_learnt[j++] = out_learnt[i];
            break; }
      }
    }
  }else i = j = out_learnt.size();

  max_literals += out_learnt.size();
  out_learnt.shrink(i - j);
  tot_literals += out_learnt.size();

  // Find correct backtrack level:
  //
  if (out_learnt.size() == 1)
    out_btlevel = 0;
  else{
    int max_i = 1;
    // Find the first literal assigned at the next-highest level:
    for (int i = 2; i < out_learnt.size(); i++)
      if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
        max_i = i;
    // Swap-in this literal at index 1:
    Lit p             = out_learnt[max_i];
    out_learnt[max_i] = out_learnt[1];
    out_learnt[1]     = p;
    out_btlevel       = level(var(p));
  }

  for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}


/**
   analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]

   Description:
   Analyze conflict and produce a reason clause.

   Pre-conditions:
   * 'out_learnt' is assumed to be cleared.
   * Current decision level must be greater than root level.

   Post-conditions:
   * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
   * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
   rest of literals. There may be others from the same level though.

*/
void Solver::analyzeLastUIP(CRef confl, vec<Lit>& out_learnt, int& out_btlevel)
{
  Lit p = lit_Undef;

  // Generate conflict clause:
  out_learnt.push();      // (leave room for the asserting literal)
  int index   = trail.size() - 1;

  do
  {
    assert(confl != CRef_Undef); // (otherwise should be UIP)
    Clause& c = ca[confl];

    if (c.learnt()) claBumpActivity(c);

    for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++)
    {
      Lit q = c[j];

      if (!seen[var(q)] && level(var(q)) > 0)
      {
        varBumpActivity(var(q));
        seen[var(q)] = 1;
        if (!(level(var(q)) >= decisionLevel())) out_learnt.push(q);
      }
    }

    // Select next clause to look at:
    while (!seen[var(trail[index--])]);
    p = trail[index+1];
    confl = reason(var(p));
    seen[var(p)] = 0;
  }while (confl != CRef_Undef);
  out_learnt[0] = ~p;

  // Simplify conflict clause:
  //
  int i, j;
  out_learnt.copyTo(analyze_toclear);
  if (ccmin_mode == 2){
    uint32_t abstract_level = 0;
    for (i = 1; i < out_learnt.size(); i++)
      abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

    for (i = j = 1; i < out_learnt.size(); i++)
      if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
        out_learnt[j++] = out_learnt[i];

  }else if (ccmin_mode == 1)
  {
    for (i = j = 1; i < out_learnt.size(); i++){
      Var x = var(out_learnt[i]);

      if (reason(x) == CRef_Undef)
        out_learnt[j++] = out_learnt[i];
      else{
        Clause& c = ca[reason(var(out_learnt[i]))];
        for (int k = 1; k < c.size(); k++)
          if (!seen[var(c[k])] && level(var(c[k])) > 0){
            out_learnt[j++] = out_learnt[i];
            break; }
      }
    }
  }else i = j = out_learnt.size();

  max_literals += out_learnt.size();
  out_learnt.shrink(i - j);
  tot_literals += out_learnt.size();

  // Find correct backtrack level:
  //
  if (out_learnt.size() == 1)
    out_btlevel = 0;
  else{
    int max_i = 1;
    // Find the first literal assigned at the next-highest level:
    for (int i = 2; i < out_learnt.size(); i++)
      if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
        max_i = i;
    // Swap-in this literal at index 1:
    Lit p             = out_learnt[max_i];
    out_learnt[max_i] = out_learnt[1];
    out_learnt[1]     = p;
    out_btlevel       = level(var(p));
  }

  for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
  analyze_stack.clear(); analyze_stack.push(p);
  int top = analyze_toclear.size();
  while (analyze_stack.size() > 0){
    assert(reason(var(analyze_stack.last())) != CRef_Undef);
    Clause& c = ca[reason(var(analyze_stack.last()))]; analyze_stack.pop();

    for (int i = 1; i < c.size(); i++){
      Lit p  = c[i];
      if (!seen[var(p)] && level(var(p)) > 0){
        if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0){
          seen[var(p)] = 1;
          analyze_stack.push(p);
          analyze_toclear.push(p);
        }else{
          for (int j = top; j < analyze_toclear.size(); j++)
            seen[var(analyze_toclear[j])] = 0;
          analyze_toclear.shrink(analyze_toclear.size() - top);
          return false;
        }
      }
    }
  }

  return true;
}// litRedundant


/*_________________________________________________________________________________________________
  |
  |  analyzeFinal : (p : Lit)  ->  [void]
  |
  |  Description:
  |    Specialized analysis procedure to express the final conflict in terms of assumptions.
  |    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
  |    stores the result in 'out_conflict'.
  |________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
  out_conflict.clear();
  out_conflict.push(p);

  if (decisionLevel() == 0) return;
  seen[var(p)] = 1;

  for (int i = trail.size()-1; i >= trail_lim[0]; i--)
  {
    Var x = var(trail[i]);
    if (seen[x])
    {
      if (reason(x) == CRef_Undef)
      {
        assert(level(x) > 0);
        out_conflict.push(~trail[i]);
      }else
      {
        Clause& c = ca[reason(x)];
        for (int j = 1; j < c.size(); j++)
          if (level(var(c[j])) > 0) seen[var(c[j])] = 1;
      }
      seen[x] = 0;
    }
  }
  seen[var(p)] = 0;
}// analyzeFinal


void Solver::uncheckedEnqueue(Lit p, CRef from)
{
  Var v = var(p);

  assert(value(p) == l_Undef);
  assigns[v] = lbool(!sign(p));
  vardata[v] = mkVarData(from, decisionLevel());
  trail.push_(p);

  if(!alreadyConsidered[v] && level(v) <= assumptions.size())
  {
    mustBeConsidered.push(p);
    alreadyConsidered[v] = true;
  }

  if(cert != nullptr && !decisionLevel())
  {
    idxClausesCpt++;
    ostream &cval = *cert;
    cval << (var(p) + 1)*(-2 * sign(p) + 1) << " 0\n";
  }
}// uncheckedEnqueue


/*_________________________________________________________________________________________________
  |
  |  propagate : [void]  ->  [Clause*]
  |
  |  Description:
  |    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
  |    otherwise CRef_Undef.
  |
  |    Post-conditions:
  |      * the propagation queue is empty, even if there was a conflict.
  |________________________________________________________________________________________________@*/
CRef Solver::propagate()
{
  CRef confl = CRef_Undef;
  int num_props = 0;
  watches.cleanAll();

  while (qhead < trail.size())
  {
    Lit p = trail[qhead++];     // 'p' is enqueued fact to propagate.

    vec<Watcher>&  ws  = watches[p];
    Watcher *i, *j, *end;
    num_props++;

    for (i = j = (Watcher*) ws, end = i + ws.size();  i != end;)
    {
      // Try to avoid inspecting the clause:
      Lit blocker = i->blocker;
      if (value(blocker) == l_True){*j++ = *i++; continue; }

      // Make sure the false literal is data[1]:
      CRef cr = i->cref;
      Clause& c = ca[cr];

      Lit false_lit = ~p;
      if (c[0] == false_lit) c[0] = c[1], c[1] = false_lit;
      assert(c[1] == false_lit);
      i++;

      // If 0th watch is true, then clause is already satisfied.
      Lit     first = c[0];
      Watcher w     = Watcher(cr, first);
      if (first != blocker && value(first) == l_True){*j++ = w; continue;}

      // Look for new watch:
      for (int k = 2; k < c.size(); k++)
        if (value(c[k]) != l_False)
        {
          c[1] = c[k]; c[k] = false_lit;
          watches[~c[1]].push(w);
          goto NextClause;
        }

      // Did not find watch -- clause is unit under assignment:
      *j++ = w;
      if (value(first) == l_False)
      {
        confl = cr;
        qhead = trail.size();
        // Copy the remaining watches:
        while (i < end) *j++ = *i++;
      }else uncheckedEnqueue(first, cr);

   NextClause:;
    }
    ws.shrink(i - j);
  }

  propagations += num_props;
  simpDB_props -= num_props;

  return confl;
}// propagate


/**

   reduceDB : ()  ->  [void]

   Description:
   Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
   clauses are clauses that are reason to some assignment. Binary clauses are never removed.
*/
struct reduceDB_lt
{
  ClauseAllocator& ca;
  reduceDB_lt(ClauseAllocator& ca_) : ca(ca_) {}
  bool operator () (CRef x, CRef y)
  {
    return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity());
  }
};
void Solver::reduceDB()
{
  int     i, j;
  double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity

  sort(learnts, reduceDB_lt(ca));
  // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
  // and clauses with activity smaller than 'extra_lim':
  for (i = j = 0 ; i < learnts.size() ; i++)
  {
    Clause& c = ca[learnts[i]];
    if(c.size() > 2 && !locked(c)&&(i < learnts.size() / 2 || c.activity() < extra_lim))
    {
      removeClause(learnts[i]);
    }
    else learnts[j++] = learnts[i];
  }
  learnts.shrink(i - j);
  checkGarbage();
}// reduceDB


void Solver::removeSatisfied(vec<CRef>& cs)
{
  int i, j;
  for (i = j = 0; i < cs.size(); i++)
  {
    Clause& c = ca[cs[i]];
    if (satisfied(c))	removeClause(cs[i]);
    else cs[j++] = cs[i];
  }
  cs.shrink(i - j);
}// removeSatisfied


void Solver::rebuildOrderHeap()
{
  vec<Var> vs;
  for (Var v = 0; v < nVars(); v++)
    if(decision[v] && value(v) == l_Undef && inTheHeap[v] == stampInTheHeap) vs.push(v);
  order_heap.build(vs);
}// rebuildOrderHeap


/**
   simplify : [void]  ->  [bool]

   Description:
   Simplify the clause database according to the current top-level assigment. Currently, the only
   thing done here is the removal of satisfied clauses, but more things can be put here.
*/
bool Solver::simplify()
{
  assert(decisionLevel() == 0);

  if (!ok || propagate() != CRef_Undef) return ok = false;
  if (nAssigns() == simpDB_assigns || (simpDB_props > 0)) return true;

  // Remove satisfied clauses:
  removeSatisfied(learnts);
  if(remove_satisfied) removeSatisfied(clauses);  // Can be turned off.

  checkGarbage();
  rebuildOrderHeap();

  simpDB_assigns = nAssigns();
  simpDB_props   = clauses_literals + learnts_literals;//(shouldn't depend on stats really, but it will do for now)

  return true;
}// simplify


/**

   search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]

   Description:
   Search for a model the specified number of conflicts.
   NOTE! Use negative value for 'nof_conflicts' indicate infinity.

   Output:
   'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
   all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
   if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
*/
lbool Solver::search(int nof_conflicts)
{
  assert(ok);
  int         backtrack_level;
  int         conflictC = 0;
  vec<Lit>    learnt_clause;
  starts++;

  for (;;)
  {
    CRef confl = propagate();

    if (confl != CRef_Undef) // CONFLICT
    {
      conflicts++; conflictC++;
      if (decisionLevel() == 0)
      {
        // we add a reason for the conflict, because it cannot exist
        if (cert != nullptr)
        {
          ostream &cval = *cert;
          cval << (var(ca[confl][0]) + 1)*(-2 * sign(ca[confl][0]) + 1) << " 0\n";
        }
        return l_False;
      }

      learnt_clause.clear();
      analyze(confl, learnt_clause, backtrack_level);
      cancelUntil(backtrack_level);

      if (cert != nullptr)
      {
        ostream &cval = *cert;
        for (int i = 0; i < learnt_clause.size(); i++)
          cval << (var(learnt_clause[i]) + 1)*(-2 * sign(learnt_clause[i]) + 1) << " ";
        cval << "0\n";
#if 0
        printf("learnt idx %d : ", idxClausesCpt);
        vec<Lit> &cl = learnt_clause;
        for(int j = 0 ; j<cl.size() ; j++)
          printf("%s%d ", sign(cl[j]) ? "-" : "", var(cl[j]) + 1);
        printf("\n");
#endif
      }

      idxClausesCpt++;
      if (learnt_clause.size() == 1)
      {
        cancelUntil(0);
        uncheckedEnqueue(learnt_clause[0]);
      }
      else
      {
        CRef cr = ca.alloc(learnt_clause, true);
        learnts.push(cr);
        attachClause(cr);
        claBumpActivity(ca[cr]);
        uncheckedEnqueue(learnt_clause[0], cr);
        ca[cr].idxReason(idxClausesCpt);
      }

      varDecayActivity();
      claDecayActivity();

      if (--learntsize_adjust_cnt == 0)
      {
        learntsize_adjust_confl *= learntsize_adjust_inc;
        learntsize_adjust_cnt = (int)learntsize_adjust_confl;
        max_learnts *= learntsize_inc;
      }
    }else // NO CONFLICT
    {
      if(decisionLevel() > assumptions.size() && nof_conflicts >= 0 &&
         (conflictC >= nof_conflicts || !withinBudget()))
      {
        // Reached bound on number of conflicts:
        progress_estimate = progressEstimate();
        cancelUntil(assumptions.size());
        return l_Undef;
      }

      if (decisionLevel() == 0 && !simplify()) return l_False;         // Simplify the set of problem clauses:
      if (decisionLevel() >= assumptions.size() && learnts.size() - nAssigns() >= max_learnts) reduceDB();

      Lit next = lit_Undef;
      while (decisionLevel() < assumptions.size()) // Perform user provided assumption:
      {
        Lit p = assumptions[decisionLevel()];

        if (value(p) == l_True) newDecisionLevel();
        else if (value(p) == l_False)
        {
          analyzeFinal(~p, conflict);
          if(cert != nullptr)
          {

            ostream &cval = *cert;
            for (int i = 0; i < conflict.size(); i++)
              cval << (var(conflict[i]) + 1)*(-2 * sign(conflict[i]) + 1) << " ";
            cval << "0\n";
          }
          idxClausesCpt++;
          if (conflict.size() == 1)
          {
            cancelUntil(0);
            idxReasonFinal = -1;
            if(decisionLevel()) uncheckedEnqueue(conflict[0]);
          }
          else
          {
            CRef cr = ca.alloc(conflict, true);
            learnts.push(cr);
            attachClause(cr);
            claBumpActivity(ca[cr]);
            ca[cr].idxReason(idxClausesCpt);
            idxReasonFinal = ca[cr].idxReason();
          }
          return l_False;
        } else{next = p; break;}
      }

      if (next == lit_Undef) // New variable decision:
      {
        decisions++;
        next = pickBranchLit();
        if (next == lit_Undef) return l_True; // Model found:
      }

      // Increase decision level and enqueue 'next'
      newDecisionLevel();
      uncheckedEnqueue(next);
    }
  }
}// search


double Solver::progressEstimate() const
{
  double  progress = 0;
  double  F = 1.0 / nVars();

  for (int i = 0; i <= decisionLevel(); i++)
  {
    int beg = i == 0 ? 0 : trail_lim[i - 1];
    int end = i == decisionLevel() ? trail.size() : trail_lim[i];
    progress += pow(F, i) * (end - beg);
  }

  return progress / nVars();
}

/*
  Finite subsequences of the Luby-sequence:

  0: 1
  1: 1 1 2
  2: 1 1 2 1 1 2 4
  3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
  ...

*/
static double luby(double y, int x)
{
  // Find the finite subsequence that contains index 'x', and the size of that subsequence:
  int size, seq;
  for (size = 1, seq = 0; size < x+1; seq++, size = 2*size+1);

  while (size-1 != x)
  {
    size = (size-1)>>1;
    seq--;
    x = x % size;
  }
  return pow(y, seq);
}// luby


// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_(bool rebuildHeap, int nbConflict)
{
  model.clear();
  conflict.clear();
  if (!ok) return l_False;

  if(rebuildHeap) rebuildOrderHeap();
  solves++;

  if(solves == 1)
  {
    max_learnts = nClauses() * learntsize_factor;
    learntsize_adjust_confl = learntsize_adjust_start_confl;
    learntsize_adjust_cnt = (int)learntsize_adjust_confl;
  }
  lbool status = l_Undef;

  // Search:
  int curr_restarts = 0;
  uint64_t initConflicts = conflicts;

  while (status == l_Undef && (!nbConflict || (conflicts - initConflicts < (uint64_t) nbConflict)))
  {
    double rest_base = luby_restart ? luby(restart_inc, curr_restarts) : pow(restart_inc, curr_restarts);
    status = nbConflict ? search(nbConflict) : search(rest_base * restart_first);
    if (!withinBudget()) break;
    curr_restarts++;
  }

  if(needModel && status == l_True)
  {
    // Extend & copy model:
    model.growTo(nVars());
    for (int i = 0; i < nVars(); i++) model[i] = value(i);
  }

  cancelUntil(assumptions.size());
  return status;
}// solve_

//=================================================================================================
// Writing CNF to DIMACS:
//
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max)
{
  if (map.size() <= x || map[x] == -1){
    map.growTo(x+1, -1);
    map[x] = max++;
  }
  return map[x];
}


void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max)
{
  if (satisfied(c)) return;

  for (int i = 0; i < c.size(); i++)
    if (value(c[i]) != l_False)
      fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max)+1);
  fprintf(f, "0\n");
}


void Solver::toDimacs(const char *file, const vec<Lit>& assumps)
{
  FILE* f = fopen(file, "wr");
  if (f == NULL)
    fprintf(stderr, "could not open file %s\n", file), exit(1);
  toDimacs(f, assumps);
  fclose(f);
}


void Solver::toDimacs(FILE* f, const vec<Lit>& assumps)
{
  // Handle case when solver is in contradictory state:
  if (!ok){
    fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
    return; }

  vec<Var> map; Var max = 0;

  // Cannot use removeClauses here because it is not safe
  // to deallocate them at this point. Could be improved.
  int cnt = 0;
  for (int i = 0; i < clauses.size(); i++)
    if (!satisfied(ca[clauses[i]]))
      cnt++;

  for (int i = 0; i < clauses.size(); i++)
    if (!satisfied(ca[clauses[i]])){
      Clause& c = ca[clauses[i]];
      for (int j = 0; j < c.size(); j++)
        if (value(c[j]) != l_False)
          mapVar(var(c[j]), map, max);
    }

  // Assumptions are added as unit clauses:
  cnt += assumptions.size();

  fprintf(f, "p cnf %d %d\n", max, cnt);

  for (int i = 0; i < assumptions.size(); i++){
    assert(value(assumptions[i]) != l_False);
    fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max)+1);
  }

  for (int i = 0; i < clauses.size(); i++)
    toDimacs(f, ca[clauses[i]], map, max);

  if (verbosity > 0)
    printf("Wrote %d clauses with %d variables.\n", cnt, max);
}


//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to)
{
  // All watchers:
  // for (int i = 0; i < watches.size(); i++)
  watches.cleanAll();
  for(int v = 0; v < nVars(); v++)
    for (int s = 0; s < 2; s++)
    {
      Lit p = mkLit(v, s);
      // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
      vec<Watcher>& ws = watches[p];
      for (int j = 0; j < ws.size(); j++) ca.reloc(ws[j].cref, to);
    }

  // All reasons:
  //
  for (int i = 0; i < trail.size(); i++)
  {
    Var v = var(trail[i]);
    if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
      ca.reloc(vardata[v].reason, to);
  }

  for (int i = 0; i < learnts.size(); i++) ca.reloc(learnts[i], to);   // All learnt:
  for (int i = 0; i < clauses.size(); i++) ca.reloc(clauses[i], to);   // All original:
  for (int i = 0; i < phantoms.size(); i++) ca.reloc(phantoms[i], to); // All phantoms:
}


void Solver::garbageCollect()
{
  // Initialize the next region to a size corresponding to the estimated utilization degree. This
  // is not precise but should avoid some unnecessary reallocations for the new region:
  ClauseAllocator to(ca.size() - ca.wasted());

  relocAll(to);
  if (verbosity >= 2)
    printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n",
           ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
  to.moveTo(ca);
}// garbageCollect


/**
   Compute the backbone.
*/
void Solver::computeBackBone()
{
  vec<lbool> currentModel;
  vec<Var> component;
  vec<Lit> unitFound;

  double initTime = cpuTime();
  needModel = true;
  for(int i = 0 ; i<nVars() ; i++) if(inTheHeap[i] == stampInTheHeap) component.push(i);

  if(!solveWithAssumptions()){printf("c The formula is unsatisfiable\ns 0\n"); exit(0);}

  // collect a first interpretation
  for(int i = 0 ; i<component.size() ; i++)
  {
    saveFree[component[i]] = 1 + toInt(model[component[i]]);
    currentModel.push(model[component[i]]);
    if(value(component[i]) != l_Undef) saveFree[component[i]] = 3; // already prove
  }

  int cpt = 0;

  // we collect the set of implied literals
  int currentLevel = decisionLevel();
  for(int i = 0 ; i<component.size() ; i++)
  {
    Var v = component[i];
    if(value(v) != l_Undef || saveFree[v] == 3) continue;

    // l is implied?
    Lit l = mkLit(v, saveFree[v] == (toInt(l_False) + 1));
    assumptions.push(~l);

    cpt++;
    for(int j = i + 1 ; j<component.size() ; j++) polarity[component[j]] = 1 - polarity[component[j]];
    bool res = solveWithAssumptions();
    assumptions.pop();

    if(res)
    {
      for(int j = i + 1 ; j<component.size() ; j++)
      {
        Var tmp = component[j];
        if(currentModel[j] != model[tmp]) saveFree[tmp] |= (1 + toInt(model[tmp]));
      }
    }
    else
    {
      assert(value(l) != l_Undef);
      unitFound.push(l);
    }
    cancelUntil(currentLevel);
  }

  for(int i = 0 ; i<component.size() ; i++) saveFree[component[i]] = 0;
  for(int i = 0 ; i<unitFound.size() ; i++) if(value(unitFound[i]) == l_Undef) uncheckedEnqueue(unitFound[i]);
  assert(!assumptions.size());

  needModel = false;
  printf("c\nc Backbone simplification\n");
  printf("c The number of unit literal found is: %d\n", unitFound.size());
  printf("c Time to realize the backbone simplification: %lf\n", cpuTime() - initTime);
  // reduceDB();
}// computeBackBone



/**
   Compute the backbone w.r.t. a set of variables.
*/
void Solver::computeBackBone(vec<Var> &component)
{
  vec<lbool> currentModel;
  vec<Lit> unitFound;

  needModel = true;

  bool res = solveWithAssumptions();
  assert(res);

  // collect a first interpretation
  for(int i = 0 ; i<component.size() ; i++)
  {
    saveFree[component[i]] = 1 + toInt(model[component[i]]);
    currentModel.push(model[component[i]]);
    if(value(component[i]) != l_Undef) saveFree[component[i]] = 3; // already prove
  }

  int cpt = 0;

  // we collect the set of implied literals
  int currentLevel = decisionLevel();
  for(int i = 0 ; i<component.size() ; i++)
  {
    Var v = component[i];
    if(value(v) != l_Undef || saveFree[v] == 3) continue;

    // l is implied?
    Lit l = mkLit(v, saveFree[v] == (toInt(l_False) + 1));
    assumptions.push(~l);

    cpt++;
    for(int j = i + 1 ; j<component.size() ; j++) polarity[component[j]] = 1 - polarity[component[j]];
    bool res = solveWithAssumptions();
    assumptions.pop();

    if(res)
    {
      for(int j = i + 1 ; j<component.size() ; j++)
      {
        Var tmp = component[j];
        if(currentModel[j] != model[tmp]) saveFree[tmp] |= (1 + toInt(model[tmp]));
      }
    }
    else
    {
      assert(value(l) != l_Undef);
      unitFound.push(l);
    }
    cancelUntil(currentLevel);
  }

  for(int i = 0 ; i<component.size() ; i++) saveFree[component[i]] = 0;
  for(int i = 0 ; i<unitFound.size() ; i++) if(value(unitFound[i]) == l_Undef) uncheckedEnqueue(unitFound[i]);
  needModel = false;
  // reduceDB();
}// computeBackBone




/**
   A at most one constraint is researched with as starting point
   the vector vc (where we have for all i, vc[0] => vc[i]).
   WARNING: vc and canBeTrue must be sort in the same way

   @param[out] vc, the start/end literals point
*/
void Solver::searchAtMostOne(vec<Lit> &vc, vec<Lit> &canBeTrue)
{
  for(int i = 1 ; i<vc.size() ; i++) vc[i] = ~vc[i];

  canBeTrue.push(vc[0]);
  vc[0] = vc.last();
  vc.pop();

  sort(vc, LitOrderLt(activity));

  for(int i = 0 ; i<vc.size() ; i++)
  {
    newDecisionLevel();
    uncheckedEnqueue(vc[i]);

    if(propagate() == CRef_Undef)
    {
      canBeTrue.push(vc[i]);
      int j, k;
      for(j = k = i + 1 ; j<vc.size() ; j++)
        if(value(vc[j]) == l_False) vc[k++] = vc[j];
      vc.shrink(j - k);
    }

    backTrack();
  }

  // shift to the right
  vc.push();
  for(int i = vc.size() - 1 ; i > 0 ; i--) vc[i] = vc[i - 1];
  vc[0] = canBeTrue[0];
}// searchAtMostOne


#if 0
/**
   Print the current CNF formula into a dot with different color
   w.r.t. the weight given by the scoring method.

*/
void Solver::showFormulaDotPrimal(vec<Var> &setOfVar, vec<Var> &priority, ScoringMethod *sm)
{
  static int cpt = 0;
  cpt++;
  // if(cpt > 200) exit(0);

  vec<bool> inCurrentComponent, isPriority;
  inCurrentComponent.initialize(nVars(), false);
  isPriority.initialize(nVars(), false);

  for(int i = 0 ; i<setOfVar.size() ; i++) inCurrentComponent[setOfVar[i]] = true;
  for(int i = 0 ; i<priority.size() ; i++) isPriority[priority[i]] = true;

  vec<double> scoreVar;
  double max = 0;

  for(int i = 0 ; i<setOfVar.size() ; i++)
  {
    scoreVar.push(0);

    if(priority.size() && isPriority[setOfVar[i]])
    {
      scoreVar.last() = sm->computeScore(setOfVar[i]);
      if(scoreVar.last() > max) max = scoreVar.last();
    }
    if(!priority.size())
    {
      scoreVar.last() = sm->computeScore(setOfVar[i]);
      if(scoreVar.last() > max) max = scoreVar.last();
    }
  }

  for(int i = 0 ; i<scoreVar.size() ; i++) scoreVar[i] = max ? scoreVar[i] / max : 1;

  // printf("<<<<< >>>>>> ");
  // showListVar(priority);
  // showListVar(setOfVar);

  printf("%d strict graph{\n", cpt);
  for(int i = 0 ; i<setOfVar.size() ; i++)
  {
    if(value(setOfVar[i]) != l_Undef) continue;

    if(priority.size())
    {
      if(!isPriority[setOfVar[i]]) printf("%d %d [shape=box color=\"1 0 1\" style=filled];\n", cpt, setOfVar[i] + 1);
      else
      {
        if(scoreVar[i] == 1)
          printf("%d %d [shape=polygon sides=6 color=\"1 %lf 1\" style=filled];\n", cpt, setOfVar[i] + 1, scoreVar[i]);
        else printf("%d %d [shape=ellipse fontcolor=\"1 %lf 1\" style=filled];\n", cpt, setOfVar[i] + 1, scoreVar[i]);
      }
    }else
    {
      if(scoreVar[i] == 1)
        printf("%d %d [shape=polygon, sides=6,color=\"1 %lf 1\",style=filled];\n", cpt, setOfVar[i] + 1, scoreVar[i]);
      else printf("%d %d [shape=box,color=\"1 %lf 1\",style=filled];\n", cpt, setOfVar[i] + 1, scoreVar[i]);
    }
  }

  for(int i = 0 ; i<clauses.size() ; i++)
  {
    Clause &c = ca[clauses[i]];
    if(clauseIsSAT(c)) continue;

    if(!inCurrentComponent[var(c[0])]) continue;
    // printf("cl: "); c.showClause();

    for(int j = 0 ; j<c.size() ; j++)
    {
      // if(value(c[j]) != l_Undef) continue;
      if(value(c[j]) != l_Undef || isPriority[var(c[j])]) continue;

      for(int k = j + 1 ; k<c.size() ; k++)
      {
        if(value(c[k]) != l_Undef || isPriority[var(c[k])]) continue;
        // if(value(c[k]) != l_Undef) continue;
        printf("%d %d -- %d;\n", cpt, var(c[j]) + 1, var(c[k]) + 1);
      }
    }
  }
  printf("%d }\n", cpt);
}// showFormulaDotPrimal


/**
   Print the current CNF formula into a dot with different color
   w.r.t. the weight given by the scoring method.

*/
void Solver::showFormulaDotDual(vec<Var> &setOfVar, vec<int> &cutSet)
{
  static int cpt = 0;
  cpt++;

  vec<bool> inCurrentComponent, markedVar;
  inCurrentComponent.initialize(nVars(), false);
  markedVar.initialize(nVars(), false);

  for(int i = 0 ; i<setOfVar.size() ; i++) inCurrentComponent[setOfVar[i]] = true;

  printf("%d strict graph{\n", cpt);
  for(int i = 0 ; i<clauses.size() ; i++)
  {
    Clause &c = ca[clauses[i]];
    if(clauseIsSAT(c)) continue;
    if(!inCurrentComponent[var(c[0])]) continue;

    bool inCutSet = false;
    for(int j = 0 ; !inCutSet && j<cutSet.size() ; j++) inCutSet = cutSet[j] == i;

    if(inCutSet) printf("%d %d [shape=polygon sides=6 color=\"1 %lf 1\" style=filled];\n", cpt,  i + 1, 50.0);
    else printf("%d %d [shape=ellipse style=filled];\n", cpt, i + 1);
  }

  for(int i = 0 ; i<clauses.size() ; i++)
  {
    Clause &c = ca[clauses[i]];
    if(clauseIsSAT(c)) continue;
    if(!inCurrentComponent[var(c[0])]) continue;
    for(int j = 0 ; j<c.size() ; j++) markedVar[var(c[j])] = true;

    for(int j = i + 1 ; j<clauses.size() ; j++)
    {
      Clause &d = ca[clauses[j]];
      if(clauseIsSAT(d)) continue;
      if(!inCurrentComponent[var(d[0])]) continue;

      bool intersect = false;
      for(int k = 0 ; !intersect && k<d.size() ; k++) intersect = markedVar[var(d[k])];
      if(intersect) printf("%d %d -- %d;\n", cpt, i + 1, j + 1);
    }
    for(int j = 0 ; j<c.size() ; j++) markedVar[var(c[j])] = false;
  }
  printf("%d }\n", cpt);
}// showFormulaDotDual


/**
   Print the current CNF formula into a dot with different color
   w.r.t. the weight given by the scoring method.

*/
void Solver::showHyperGraph(vec<Var> &setOfVar)
{
  vec<bool> inCurrentComponent;
  inCurrentComponent.initialize(nVars(), false);
  for(int i = 0 ; i<setOfVar.size() ; i++) inCurrentComponent[setOfVar[i]] = true;

  int cpt = 0;
  for(int i = 0 ; i<clauses.size() ; i++)
  {
    Clause &c = ca[clauses[i]];
    if(clauseIsSAT(c)) continue;
    if(!inCurrentComponent[var(c[0])]) continue;
    cpt++;
  }

  printf("%d %d\n", cpt, setOfVar.size());
  for(int i = 0 ; i<clauses.size() ; i++)
  {
    Clause &c = ca[clauses[i]];
    if(clauseIsSAT(c)) continue;
    if(!inCurrentComponent[var(c[0])]) continue;

    for(int j = 0 ; j<c.size() ; j++)
    {
      if(!inCurrentComponent[var(c[j])]) continue;
      printf("%d ", var(c[j]) + 1);
    }
    printf("\n");
  }
}// showHyperGraph


/**
   Print the current CNF formula into a dot with different color
   w.r.t. the weight given by the scoring method.

*/
void Solver::showFormulaDotIncidence(vec<Var> &setOfVar, vec<Var> &priority, ScoringMethod *sm)
{
  static int cpt = 0;
  cpt++;

  vec<bool> inCurrentComponent;
  inCurrentComponent.initialize(nVars(), false);
  for(int i = 0 ; i<setOfVar.size() ; i++) inCurrentComponent[setOfVar[i]] = true;

  vec<double> scoreVar;
  double max = 0;
  for(int i = 0 ; i<setOfVar.size() ; i++)
  {
    scoreVar.push(sm->computeScore(setOfVar[i]));
    if(scoreVar.last() > max) max = scoreVar.last();
  }

  for(int i = 0 ; i<scoreVar.size() ; i++) scoreVar[i] = max ? scoreVar[i] / max : 1;

  printf("%d strict graph{\n", cpt);
  for(int i = 0 ; i<setOfVar.size() ; i++)
  {
    if(value(setOfVar[i]) != l_Undef) continue;
    if(scoreVar[i] == 1) printf("%d %d [shape=polygon, sides=6,color=\"1 %lf 1\",style=filled];\n",
                                cpt, setOfVar[i] + 1, scoreVar[i]);
    else printf("%d %d [shape=box,color=\"1 %lf 1\",style=filled];\n", cpt, setOfVar[i] + 1, scoreVar[i]);
  }

  for(int i = 0 ; i<clauses.size() ; i++)
  {
    Clause &c = ca[clauses[i]];
    if(clauseIsSAT(c)) continue;

    if(!inCurrentComponent[var(c[0])]) continue;
    // printf("cl: "); c.showClause();

    for(int j = 0 ; j<c.size() ; j++)
    {
      if(value(c[j]) != l_Undef) continue;
      printf("%d %d -- c%d;\n", cpt, var(c[j]) + 1, i);
    }
  }
  printf("%d }\n", cpt);
}// showFormulaDotIncidence
#endif
