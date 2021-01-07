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
#ifndef DAG_True_h
#define DAG_True_h

#include "DAG.hh"

template<class T> class DAG;
template<class T> class trueNode : public DAG<T>
{
public:
  using DAG<T>::globalStamp;
  using DAG<T>::idxOutputStruct;
  using DAG<T>::stamp;

  inline void printNNF(std::ostream& out, bool certif)
  {
    if(stamp >= globalStamp) return;
    stamp = globalStamp + idxOutputStruct + 1;
    int idxCurrent = ++idxOutputStruct;

    out << "t " << idxCurrent << " 0" << endl;
  }

  inline bool isSAT(vec<Lit> &unitsLitBranches){return true;}
  inline T computeNbModels() { return 1; }
};

#endif
