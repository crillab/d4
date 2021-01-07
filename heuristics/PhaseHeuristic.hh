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

class PhaseSelection
{
public:
  virtual ~PhaseSelection(){;}
  virtual bool selectPhase(Var v) = 0;
};

class PhaseTrue : public PhaseSelection
{
public:
  bool selectPhase(Var v){return true;}
};

class PhaseFalse : public PhaseSelection
{
public:
  bool selectPhase(Var v){return false;}
};

class PhasePolarity : public PhaseSelection
{
private:
  vec<char> &polarity;  
    
public:
  PhasePolarity(vec<char> &p) : polarity(p){}    
  bool selectPhase(Var v){return polarity[v];}
};

class PhaseOccurrence : public PhaseSelection
{
private:
  OccurrenceManagerInterface *om;
    
public:
  PhaseOccurrence(OccurrenceManagerInterface *_om) : om(_om){}    

  inline bool selectPhase(Var v){assert(om); return om->getNbClause(mkLit(v, false)) < om->getNbClause(mkLit(v, true));}
};
