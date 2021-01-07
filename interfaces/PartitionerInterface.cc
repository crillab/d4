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
#include "../interfaces/OccurrenceManagerInterface.hh"
#include "../mtl/Vec.hh"
#include "../utils/SolverTypes.hh"

#include "../interfaces/PartitionerInterface.hh"
#include "../heuristics/ClauseBipartiteGraphPartitioner.hh"
#include "../heuristics/VarBipartiteGraphPartitioner.hh"

PartitionerInterface *PartitionerInterface::getPartitioner(Solver &s, OccurrenceManagerInterface *om, OptionManager &optList)
{
  PartitionerInterface *pv = NULL;
  if(!strcmp(optList.partitionHeuristic, "NO")) pv = NULL;
  else if(!strcmp(optList.partitionHeuristic, "CB"))
    {
      pv = new ClauseBipartiteGraphPartitioner(s, om);
      pv->setReduceFormula(optList.reducePrimalGraph);
      pv->setEquivSimp(optList.equivSimplification);
    }
  else if(!strcmp(optList.partitionHeuristic, "VB"))
    {
      pv = new VarBipartiteGraphPartitioner(s, om);
      pv->setReduceFormula(optList.reducePrimalGraph);
      pv->setEquivSimp(optList.equivSimplification);
    }
  else
    {
      cerr << "Not available partioner" << endl;
      exit(12);
    }
  
  return pv;
}// getPartioner
