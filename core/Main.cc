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
#include <cassert>


#include "../manager/OptionManager.hh"
#include "../modelCounters/ModelCounter.hh"

#include "../compilers/dDnnfCompiler.hh"
#include "../preproc/Preproc.hh"

#include "../utils/System.hh"
#include "../utils/Options.hh"
#include "../utils/Solver.hh"

#include "../manager/ParserProblem.hh"

#include <boost/multiprecision/gmp.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <iterator>

using namespace boost::multiprecision;
using namespace std;


#include "../manager/dynamicOccurrenceManager.hh"
#include "../manager/BucketManager.hh"
#include "../manager/CacheCNFManager.hh"


/**
   Call a model counter.

   @param[in] clauses, the input CNF formula (that means a set of clauses)
   @param[in] weightLit, the weight of the literals
   @param[in] optList, the list of options
   @param[in] isProjectedVar, boolean vector used to decide if a variable is projected (true) or not (false)
 */
template<typename T> void modelCounting(vec<vec<Lit> > &clauses, vec<double> &weightLit,
                                        OptionManager &optList, vec<bool> &isProjectedVar)
{
  ModelCounter<T> *tmp = new ModelCounter<T>(clauses, weightLit, optList, isProjectedVar);
  T d = tmp->computeNbModel();
  cout << std::fixed << "s " << d << endl;
  delete tmp;
}// modelCounting


template<typename T> void runQueries(DAG<T> *t)
{
  vec<Lit> queryRead;

  do
    {
      queryRead.clear();

      int type = 0;
      while(type != -1 && type != 'm' && type != 'd') type = getchar();

      if(type != -1)
        {
          assert(type == 'm' || type == 'd');

          // read the next query
          int r = 0;
          while(fscanf(stdin, "%d", &r) && r)
            {
              if(r > 0) queryRead.push(mkLit(r - 1, false));
              else queryRead.push(mkLit(-r - 1, true));
            }

          if(queryRead.size())
            {
              cout << "c query: ";
              showListLit(queryRead);

              if(type == 'm')
                {
                  T t1 = t->computeNbModelsConditioning(queryRead);
                  cout << "s " << t1 << endl;
                }else if(type == 'd')
                {
                  bool res = t->isSATConditioning(queryRead);
                  cout << "s " << (res ? "SAT" : "UNS") << endl;
                }else assert(0);
            }
        }
    }while(queryRead.size());
}// runQueries


/**
   Compile a CNF formula into a d-DNNF formula.

   @param[in] cls, the input CNF formula (that means a set of clauses)
   @param[in] wLit, the weight of the literals
   @param[in] opt, the list of options
   @param[in] out, the stream where the sover writes its output
   @param[in] isProjectedVar, boolean vector used to decide if a variable is projected (true) or not (false)
*/
template<typename T> void compileDDNNF(vec<vec<Lit> > &cls, vec<double> &wLit, OptionManager &opt, ostream* out,
                                       vec<bool> &isProjectedVar, bool query, ostream* dratOut)
{
  DDnnfCompiler<T> *dDnnfCompiler = new DDnnfCompiler<T>(cls, wLit, opt, isProjectedVar, dratOut);
  DAG<T> *t = dDnnfCompiler->compile();
  if(out != nullptr) t->printNNF(*out, dratOut);

  if(query) runQueries<T>(t);
  else
    {
      T t1 = t->computeNbModels();
      cout << std::fixed << "s " << t1 << endl;
    }
}// compileDDNNF

/**
   Collect the projected variable given in an input file.

   @param[in] fileP, the path
   @param[in] nbVar, the number of variables
   @param[out] pvar, a boolean vector that is map each projected variable to
   true if the varaible is projected, false otherwise.
*/
void parseProjectedVariable(const char *fileP, int nbVar, vec<bool> &varProjected)
{
  if(!strcmp(fileP, "/dev/null")) for(int i = 0 ; i<nbVar ; i++) varProjected.push(true);
  else
  {
    for(int i = 0 ; i<nbVar ; i++) varProjected.push(false);

    cout << "c We are collected the projected variables ... ";
    ifstream file(fileP);

    string line;
    while (getline(file, line))
    {
      stringstream linestream(line);
      string item;
      while (getline(linestream, item, ','))
      {
        int v = stoi(item) - 1;
        assert(v >= 0 && v < varProjected.size());
        varProjected[v] = true;
      }
    }
    for(int i = 0 ; i<varProjected.size() ; i++) if(varProjected[i]) cout << i + 1 << " ";
    cout << " ... done" << endl;
  }
}// parseProjectedVariable

/**
   Print a CNF formula given in parameter following the Dimacs format.

   @param[in] clauses, the clauses
   @param[in] nbVar, the number of variables
 */
void printCNFtoDimacs(vec<vec<Lit> > &clauses, int nbVar)
{
  cout << "p cnf " << nbVar << " " << clauses.size() << endl;
  for(int i = 0 ; i<clauses.size() ; i++)
    {
      for(int j = 0 ; j<clauses[i].size() ; j++) cout << readableLit(clauses[i][j]) << " ";
      cout << "0" << endl;
    }
}// printCNFtoDimacs


/**
   The main function!
 */
int main(int argc, char** argv)
{
  setUsageHelp("USAGE: %s <input-file> [options]\n\n  where input may be either in plain or gzipped DIMACS.\n");
  BoolOption modelCounter("MAIN", "mc", "Only compute the number of model\n", false);
  BoolOption dDNNF("MAIN", "dDNNF", "Compile the problem into a decision-DNNF formula\n", false);
  BoolOption printCNF("MAIN", "print", "Print the input formula (maybe after applying preproc)\n", false);
  BoolOption query("MAIN", "query", "Compute a set of queries given on the input stream\n", false);

  // options:
  BoolOption optAnd("MAIN", "optAnd", "And decomposition activate\n", true);
  BoolOption rPolarity("MAIN", "rp", "Reverse the polarity heuristic in the compiler\n", false);
  BoolOption reducePrimalGraph("MAIN", "rpg",
                "Try to reduce the primal graph before running the partitioner\n", true);
  BoolOption equivSimp("MAIN", "eqs", "Compute literal equivalence to simplify the primal graph\n", true);

  StringOption cacheStore("MAIN", "cs",
                "Define which part of the formula is cached: ALL, NB (no binary), NT (no touche)\n",
                "NT");
  StringOption cacheRepresentation("MAIN", "cr", "CL\n", "CL");
  StringOption varHeuristic("MAIN", "vh", "Heuristic implemented: VSADS, VSIDS, DLCS, JW-TS, MOM\n", "VSADS");
  StringOption phaseHeuristic("MAIN", "ph", "Heuristic implemented: TRUE, FALSE, POLARITY, OCCURRENCE\n", "TRUE");
  StringOption partitionHeuristic("MAIN", "pv",
                "Partition Variable Heuristic implemented: NO, CB (clause bipartite) and VB (var bipartite)\n",
                "CB");
  StringOption fileWeights("MAIN", "wFile", "File where we can find for some literals a weight", "/dev/null");
  StringOption ddnnfOutput("MAIN", "out",
                "File where the d-DNNF representation of the DAG should be output", "/dev/null");
  StringOption dratOutput("MAIN", "drat", "File where the drat should be output", "/dev/null");

  StringOption fileP("MAIN", "fpv", "File where we can find the projected variable", "/dev/null");
  StringOption optPreproc("MAIN", "preproc",
               "Available preproc: backbone, vivification, occElimination (can be combine with +)", "");

  IntOption optCache("MAIN", "optCache", "Cache activate: 0 (not active), 1 (classic), 2 (dynamic)\n", 1);
  IntOption precision("MAIN", "precision", "The precision used for the mpf_class", 128);
  IntOption reduceCache("MAIN",
               "reduce-cache", "Set the periodicity of the cache to 1<<value (0 to deactivate)\n",
                        20, IntRange(0, 31));
  IntOption freqLimitDyn("MAIN",
               "frequence-update-limit", "Set the periodicity to update the size limit of we cache\n",
               18, IntRange(0, 31));
  IntOption strategyRedCache("MAIN", "strategy-reduce-cache",
               "Set the strategy for the aging about the cache entries (0 = dec, 1 = div)\n", 0, IntRange(0,2));


  parseOptions(argc, argv, true);

  ofstream out{ddnnfOutput};
  if (!out.is_open()) printf("c WARNING! Could not write output d-DNNF file %s?\n", (const char *) ddnnfOutput);

  ofstream dratOut{dratOutput};
  if (!dratOut.is_open()) printf("c WARNING! Could not write output drat file %s?\n", (const char *) dratOutput);

  OptionManager optList(optCache, optAnd, rPolarity, reducePrimalGraph, equivSimp, cacheStore, varHeuristic,
                        phaseHeuristic, partitionHeuristic, cacheRepresentation, reduceCache,
                        strategyRedCache, freqLimitDyn);

  // parse the input: CNF, weight of the literal and projected variables
  vec<vec<Lit> > clauses;
  vec<bool> isProjectedVar;
  vec<double> weightLit;

  ParserProblem p;
  int nbVar = p.parseProblem(argv[1], fileWeights, clauses, weightLit);
  parseProjectedVariable(fileP, nbVar, isProjectedVar);

  assert(isProjectedVar.size() >= nbVar);

  Preproc preproc;
  bool state = preproc.run(clauses, nbVar, isProjectedVar, string(optPreproc));
  if(!state)
    {
      clauses.push(); clauses.last().push(mkLit(1, false));
      clauses.push(); clauses.last().push(mkLit(1, true));
    }

  // check if we can only use integer.
  bool isInteger = true;
  double e;
  for(int i = 0 ; isInteger && i<weightLit.size() ; i++) isInteger = modf(weightLit[i], &e) == 0.0;
  cout << "c " << (isInteger ? "Integer" : "Float") << " mode " << endl;
  if(!isInteger) mpf_float::default_precision(precision); // we set the precision
  
  if(modelCounter)
    {
      if(isInteger) modelCounting<mpz_int>(clauses, weightLit, optList, isProjectedVar);
      else modelCounting<mpf_float>(clauses, weightLit, optList, isProjectedVar);
    }
  else if(dDNNF)
    {
      ofstream *outFile = (strcmp((const char*)ddnnfOutput, "/dev/null") == 0) ? nullptr: &out;
      ofstream *dratFile = (strcmp((const char*)dratOutput, "/dev/null") == 0) ? nullptr: &dratOut;

      if(isInteger) compileDDNNF<mpz_int>(clauses, weightLit, optList, outFile, isProjectedVar, query, dratFile);
      else compileDDNNF<mpf_float>(clauses, weightLit, optList, outFile, isProjectedVar, query, dratFile);

      if (dratFile) dratFile->close();
      if(outFile) outFile->close();
    }
  else if(printCNF) printCNFtoDimacs(clauses, nbVar);

  exit(0);
}// main
