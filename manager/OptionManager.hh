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
#ifndef MANAGER_OPTIONMANAGER_h
#define MANAGER_OPTIONMANAGER_h

#include <iostream>
#include "../utils/SolverTypes.hh"
#include "../mtl/Vec.hh"

class OptionManager
{
public:
  int optCache;
  bool optDecomposableAndNode;
  bool reversePolarity;
  bool reducePrimalGraph;
  bool equivSimplification;

  int freqLimitDyn;
  int reduceCache, strategyRedCache;

  const char *cacheStore;
  const char *varHeuristic;
  const char *phaseHeuristic;
  const char *partitionHeuristic;
  const char *cacheRepresentation;

  OptionManager(int _optCache, bool _optAnd, bool _reversePolarity, bool _reducePrimalGraph,
                bool _equivSimplification, const char *_cacheStore, const char *_varHeuristic,
                const char *_phaseHeuristic, const char *_partitionHeuristic,
                const char *_cacheRepresentation, int rdCache, int strCache, int frqLimit)
  {
    freqLimitDyn = frqLimit;
    strategyRedCache = strCache;
    reduceCache = rdCache;
    cacheRepresentation = _cacheRepresentation;
    optCache = _optCache;
    optDecomposableAndNode = _optAnd;
    reversePolarity = _reversePolarity;
    reducePrimalGraph = _reducePrimalGraph;
    equivSimplification = _equivSimplification;

    varHeuristic = _varHeuristic;
    phaseHeuristic = _phaseHeuristic;
    partitionHeuristic = _partitionHeuristic;
    cacheStore = _cacheStore;
  }// constructor

  inline void printOptions()
  {
    printf("c\nc \033[1m\033[32mOption list \033[0m\n");
    printf("c Caching: %d\n", optCache);
    printf("c Reduce cache procedure level: %d\n", reduceCache);
    printf("c Strategy for Reducing the cache: %d\n", strategyRedCache);
    printf("c Cache representation: %s\n", cacheRepresentation);
    printf("c Part of the formula that is cached: %s\n", cacheStore);
    printf("c Variable heuristic: %s\n", varHeuristic);
    printf("c Phase heuristic: %s%s\n", (reversePolarity) ? "reverse " : "", phaseHeuristic);
    printf("c Partitioning heuristic: %s%s%s\n", partitionHeuristic,
           (reducePrimalGraph) ? " + graph reduction" : "",
           (equivSimplification) ? " + equivalence simplication" : "");
    printf("c\n");
  }
};
#endif
