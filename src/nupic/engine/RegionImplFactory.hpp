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

/** @file
 * Definition of the RegionImpl Factory API
 *
 * A RegionImplFactory creates RegionImpls upon request.
 * Pynode creation is delegated to another class (TBD).
 * Because all C++ RegionImpls are compiled in to NuPIC,
 * the RegionImpl factory knows about them explicitly.
 *
 */

#ifndef NTA_REGION_IMPL_FACTORY_HPP
#define NTA_REGION_IMPL_FACTORY_HPP

#include <map>
#include <memory>
#include <string>

namespace nupic
{

  class RegionImpl;
  class Region;
  class DynamicPythonLibrary;
  class Spec;
  class BundleIO;
  class ValueMap;
  class RegisteredRegionImpl;

  class RegionImplFactory
  {
  public:
    static RegionImplFactory & getInstance();

    // RegionImplFactory is a lightweight object
    ~RegionImplFactory() 
    {
      cleanup();
    };

    // Create a RegionImpl of a specific type; caller gets ownership.
    RegionImpl* createRegionImpl(const std::string nodeType,
                                 const std::string nodeParams,
                                 Region* region);

    // Create a RegionImpl from serialized state; caller gets ownership.
    RegionImpl* deserializeRegionImpl(const std::string nodeType,
                                      BundleIO& bundle,
                                      Region* region);

    // Returns nodespec for a specific node type; Factory retains ownership.
    Spec* getSpec(const std::string nodeType);

    // RegionImplFactory caches nodespecs and the dynamic library reference in the registered RegionImpl wrapper
    // This frees up the cached information.
    // Should be called only if there are no outstanding
    // nodespec references (e.g. in NuPIC shutdown) or pynodes.
    void cleanup();

    // Allows the user to load custom C++ regions
    static void registerCPPRegion(const std::string nodetype, RegisteredRegionImpl * wrapper);
    // Allows the user to unregister C++ regions
    static void unregisterCPPRegion(const std::string nodetype);

  private:
    RegionImplFactory() {};
    RegionImplFactory(const RegionImplFactory &);


  };
}

#endif // NTA_REGION_IMPL_FACTORY_HPP
