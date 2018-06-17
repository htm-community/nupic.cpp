/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013-2015, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
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

#include <stdexcept>

#include <nupic/engine/RegionImplFactory.hpp>
#include <nupic/engine/RegionImpl.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/RegisteredRegionImpl.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/os/DynamicLibrary.hpp>
#include <nupic/os/Path.hpp>
#include <nupic/os/OS.hpp>
#include <nupic/os/Env.hpp>
#include <nupic/ntypes/Value.hpp>
#include <nupic/ntypes/BundleIO.hpp>
#include <nupic/engine/YAMLUtils.hpp>
#include <nupic/utils/Log.hpp>
#include <nupic/utils/StringUtils.hpp>

// Built-in Plugins
#include <nupic/engine/TestNode.hpp>
#include <nupic/encoders/ScalarSensor.hpp>
#include <nupic/regions/VectorFileEffector.hpp>
#include <nupic/regions/VectorFileSensor.hpp>
#include <nupic/regions/SPregion.hpp>




 // from http://stackoverflow.com/a/9096509/1781435
#define stringify(x)  #x
#define expand_and_stringify(x) stringify(x)


namespace nupic
{
  // Mappings for C++ regions
  static std::map<const std::string, GenericRegisteredRegionImpl*> cppRegions;

  bool initializedRegions = false;



  void RegionImplFactory::registerCPPRegion(const std::string nodetype, GenericRegisteredRegionImpl * wrapper)
  {
    if (cppRegions.find(nodetype) != cppRegions.end())
    {
      NTA_WARN << "A CPPRegion already exists with the name '"
        << nodetype << "'. Overwriting it...";
    }
    cppRegions[nodetype] = wrapper;
  }


  void RegionImplFactory::unregisterCPPRegion(const std::string nodetype)
  {
    if (cppRegions.find(nodetype) != cppRegions.end())
    {
      cppRegions.erase(nodetype);
      return;
    }
  }



  RegionImplFactory & RegionImplFactory::getInstance()
  {
    static RegionImplFactory instance;

    // Initialize Regions
    if (!initializedRegions)
    {
      // Create C++ regions
      cppRegions["ScalarSensor"] = new RegisteredRegionImpl<ScalarSensor>();
      cppRegions["TestNode"] = new RegisteredRegionImpl<TestNode>();
      cppRegions["VectorFileEffector"] = new RegisteredRegionImpl<VectorFileEffector>();
      cppRegions["VectorFileSensor"] = new RegisteredRegionImpl<VectorFileSensor>();
      cppRegions["SPRegion"] = new RegisteredRegionImpl<SPRegion>();

      initializedRegions = true;
    }

    return instance;
  }




  RegionImpl* RegionImplFactory::createRegionImpl(const std::string nodeType,
    const std::string nodeParams,
    Region* region)
  {

    RegionImpl *impl = nullptr;
    Spec *ns = getSpec(nodeType);
    ValueMap vm = YAMLUtils::toValueMap(
      nodeParams.c_str(),
      ns->parameters,
      nodeType,
      region->getName());

    if (cppRegions.find(nodeType) != cppRegions.end())
    {
      impl = cppRegions[nodeType]->createRegionImpl(vm, region);
    }
    else
    {
      NTA_THROW << "Unsupported node type '" << nodeType << "'";
    }

    return impl;
  }


  RegionImpl* RegionImplFactory::deserializeRegionImpl(const std::string nodeType,
    BundleIO& bundle,
    Region* region)
  {

    RegionImpl *impl = nullptr;

    if (cppRegions.find(nodeType) != cppRegions.end())
    {
      impl = cppRegions[nodeType]->deserializeRegionImpl(bundle, region);
    }
    else
    {
      NTA_THROW << "Unsupported node type '" << nodeType << "'";
    }
    return impl;
  }


  Spec * RegionImplFactory::getSpec(const std::string nodeType)
  {
    // grab the nodespec and cache it
    // one entry per supported node type
    // Note that each region caches its own spec in RegisteredRegionImpl.
    Spec * ns = nullptr;
    if (cppRegions.find(nodeType) != cppRegions.end())
    {
      ns = cppRegions[nodeType]->createSpec();
    }
    else
    {
      NTA_THROW << "getSpec() -- Unsupported node type '" << nodeType << "'";
    }

    if (!ns)
      NTA_THROW << "Unable to get node spec for: " << nodeType;

    return ns;
  }

  void RegionImplFactory::cleanup()
  {
    // destroy all RegisteredRegionImpls and cached specs
    for (auto rri = cppRegions.begin(); rri != cppRegions.end(); rri++)
    {
      NTA_ASSERT(rri->second != nullptr);
      delete rri->second;
      rri->second = nullptr;
    }
    cppRegions.clear();
    initializedRegions = false;

  }



}
