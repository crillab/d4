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
/****************************************************************************************[System.h]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef System_h
#define System_h

#if defined(__linux__)
#include <fpu_control.h>
#endif

#include "../mtl/IntTypes.hh"

//-------------------------------------------------------------------------------------------------

static inline double cpuTime(void); // CPU-time in seconds.
extern double memUsed();            // Memory in mega bytes (returns 0 for unsupported architectures).
extern double memUsedPeak();        // Peak-memory in mega bytes (returns 0 for unsupported architectures).


//-------------------------------------------------------------------------------------------------
// Implementation of inline functions:

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <time.h>
static inline double cpuTime(void) { return (double)clock() / CLOCKS_PER_SEC; }
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>


static struct timeval initTime;

static inline double init_wall_time(){  
  gettimeofday(&initTime,NULL);
  return (double) initTime.tv_sec + (double)initTime.tv_usec * .000001;
}


static inline double cpuTime(void)
{
  static double start_wall_time = init_wall_time();

  gettimeofday(&initTime, NULL);
  double current_wall_time = (double)initTime.tv_sec + (double)initTime.tv_usec * .000001;
  double result = current_wall_time-start_wall_time;
  return result; 
}

#endif

#endif
