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
#ifndef NTA_coordinateencoder_HPP
#define NTA_coordinateencoder_HPP

#include <algorithm> //min
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <tuple> 
#include <algorithm>

#include <nupic/encoders/Encoder.hpp>
#include <nupic/encoders/Utils.hpp>
#include <nupic/encoders/md5.hpp> //TODO possible update to https://tls.mbed.org/md5-source-code , but it's C
#include <nupic/utils/Random.hpp>

#define _USE_BIG_INTEGER__ //Always ise BIG_INTEGER, the other way does not work
#ifdef _USE_BIG_INTEGER__
#include <nupic/encoders/bigInt.h>
#endif

using namespace std;
using namespace nupic;
using coord_t = vector<int>;

namespace encoders {

////// INTERFACE 
        class CoordinateOrder {     // An interface class
        public:
                CoordinateOrder() {};
                virtual ~CoordinateOrder() {};   // destructor, use it to call destructor of the inherit classes

                /**
                * Returns the order for a coordinate.
                *
                * @param coordinate             coordinate array
                *
                * @return       A value in the interval [0, 1), representing the
                *          order of the coordinate
                */
                virtual double orderForCoordinate(coord_t coordinate) const =0;

        }; // end class CoordinateOrder

////////

	/**
	* Given a coordinate in an N-dimensional space, and a radius around
	* that coordinate, the Coordinate Encoder returns an SDR representation
	* of that position.
	* The Coordinate Encoder uses an N-dimensional integer coordinate space.
	* For example, a valid coordinate in this space is (150, -49, 58), whereas
	* an invalid coordinate would be (55.4, -5, 85.8475).
	* It uses the following algorithm:
	* 1. Find all the coordinates around the input coordinate, within the
	* specified radius.
	* 2. For each coordinate, use a uniform hash function to
	* deterministically map it to a real number between 0 and 1. This is the
	* "order" of the coordinate.
	* 3. Of these coordinates, pick the top W by order, where W is the
	* number of active bits desired in the SDR.
	* 4. For each of these W coordinates, use a uniform hash function to
	* deterministically map it to one of the bits in the SDR. Make this bit active.
	* 5. This results in a final SDR with exactly W bits active
	* (barring chance hash collisions).
	* 
	*/
	template<typename T >
	class CoordinateEncoder :  public Encoder<T>, public CoordinateOrder
	{
	public:
		/***************** constructors and destructor *****************/

		/**
		* Package private to encourage construction using the Builder Pattern
		* but still allow inheritance.
		*/
		CoordinateEncoder(int w =21, int n =1024, double radius=1.0, bool retinaMode=false, vector<UInt> DIMs_nD=vector<UInt>{}); //TODO separate Retina to a standalone constructor?
		~CoordinateEncoder();

		/***************** methods *****************/
        vector<nupic::UInt> encode(coord_t coordinate, double radius=0.0); //TODO move this to Encoder itself! //TODO if we made radius part of constructor, we can use default Encode.encode()

		/**
		* Returns coordinates around given coordinate, within given radius.
		* Includes given coordinate.
		*
		* @param coordinate		Coordinate whose neighbors to find
		* @param radius			Radius around `coordinate`
		* @return
		*/
		vector<coord_t> neighbors(coord_t coordinate, int radius) const;

		/**
		* Returns the top W coordinates by order.
		*
		* @param co				Implementation of {@link CoordinateOrder}
		* @param coordinates	A 2D array, where each element
		*						is a coordinate
		* @param w				(int) Number of top coordinates to return
		* @return
		*/
		vector<coord_t > topWCoordinates(CoordinateOrder *co, vector<coord_t >  coordinates, UInt w) const;

		/**
		* see nupic coordinate.py for more information
		*
		* Hash a coordinate to a 64 bit integer.
		* @return
		*/
		unsigned long long hashCoordinate(coord_t coordinate) const;

		/**
		* Returns the order for a coordinate.
		*
		* @param coordinate		coordinate array
		*
		* @return	A value in the interval [0, 1), representing the
		*          order of the coordinate
		* @Override
		*/	
		virtual double orderForCoordinate(coord_t coordinate) const override;

		/**
		* see nupic coordinate.py for more information
		*
		* Maps the coordinate to a bit in the SDR.
		* Returns the order for a coordinate.
		*
		* @param coordinate		coordinate array
		* @param n				the number of available bits in the SDR
		*
		* @return	The index to a bit in the SDR
		*/
		UInt bitForCoordinate(coord_t  coordinate, UInt n) const;

		/**
		* Encodes inputData and puts the encoded value into the output array,
		* which is a 1-D array of length returned by {@link Connections#getW()}.
		*
		* Note: The output array is reused, so clear it before updating it.
		*
		* @param inputData		Data to encode. This should be validated by the encoder.
		* @param output			1-D array of same length returned by {@link Connections#getW()}
		* @Override
		*/
		virtual void encodeIntoArray(T inputData, vector<nupic::UInt>& output) override;

		/**
		* Should return the output width, in bits.
		* @Override
		*/
		virtual int getWidth() const override;
		 		  
		virtual DecodeResult decode(vector<nupic::UInt> encoded, std::string parentFieldName) override;

		virtual vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownCompute(vector<nupic::UInt> encoded) override;

	private:
#ifndef _USE_BIG_INTEGER__		
		double pow2_64 = pow(2.,64);
#endif
vector<UInt> DIMs_nD;
bool retina_;

	}; // end class CoordinateEncoder


	/***************** Start of Implementation ******************/
	template<class T>
	CoordinateEncoder<T>::CoordinateEncoder(int w, int n, double radius, bool retinaMode, vector<UInt> DIMs_nD) : //FIXME fails for w>=27, crash for w>=27 &range <=2
          Encoder<T>(w, n),
     DIMs_nD(DIMs_nD),
  retina_(retinaMode)
	{
if(retina_) {
  cout << "Using RETINA mode!"<<endl;
}
	this->setRadius(radius);
	
		//description has a List of Tuple containing		
		this->description_.push_back(tuple<string, int> ("coordinate", 0));
		this->description_.push_back(tuple<string, int> ("radius", 1));
    NTA_CHECK(radius >= 1);
    if(retinaMode) { 
     const auto product = std::accumulate(DIMs_nD.begin(), DIMs_nD.end(), 1, std::multiplies<int>());
      NTA_CHECK(product == n); 
    }

	}

	template<class T>
	CoordinateEncoder<T>::~CoordinateEncoder() {}

    // convenience method encode()
    template<class T>
    vector<nupic::UInt> CoordinateEncoder<T>::encode(coord_t coordinate, double radius){
    assert(coordinate.size()>0);
    assert(radius>0);
  this->setRadius(radius);
 
		vector<nupic::UInt> output(this->getWidth(), 0);
		this->encodeIntoArray(coordinate, output); 
//!    assert(Utils<nupic::UInt>::sum(output)==(UInt)this->getW()); //Coord does NOT honor w bits in output //TODO fix?
		return output;
    }


	template<class T>
	vector<coord_t> CoordinateEncoder<T>::neighbors(coord_t coordinate, int radius) const{
		vector<coord_t> ranges;
		ranges.reserve(coordinate.size()); 

		for(coord_t::const_iterator it=coordinate.begin(); it != coordinate.end(); ++it)
			ranges.push_back( Utils<int>::range(*it - radius, *it + radius + 1));
		
		vector<coord_t > retVal;
		int len = ranges.size() == 1 ? 1 : ranges[0].size();
		retVal.reserve(ranges[0].size() * len);

		for(vector <int>::const_iterator it=ranges[0].begin(); it != ranges[0].end(); ++it) {
			for(int j = 0;j < len;j++) {
				coord_t entry(ranges.size(), 0);
				entry[0] = *it;
				for(size_t i = 1; i < ranges.size();i++) {
					entry[i] = ranges[i][j];
				}
				retVal.push_back(entry);
			}
		} 
		return retVal;
	}

	template<class T>
	vector<coord_t > CoordinateEncoder<T>::topWCoordinates(CoordinateOrder *co, vector<coord_t >  coordinates, UInt w) const{
      assert(co!=nullptr);
      assert(w > 0);
//!      assert(coordinates.size()>= w);  //sometimes neighbors() cannot provide more to >= w -> crop w
  w = min((size_t)w, coordinates.size());

		vector<pair<double, int>> pairs;
		pairs.reserve (coordinates.size());
		size_t i = 0;
		for(auto coord : coordinates)
			pairs.push_back(pair<double, int>(co->orderForCoordinate(coord), i++));		 
		
		sort(pairs.begin(), pairs.end());
		vector<coord_t > topCoordinates;
		topCoordinates.reserve(w); 

		for(UInt wIdx = pairs.size() -w; wIdx <pairs.size() ; wIdx++)
			topCoordinates.push_back ( coordinates[pairs[wIdx].second]); 
		
                assert(topCoordinates.size()==w);
		return topCoordinates;		 
	}


	template<class T>
	unsigned long long CoordinateEncoder<T>::hashCoordinate(coord_t coordinate) const{

		if (coordinate.empty()) return 0;

		ostringstream coordinateStr ;

		// Convert all but the last element to avoid a trailing ","
		copy(coordinate.begin(), coordinate.end()-1, ostream_iterator<int>(coordinateStr, ",")); //TODO use VectorHelpers::printVector() instead?
		// Now add the last element with no delimiter
		coordinateStr << coordinate.back();

		// Compute the hash and convert to 64 bit int.
		string hexString = md5( coordinateStr.str());
#ifdef _USE_BIG_INTEGER__
		BigInt::Rossi d2p64("18446744073709551616", BigInt::DEC_DIGIT);  // value of pow(2,64)
		BigInt::Rossi rHexStr(hexString, BigInt::HEX_DIGIT); //TODO implement this in VectorHelpers::hash() ?
		rHexStr = rHexStr % d2p64;
		unsigned long long hash = rHexStr.toUnit();
#else
		double  hexToInt = strtod(hexString.c_str(), NULL);
		unsigned long long hash = static_cast<unsigned long long>(fmod(hexToInt , pow2_64) ); //FIXME this section doesn't work
#endif
		//std::cout << "hash:" << hash << std::endl;
		return hash;
	}


	template<class T>
	double CoordinateEncoder<T>::orderForCoordinate(coord_t coordinate) const { 
		// see nupic coordinate.py for more information
		nupic::UInt64 seed = static_cast<nupic::UInt64>(hashCoordinate(coordinate));
		return Random(seed).getReal64();					
	}

	template<class T>
	UInt CoordinateEncoder<T>::bitForCoordinate(coord_t  coordinate, UInt n) const {
		// see nupic coordinate.py for more information
		nupic::UInt64 seed = static_cast<nupic::UInt64>(hashCoordinate(coordinate));
		return Random(seed).getUInt32(n);
	}

	template<class T>		 
	void CoordinateEncoder<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output){
          if(retina_) { //RETINA mode (completely different from Coord, just hacked in here
            NTA_CHECK(inputData.size()>=DIMs_nD.size());
            NTA_CHECK(output.size()==(size_t)this->getWidth()); //=n
            const auto dimX = DIMs_nD[0];
            const auto x = inputData[0];
NTA_ASSERT(x < static_cast<const int>(dimX)); //x in [0..DIMX-1]
            const auto dimY = DIMs_nD[1];
            const auto y = inputData[1];
NTA_ASSERT(y < static_cast<const int>(dimY)); //y in 0..DIM_Y-1
//            if(DIMs_nD.size()>2)  //add polarity as 3rd, Z-dimension
            const auto flattened = y*dimX + x;
NTA_ASSERT(flattened >=0);

            output[flattened] = 1;
            return;
          }

        auto input = boost::any_cast<coord_t>(inputData); //TODO try replace this cast by using T everywhere
		vector<coord_t > neighs = neighbors( input, this->getRadius());
		vector< coord_t> winners = topWCoordinates(this, neighs, this->getW());

	for(auto win: winners) {
	  UInt bit = bitForCoordinate(win, this->getN());
	  output[bit] = 1;
	}
	}

	template<class T>
	int CoordinateEncoder<T>::getWidth() const{
		return this->getN();
	}
	 
	template<class T>
	DecodeResult CoordinateEncoder<T>::decode(vector<nupic::UInt> encoded, std::string parentFieldName) {	
		return DecodeResult(); //TODO dummy, implement proper
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > CoordinateEncoder<T>::topDownCompute(vector<nupic::UInt> encoded) {
		return vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >(); //TODO dummy, implement proper
	}

}; // end namespace  
#endif // NTA_coordinateencoder_HPP
