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
/****************************************************************************************[Dimacs.h]
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

#ifndef Dimacs_h
#define Dimacs_h

#include <stdio.h>

#include "../utils/ParseUtils.hh"
#include "../utils/SolverTypes.hh"

//=================================================================================================
// DIMACS Parser:

template<class B> static void readClause(B& in, vec<Lit>& lits) 
{
  int parsed_lit, var;
  lits.clear();
  for (;;)
    {
      parsed_lit = parseInt(in);
      if (parsed_lit == 0) break;
      var = abs(parsed_lit)-1;
      lits.push( (parsed_lit > 0) ? mkLit(var) : ~mkLit(var) );
    }
}// readClause


template<class B> static void collectLit(B& in, vec<Lit>& lits) 
{
  int parsed_lit, var;
  lits.clear();
  for (;;)
    {
      parsed_lit = parseInt(in);
      if(parsed_lit == 0) break;
      var = abs(parsed_lit) - 1;
      lits.push( (parsed_lit > 0) ? mkLit(var) : ~mkLit(var) );
    }
}// readClause

template<class B> static int parse_DIMACS_main(B& in, vec<vec<Lit> > &clauses) 
{
  vec<Lit> lits;
  int vars    = 0;
  int nbclauses = 0;
  int cnt     = 0;
  for (;;)
    {
      skipWhitespace(in);
      if (*in == EOF) break;
      else if (*in == 'p')
        {          
          if(eagerMatch(in, "p cnf"))
            {
              vars    = parseInt(in);
              nbclauses = parseInt(in);              
            }else printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
        }
      else if (*in == 'c' || *in == 'p' || *in == 'w') skipLine(in);
      else
        {
          cnt++;
          readClause(in, lits);
          clauses.push();
          lits.copyTo(clauses.last());
        }
    }
  
  if (cnt  != clauses.size()) fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
  return vars;
}
 
// Inserts problem into solver.
static int parse_DIMACS(gzFile input_stream, vec<vec<Lit> > &clauses) 
{
  StreamBuffer in(input_stream);
  return parse_DIMACS_main(in, clauses);
}

#endif
