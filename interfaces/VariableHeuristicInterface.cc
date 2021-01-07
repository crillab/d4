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
#include <fstream>
#include <sstream>
#include "string.h"

#include "../utils/System.hh"
#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"
#include "../mtl/Sort.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Heap.hh"
#include "../mtl/Alg.hh"
#include "../DAG/DAG.hh"
#include "../interfaces/OccurrenceManagerInterface.hh"


#include "../interfaces/VariableHeuristicInterface.hh"

using namespace std;

/**
   The constructor with a lot of options ....
   
   @param[in] isProjectedVar, boolean vector used to decide if a variable is projected (true) or not (false)
 */
VariableHeuristicInterface::VariableHeuristicInterface(Solver &_s, OccurrenceManagerInterface *_occM, const char *v, const char *p, vec<bool> &isProj) :
  s(_s), occM(_occM)
{    
  if(!strcmp(v, "VSADS")) sm = new Vsads(occM, s.scoreActivity);
  else if(!strcmp(v, "VSIDS")) sm = new Vsids(s.scoreActivity);
  else if(!strcmp(v, "DLCS")) sm = new Dlcs(s, occM);
  else if(!strcmp(v, "JW-TS")) sm = new Jwts(s, occM);
  else if(!strcmp(v, "MOM")) sm = new Mom(s, occM);
  else {fprintf(stderr, "%s: this variable heuristic is unknow\n", v); exit(32);}

  if(!strcmp(p, "OCCURRENCE")) ps= new PhaseOccurrence(occM);
  else if(!strcmp(p, "TRUE")) ps = new PhaseTrue();
  else if(!strcmp(p, "FALSE")) ps = new PhaseFalse();
  else if(!strcmp(p, "POLARITY")) ps = new PhasePolarity(s.getPolarity());
  else {fprintf(stderr, "%s: this phase heuristic is unknow\n", p); exit(31);}

  isProj.copyTo(varProjected);
}// constructor


/**
   Select a new decision variable w.r.t. the scoring function
   initially chosen (see the constructor for more information).

   @param[in] component, the current problem's variables.
   \return a variable to be assigned, var_undef if any variable can be assigned
*/
Var VariableHeuristicInterface::selectVariable(vec<Var> &component)
{    
  Var next = var_Undef; 
  double maxScore = -1;
  sm->initStructure(component);
  
  for(int i = 0 ; i<component.size() ; i++) 
    {      
      Var v = component[i];      
      if(s.value(v) != l_Undef || !varProjected[v]) continue;
      
      double score = sm->computeScore(v);	  
      if(next == var_Undef || score > maxScore){next = v; maxScore = score;}
    }

  assert(next == var_Undef || varProjected[next]);
  return next;
}// selectVariable
