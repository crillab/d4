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
#include "Communicator.hh"


/**
   This function sends to destination the set of elements of primitive
   type int contained in v using MPI w.r.t. a communicator and
   assigned to a tag given in parameter.

   @param[in] v, the set of cpp_int we want to send
   @param[in] destination, the MPI rank  of the destination
   @param[in] tag, the tag for the message
   @param[in] communicator, the communicator we want to send the message on.
 */
void Communicator::sendVecOf(vector<int> &v, int destination, int tag, MPI_Comm communicator)
{
  int vt[v.size()];
  int smsg = v.size();
  for(int i = 0 ; i<v.size() ; i++) vt[i] = v[i];

  MPI_Send(&smsg, 1, MPI_INT, destination, tag, communicator);
  if(smsg) MPI_Send(vt, smsg, MPI_INT, destination, tag, communicator);
}// sendVecOfPrimitive

/**
   This function sends to destination the set of elements of primitive
   type char contained in v using MPI w.r.t. a communicator and
   assigned to a tag given in parameter.

   @param[in] v, the set of cpp_int we want to send
   @param[in] destination, the MPI rank  of the destination
   @param[in] tag, the tag for the message
   @param[in] communicator, the communicator we want to send the message on.
 */
void Communicator::sendVecOf(vector<char> &v, int destination, int tag, MPI_Comm communicator)
{
  char vt[v.size()];
  int smsg = v.size();
  for(int i = 0 ; i<v.size() ; i++) vt[i] = v[i];

  MPI_Send(&smsg, 1, MPI_INT, destination, tag, communicator);
  if(smsg) MPI_Send(vt, smsg, MPI_CHAR, destination, tag, communicator);
}// sendVecOfPrimitive


/**
   This function sends to destination the set of elements contained in
   v (that is cpp_int elements) using MPI w.r.t. a communicator and
   assigned to a tag given in parameter.

   @param[in] v, the set of cpp_int we want to send
   @param[in] destination, the MPI rank  of the destination
   @param[in] tag, the tag for the message
   @param[in] communicator, the communicator we want to send the message on.
 */
void Communicator::sendVecOf(vector<mpz_int> &v, int destination, int tag, MPI_Comm communicator)
{
  // send the number of element we plan to send
  int smsg = v.size();
  MPI_Send(&smsg, 1, MPI_INT, destination, tag, communicator);

  // send the serialized element of the vector one by one
  for(int i = 0 ; i<v.size() ; i++)
    {
      vector<unsigned char> w;
      cpp_int tmp = cpp_int(v[i]);
      export_bits(tmp, back_inserter(w), 8);

      unsigned char tsend[w.size()];
      for(int j = 0 ; j<w.size() ; j++) tsend[j] = w[j];

      smsg = w.size();
      MPI_Send(&smsg, 1, MPI_INT, destination, tag, communicator);
      MPI_Send(tsend, smsg, MPI_CHAR, destination, tag, communicator);
    }
}// sendVecOfCppInt


/**
   This function sends to destination the set of elements contained in
   v (that is cpp_int elements) using MPI w.r.t. a communicator and
   assigned to a tag given in parameter.

   @param[in] v, the set of cpp_int we want to send
   @param[in] destination, the MPI rank  of the destination
   @param[in] tag, the tag for the message
   @param[in] communicator, the communicator we want to send the message on.
 */
void Communicator::sendVecOf(vector<mpf_float> &v, int destination, int tag, MPI_Comm communicator)
{
  assert(mpf_float::default_precision() <= MAX_PRECISION);

  // send the number of element we plan to send
  int smsg = v.size();

  cerr << "send " << smsg << endl;
  MPI_Send(&smsg, 1, MPI_INT, destination, tag, communicator);

  // send the serialized element of the vector one by one
  for(int i = 0 ; i<v.size() ; i++)
    {
      // send the exponent
      number<cpp_bin_float<MAX_PRECISION> > tmp = number<cpp_bin_float<MAX_PRECISION> >(v[i]);

      smsg = tmp.backend().exponent();
      MPI_Send(&smsg, 1, MPI_INT, destination, tag, communicator);

      vector<unsigned char> w;
      export_bits(cpp_int(tmp.backend().bits()), back_inserter(w), 8);

      unsigned char tsend[w.size()];
      for(int j = 0 ; j<w.size() ; j++) tsend[j] = w[j];

      smsg = w.size();
      MPI_Send(&smsg, 1, MPI_INT, destination, tag, communicator);
      MPI_Send(tsend, smsg, MPI_CHAR, destination, tag, communicator);
    }
}// sendVecOfCppInt


/**
   This function receives from source a set of elements of primitive
   type int contained that is strored in v using MPI w.r.t. a
   communicator and assigned to a tag given in parameter.

   @param[out] v, the set of cpp_int we want to send
   @param[in] destination, the MPI rank  of the destination
   @param[in] tag, the tag for the message
   @param[in] communicator, the communicator we want to send the message on.
   @param[out] status, what is the status of the recv message
 */
void Communicator::recvVecOf(vector<int> &v, int source, int tag, MPI_Comm communicator, MPI_Status* status)
{
  v.clear();

  int recv;
  MPI_Recv(&recv, 1, MPI_INT, source, tag, communicator, status);

  if(recv)
    {
      int rtmp[recv];
      MPI_Recv(rtmp, recv, MPI_INT, source, tag, communicator, status);
      for(int i = 0 ; i<recv ; i++) v.push_back(rtmp[i]);
    }
}// recvVecOfPrimitive


/**
   This function receives from source a set of elements of primitive
   type char contained that is strored in v using MPI w.r.t. a
   communicator and assigned to a tag given in parameter.

   @param[out] v, the set of cpp_int we want to send
   @param[in] destination, the MPI rank  of the destination
   @param[in] tag, the tag for the message
   @param[in] communicator, the communicator we want to send the message on.
   @param[out] status, what is the status of the recv message
 */
void Communicator::recvVecOf(vector<char> &v, int source, int tag, MPI_Comm communicator, MPI_Status* status)
{
  v.clear();

  int recv;
  MPI_Recv(&recv, 1, MPI_INT, source, tag, communicator, status);

  if(recv)
    {
      char rtmp[recv];
      MPI_Recv(rtmp, recv, MPI_CHAR, source, tag, communicator, status);
      for(int i = 0 ; i<recv ; i++) v.push_back(rtmp[i]);
    }
}// recvVecOfPrimitive

/**
   This function receives from source a set of elements contained that
   is strored in v (that is cpp_int elements) using MPI w.r.t. a
   communicator and assigned to a tag given in parameter.

   @param[out] v, the set of cpp_int we want to send
   @param[in] destination, the MPI rank  of the destination
   @param[in] tag, the tag for the message
   @param[in] communicator, the communicator we want to send the message on.
   @param[out] status, what is the status of the recv message
 */
void Communicator::recvVecOf(vector<mpz_int> &v, int source, int tag, MPI_Comm communicator, MPI_Status* status)
{
  v.clear();

  // what is the number of element we are going to receive
  int recv;
  MPI_Recv(&recv, 1, MPI_INT, source, tag, communicator, status);

  for(int i = 0 ; i<recv ; i++)
    {
      int nbElt;
      MPI_Recv(&nbElt, 1, MPI_INT, source, tag, communicator, status);

      unsigned char trecv[nbElt];
      MPI_Recv(trecv, nbElt, MPI_CHAR, source, tag, communicator, status);

      vector<unsigned char> w;
      for(int j = 0 ; j<nbElt ; j++) w.push_back(trecv[j]);

      cpp_int rval;
      import_bits(rval, w.begin(), w.end());
      v.push_back(mpz_int(rval));
    }
}// recvVecOfCppInt


/**
   This function receives from source a set of elements contained that
   is strored in v (that is cpp_int elements) using MPI w.r.t. a
   communicator and assigned to a tag given in parameter.

   @param[out] v, the set of cpp_int we want to send
   @param[in] destination, the MPI rank  of the destination
   @param[in] tag, the tag for the message
   @param[in] communicator, the communicator we want to send the message on.
   @param[out] status, what is the status of the recv message
 */
void Communicator::recvVecOf(vector<mpf_float> &v, int source, int tag, MPI_Comm comm, MPI_Status* status)
{
  assert(mpf_float::default_precision() <= MAX_PRECISION);
  v.clear();

  // what is the number of element we are going to receive
  int recv;
  cerr << "recv " << recv << endl;
  MPI_Recv(&recv, 1, MPI_INT, source, tag, comm, status);

  for(int i = 0 ; i<recv ; i++)
    {
      // collect the exponent
      int e;
      MPI_Recv(&e, 1, MPI_INT, source, tag, comm, status);

      int nbElt;
      MPI_Recv(&nbElt, 1, MPI_INT, source, tag, comm, status);

      unsigned char trecv[nbElt];
      MPI_Recv(trecv, nbElt, MPI_CHAR, source, tag, comm, status);

      vector<unsigned char> w;
      for(int j = 0 ; j<nbElt ; j++) w.push_back(trecv[j]);

      cpp_int rbits;
      import_bits(rbits, w.begin(), w.end());

      number<cpp_bin_float<MAX_PRECISION> > rval(rbits);
      rval.backend().exponent() = e;
      v.push_back(mpf_float(rval));
    }
}// recvVecOfCppInt
#endif
