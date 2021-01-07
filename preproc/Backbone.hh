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
#ifndef PREPROC_BACKBONE
#define PREPROC_BACKBONE

#include <iostream>

#include "../utils/System.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Heap.hh"
#include "../mtl/Alg.hh"
#include "PreprocSolver.hh"

using namespace std;

class Backbone
{    
public:
  Backbone(PreprocSolver &_os) : os(_os)
  {
    timeBackone = 0;
  }
  
  void run();
  vec<Lit> &getBackbone(){return backbone;}
  
  inline void displayStat()
  {
    fprintf(stderr, "c\nc Backbone, total time: %lf\n", timeBackone);
    fprintf(stderr, "c Backbone size: %d\n", backbone.size());
  }// displayStat
  
private:
  PreprocSolver &os;
  double timeBackone;

  vec<Lit> backbone;
};

#endif
