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
#include <tgmath.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/one_bit_color_map.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/typeof/typeof.hpp>

#include "../interfaces/OccurrenceManagerInterface.hh"
#include "../mtl/Vec.hh"
#include "../utils/SolverTypes.hh"

#include "../interfaces/PartitionerInterface.hh"
#include "../heuristics/MinCutPartitioner.hh"

using namespace std;

/**
   This function compute a min cut of the graph.
   
*/
void MinCutPartition::minCut(vec<Var> &component)
{
  for(int i = 0 ; i<component.size() ; i++) inCurrentComponent[component[i]] = true;

  vec<int> idxClauses;
  for(int i = 0 ; i<om->getNbClause() ; i++)
    {
      vec<Lit> &c = om->getClause(i);
      if(om->isSatisfiedClause(i)) continue;
      if(!inCurrentComponent[var(c[0])]) continue;
      idxClauses.push(i);
    }

  if(idxClauses.size() <= 1) return;

  vec<edge_t> vecEdges;
  for(int i = 0 ; i<idxClauses.size() ; i++)
    {
      vec<Lit> &c = om->getClause(idxClauses[i]);
      for(int j = 0 ; j<c.size() ; j++) markedVar[var(c[j])] = true;

      for(int j = i + 1 ; j<idxClauses.size() ; j++)
        {
          vec<Lit> &d = om->getClause(idxClauses[j]);
          bool intersect = false;
          for(int k = 0 ; !intersect && k<d.size() ; k++) intersect = markedVar[var(d[k])];
          if(intersect) vecEdges.push({i, j});
        }
      
      for(int j = 0 ; j<c.size() ; j++) markedVar[var(c[j])] = true;       
    }
  
  for(int i = 0 ; i<component.size() ; i++) inCurrentComponent[component[i]] = false;
  
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                boost::no_property, boost::property<boost::edge_weight_t, int> > undirected_graph;
  typedef boost::property_map<undirected_graph, boost::edge_weight_t>::type weight_map_type;
  typedef boost::property_traits<weight_map_type>::value_type weight_type;
  
  // construct the graph object
  edge_t edges[vecEdges.size()];
  weight_type ws[vecEdges.size()];
  for(int i = 0 ; i<vecEdges.size() ; i++)
    {
      edges[i] = vecEdges[i];
      ws[i] = 1;
    }  

  undirected_graph g(edges, edges + vecEdges.size(), ws, idxClauses.size(), vecEdges.size());
  
  // define a property map, `parities`, that will store a boolean value for each vertex.
  // Vertices that have the same parity after `stoer_wagner_min_cut` runs are on the same side of the min-cut.
  BOOST_AUTO(parities, boost::make_one_bit_color_map(num_vertices(g), get(boost::vertex_index, g)));
  
  // run the Stoer-Wagner algorithm to obtain the min-cut weight. `parities` is also filled in.
  int w = boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g), boost::parity_map(parities));

  assert(w);
  size_t i;

  int cpt1 = 0, cpt2 = 0;
  for (i = 0; i < num_vertices(g); ++i) {
    if (get(parities, i)) cpt1++;
  }
  
  for (i = 0; i < num_vertices(g); ++i) {
    if (!get(parities, i)) cpt2++;
  }  
  exit(0);
}// minCut
