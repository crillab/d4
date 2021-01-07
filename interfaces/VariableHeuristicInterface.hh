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
#ifndef HEURISTICS_VARIABLE_HEURISTIC_INTERFACE
#define HEURISTICS_VARIABLE_HEURISTIC_INTERFACE

#include "../interfaces/OccurrenceManagerInterface.hh"
#include "../utils/SolverTypes.hh"
#include "../utils/Solver.hh"
#include "../heuristics/ScoringMethod.hh"
#include "../heuristics/PhaseHeuristic.hh"

class VariableHeuristicInterface
{
private:    
  Solver &s;
  OccurrenceManagerInterface *occM;
  ScoringMethod *sm;
  PhaseSelection *ps;
    
public:
  vec<bool> varProjected;
  VariableHeuristicInterface(Solver &_s, OccurrenceManagerInterface *occM, const char *v, const char *p, vec<bool> &isProjectedVar);
  ~VariableHeuristicInterface(){delete sm; delete ps;}
  
  Var selectVariable(vec<Var> &component);
  bool selectPhase(Var v){return ps->selectPhase(v);}
  bool isProjected(Var v){return varProjected[v];}
  
  inline ScoringMethod *getScoringFunction(){return sm;}
};

#endif

