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
#ifndef STATIC_COMP
#ifndef UTILS_COMMUNICATOR
#define UTILS_COMMUNICATOR

#include <vector>
#include <mpi.h>

#include "../mtl/Vec.hh"

#include <boost/multiprecision/gmp.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

using namespace boost::multiprecision;
using namespace std;

#define MAX_PRECISION 8192

class Communicator
{
public:
  void sendVecOf(vector<int> &v, int destination, int tag, MPI_Comm communicator);
  void sendVecOf(vector<char> &v, int destination, int tag, MPI_Comm communicator);
  void sendVecOf(vector<mpz_int> &v, int destination, int tag, MPI_Comm communicator);
  void sendVecOf(vector<mpf_float> &v, int destination, int tag, MPI_Comm communicator);

  void recvVecOf(vector<int> &v, int source, int tag, MPI_Comm communicator, MPI_Status* status);
  void recvVecOf(vector<char> &v, int source, int tag, MPI_Comm communicator, MPI_Status* status);
  void recvVecOf(vector<mpz_int> &v, int source, int tag, MPI_Comm communicator, MPI_Status* status);
  void recvVecOf(vector<mpf_float> &v, int source, int tag, MPI_Comm communicator, MPI_Status* status);
};

#endif
#endif
