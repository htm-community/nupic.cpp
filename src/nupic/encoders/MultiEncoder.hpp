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

#ifndef NTA_multiencoder_HPP
#define NTA_multiencoder_HPP

#include <iostream>
#include <string> 
#include <vector>
#include <tuple>
#include <map> 

#include "Encoder.hpp"       
#include "ScalarEncoder2.hpp"    
#include "AdaptiveScalarEncoder.hpp"    
#include "DateEncoder.hpp"    
#include "CategoryEncoder.hpp"    
#include "SDRCategoryEncoder.hpp"    
#include "PassThroughEncoder.hpp"    
#include "SparsePassThroughEncoder.hpp"    
#include "CoordinateEncoder.hpp"    
#include "GeospatialCoordinateEncoder.hpp"    
#include "RandomDistributedScalarEncoder.hpp"   

#include "Utils.hpp" 
#include "RangeList.hpp"
#include "DecodeResult.hpp"

#include <boost/any.hpp>

using namespace std;

namespace encoders {

	/**
	* A MultiEncoder encodes a dictionary or object with
	* multiple components. A MultiEncode contains a number
	* of sub-encoders, each of which encodes a separate component.
	*
	* @author wlmiller
	*/
	template<typename TMulti>
	class MultiEncoder {

	public:

		/***************** constructors and destructor *****************/
		MultiEncoder();
		~MultiEncoder();

		/***************** methods *****************/

		vector<tuple<string, int> > getDescription() const { return description_M; };

		vector<tuple<string, Encoder<boost::any>*, nupic::UInt> >  getMultiEncoder() { return multiEncoder; };

		nupic::UInt getWidth() { return width_M; };

		void setName(string name) { name_M = name; };
		/////////////////

		void addEncoder(Encoder<boost::any>* enc, string encName);

		vector<nupic::UInt> encode(TMulti inputData);

		DecodeResult decode(vector<nupic::UInt> encoded, string parentFieldName);

		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownCompute(vector<nupic::UInt> encoded);


	protected:

		nupic::UInt width_M;

		vector<tuple<string, int> > description_M;

		string name_M;
		///////////

		vector<tuple<string, Encoder<boost::any>*, nupic::UInt> > multiEncoder;

	}; // end class MultiEncoder


	   /***************** Start of Implementation ******************/
	template<typename TMulti>
	MultiEncoder<TMulti>::MultiEncoder() {}

	template<typename TMulti>
	MultiEncoder<TMulti>::~MultiEncoder() {}


	template<typename TMulti>
	void MultiEncoder<TMulti>::addEncoder(Encoder<boost::any>* enc, string encName) {

		multiEncoder.push_back(tuple<string, Encoder<boost::any>*, nupic::UInt>(encName, enc, width_M));

		for (auto d : enc->getDescription()) {
			tuple<string, int> dT = d;
			description_M.push_back(tuple<string, int>(get<0>(dT), (int)get<1>(dT) + width_M));
		}

		width_M += enc->getWidth(); 
	}


	template<typename TMulti>
	vector<nupic::UInt> MultiEncoder<TMulti>::encode(TMulti inputData) {

		vector<nupic::UInt> multiOutput;

		for (tuple<string, Encoder<boost::any> *, nupic::UInt> t : multiEncoder) {
			string name = get<0>(t);
			Encoder<boost::any> * enc = get<1>(t);
			vector<nupic::UInt> scalarOutput = enc->encode(inputData[name]);
			multiOutput.insert(multiOutput.end(), scalarOutput.begin(), scalarOutput.end());

		}

		return multiOutput;
	}

	template<typename TMulti>
	DecodeResult MultiEncoder<TMulti>::decode(vector<nupic::UInt> encoded, string parentFieldName) {

		map<string, RangeList> fieldsMap;
		vector<string> fieldsOrder;
		vector<tuple<string, Encoder<boost::any> *, nupic::UInt> > enc = multiEncoder;

		string parentName = parentFieldName.empty() || parentFieldName == "" ? name_M : parentFieldName + "." + name_M;

		const int len = enc.size();
		for (int i = 0; i < len; i++) {
			int nextOffset = 0;
			if (i < len - 1) {
				nextOffset = (int)get<2>(enc[i + 1]);
			}
			else {
				nextOffset = width_M;
			}
			vector<nupic::UInt> fieldOutput = Utils<nupic::UInt>::sub(encoded, Utils<int>::range((int)get<2>(enc[i]), nextOffset));
			DecodeResult result = get<1>(enc[i])->decode(fieldOutput, parentName);

			map<string, RangeList> fields = result.getFields();
			vector<string> fieldsDesc = result.getDescriptions();

			fieldsMap.insert(fields.begin(), fields.end());
			fieldsOrder.insert(fieldsOrder.end(), fieldsDesc.begin(), fieldsDesc.end());
		}
		return DecodeResult(fieldsMap, fieldsOrder);

	}

	template<typename TMulti>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > MultiEncoder<TMulti>::topDownCompute(vector<nupic::UInt> encoded) {
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > retVals;

		vector<tuple<string, Encoder<boost::any> *, nupic::UInt> > encoders = multiEncoder;
		cout << " test" << endl;
		const int len = encoders.size();
		for (int i = 0; i < len; i++) {
			int offset = (int)get<2>(encoders[i]);

			int nextOffset;
			if (i < len - 1) {
				nextOffset = (int)get<2>(encoders[i + 1]);
			}
			else {
				nextOffset = width_M;
			}
			cout << " test 3" << endl;
			vector<nupic::UInt> fieldOutput = Utils<nupic::UInt>::sub(encoded, Utils<int>::range(offset, nextOffset));
			cout << " test 4" << endl; 
			vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > values = get<1>(encoders[i])->topDownCompute(fieldOutput);


			retVals.insert(retVals.end(), values.begin(), values.end());
		}

		return retVals;
	}

}; // end namespace 
#endif // NTA_multiencoder_HPP
