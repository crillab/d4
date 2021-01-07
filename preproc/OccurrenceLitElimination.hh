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
#ifndef PREPROC_OCCURRENCE_LIT_ELIMINATION
#define PREPROC_OCCURRENCE_LIT_ELIMINATION

#include <iostream>

#include "../utils/System.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Heap.hh"
#include "../mtl/Alg.hh"
#include "PreprocSolver.hh"

using namespace std;

struct LitOrderOccurrenceLt {
  PreprocSolver &os;
  bool operator () (Lit x, Lit y) const { return os.getNbOccLit(x) < os.getNbOccLit(y);}
  LitOrderOccurrenceLt(PreprocSolver &_os) : os(_os) { }
};


class OccurrenceLitElimination
{
public:
  OccurrenceLitElimination(PreprocSolver &_os) : os(_os)
  {
    timeOccRm = 0;
    nbRemovedLitOccRm = 0;
  }
  
  void run(Lit l);
  void run();
  
  void displayStat()
  {
    fprintf(stderr, "c\nc Occurrence Elimination, total time: %lf\n", timeOccRm);
    fprintf(stderr, "c Occurrence Elimination, number of literals removed: %d\n", nbRemovedLitOccRm);
  }// displayStat
  
private:
  PreprocSolver &os;
  double timeOccRm;
  int nbRemovedLitOccRm;  
};

#endif
