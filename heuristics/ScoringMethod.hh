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
#ifndef MODELCOUNTER_VARIABLE_SELECTION
#define MODELCOUNTER_VARIABLE_SELECTION

#include "../interfaces/OccurrenceManagerInterface.hh"
#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"

class ScoringMethod
{    
public:
  virtual ~ScoringMethod(){;}
  virtual double computeScore(Var v) = 0;
  virtual void initStructure(vec<Var> &component){}
};

/**
   The well-known MOM heuristic.
     
   D. Pretolani. Efficiency and stability of hypergraph sat
   algorithms. In D. S.  Johnson and M. A. Trick, editors, Second
   DIMACS Implementation Challenge.  American Mathematical Society,
   1993.
*/
class Mom : public ScoringMethod
{
private:
  Solver &s;
  OccurrenceManagerInterface *om;
  
public:
  Mom(Solver &_s, OccurrenceManagerInterface *_om): s(_s), om(_om){}
    
  inline double computeScore(Var v)
  {
    assert(om);
    int nbBin = om->getNbBinaryClause(v);
    Lit lp = mkLit(v, false);
      
    for(int sign = 0 ; sign<2 ; sign++)
      { 
        vec<int> &occ = om->getVecIdxClause(sign ? lp : ~lp);
          
        for(int i = 0 ; i<occ.size() ; i++) 
          {
            Clause &c = s.ca[s.clauses[occ[i]]];
              
            int nbUnAssigned = 0;
            for(int j = 0 ; j<c.size() ; j++) if(s.value(c[j]) == l_Undef) nbUnAssigned++;
            if(nbUnAssigned > 2) continue;
            nbBin++;
          }
      }
      
    return nbBin * 0.25;
  }// computeScore
};

/**
   The classical VSIDS heuristic.
     
   Matthew W. Moskewicz, Conor F. Madigan, Ying Zhao, Lintao Zhang,
   and Sharad Malik. Chaff: Engineering an Efficient SAT Solver. In
   Proceedings of the 38th Design Automation Conference (DAC’01), 2001
*/
class Vsids : public ScoringMethod
{
private:
  vec<double> &activity;

public:
  Vsids(vec<double> &ac) : activity(ac){}    
  inline double computeScore(Var v){return activity[v];}
};


/**
   This scoring function favorises the variables which appear in
   most clauses.
*/
class Dlcs : public ScoringMethod
{
private:
  Solver &s;
  OccurrenceManagerInterface *om;
    
public:
  Dlcs(Solver &_s, OccurrenceManagerInterface *_om): s(_s), om(_om){}    
  inline double computeScore(Var v){assert(om); return om->getNbClause(v);}
};


/**
   This scoring function favorises the varaibles which appear in
   most clauses.
     
   R. G. Jeroslow and J. Wang. Solving propositional satisfiability
   problems. Annals of Mathematics and Artificial Intelligence,
   1:167–187, 1990.
*/
class Jwts : public ScoringMethod
{
private:
  Solver &s;
  OccurrenceManagerInterface *om;
  
public:
  Jwts(Solver &_s, OccurrenceManagerInterface *_om): s(_s), om(_om){}
    
  inline double computeScore(Var v)
  {
    assert(om);
    int nbBin = om->getNbBinaryClause(v);
    Lit lp = mkLit(v, false);
      
    double res = nbBin * 0.25;
    for(int sign = 0 ; sign<2 ; sign++)
      { 
        vec<int> &occ = om->getVecIdxClause(sign ? lp : ~lp);
          
        for(int i = 0 ; i<occ.size() ; i++) 
          {
            Clause &c = s.ca[s.clauses[occ[i]]];
            if(c.size() > 5) continue;
              
            int nbUnAssigned = 0;
            for(int j = 0 ; j<c.size() ; j++) if(s.value(c[j]) == l_Undef) nbUnAssigned++;
            res += ((double) 1.0) / (1<<nbUnAssigned);              
          }
      }
      
    return res;
  }// computeScore
};

/**
   The well-known VSADS heuristic.

   Sang, T.; Beame, P.; and Kautz, H. Heuristics for Fast
   Exact Model Counting. In Proceedings of the 8th International
   Conference on Theory and Applications of Satisfiability Testing.
*/
class Vsads : public ScoringMethod
{
private:
  OccurrenceManagerInterface *om;
  vec<double> &score;

public:
  Vsads(OccurrenceManagerInterface *_om, vec<double> &ac) : om(_om), score(ac){}    
  inline double computeScore(Var v){assert(om);return score[v] + om->getNbClause(v);}
};
  
#endif
