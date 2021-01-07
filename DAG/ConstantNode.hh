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
#ifndef DAG_ConstantNode_h
#define DAG_ConstantNode_h

#include "DAG.hh"

template<class T> class DAG;
template<class T> class ConstantNode : public DAG<T>
{
public:
  T nbModels;

  ConstantNode(T _nbModels) : nbModels(_nbModels) {}
  
  inline void toNNF(std::ostream& out)
  {
    assert(false); // This should never get called since is a singleton
    out << "A 0" << std::endl;
  }

  inline void printNNF(){out << T;}// printNNF

  inline bool isSAT(){ return (nbModels > 0);}
  inline T computeNbModels() { return nbModels; }
};

#endif
