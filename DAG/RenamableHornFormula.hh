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
#ifndef DAG_RenambleHornFormula_h
#define DAG_RenambleHornFormula_h

#include "DAG.hh"

template<class T> class DAG;
template<class T> class RenamableHornFormula : public DAG<T>
{
  using DAG<T>::nbEdges;
  using DAG<T>::globalStamp;
  using DAG<T>::fixedValue;
  
private:
  vec< vec<Lit> > clauses;
  int stamp;
  bool saveDecision;
  vec<Lit> unit;

  vec< vec<int> > occurrence;
  
public:
  RenamableHornFormula()
  {
    stamp = 0;
  }

  RenamableHornFormula(vec< vec<Lit> > _clauses)
  {
    stamp = 0;
    for(int i = 0 ; i< _clauses.size() ; i++) addClause(_clauses[i]);
  }

  inline void addClause(vec<Lit> &cl)
  {
    clauses.push();
    cl.copyTo(clauses.last());
    nbEdges += 1 + cl.size();  

    for(int i = 0 ; i<cl.size() ; i++)
      {
        while(((1 + var(cl[i]))<<1) >= occurrence.size()) occurrence.push();
        occurrence[toInt(cl[i])].push(clauses.size() - 1);
      }
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
        if(i) printf(", ");
        for(int j = 0 ; j<clauses[i].size() ; j++)
          {
            if(j) printf(" ");
            printf("%d", readableLit(clauses[i][j]));
          }        
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

    vec<bool> isSAT;
    vec<int> nbFalse;
    for(int i = 0 ; i<clauses.size() ; i++)
      {
        isSAT.push(false);
        nbFalse.push(0);
      }

    // init
    for(int i = 0 ; saveDecision && i<clauses.size() ; i++)
      {
        vec<Lit> &cl = clauses[i];
        for(int j = 0 ; !isSAT[i] && j<cl.size() ; j++)
          {
            if(stateLit(cl[j]) == IS_TRUE) isSAT[i] = true;
            if(stateLit(cl[j]) == IS_FALSE) nbFalse[i]++;
          }
        if(nbFalse[i] == cl.size()) saveDecision = false;
        if(!isSAT[i] && nbFalse[i] == (cl.size() - 1))
          {
            Lit pl = lit_Undef;
            for(int j = 0 ; pl == lit_Undef && j<clauses[i].size() ; j++)
              if(stateLit(clauses[i][j]) != IS_FALSE) pl = clauses[i][j];
            assert(pl != lit_Undef);            
            stackPropagated.push(pl);
          }
      }

    while(saveDecision && pos < stackPropagated.size())
      {
        Lit l = stackPropagated[pos++];

        if(stateLit(l) == IS_FALSE)
          {
            saveDecision = false;
            break;
          }
        
        if(stateLit(l) == IS_NOT_ASSIGN)
          {
            fixedValue[var(l)] = sign(l) ? IS_FALSE : IS_TRUE;
        
            for(int i = 0 ; i<occurrence[toInt(l)].size() ; i++) isSAT[occurrence[toInt(l)][i]] = true; 
            for(int i = 0 ; i<occurrence[toInt(~l)].size() ; i++)
              { 
                int idx = occurrence[toInt(~l)][i];
                if(isSAT[idx]) continue;
            
                nbFalse[idx]++;
                if(clauses[idx].size() == nbFalse[idx]) saveDecision = false;
                if((clauses[idx].size() - 1) == nbFalse[idx]) // propagate
                  {
                    Lit pl = lit_Undef;
                    for(int j = 0 ; pl == lit_Undef && j<clauses[idx].size() ; j++)
                      if(stateLit(clauses[idx][j]) != IS_FALSE) pl = clauses[idx][j];

                    if(pl == lit_Undef) saveDecision = false;
                    else stackPropagated.push(pl);
                  }
              }
          }
      }

    for(int i = 0 ; i<stackPropagated.size() ; i++) fixedValue[var(stackPropagated[i])] = IS_NOT_ASSIGN;
    return saveDecision;
  }
};

#endif
