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
#ifndef COMPILERS_DDNNF_COMPILER
#define COMPILERS_DDNNF_COMPILER

#include <iostream>
#include <boost/multiprecision/gmp.hpp>

#include "../interfaces/OccurrenceManagerInterface.hh"
#include "../interfaces/PartitionerInterface.hh"
#include "../interfaces/VariableHeuristicInterface.hh"

#include "../manager/dynamicOccurrenceManager.hh"
#include "../manager/BucketManager.hh"
#include "../manager/CacheCNFManager.hh"

#include "../utils/System.hh"
#include "../utils/SolverTypes.hh"
#include "../utils/Dimacs.hh"
#include "../utils/Solver.hh"

#include "../mtl/Sort.hh"
#include "../mtl/Vec.hh"
#include "../mtl/Heap.hh"
#include "../mtl/Alg.hh"

#include "../DAG/UnaryNode.hh"
#include "../DAG/UnaryNodeCertified.hh"
#include "../DAG/BinaryDeterministicOrNode.hh"
#include "../DAG/BinaryDeterministicOrNodeCertified.hh"
#include "../DAG/DecomposableAndNodeCerified.hh"
#include "../DAG/DecomposableAndNode.hh"
#include "../DAG/DAG.hh"

#include "../manager/OptionManager.hh"
#include "../core/ShareStructures.hh"


#define NB_SEP_DNNF_COMPILER 154

using namespace boost::multiprecision;
using namespace std;


struct onTheBranch
{
  vec<Lit> units;
  vec<Var> free;
  vec<int> idxReason;
};

template <class T> class DDnnfCompiler
{
 private:
  // statistics
  int nbNodeInCompile;
  int nbCallCompile;
  int nbSplit;
  int callEquiv, callPartitioner;
  double currentTime;

  int freqBackbone;
  double sumAffectedAndNode;
  int minAffectedAndNode;

  int freqLimitDyn;
  unsigned int nbDecisionNode;
  unsigned int nbDomainConstraintNode;
  unsigned int nbAndNode, nbAndMinusNode;
  CacheCNF<DAG<T> *> *cache;

  vec<unsigned> stampVar;
  vec<bool> alreadyAdd;
  unsigned stampIdx;

  bool optBackbone;
  int optCached;
  bool optDecomposableAndNode;
  bool optDomConst;
  bool optReversePolarity;
  bool isCertified;

  VariableHeuristicInterface *vs;
  BucketManager<DAG<T> *> *bm;
  PartitionerInterface *pv;

  EquivManager em;

  DAG<T> *globalTrueNode, *globalFalseNode;

  Solver s;
  OccurrenceManagerInterface *occManager;
  vec<vec<Lit> > clauses;

  bool initUnsat;
  TmpEntry<DAG<T> *> NULL_CACHE_ENTRY;


  /**
     Manage the case where it is unsatisfiable.
  */
  DAG<T>* manageUnsat(Lit l, onTheBranch &onB, vec<int> &idxReason)
  {
    // we need to get a reason for why the problem is unsat.
    onB.units.push(l);
    if(!isCertified) return globalFalseNode;
    if(s.idxReasonFinal >= 0) idxReason.push(s.idxReasonFinal);
    return globalFalseNode;
  }// manageUnsat

  /**
     Compile the CNF formula into a D-FPiBDD.

     @param[in] setOfVar, the current set of considered variables
     @param[in] priority, select in priority these variable to the next decision node
     @param[in] dec, the decision literal
     @param[out] onB, information about units, free variables on the branch
     @param[out] fromCache, to know if the DAG returned is from cache
     @param[out] idxReason, the reason for the units (please only add and do not clean this variable, reuse after)

     \return a compiled formula (fpibdd or fbdd w.r.t. the options selected).
  */
  DAG<T>* compile_(vec<Var> &setOfVar, vec<Var> &priorityVar, Lit dec, onTheBranch &onB,
                   bool &fromCache, vec<int> &idxReason)
  {
    fromCache = false;
    showRun(); nbCallCompile++;
    s.rebuildWithConnectedComponent(setOfVar);

    if(!s.solveWithAssumptions()) return manageUnsat(dec, onB, idxReason);
    s.collectUnit(setOfVar, onB.units, dec); // collect unit literals
    occManager->preUpdate(onB.units);

    // compute the connected composant
    vec<Var> reallyPresent;
    vec< vec<Var> > varConnected;
    int nbComponent = occManager->computeConnectedComponent(varConnected, setOfVar, onB.free, reallyPresent);

    if(nbComponent && !optDecomposableAndNode)
    {
      for(int i =  1 ; i<varConnected.size() ; i++)
        for(int j = 0 ; j<varConnected[i].size() ; j++) varConnected[0].push(varConnected[i][j]);
      nbComponent = 1;
    }

    vec<bool> comeFromCache;
    DAG<T> *ret = NULL;
    if(!nbComponent)
    {
      comeFromCache.push(false);
      ret = globalTrueNode; // tautologie modulo unit literal
    }
    else
    {
      // we considere each component one by one
      vec<DAG<T> *> andDecomposition;

      nbSplit += (nbComponent > 1) ? nbComponent : 0;
      for(int cp = 0 ; cp<nbComponent ; cp++)
      {
        vec<Var> &connected = varConnected[cp];
        bool localCache = optCached;

        occManager->updateCurrentClauseSet(connected);
        TmpEntry<DAG<T> *> cb = (localCache) ? cache->searchInCache(connected, bm) : NULL_CACHE_ENTRY;

        if(localCache && cb.defined)
        {
          comeFromCache.push(true);
          andDecomposition.push(cb.getValue());
        }
        else
        {
          // compute priority list
          vec<Var> currPriority;
          comeFromCache.push(false);

          stampIdx++;
          for(int i = 0 ; i<connected.size() ; i++) stampVar[connected[i]] = stampIdx;
          bool propagatePriority = 1 || onB.units.size() < (setOfVar.size() / 10);

          for(int i = 0 ; propagatePriority && i<priorityVar.size() ; i++)
            if(stampVar[priorityVar[i]] == stampIdx && s.value(priorityVar[i]) == l_Undef)
              currPriority.push(priorityVar[i]);

          ret = compileDecisionNode(connected, currPriority);
          andDecomposition.push(ret);
          if(localCache) cache->addInCache(cb, ret);
        }
        occManager->popPreviousClauseSet();
      }

      assert(nbComponent);
      if(nbComponent <= 1)
      {
        fromCache = comeFromCache[0];
        ret = andDecomposition[0];
      }
      else
      {
        if(isCertified) ret = new DecomposableAndNodeCertified<T>(andDecomposition, comeFromCache);
        else ret = new DecomposableAndNode<T>(andDecomposition);
        nbAndNode++;

        // statistics
        if(minAffectedAndNode > (s.trail).size()) minAffectedAndNode = (s.trail).size();
        sumAffectedAndNode += (s.trail).size();
      }
    }

    assert(ret);
    occManager->postUpdate(onB.units);

    if(isCertified)
    {
      if(s.decisionLevel() != s.assumptions.size()) s.refillAssums();
      for(int i = 0 ; i<setOfVar.size() ; i++)
      {
        Var v = setOfVar[i];
        if(s.value(v) != l_Undef && s.reason(v) != CRef_Undef) idxReason.push(s.ca[s.reason(v)].idxReason());
      }
    } else if(s.decisionLevel() != s.assumptions.size()) s.refillAssums();

    return ret;
  }// compile_


  /**
     Create a decision node in purpose.
  */
  DAG<T> *createObjectDecisionNode(DAG<T> *pos, onTheBranch &bPos, bool fromCachePos,
                                   DAG<T> *neg, onTheBranch &bNeg, bool fromCacheNeg,
                                   vec<int> &idxReason)
  {
    if(isCertified)
      return new BinaryDeterministicOrNodeCertified<T>(pos, bPos.units, bPos.free, fromCachePos,
                                               neg, bNeg.units, bNeg.free, fromCacheNeg, idxReason);
    return new BinaryDeterministicOrNode<T>(pos, bPos.units, bPos.free, neg, bNeg.units, bNeg.free);
  }// createDecisionNode


  /**
     This function select a variable and compile a decision node.

     @param[in] connected, the set of variable present in the current problem
     \return the compiled formula
  */
  DAG<T> *compileDecisionNode(vec<Var> &connected, vec<Var> &priorityVar)
  {
    if(s.assumptions.size() && s.assumptions.size() < 5){cout << "c top 5: "; showListLit(s.assumptions);}

    bool weCall = false;
    if(pv && !priorityVar.size() && connected.size() > 10 && connected.size() < 5000)
    {
      weCall = true;
      vec<int> cutSet;
      pv->computePartition(connected, cutSet, priorityVar, vs->getScoringFunction());

      // normally priority var is a subset of connect ???
      for(int i = 0 ; i<priorityVar.size() ; i++)
      {
        bool isIn = false;
        for(int j = 0 ; !isIn && j < connected.size() ; j++) isIn = connected[j] == priorityVar[i];
        assert(isIn);
      }

      callPartitioner++;
    }

    Var v = var_Undef;
    if(priorityVar.size()) v = vs->selectVariable(priorityVar); else v = vs->selectVariable(connected);
    if(v == var_Undef) return createTrueNode(connected);

    Lit l = mkLit(v, optReversePolarity - vs->selectPhase(v));
    nbDecisionNode++;

    onTheBranch bPos, bNeg;
    bool fromCachePos, fromCacheNeg;
    vec<int> idxReason;

    // compile the formula where l is assigned to true
    assert(s.value(l) == l_Undef);
    (s.assumptions).push(l);
    DAG<T> *pos = compile_(connected, priorityVar, l, bPos, fromCachePos, idxReason);
    (s.assumptions).pop();
    (s.cancelUntil)((s.assumptions).size());

    // compile the formula where l is assigned to true
    (s.assumptions).push(~l);
    DAG<T> *neg = compile_(connected, priorityVar, ~l, bNeg, fromCacheNeg, idxReason);

    (s.assumptions).pop();
    (s.cancelUntil)((s.assumptions).size());

    DAG<T> *ret = createObjectDecisionNode(pos, bPos, fromCachePos, neg, bNeg, fromCacheNeg, idxReason);
    return ret;
  }// compileDecisionNode


  inline void showHeader()
  {
    separator();
    printf("c %10s | %10s | %10s | %10s | %10s | %10s | %10s | %10s | %10s | %10s | %11s | %10s | \n",
           "#compile", "time", "#posHit", "#negHit", "#split", "Mem(MB)",
           "#nodes", "#edges", "#equivCall", "#Dec. Node", "#paritioner", "limit dyn");
    separator();
  }


  inline void showInter()
  {
    double now = cpuTime();

    printf("c %10d | %10.2lf | %10d | %10d | %10d | %10.0lf | %10d | %10d | %10d | %10d | %11d | %10d | \n",
           nbCallCompile, now - currentTime, cache->getNbPositiveHit(),
           cache->getNbNegativeHit(), nbSplit, memUsedPeak(), DAG<T>::nbNodes, DAG<T>::nbEdges,
           callEquiv, nbDecisionNode, callPartitioner, 0);
  }

  inline void printFinalStatsCache()
  {
    separator();
    printf("c\n");
    printf("c \033[1m\033[31mStatistics \033[0m\n");
    printf("c \033[33mCompilation Information\033[0m\n");
    printf("c Number of compiled node: %d\n", nbCallCompile);
    printf("c Number of split formula: %d\n", nbSplit);
    printf("c Number of decision node: %u\n", nbDecisionNode);
    printf("c Number of node built on domain constraints: %u\n", nbDomainConstraintNode);
    printf("c Number of decomposable AND nodes: %u\n", nbAndNode);
    printf("c Number of backbone calls: %u\n", callEquiv);
    printf("c Number of partitioner calls: %u\n", callPartitioner);
    printf("c Average number of assigned literal to obtain decomposable AND nodes: %.2lf/%d\n",
           nbAndNode ? sumAffectedAndNode / nbAndNode : s.nVars(), s.nVars());
    printf("c Minimum number of assigned variable where a decomposable AND appeared: %u\n", minAffectedAndNode);
    printf("c \n");
    printf("c \033[33mGraph Information\033[0m\n");
    printf("c Number of nodes: %d\n", DAG<T>::nbNodes);
    printf("c Number of edges: %d\n", DAG<T>::nbEdges);
    printf("c \n");
    cache->printCacheInformation();
    printf("c Final time: %lf\n", cpuTime());
    printf("c \n");
  }// printFinalStat

  inline void showRun()
  {
    if(!(nbCallCompile & (MASK_HEADER))) showHeader();
    if(nbCallCompile && !(nbCallCompile & MASK)) showInter();
  }

  inline void separator(){ printf("c "); for(int i = 0 ; i<NB_SEP_DNNF_COMPILER ; i++) printf("-"); printf("\n");}

  inline DAG<T> *createTrueNode(vec<Var> &setOfVar)
  {
    vec<Lit> unitLit;
    s.collectUnit(setOfVar, unitLit); // collect unit literals
    if(unitLit.size())
    {
      vec<Var> freeVar;
      if(!isCertified) return new UnaryNode<T>(globalTrueNode, unitLit, freeVar);

      vec<int> idxReason;
      assert(s.decisionLevel() == s.assumptions.size());
      for(int i = 0 ; i<setOfVar.size() ; i++)
      {
        Var v = setOfVar[i];
        if(s.value(v) != l_Undef && s.reason(v) != CRef_Undef) idxReason.push(s.ca[s.reason(v)].idxReason());
      }

      return new UnaryNodeCertified<T>(globalTrueNode, unitLit, false, idxReason, freeVar);
    }
    return globalTrueNode;
  }// createTrueNode

 public:
  /**
     Constructor of dDNNF compiler.

     @param[in] cnf, set of clauses
     @param[in] fWeights, the vector of literal's weight
     @param[in] c, true if the cache is activated, false otherwise
     @param[in] h, the variable heuristic name
     @param[in] p, the polarity phase heuristic name
     @param[in] _pv, the partitioner heuristic name
     @param[in] rp, true if we reverse the polarity, false otherwise
     @param[in] isProjectedVar, boolean vector used to decide if a variable is projected (true) or not (false)
  */
  DDnnfCompiler(vec<vec<Lit> > &cnf, vec<double> &wl, OptionManager &optList, vec<bool> &isProjectedVar,
                ostream *certif) : s(certif)
  {
    isCertified = certif != NULL;
    for(int i = 0 ; i<wl.size()>>1 ; i++) s.newVar();
    for(int i = 0 ; i<cnf.size() ; i++) s.addClause_(cnf[i]);

    initUnsat = !s.solveWithAssumptions();

    if(!initUnsat)
    {
      s.simplify();
      s.remove_satisfied = false;
      s.setNeedModel(false);

      callPartitioner = callEquiv = 0;
      optCached = optList.optCache;
      optDecomposableAndNode = optList.optDecomposableAndNode;
      optReversePolarity = optList.reversePolarity;
      optList.printOptions();

      // initialized the data structure
      prepareVecClauses(clauses, s);
      occManager = new DynamicOccurrenceManager(clauses, s.nVars());

      freqLimitDyn = optList.freqLimitDyn;
      cache = new CacheCNF<DAG<T> *>(optList.reduceCache, optList.strategyRedCache);
      cache->initHashTable(occManager->getNbVariable(), occManager->getNbClause(),
                           occManager->getMaxSizeClause());

      vs = new VariableHeuristicInterface(s, occManager, optList.varHeuristic,
                                          optList.phaseHeuristic, isProjectedVar);
      bm = new BucketManager<DAG<T> *>(occManager, optList.strategyRedCache);
      pv = PartitionerInterface::getPartitioner(s, occManager, optList);

      alreadyAdd.initialize(s.nVars(), false);

      stampIdx = 0;
      stampVar.initialize(s.nVars(), 0);
      em.initEquivManager(s.nVars());

      globalTrueNode = new trueNode<T>();
      globalFalseNode = new falseNode<T>();

      // statistics initialization
      minAffectedAndNode = s.nVars();
      nbSplit = nbCallCompile = 0;
      currentTime = cpuTime();
      nbAndMinusNode = nbAndNode = nbDecisionNode = nbDomainConstraintNode = nbNodeInCompile = 0;
      sumAffectedAndNode = 0;
    }

    isProjectedVar.copyTo(DAG<T>::varProjected);
    wl.copyTo(DAG<T>::weights);
    for(int i = 0 ; i<s.nVars() ; i++) DAG<T>::weightsVar.push(wl[i<<1] + wl[(i<<1) | 1]);
    if (!initUnsat) cache->setInfoFormula(s.nVars(), cnf.size(), occManager->getMaxSizeClause());
  }// DDnnfCompiler


  ~DDnnfCompiler()
  {
    if(pv) delete pv;
    delete cache; delete vs; delete bm;
    delete occManager;
  }

  /**
     Compile the CNF formula into a dDNNF structure.

     \return a DAG
  */
  rootNode<T>* compile()
  {
    DAG<T>* d = NULL;
    rootNode<T> *root = new rootNode<T>(s.nVars());
    vec<Var> freeVariable, setOfVar, priorityVar;
    DAG<T>::initSizeVector(s.nVars());
    vec<int> idxReason;

    if(initUnsat) root->assignRootNode(s.trail, new falseNode<T>(), false, s.nVars(), freeVariable, idxReason);
    else
    {
      bool fromCache = false;
      onTheBranch bData;
      if(!s.solveWithAssumptions()) d = globalFalseNode;
      else
      {
        for(int i = 0 ; i<s.nVars() ; i++) setOfVar.push(i);
        d = compile_(setOfVar, priorityVar, lit_Undef, bData, fromCache, idxReason);
      }

      assert(s.decisionLevel() == 0 && d);
      printFinalStatsCache();
      root->assignRootNode(bData.units, d, fromCache, s.nVars(), bData.free, idxReason);
    }
    return root;
  }// compile
};

#endif
