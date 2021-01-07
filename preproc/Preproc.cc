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
#include <string>
#include <boost/tokenizer.hpp>

#include "../utils/System.hh"
#include "../utils/SolverTypes.hh"
#include "../utils/Dimacs.hh"
#include "../utils/Solver.hh"

#include "OccurrenceLitElimination.hh"
#include "Vivification.hh"
#include "Backbone.hh"
#include "Preproc.hh"

using namespace std;
using namespace boost;


/**
   Parse the option given in paremeter and apply the preprocessing on
   the set of given clauses. Consequently at the end of the procedure
   the input clauses can be changed.

   @param[out] clauses, the set of clauses
   @param[in] opt, the option
 */
bool Preproc::run(vec<vec<Lit> > &clauses, int nbVar, vec<bool> &isProtectedVar, string opt)
{
  double initTime = cpuTime();
  cout << "c Preproc options: " << opt << endl;
  return true; // no preproc.
  
  // create the preproc solver.
  Solver s;
  for(int i = 0 ; i<nbVar ; i++) s.newVar(); 
  for(int i = 0 ; i<clauses.size() ; i++) s.addClause_(clauses[i]);

  if(!s.okay()) return false;
  
  PreprocSolver os(s, isProtectedVar);
  
  char_separator<char> sep("+");
  tokenizer<char_separator<char>> tokens(opt, sep);
  for (const auto& t : tokens)
    {
      if(t == "backbone")
        {	  
          cout << "c Run Backbone" << endl;
          Backbone b(os);
          b.run();
          b.displayStat();
        }

      if(t == "vivification")
        {
          cout << "c Run Vivification" << endl;
          Vivification v(os);
          v.run();
          v.displayStat();
        }

      if(t == "occElimination")
        {
          cout << "c Run Occurrence Elimination" << endl;
          OccurrenceLitElimination o(os);
          o.run();
          o.displayStat();
        }
    }

  // modify the input clauses in order to take into account the preprocessing.
  clauses.clear();
  os.collectInitFormula(clauses);
  
  cout << "c" << endl << "c Preproc time: " << fixed << cpuTime() - initTime << endl;
  return true;
}// run
