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
#include "Backbone.hh"

using namespace std;

/**
  We compute the backbone.
 */
void Backbone::run()
{
  double currTime = cpuTime();  

  vec<lbool> currentModel;
  os.setNeedModel(true);
  bool ret = os.solve();
  if(!ret)
    {
      cerr << "c Warning the problem is UNSAT" << endl;
      return;
    }
  (os.model()).copyTo(currentModel);

  for(int v = 0 ; v<os.nVars() ; v++)
    {       
      // l is implied?
      if(os.value(v) != l_Undef || currentModel[v] == l_Undef) continue;
      Lit l = mkLit(v, currentModel[v] == l_False);
      
      if(!os.solve(~l)){if(os.value(l) == l_Undef) os.uncheckedEnqueue(l);}
      else
        {
          for(int i = v ; i<os.nVars() ; i++)
            if(currentModel[i] != os.model()[i]) currentModel[i] = l_Undef;
        }
      os.cancelUntil(0);      
    }
  
  (os.getTrail()).copyTo(backbone);
  timeBackone += cpuTime() - currTime;
  os.setNeedModel(false);
}// run
