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
#ifndef NTA_geospatialcoordinateencoder_HPP
#define NTA_geospatialcoordinateencoder_HPP

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <algorithm>
#define _USE_MATH_DEFINES 
#include <math.h>

#include "Encoder.hpp"
#include "CoordinateEncoder.hpp" 

using namespace std;
namespace encoders {

	/**
	* Given a GPS coordinate and a speed reading, the
	* Geospatial Coordinate Encoder returns an SDR representation
	* of that position.
	*/
	template<typename T>
	class GeospatialCoordinateEncoder : public CoordinateEncoder<T>
	{
	public:
		/***************** constructors and destructor *****************/
		GeospatialCoordinateEncoder();
		~GeospatialCoordinateEncoder();

		/***************** methods *****************/

		/**
		* @param n		--	is  total bits in output
		* @param w		-- number of bits to set in output
		* @param name	-- an optional string which will become part of the description
		*/
		void init(int n, int w, string name, int scale, int timeStep);

		/**
		* Encodes inputData and puts the encoded value into the output array,
		* which is a 1-D array of length returned by {@link Connections#getW()}.
		*
		* Note: The output array is reused, so clear it before updating it.
		* @param inputData		Data to encode. This should be validated by the encoder.
		* @param output			1-D array of same length returned by {@link Connections#getW()}
		* @Override
		*/
		virtual void encodeIntoArrayB(double longitude, double lattitude, double radius, vector<UInt>& output);
		//virtual void encodeIntoArray(T inputData, vector<UInt>& output);

		/**
		* Returns coordinate for given GPS position.
		*
		* @param longitude	(float) Longitude of position
		* @param latitude	(float) Latitude of position
		* @param altitude	(float) Altitude of position
		* @return (numpy.array) Coordinate that the given GPS position maps to
		*
		*/
		vector<int> coordinateForPosition(double longitude, double lattitude);

		/**
		* Returns coordinates converted to Mercator Spherical projection
		*
		* @param longitude	the longitude
		* @param lattitude	the lattitude
		* @return
		*/
		vector<double> toMercator(double longitude, double lattitude);

		/**
		* Returns coordinates converted to Long/Lat from Mercator Spherical projection
		*
		* @param longitude	the longitude
		* @param lattitude	the lattitude
		* @return
		*/
		vector<double> inverseMercator(double x, double y);

		/**
		* Tries to get the encodings of consecutive readings to be
		* adjacent with some overlap.
		*
		* @param speed	Speed (in meters per second)
		* @return	Radius for given speed
		*/
		double radiusForSpeed(double speed);

		/**
		* Scale of the map, as measured by
		* distance between two coordinates
		* (in meters per dimensional unit)
		* @param scale
		* @return
		*/
		void setScale(int scale);

		/**
		* Time between readings
		* @param timestep
		* @return
		*/
		void setTimestep(int timestep);

		/**
		* @return scale of the map
		*/
		int getScale();

		/**
		* @return Time between readings
		*/
		int getTimestep();
 
	private:

		/**
		* Scale of the map, as measured by
		* distance between two coordinates
		* (in meters per dimensional unit)
		*/
		int scale_;

		/* Time between readings (in seconds) */
		int timestep_;

	}; // end class GeospatialCoordinateEncoder


	/***************** Start of Implementation ******************/

	template<class T>
	GeospatialCoordinateEncoder<T>::GeospatialCoordinateEncoder() : scale_(30), timestep_(60)
	{
		this->description_.push_back(tuple<string, int>("longitude", 0));
		this->description_.push_back(tuple<string, int>("lattitude", 1));
		this->description_.push_back(tuple<string, int>("speed", 2));
	}

	template<class T>
	GeospatialCoordinateEncoder<T>::~GeospatialCoordinateEncoder() {}

	template<class T>
	void GeospatialCoordinateEncoder<T>::init(int n, int w, string name, int scale, int timeStep) {
		this->setN(n);
		this->setW(w);
		this->setName(name);
		setScale(scale);
		setTimestep(timeStep);

		//  Implementing classes would do setting of specific  
		//  vars here together with any sanity checking  
		if (getScale() == 0 || getTimestep() == 0) {
			NTA_THROW << "Scale or Timestep not set"; 
		}
		if (this->getW() <= 0 || this->getW() % 2 == 0) {
			NTA_THROW << "w must be odd, and must be a positive integer"; 
		}
		if (this->getN() <= 6 * this->getW()) {
			NTA_THROW << "n must be an int strictly greater than 6*w. For good results we recommend n be strictly greater than 11*w"; 
		}
		if (this->getName() == "" || this->getName() == "None") {
			this->setName("[" + to_string(this->getN()) + ":" + to_string(this->getW()) + "]");
		}
	}

	template<class T>
	//void GeospatialCoordinateEncoder<T>::encodeIntoArray(T inputData, vector<UInt>& output) {
	void GeospatialCoordinateEncoder<T>::encodeIntoArrayB(double longitude, double lattitude, double speed, vector<UInt>& output) {
		vector<int> coordinate = coordinateForPosition(longitude, lattitude);
		const double radius = radiusForSpeed(speed);

		output = CoordinateEncoder<T>::encode(coordinate, radius);
	}

	template<class T>
	vector<int> GeospatialCoordinateEncoder<T>::coordinateForPosition(double longitude, double lattitude) {
		vector<double> coordinate = toMercator(longitude, lattitude);

		return { static_cast<int>(coordinate[0] / scale_) , static_cast<int>(coordinate[1] / scale_) };
	}

	template<class T>
	vector<double> GeospatialCoordinateEncoder<T>::toMercator(double longitude, double lattitude) {
		const double x = longitude * 20037508.34 / 180;
		double y = std::log(std::tan((90 + lattitude) * M_PI / 360)) / (M_PI / 180);
		y = y * 20037508.34 / 180;

		return { x, y };
	}

	template<class T>
	double GeospatialCoordinateEncoder<T>::radiusForSpeed(double speed) {
		const double overlap = 1.5;
		const double coordinatesPerTimestep = speed * timestep_ / scale_;
		const double radius 	= (const int)round(coordinatesPerTimestep / 2.0 * overlap);
		const double minRadius 	=  (const int)ceil((sqrt(this->getW()) - 1) / 2);

		return  max(radius, minRadius);
	}

	template<class T>
	vector<double> GeospatialCoordinateEncoder<T>::inverseMercator(double x, double y) {
		const double longitude = (x / 20037508.34) * 180;
		double lattitude = (y / 20037508.34) * 180;
		lattitude = 180 / M_PI * (2 * atan(exp(lattitude * M_PI / 180)) - M_PI / 2);

		return { longitude, lattitude };
	}

	template<class T>
	void GeospatialCoordinateEncoder<T>::setScale(int scale) {
		scale_ = scale;
	}

	template<class T>
	void GeospatialCoordinateEncoder<T>::setTimestep(int timestep) {
		timestep_ = timestep;
	}

	template<class T>
	int GeospatialCoordinateEncoder<T>::getTimestep() {
		return timestep_;
	}

	template<class T>
	int GeospatialCoordinateEncoder<T>::getScale() {
		return scale_;
	}

}; // end namespace  
#endif // NTA_GeospatialCoordinateEncoder_HPP
