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
#ifndef HEURISTIC_MINCUT_PARTIONER
#define HEURISTIC_MINCUT_PARTIONER

struct edge_t
{
  int first;
  int second;
};

/**
   Method which consist in computing some shortest path in the
   graph.
*/
class MinCutPartition : public PartitionerInterface
{
private:
  OccurrenceManagerInterface *om;
  vec<bool> inCurrentComponent, markedVar;
    
public:
  MinCutPartition(OccurrenceManagerInterface *_om): om(_om)
  {
    inCurrentComponent.initialize(om->getNbVariable(), false);
    markedVar.initialize(om->getNbVariable(), false);
  }// MinCutPartition Constructor
    
  void minCut(vec<Var> &component);
    
};
#endif
