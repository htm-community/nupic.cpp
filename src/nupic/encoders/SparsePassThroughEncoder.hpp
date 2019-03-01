/* ---------------------------------------------------------------------
* Numenta Platform for Intelligent Computing (NuPIC)
* Copyright (C) 2014-2016, Numenta, In  Unless you have an agreement
* with Numenta, In, for a separate license for this software code, the
* following terms and conditions apply:
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero Public License version 3 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Affero Public License for more details.
*
* You should have received a copy of the GNU Affero Public License
* along with this program.  If not, see http://www.gnu.org/licenses.
*
* http://numenta.org/licenses/
* ---------------------------------------------------------------------
*/

#ifndef NTA_sparsepassthroughencoder_HPP
#define NTA_sparsepassthroughencoder_HPP

#include <iostream>
#include <string> 
#include <vector>
#include <tuple>
#include <map> 

#include <nupic/encoders/PassThroughEncoder.hpp>

using namespace std;
using nupic::UInt;

namespace encoders {

	/**
	* Sparse Pass Through Encoder
	* Convert a bitmap encoded as array indices to an SDR
	* Each encoding is an SDR in which w out of n bits are turned on.
	* The input should be an array or string of indices to turn on
	* Note: the value for n must equal input length * w
	* i.e. for n=8 w=1 [0,2,5] => 101001000
	* or for n=8 w=1 "0,2,5" => 101001000
	* i.e. for n=24 w=3 [0,2,5] => 111000111000000111000000000
	* or for n=24 w=3 "0,2,5" => 111000111000000111000000000
	*
	* @author wilsondy (from Python original)
	*/
	template<typename T>
	class SparsePassThroughEncoder : public PassThroughEncoder<T> {

	public:

		/***************** constructors and destructor *****************/
		SparsePassThroughEncoder(); 
		~SparsePassThroughEncoder();


  SparsePassThroughEncoder(int w, int n) {
  init(n, w);
  }
		/***************** methods *****************/

		/**		
		* @param n				 -- is the total #bits in output
		* @param w				 -- is used to normalize the sparsity of the output, exactly w bits ON,
		*						    if 1 (default) - do not alter the input, just pass it further.
		* @param name			 -- an optional string which will become part of the description
		*
		* @Override
		*/
		void init( int n, int w=1, string name="SparsePassThru");

		/**
		* Convert the array of indices to a bit array and then pass to parent.
		*
		* @param inputData		Data to encode. This should be validated by the encoder.
		* @param output			1-D array of same length returned by getWidth()
		*
		* @Override
		*/
		virtual void encodeIntoArray(T inputData, vector<nupic::UInt> &output) override;

  //helper to encode 2d coords
    vector<UInt> encode2d(int x, int y, size_t dim_x, size_t dim_y);

	}; // end class SparsePassThroughEncoder

	/***************** Start of Implementation ******************/
	template<class T>
	SparsePassThroughEncoder<T>::SparsePassThroughEncoder(){}

	template<class T>
	SparsePassThroughEncoder<T>::~SparsePassThroughEncoder() {} 

	template<class T>
	void SparsePassThroughEncoder<T>::init( int n, int w, string name) {
		PassThroughEncoder<T>::init( n, w, name);
	}

	template<class T>
	void SparsePassThroughEncoder<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output) {
          T dense(output.size());
          for(const auto i: inputData) {
            dense.at(i) = 1;
          }
		PassThroughEncoder<T>::encodeIntoArray(dense, output);
	} 	

}; // end namespace 
#endif // NTA_sparsepassthroughencoder_HPP
