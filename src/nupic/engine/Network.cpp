/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013-2017, Numenta, Inc.  Unless you have an agreement
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
Implementation of the Network class
*/

#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <nupic/engine/Input.hpp>
#include <nupic/engine/Link.hpp>
#include <nupic/engine/Network.hpp>
#include <nupic/engine/NuPIC.hpp> // for register/unregister
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/ntypes/BundleIO.hpp>
#include <nupic/os/Directory.hpp>
#include <nupic/os/FStream.hpp>
#include <nupic/os/Path.hpp>
#include <nupic/types/BasicType.hpp>
#include <nupic/utils/Log.hpp>
#include <nupic/utils/StringUtils.hpp>

namespace nupic {

class GenericRegisteredRegionImpl;

Network::Network() {
  commonInit();
  NuPIC::registerNetwork(this);
}



void Network::commonInit() {
  initialized_ = false;
  iteration_ = 0;
  minEnabledPhase_ = 0;
  maxEnabledPhase_ = 0;
  // automatic initialization of NuPIC, so users don't
  // have to call NuPIC::initialize
  NuPIC::init();
}

Network::~Network() {
  NuPIC::unregisterNetwork(this);
  /**
   * Teardown choreography:
   * - unintialize all regions because otherwise we won't be able to disconnect
   * - remove all links, because we can't delete connected regions
   * - delete the regions themselves.
   */

  // 1. uninitialize
  for (auto &regionTuple : regions_) {
    regionTuple.second->uninitialize();
  }

  // 2. remove all links
  for (auto &regionTuple : regions_) {
    regionTuple.second->removeAllIncomingLinks();
  }

  // 3. delete the regions
  // The RegionMap container contains a smart pointer to the region
  // so when the container is deleted, the regions are deleted.
}

Region *Network::addRegion(const std::string &name, const std::string &nodeType,
                           const std::string &nodeParams) {
  if (regions_.find(name) != regions_.end())
    NTA_THROW << "Region with name '" << name << "' already exists in network";
  // note: std::make_unique() not available until C++13
  std::unique_ptr<Region> uptr(new Region(name, nodeType, nodeParams, this));
  Region *r = uptr.get();
  regions_.insert(std::make_pair(name, std::move(uptr)));
  initialized_ = false;

  setDefaultPhase_(r);
  return r;
}

Region*  Network::addRegion( std::istream &stream, std::string name) {
  // note: std::make_unique() not available until C++13
  std::unique_ptr<Region> uptr(new Region(this));
  Region *r = uptr.get();
  r->load(stream);
  if (!name.empty())
    r->name_ = name;
  regions_.insert(std::make_pair(r->getName(), std::move(uptr)));

  // We must make a copy of the phases set here because
  // setPhases_ will be passing this back down into
  // the region.
  std::set<UInt32> phases = r->getPhases();
  setPhases_(r, phases);
  return r;
}
void Network::setDefaultPhase_(Region *region) {
  UInt32 newphase = (UInt32)phaseInfo_.size();
  std::set<UInt32> phases;
  phases.insert(newphase);
  setPhases_(region, phases);
}

void Network::setPhases_(Region *r, std::set<UInt32> &phases) {
  if (phases.empty())
    NTA_THROW << "Attempt to set empty phase list for region " << r->getName();

  UInt32 maxNewPhase = *(phases.rbegin());
  UInt32 nextPhase = (UInt32)phaseInfo_.size();
  if (maxNewPhase >= nextPhase) {
    // It is very unlikely that someone would add a region
    // with a phase much greater than the phase of any other
    // region. This sanity check catches such problems,
    // though it should arguably be legal to set any phase.
    if (maxNewPhase - nextPhase > 3)
      NTA_THROW << "Attempt to set phase of " << maxNewPhase
                << " when expected next phase is " << nextPhase
                << " -- this is probably an error.";

    phaseInfo_.resize(maxNewPhase + 1);
  }
  for (UInt i = 0; i < phaseInfo_.size(); i++) {
    bool insertPhase = false;
    if (phases.find(i) != phases.end())
      insertPhase = true;

    // remove previous settings for this region
    std::set<Region *>::iterator item;
    item = phaseInfo_[i].find(r);
    if (item != phaseInfo_[i].end() && !insertPhase) {
      phaseInfo_[i].erase(item);
    } else if (insertPhase) {
      phaseInfo_[i].insert(r);
    }
  }

  // keep track (redundantly) of phases inside the Region also, for
  // serialization
  r->setPhases(phases);

  resetEnabledPhases_();
}

void Network::resetEnabledPhases_() {
  // min/max enabled phases based on what is in the network
  minEnabledPhase_ = getMinPhase();
  maxEnabledPhase_ = getMaxPhase();
}

void Network::setPhases(const std::string &name, std::set<UInt32> &phases) {
  RegionMap::iterator itr = regions_.find(name);
  if (itr == regions_.end())
    NTA_THROW << "setPhases -- no region exists with name '" << name << "'";

  Region* r = itr->second.get();
  setPhases_(r, phases);
}

std::set<UInt32> Network::getPhases(const std::string &name) const {
  RegionMap::const_iterator itr = regions_.find(name);
  if (itr == regions_.end())
    NTA_THROW << "setPhases -- no region exists with name '" << name << "'";

  Region *r = itr->second.get();

  std::set<UInt32> phases;
  // construct the set of phases enabled for this region
  for (UInt32 i = 0; i < phaseInfo_.size(); i++) {
    if (phaseInfo_[i].find(r) != phaseInfo_[i].end()) {
      phases.insert(i);
    }
  }
  return phases;
}

void Network::removeRegion(const std::string &name) {
  RegionMap::const_iterator itr = regions_.find(name);
  if (itr == regions_.end())
    NTA_THROW << "removeRegion: no region named '" << name << "'";

  Region *r = itr->second.get();
  if (r->hasOutgoingLinks())
    NTA_THROW << "Unable to remove region '" << name
              << "' because it has one or more outgoing links";

  // Network does not have to be uninitialized -- removing a region
  // has no effect on the network as long as it has no outgoing links,
  // which we have already checked.
  // initialized_ = false;

  // Must uninitialize the region prior to removing incoming links
  r->uninitialize();

  auto phase = phaseInfo_.begin();
  for (; phase != phaseInfo_.end(); phase++) {
    auto toremove = phase->find(r);
    if (toremove != phase->end())
      phase->erase(toremove);
  }

  // Trim phaseinfo as we may have no more regions at the highest phase(s)
  for (size_t i = phaseInfo_.size() - 1; i > 0; i--) {
    if (phaseInfo_[i].empty())
      phaseInfo_.resize(i);
    else
      break;
  }
  resetEnabledPhases_();

  regions_.erase(itr);
  // Note: the regions_ container holds unique_ptr's to Regions.
  //       When a region is removed from the map, its pointer is deleted.
  //       When the Region object is deleted its implementation instance
  //       is deleted in the Region's destructor.

  return;
}

void Network::link(const std::string &srcRegionName,
                   const std::string &destRegionName,
                   const std::string &linkType, const std::string &linkParams,
                   const std::string &srcOutputName,
                   const std::string &destInputName,
                   const size_t propagationDelay) {

  // Find the regions
  RegionMap::const_iterator itr;
  itr = regions_.find(srcRegionName);
  if (itr == regions_.end())
    NTA_THROW << "Network::link -- source region '" << srcRegionName << "' does not exist";
  const Region *srcRegion = itr->second.get();

  itr = regions_.find(destRegionName);
  if (itr == regions_.end())
    NTA_THROW << "Network::link -- dest region '" << destRegionName << "' does not exist";
  const Region *destRegion = itr->second.get();

  // Find the inputs/outputs
  const Spec *srcSpec = srcRegion->getSpec();
  std::string outputName = srcOutputName;
  if (outputName == "")
    outputName = srcSpec->getDefaultOutputName();

  Output *srcOutput = srcRegion->getOutput(outputName);
  if (srcOutput == nullptr)
    NTA_THROW << "Network::link -- output " << outputName
              << " does not exist on region " << srcRegionName;

  const Spec *destSpec = destRegion->getSpec();
  std::string inputName;
  if (destInputName == "")
    inputName = destSpec->getDefaultInputName();
  else
    inputName = destInputName;

  Input *destInput = destRegion->getInput(inputName);
  if (destInput == nullptr) {
    NTA_THROW << "Network::link -- input '" << inputName
              << " does not exist on region " << destRegionName;
  }

  // Validate link data types
  if (srcOutput->isSparse() == destInput->isSparse()) {
    NTA_CHECK(srcOutput->getDataType() == destInput->getDataType())
        << "Network::link -- Mismatched data types."
        << BasicType::getName(srcOutput->getDataType())
        << " != " << BasicType::getName(destInput->getDataType());
  } else if (srcOutput->isSparse()) {
    // Sparse to dense: unit32 -> bool
    NTA_CHECK(srcOutput->getDataType() == NTA_BasicType_UInt32 &&
              destInput->getDataType() == NTA_BasicType_Bool)
        << "Network::link -- Sparse to Dense link: source must be uint32 and "
           "destination must be boolean";
  } else if (destInput->isSparse()) {
    // Dense to sparse:  NTA_BasicType -> uint32
    NTA_CHECK(destInput->getDataType() == NTA_BasicType_UInt32)
        << "Network::link -- Dense to Sparse link: destination must be uint32";
  }

  // Create the link itself
  auto link =
      new Link(linkType, linkParams, srcOutput, destInput, propagationDelay);
  destInput->addLink(link, srcOutput);
}

void Network::removeLink(const std::string &srcRegionName,
                         const std::string &destRegionName,
                         const std::string &srcOutputName,
                         const std::string &destInputName) {
  // Find the regions
  RegionMap::const_iterator itr;
  itr = regions_.find(srcRegionName);
  if (itr == regions_.end())
    NTA_THROW << "Network::unlink -- source region '" << srcRegionName << "' does not exist";
  const Region *srcRegion = itr->second.get();

  itr = regions_.find(destRegionName);
  if (itr == regions_.end())
    NTA_THROW << "Network::unlink -- dest region '" << destRegionName << "' does not exist";
  const Region *destRegion = itr->second.get();

  // Find the inputs
  const Spec *srcSpec = srcRegion->getSpec();
  const Spec *destSpec = destRegion->getSpec();
  std::string inputName;
  if (destInputName == "")
    inputName = destSpec->getDefaultInputName();
  else
    inputName = destInputName;

  Input *destInput = destRegion->getInput(inputName);
  if (destInput == nullptr) {
    NTA_THROW << "Network::unlink -- input '" << inputName
              << " does not exist on region " << destRegionName;
  }

  std::string outputName = srcOutputName;
  if (outputName == "")
    outputName = srcSpec->getDefaultOutputName();
  Link *link = destInput->findLink(srcRegionName, outputName);

  if (link == nullptr)
    NTA_THROW << "Network::unlink -- no link exists from region "
              << srcRegionName << " output " << outputName << " to region "
              << destRegionName << " input " << destInput->getName();

  // Finally, remove the link
  destInput->removeLink(link);
}

void Network::run(int n) {
  if (!initialized_) {
    initialize();
  }

  if (phaseInfo_.empty())
    return;

  NTA_CHECK(maxEnabledPhase_ < phaseInfo_.size())
      << "maxphase: " << maxEnabledPhase_ << " size: " << phaseInfo_.size();

  for (int iter = 0; iter < n; iter++) {
    iteration_++;

    // compute on all enabled regions in phase order
    for (UInt32 phase = minEnabledPhase_; phase <= maxEnabledPhase_; phase++) {
      for (auto r : phaseInfo_[phase]) {
        r->prepareInputs();
        r->compute();
      }
    }

    // invoke callbacks
    for (UInt32 i = 0; i < callbacks_.getCount(); i++) {
      const std::pair<std::string, callbackItem> &callback = callbacks_.getByIndex(i);
      callback.second.first(this, iteration_, callback.second.second);
    }

    // Refresh all links in the network at the end of every timestamp so that
    // data in delayed links appears to change atomically between iterations
    for (const auto &regionTuple : regions_) {
      const Region* r = regionTuple.second.get();

      for (const auto &inputTuple : r->getInputs()) {
        for (const auto pLink : inputTuple.second->getLinks()) {
          pLink->shiftBufferedData();
        }
      }
    }

  } // End of outer run-loop

  return;
}

void Network::initialize() {
  RegionMap::const_iterator itr;

  /*
   * Do not reinitialize if already initialized.
   * Mostly, this is harmless, but it has a side
   * effect of resetting the max/min enabled phases,
   * which causes havoc if we are in the middle of
   * a computation.
   */
  if (initialized_)
    return;

  /*
   * 1. Calculate all region dimensions by
   * iteratively evaluating links to induce
   * region dimensions.
   */

  // Iterate until all regions have finished
  // evaluating their links. If network is
  // incompletely specified, we'll never finish,
  // so make sure we make progress each time
  // through the network.

  size_t nLinksRemainingPrev = std::numeric_limits<size_t>::max();
  size_t nLinksRemaining = nLinksRemainingPrev - 1;

  std::vector<Region *>::iterator r;
  while (nLinksRemaining > 0 && nLinksRemainingPrev > nLinksRemaining) {
    nLinksRemainingPrev = nLinksRemaining;
    nLinksRemaining = 0;

    for (const auto &regionTuple : regions_) {
      Region* r = regionTuple.second.get();
      // evaluateLinks returns the number
      // of links which still need to be
      // evaluated.
      nLinksRemaining += r->evaluateLinks();
    }
  }

  if (nLinksRemaining > 0) {
    // Try to give complete information to the user
    std::stringstream ss;
    ss << "Network::initialize() -- unable to evaluate all links\n"
       << "The following links could not be evaluated:\n";
    for (const auto &regionTuple : regions_) {
      const Region* r = regionTuple.second.get();
      std::string errors = r->getLinkErrors();
      if (errors.size() == 0)
        continue;
      ss << errors << "\n";
    }
    NTA_THROW << ss.str();
  }

  // Make sure all regions now have dimensions
  for (const auto &regionTuple : regions_) {
    const Region* r = regionTuple.second.get();
    const Dimensions &d = r->getDimensions();
    if (d.isUnspecified()) {
      NTA_THROW << "Network::initialize() -- unable to complete initialization "
                << "because region '" << r->getName() << "' has unspecified "
                << "dimensions. You must either specify dimensions directly or "
                << "link to the region in a way that induces dimensions on the "
                   "region.";
    }
    if (!d.isValid()) {
      NTA_THROW << "Network::initialize() -- invalid dimensions "
                << d.toString() << " for Region " << r->getName();
    }
  }

  /*
   * 2. initialize outputs:
   *   - . Delegated to regions
   */
  for (const auto &regionTuple : regions_) {
    Region* r = regionTuple.second.get();
    r->initOutputs();
  }

  /*
   * 3. initialize inputs
   *    - Delegated to regions
   */
  for (const auto &regionTuple : regions_) {
    const Region* r = regionTuple.second.get();
    r->initInputs();
  }

  /*
   * 4. initialize region/impl
   */
  for (const auto &regionTuple : regions_) {
    Region* r = regionTuple.second.get();
    r->initialize();
  }

  /*
   * 5. Enable all phases in the network
   */
  resetEnabledPhases_();

  /*
   * Mark network as initialized.
   */
  initialized_ = true;
}


Collection<Link *> Network::getLinks() {
  Collection<Link *> links;

  for (UInt32 phase = minEnabledPhase_; phase <= maxEnabledPhase_; phase++) {
    for (auto r : phaseInfo_[phase]) {
      for (auto &input : r->getInputs()) {
        for (auto &link : input.second->getLinks()) {
          links.add(link->toString(), link);
        }
      }
    }
  }

  return links;
}

Collection<Network::callbackItem> &Network::getCallbacks() {
  return callbacks_;
}

UInt32 Network::getMinPhase() const {
  UInt32 i = 0;
  for (; i < phaseInfo_.size(); i++) {
    if (!phaseInfo_[i].empty())
      break;
  }
  return i;
}

UInt32 Network::getMaxPhase() const {
  /*
   * phaseInfo_ is always trimmed, so the max phase is
   * phaseInfo_.size()-1
   */

  if (phaseInfo_.empty())
    return 0;

  return phaseInfo_.size() - 1;
}

void Network::setMinEnabledPhase(UInt32 minPhase) {
  if (minPhase >= phaseInfo_.size())
    NTA_THROW << "Attempt to set min enabled phase " << minPhase
              << " which is larger than the highest phase in the network - "
              << phaseInfo_.size() - 1;
  minEnabledPhase_ = minPhase;
}

void Network::setMaxEnabledPhase(UInt32 maxPhase) {
  if (maxPhase >= phaseInfo_.size())
    NTA_THROW << "Attempt to set max enabled phase " << maxPhase
              << " which is larger than the highest phase in the network - "
              << phaseInfo_.size() - 1;
  maxEnabledPhase_ = maxPhase;
}

UInt32 Network::getMinEnabledPhase() const { return minEnabledPhase_; }

UInt32 Network::getMaxEnabledPhase() const { return maxEnabledPhase_; }




void Network::save(std::ostream &f) const {
  // save Network, Region, Links

  f << "Network " << getSerializableVersion() << std::endl;
  f << "{\n";
  f << "iteration: " << iteration_ << "\n";
  f << "Regions: " << "[ " << regions_.size() << "\n";

  Size count = 0;
  for (const auto &regionTuple : regions_) {
      const Region* r = regionTuple.second.get();
      r->save(f);

      // While we are here, lets get a count of links.
      const std::map<std::string, Input*> inputs = r->getInputs();
      for (const auto & inputs_input : inputs)
      {
        const std::vector<Link*>& links = inputs_input.second->getLinks();
        count += links.size();
      }
  }
  f << "]\n"; // end of regions

  f << "Links: [ " << count << "\n";

  // Now serialize the links
  for (const auto &regionTuple : regions_) {
    const Region* r = regionTuple.second.get();
    const std::map<std::string, Input*> inputs = r->getInputs();
    for (const auto & inputs_input : inputs)
    {
      const std::vector<Link*>& links = inputs_input.second->getLinks();
      for (const auto & links_link : links)
      {
        auto l = links_link;
        l->serialize(f);
      }

    }
  }
  f << "]\n"; // end of links

  f << "}\n"; // end of network
  f << std::endl;
}




void Network::load(std::istream &f) {

  std::string tag;
  int version;
  int minimum_version = 1; // the lowest acceptable version
  Size count;

  // Remove all existing regions and links
  for (const auto &regionTuple : regions_) {
      const Region* r = regionTuple.second.get();
    removeRegion(r->getName());
  }
  initialized_ = false;


  f >> tag;
  NTA_CHECK(tag == "Network")  << "Invalid network structure file -- does not contain 'Network' as starting tag.";
  f >> version;
  NTA_CHECK(version >= minimum_version) << "Expecting at least version "
          << minimum_version << " for Network stream.";
  f >> tag;
  NTA_CHECK(tag == "{") << "Expected beginning of a map.";
  f >> tag;
  NTA_CHECK(tag == "iteration:");
  f >> iteration_;

  // Regions
  f >> tag;
  NTA_CHECK(tag == "Regions:");
  f >> tag;
  NTA_CHECK(tag == "[") << "Expected the beginning of a list";
  f >> count;
  for (Size n = 0; n < count; n++)
  {
    std::unique_ptr<Region> uptr(new Region(this));
	Region *r = uptr.get();
    r->load(f);
    regions_.insert(std::make_pair(r->getName(), std::move(uptr)));

    // We must make a copy of the phases set here because
    // setPhases_ will be passing this back down into
    // the region.
    std::set<UInt32> phases = r->getPhases();
    setPhases_(r, phases);

  }
  f >> tag;
  NTA_CHECK(tag == "]") << "Expected end of list of regions.";


  //  Links
  f >> tag;
  NTA_CHECK(tag == "Links:");
  f >> tag;
  NTA_CHECK(tag == "[") << "Expected beginning of list of links.";
  f >> count;

  for (Size n=0; n < count; n++)
  {
    // Create the link
    auto newLink = new Link();
    newLink->deserialize(f);

  // Now connect the links to the regions
    const std::string srcRegionName = newLink->getSrcRegionName();
    NTA_CHECK(regions_.find(srcRegionName) != regions_.end()) << "Invalid network structure file -- link specifies source region '"
          << srcRegionName << "' but no such region exists";
    Region *srcRegion = regions_[srcRegionName].get();

    const std::string destRegionName = newLink->getDestRegionName();
    NTA_CHECK(regions_.find(destRegionName) != regions_.end()) << "Invalid network structure file -- link specifies destination region '"
                << destRegionName << "' but no such region exists";
    Region *destRegion = regions_[destRegionName].get();

    const std::string srcOutputName = newLink->getSrcOutputName();
    Output *srcOutput = srcRegion->getOutput(srcOutputName);
    NTA_CHECK(srcOutput != nullptr) << "Invalid network structure file -- link specifies source output '"
          << srcOutputName << "' but no such name exists";

    const std::string destInputName = newLink->getDestInputName();
    Input *destInput = destRegion->getInput(destInputName);
    NTA_CHECK(destInput != nullptr) << "Invalid network structure file -- link specifies destination input '"
                << destInputName << "' but no such name exists";

    newLink->connectToNetwork(srcOutput, destInput);
    destInput->addLink(newLink, srcOutput);

    // The Links will not be initialized. So must call net.initialize() after load().
  } // links

  f >> tag;
  NTA_CHECK(tag == "]");  // end of links
  f >> tag;
  NTA_CHECK(tag == "}");  // end of network
  f.ignore(1);

  // Post Load operations
  initialize();   //  re-initialize everything
  NTA_CHECK(maxEnabledPhase_ < phaseInfo_.size())
      << "maxphase: " << maxEnabledPhase_ << " size: " << phaseInfo_.size();

  // Note: When serialized, the output buffers are saved
  //       by each RegionImpl.  After restore we need to
  //       copy restored outputs to connected inputs.
  //
  //       Input buffers are not saved, they are restored by
  //       copying from their source output buffers via links.
  //       If an input is manually set (as in some unit tests)
  //       then the input would be lost after restore.

  for (const auto &regionTuple : regions_) {
    Region* r = regionTuple.second.get();

    // If a propogation Delay is specified, the Link serialization
	// saves the current input buffer at the top of the
	// propogation Delay array because it will be pushed to
	// the input during prepareInputs();
	// It then saves all but the back buffer
    // (the most recent) of the Propogation Delay array because
    // that buffer is the same as the most current output.
	// So after restore we need to call prepareInputs() and
	// shift the current outputs into the Propogaton Delay array.
    r->prepareInputs();

    for (const auto &inputTuple : r->getInputs()) {
      for (const auto pLink : inputTuple.second->getLinks()) {
        pLink->shiftBufferedData();
      }
    }
  }
}

void Network::enableProfiling() {
  for (const auto &regionTuple : regions_) {
      regionTuple.second->enableProfiling();
  }
}

void Network::disableProfiling() {
  for (const auto &regionTuple : regions_) {
      regionTuple.second->disableProfiling();
  }
}

void Network::resetProfiling() {
  for (const auto &regionTuple : regions_) {
      regionTuple.second->resetProfiling();
  }
}

void Network::registerPyRegion(const std::string module,
                               const std::string className) {
  Region::registerPyRegion(module, className);
}

void Network::registerCPPRegion(const std::string name,
                                GenericRegisteredRegionImpl *wrapper) {
  Region::registerCPPRegion(name, wrapper);
}

void Network::unregisterPyRegion(const std::string className) {
  Region::unregisterPyRegion(className);
}

void Network::unregisterCPPRegion(const std::string name) {
  Region::unregisterCPPRegion(name);
}

bool Network::operator==(const Network &o) const {

  if (initialized_ != o.initialized_ || iteration_ != o.iteration_ ||
      minEnabledPhase_ != o.minEnabledPhase_ ||
      maxEnabledPhase_ != o.maxEnabledPhase_ ||
      regions_.size() != o.regions_.size()) {
    return false;
  }

  RegionMap::const_iterator itr2;
  for (const auto &regionTuple : regions_) {
    const Region* r1 = regionTuple.second.get();

    itr2 = o.regions_.find(r1->getName());
    if (itr2 == o.regions_.end())
      return false;
    const Region *r2 = itr2->second.get();
    if (*r1 != *r2) {
      return false;
    }
  }
  return true;
}

} // namespace nupic
