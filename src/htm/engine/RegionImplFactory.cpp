/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013-2015, Numenta, Inc.
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

#include <stdexcept>


#include <htm/engine/Region.hpp>
#include <htm/engine/RegionImpl.hpp>
#include <htm/engine/RegionImplFactory.hpp>
#include <htm/engine/RegisteredRegionImpl.hpp>
#include <htm/engine/RegisteredRegionImplCpp.hpp>
#include <htm/engine/Output.hpp>
#include <htm/engine/Input.hpp>
#include <htm/engine/RawInput.hpp>
#include <htm/engine/Spec.hpp>
#include <htm/ntypes/Value.hpp>
#include <htm/os/Env.hpp>
#include <htm/os/Path.hpp>

// Built-in Region implementations
#include <htm/regions/TestNode.hpp>
#include <htm/regions/DateEncoderRegion.hpp>
#include <htm/regions/ScalarEncoderRegion.hpp>
#include <htm/regions/RDSEEncoderRegion.hpp>
#include <htm/regions/FileOutputRegion.hpp>
#include <htm/regions/FileInputRegion.hpp>
#include <htm/regions/DatabaseRegion.hpp>
#include <htm/regions/SPRegion.hpp>
#include <htm/regions/TMRegion.hpp>
#include <htm/regions/ClassifierRegion.hpp>


#include <htm/utils/Log.hpp>

// from http://stackoverflow.com/a/9096509/1781435
#define stringify(x) #x
#define expand_and_stringify(x) stringify(x)

namespace htm {



void RegionImplFactory::registerRegion(const std::string& nodeType, RegisteredRegionImpl *wrapper) {
  RegionImplFactory& instance = getInstance();
  if (instance.regionTypeMap.find(nodeType) != instance.regionTypeMap.end()) {
	std::shared_ptr<RegisteredRegionImpl>& reg = instance.regionTypeMap[nodeType];
	if (reg->className() == wrapper->className() && reg->moduleName() == wrapper->moduleName()) {
		NTA_WARN << "A Region Type already exists with the name '" << nodeType
				 << "'. Overwriting it...";
		reg.reset(wrapper);  // replace this impl
	} else {
        NTA_THROW << "A region Type with name '" << nodeType
                  << "' already exists. Class name='" << reg->className()
                  << "'  Module='" << reg->moduleName() << "'. "
                  << "Unregister the existing region Type or register the new "
                     "region Type using a "
                  << "different name.";
	}
  } else {
    instance.addRegionType(nodeType, wrapper);
  }
}


void RegionImplFactory::unregisterRegion(const std::string nodeType) {
  RegionImplFactory& instance = getInstance();
  if (instance.regionTypeMap.find(nodeType) != instance.regionTypeMap.end()) {
    instance.regionTypeMap.erase(nodeType);
  }
}

std::string RegionImplFactory::getRegistrations() {
  RegionImplFactory& instance = getInstance();  // force load of built-ins.
  
  std::string json = "{\n";
  for (auto iter = instance.regionTypeMap.begin(); iter != instance.regionTypeMap.end(); ++iter) {
     if (iter->first != "RawInput") {
       if (json.size() > 3) json += ",\n";
       json += "  \""+iter->first+"\": "
               "{\"class\": \""+iter->second->className()+"\""
               ", \"module\": \""+iter->second->moduleName()+"\"}";
     }
  }
  json += "\n}";
  return json;
}

RegionImplFactory &RegionImplFactory::getInstance() {
  static RegionImplFactory instance;

  // Initialize the Built-in Regions
  if (instance.regionTypeMap.empty()) {
    // Create internal C++ regions
	  instance.addRegionType("DateEncoderRegion",  new RegisteredRegionImplCpp<DateEncoderRegion>());
    instance.addRegionType("ScalarEncoderRegion", new RegisteredRegionImplCpp<ScalarEncoderRegion>());
    instance.addRegionType("RDSEEncoderRegion",  new RegisteredRegionImplCpp<RDSEEncoderRegion>());
    instance.addRegionType("TestNode",           new RegisteredRegionImplCpp<TestNode>());
    instance.addRegionType("FileOutputRegion",   new RegisteredRegionImplCpp<FileOutputRegion>());
    instance.addRegionType("FileInputRegion",    new RegisteredRegionImplCpp<FileInputRegion>());
    instance.addRegionType("DatabaseRegion",     new RegisteredRegionImplCpp<DatabaseRegion>());
    instance.addRegionType("SPRegion",           new RegisteredRegionImplCpp<SPRegion>());
    instance.addRegionType("TMRegion",           new RegisteredRegionImplCpp<TMRegion>());
    instance.addRegionType("ClassifierRegion",   new RegisteredRegionImplCpp<ClassifierRegion>());

    // Renamed Regions
    instance.addRegionType("ScalarSensor", new RegisteredRegionImplCpp<ScalarEncoderRegion>());
    instance.addRegionType("RDSERegion", new RegisteredRegionImplCpp<RDSEEncoderRegion>());
    instance.addRegionType("VectorFileEffector", new RegisteredRegionImplCpp<FileOutputRegion>());
    instance.addRegionType("VectorFileSensor", new RegisteredRegionImplCpp<FileInputRegion>());

    // Infrastructure
    instance.addRegionType("RawInput", new RegisteredRegionImplCpp<RawInput>());
  }

  return instance;
}

void RegionImplFactory::addRegionType(const std::string nodeType, RegisteredRegionImpl* wrapper) {
	std::shared_ptr<RegisteredRegionImpl> reg(wrapper);
  	regionTypeMap[nodeType] = reg;

	std::shared_ptr<Spec> ns(reg->createSpec());
	regionSpecMap[nodeType] = ns;
}


RegionImpl *RegionImplFactory::createRegionImpl(const std::string nodeType,
                                                ValueMap vm,
                                                Region *region) {

  RegionImpl *impl = nullptr;

  if (regionTypeMap.find(nodeType) != regionTypeMap.end()) {
    impl = regionTypeMap[nodeType]->createRegionImpl(vm, region);
  } else {
    NTA_THROW << "Unregistered node type '" << nodeType << "'";
  }

  // If the parameter 'dim' was defined, parse that out as a global parameter.
  // This parameter can be used with any Region without the region needing to define it in its Spec.
  // dim: [1,2]                 means set the default dimension for the region.
  // dim: 25                    means set the default dimension to this single dimension.
  // dim: {"bottomUpIn": [5]}   means set the dimensions on the input or output with the name "bottomUpIn"
  // dim: {"bottomUpIn": [5], "bottomUpOut": [2000]}   means set both of these dimensions.

  if (vm.contains("dim")) {
    const Value vm1 = vm["dim"];
    if (vm1.isSequence()) {
      Dimensions dim(vm1.asVector<UInt>());
      impl->setDimensions(dim);
    } else if (vm1.isScalar()) {
      Dimensions dim(vm1.as<UInt>());
      impl->setDimensions(dim);
    } else if (vm1.isMap()) {
      for (auto itr : vm1) {
        std::string name = itr.first;
        Value &vm3 = itr.second;
        if (vm3.isScalar()) {
          Dimensions dim(vm3.as<UInt>());
          if (region->hasOutput(name))
            region->setOutputDimensions(name, dim);
          else if (region->hasInput(name))
            region->setInputDimensions(name, dim);
        } else if (vm3.isSequence()) {
          Dimensions dim(vm3.asVector<UInt>());
          if (region->hasOutput(name))
            region->setOutputDimensions(name, dim);
          else if (region->hasInput(name))
            region->setInputDimensions(name, dim);
        } else {
          NTA_THROW << "Syntax error in parameter 'dim', name='" << name << "'";
        }
      }
    } else {
      NTA_THROW << "Syntax error in parameter 'dim'";
    }
  }

  return impl;
}

RegionImpl *RegionImplFactory::deserializeRegionImpl(const std::string nodeType,
                                                     ArWrapper &wrapper,
                                                     Region *region) {
  RegionImpl *impl = nullptr;
  if (regionTypeMap.find(nodeType) != regionTypeMap.end()) {
    impl = regionTypeMap[nodeType]->deserializeRegionImpl(wrapper, region);
  } else {
    NTA_THROW << "Unsupported node type '" << nodeType << "'";
  }
  return impl;
}



std::shared_ptr<Spec> RegionImplFactory::getSpec(const std::string nodeType) {
  auto it = regionSpecMap.find(nodeType);
  if (it == regionSpecMap.end()) {
	NTA_THROW << "getSpec() -- unknown node type: '" << nodeType
		      << "'.  Custom node types must be registed before they can be used.";
  }
  return it->second;
}



void RegionImplFactory::cleanup() {
  RegionImplFactory& instance = getInstance();
  instance.regionTypeMap.clear();
  instance.regionSpecMap.clear();
}

// definitions for our class variables.
std::map<const std::string, std::shared_ptr<RegisteredRegionImpl> > RegionImplFactory::regionTypeMap;
std::map<const std::string, std::shared_ptr<Spec> > RegionImplFactory::regionSpecMap;


} // namespace htm
