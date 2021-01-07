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
#ifndef DAG_KromFormula_h
#define DAG_KromFormula_h

#include "DAG.hh"

template<class T> class DAG;
template<class T> class KromFormula : public DAG<T>
{
  using DAG<T>::nbEdges;
  using DAG<T>::globalStamp;
  using DAG<T>::fixedValue;
  
private:
  vec< vec<Lit> > clauses;
  int stamp;
  bool saveDecision;
  vec<Lit> unit;
  
public:
  KromFormula()
  {
    stamp = 0;
  }

  KromFormula(vec< vec<Lit> > _clauses)
  {
    stamp = 0;

    for(int i = 0 ; i< _clauses.size() ; i++)
      {
        clauses.push();
        _clauses[i].copyTo(clauses.last());
      }
  }

  inline void addClause(vec<Lit> &cl)
  {
    clauses.push();
    cl.copyTo(clauses.last());
    nbEdges += 3;
  }// addClause


  inline void addUnitLit(Lit l ){unit.push(l);}
    
  inline T computeNbModels() { assert(0);}
  inline void toNNF(std::ostream& out)
  {
    assert(false); // This should never get called since trueNode is a singleton
  }
  
  inline void printNNF()
  {
    printf("(");
    for(int i = 0 ; i<clauses.size() ; i++)
      {
        for(int j = 0 ; j<clauses[i].size() ; j++)
          {
            if(j) printf("@");
            printf("%d", readableLit(clauses[i][j]));
          }
        printf(", ");
      }
    printf(")");
  }// printNNF


  inline int stateLit(Lit l)
  {
    if(fixedValue[var(l)] == IS_NOT_ASSIGN) return IS_NOT_ASSIGN;
    if(sign(l) && fixedValue[var(l)] == IS_FALSE) return IS_TRUE;
    if(sign(l) && fixedValue[var(l)] == IS_TRUE) return IS_FALSE;
    if(!sign(l) && fixedValue[var(l)] == IS_TRUE) return IS_TRUE;
    if(!sign(l) && fixedValue[var(l)] == IS_FALSE) return IS_FALSE;
    assert(0);
    return IS_NOT_ASSIGN;
  }// stateLit
  
  
  inline bool isSAT()
  {
    if(stamp == globalStamp) return saveDecision;
    stamp = globalStamp;

    int pos = 0;
    vec<Lit> stackPropagated;
    saveDecision = true;

    for(int i = 0 ; i<unit.size() ; i++)
      if(stateLit(unit[i]) == IS_FALSE) return saveDecision = false;

    do
      {
        pos = stackPropagated.size();

        for(int i = 0 ; i<clauses.size() && saveDecision ; i++)
          {
            int s1 = stateLit(clauses[i][0]), s2 = stateLit(clauses[i][1]);
            if(s1 == IS_FALSE && s2 == IS_NOT_ASSIGN)
              {
                fixedValue[var(clauses[i][1])] = sign(clauses[i][1]) ? IS_FALSE : IS_TRUE;
                stackPropagated.push(clauses[i][1]);
              }
            else if(s2 == IS_FALSE && s1 == IS_NOT_ASSIGN)
              {
                fixedValue[var(clauses[i][0])] = sign(clauses[i][0]) ? IS_FALSE : IS_TRUE;
                stackPropagated.push(clauses[i][0]);
              }
            else if(s2 == IS_FALSE && s1 == IS_FALSE) saveDecision = false;            
          }
      }    
    while(saveDecision && pos < stackPropagated.size());

    for(int i = 0 ; i<stackPropagated.size() ; i++) fixedValue[var(stackPropagated[i])] = IS_NOT_ASSIGN;

    return saveDecision;
  }
};

#endif
