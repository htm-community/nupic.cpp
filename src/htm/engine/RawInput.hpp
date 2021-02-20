/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2020, Numenta, Inc.
 *
 * Author: David Keeney, July. 2020   dkeeney@gmail.com
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
 * --------------------------------------------------------------------- */

/** @file
 * Implementation of a network region that handles direct input to a region.
 */

#ifndef NTA_RAW_INPUT_HPP
#define NTA_RAW_INPUT_HPP

#include <string>
#include <vector>

#include <htm/engine/RegionImpl.hpp>
#include <htm/ntypes/Value.hpp>
#include <htm/types/Serializable.hpp>

namespace htm {

/**
 * A facade network region that handles direct input to a region.
 *
 * @b Description
 * Implementation of a network region that handles direct input to a region.
 * This is not a normal region.  It does not need to be declared with an addRegion
 * and is inferred when the name is used in a link.
 * It facilitates the ability to directly set data into
 * another region's input.
 *
 *To use this, the user will declare a Link in the configuration which uses the
 * special source region name of "INPUT" and a source name of their choosing.
 * The destination region and destination input name is the target input to be set.
 * The parameter 'dim' will specify the dimensions for the data.
 *
 * At runtime, between iterations, a C++ or Python app can then call
 *       net.setInputData( "INPUT", source_name, dest_region, dest_input, a)
 *            where dest_region and dest_input are the names for the target and
 *            'a' is an Array object containing the data to set.
 * or on the REST api
 *      PUT  /network/<id>/region/INPUT/input/<source name>
 *                          The <data> will be in the body.
 * The source name will identify the Link to use to direct the data to the target region's input.
 */
 
class RawInput : public RegionImpl, Serializable {
public:
  RawInput(const ValueMap &params, Region *region) : RegionImpl(region) {}
  RawInput(ArWrapper &wrapper, Region *region) : RegionImpl(region) {}

  virtual ~RawInput() override {}

  static Spec *createSpec() {
    Spec *ns = new Spec();
    ns->parseSpec("{name: \"RawInput\"}");
    return ns;
  }
  
  virtual void initialize() override { };

  virtual void compute() override { };


  CerealAdapter;  // see Serializable.hpp
  // FOR Cereal Serialization
  template<class Archive>
  void save_ar(Archive& ar) const {
  }
  // FOR Cereal Deserialization
  // NOTE: the Region Implementation must have been allocated
  //       using the RegionImplFactory so that it is connected
  //       to the Network and Region objects. This will populate
  //       the region_ field in the Base class.
  template<class Archive>
  void load_ar(Archive& ar) {
  }


};

} // namespace

#endif // NTA_RAW_INPUT_HPP