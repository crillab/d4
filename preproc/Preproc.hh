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
#ifndef PREPROC_LAUNCHER
#define PREPROC_LAUNCHER

#include <iostream>

#include "../utils/System.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Heap.hh"
#include "../mtl/Alg.hh"

#include "PreprocSolver.hh"

using namespace std;

class Preproc
{    
public:
  Preproc(){}
  
  bool run(vec<vec<Lit> > &clauses, int nbVar, vec<bool> &isProtectedVar, string opt);
  
  inline void displayStat()
  {
  }// displayStat
};

#endif
