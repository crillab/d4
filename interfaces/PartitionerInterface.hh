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
#ifndef PARTITIONER_INTERFACE
#define PARTITIONER_INTERFACE

#include "../manager/OptionManager.hh"
#include <cstring>

using namespace std;

class PartitionerInterface
{
public:
  virtual ~PartitionerInterface(){}
  bool reduceFormula;
  bool equivSimp;

  inline void setReduceFormula(bool b){reduceFormula = b;}
  inline void setEquivSimp(bool b){equivSimp = b;}
    
  virtual void computePartition(vec<Var> &component, vec<Var> &partition, vec<int> &cutVar, ScoringMethod *sm)
  {
    component.copyTo(partition);
  }

  static PartitionerInterface *getPartitioner(Solver &s, OccurrenceManagerInterface *om, OptionManager &optList);
};  
#endif
