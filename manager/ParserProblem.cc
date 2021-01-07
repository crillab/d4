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
#include <zlib.h>
#include <errno.h>
#include <cstring>
#include <cassert>

#include "../utils/SolverTypes.hh"
#include "../utils/Dimacs.hh"
#include "../utils/Solver.hh"

#include "../DAG/DAG.hh"
#include "../manager/ParserProblem.hh"

int ParserProblem::parseCNF(char *benchName, vec<vec<Lit> > &clauses, bool verb)
{
  gzFile in = gzopen(benchName, "rb");
  if (in == NULL) printf("ERROR! Could not open file: %s\n", benchName), exit(1);

  int nbVar = parse_DIMACS(in, clauses);

  gzclose(in);

  if(verb)
    {
      int nbLit = 0;
      for(int i = 0 ; i<clauses.size() ; i++) nbLit += clauses[i].size();

      printf("c \033[33mBenchmark Information\033[0m\n");
      printf("c Number of variables: %d\n", nbVar);
      printf("c Number of clauses: %d\n", clauses.size());
      printf("c Number of literals: %d\n", nbLit);
    }

  return nbVar;
}// parseCNF


void ParserProblem::parseWeight(const char *fileWeights, vec<double> &weightLit, int nbVar)
{
  // transfer the clause
  for(int i = 0 ; i < nbVar ; i++){weightLit.push(1); weightLit.push(1);}
  if(strcmp(fileWeights, "/dev/null"))
    {
      printf("c External file to collect weight: %s\n", fileWeights);
      std::ifstream fichier(fileWeights);

      if(fichier)
        {
          double l, w;
          while(fichier >> l) // normally is a lit
            {
              fichier >> w;

              unsigned int tmp = ((l > 0) ? 2 * l : (-2 * l) + 1) - 2;
              weightLit[tmp] = w;
            }
        }
      else printf("ERROR: Impossible to open the file of weights %s\n", fileWeights), exit(12);
    }

  weightLit.copyTo(DAG<double>::weights);
}// parseWeight



/**
   Collect the information to construct the problem.

   @param[in] benchName, the path where we can find the CNF formula
   @param[in] fileWeights, the path where the file containng the literal 's weight can be found
*/
int ParserProblem::parseProblem(char *benchName, const char *fileWeights, vec<vec<Lit> > &clauses,
                                vec<double> &weightLit, bool verb)
{
  int nbVar = parseCNF(benchName, clauses, verb);
  parseWeight(fileWeights, weightLit, nbVar);

  return nbVar;
}// parseProblem
