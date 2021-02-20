/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013-2017, Numenta, Inc.
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
Implementation of the Network class
*/

#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <htm/engine/Input.hpp>
#include <htm/engine/Link.hpp>
#include <htm/engine/Network.hpp>
#include <htm/engine/Output.hpp>
#include <htm/engine/Region.hpp>
#include <htm/engine/RegionImplFactory.hpp>
#include <htm/engine/Spec.hpp>
#include <htm/os/Directory.hpp>
#include <htm/os/Path.hpp>
#include <htm/ntypes/BasicType.hpp>
#include <htm/utils/Log.hpp>
#include <htm/ntypes/Value.hpp>

namespace htm {

class RegisteredRegionImpl;

thread_local LogLevel NTA_LOG_LEVEL; 


Network::Network() {
  commonInit();
}

// move constructor
Network::Network(Network &&n) noexcept {
  regions_ = std::move(n.regions_);
  minEnabledPhase_ = n.minEnabledPhase_;
  maxEnabledPhase_ = n.maxEnabledPhase_;
  phaseInfo_ = std::move(n.phaseInfo_);
  callbacks_ = n.callbacks_;
  iteration_ = n.iteration_;
}

Network::Network(const std::string& filename) {
  commonInit();
  loadFromFile(filename);
}


void Network::commonInit() {
  initialized_ = false;
  iteration_ = 0;
  minEnabledPhase_ = 0;
  maxEnabledPhase_ = 0;
}

Network::~Network() {
  /**
   * Teardown choreography:
   * - unintialize all regions because otherwise we won't be able to disconnect
   * - remove all links, because we can't delete connected regions
   *   This also removes Input and Output objects.
   * - delete the regions themselves.
   */

  // 1. uninitialize
  for(auto p: regions_) {
    std::shared_ptr<Region> r = p.second;
    r->uninitialize();
  }

  // 2. remove all links
  for(auto p: regions_) {
    std::shared_ptr<Region> r = p.second;
    r->removeAllIncomingLinks();
  }

  // 3. delete the regions
  // They are in a map of Shared_ptr so regions are deleted when it goes out of scope.
}


void Network::configure(const std::string &yaml) {
  ValueMap vm;
  vm.parse(yaml);

  NTA_CHECK(vm.isMap() && vm.contains("network")) << "Expected yaml string to start with 'network:'.";
  Value &v1 = vm["network"];
  NTA_CHECK(v1.isSequence()) << "Expected a sequence of entries starting with a command.";
  for (size_t i = 0; i < v1.size(); i++) {
    NTA_CHECK(v1[i].isMap()) << "Expcted a command";
    for (auto cmd : v1[i]) {
      if (cmd.first == "registerRegion") {
        std::string type = cmd.second["type"].str();
        std::string path = cmd.second["path"].str();
        std::string classname = cmd.second["classname"].str();
        // TODO:  
        NTA_THROW << "For now you can only use the built-in C++ regions with the REST API.";
      } else if (cmd.first == "addRegion") {
        std::string name = cmd.second["name"].str();
        std::string type = cmd.second["type"].str();
        ValueMap params;
        if (cmd.second.contains("params")) params = cmd.second["params"];
        addRegion(name, type, params);

        UInt32 phase = 0;
        if (cmd.second.contains("phase")) {
          phase = cmd.second["phase"].as<int>();
          std::set<UInt32> phases;
          phases.insert(phase);
          setPhases(name, phases);
        }
      } else if (cmd.first == "addLink") {
        std::string src = cmd.second["src"].str();
        std::string dest = cmd.second["dest"].str();
        std::string dim = "";
        std::vector<std::string> vsrc = Path::split(src, '.');
        std::vector<std::string> vdest = Path::split(dest, '.');
        int propagationDelay = 0;
        if (cmd.second.contains("delay"))
          propagationDelay = cmd.second["delay"].as<int>();
        if (cmd.second.contains("dim")) {
          dim = cmd.second.to_json();
        }
        NTA_CHECK(vsrc.size() == 2) << "Expecting source domain name '.' output name.";
        NTA_CHECK(vdest.size() == 2) << "Expecting destination domain name '.' input name.";

        link(vsrc[0], vdest[0], "", dim, vsrc[1], vdest[1], propagationDelay);
      }
    }
  }
}


std::shared_ptr<Region> Network::addRegion(const std::string &name, 
                                           const std::string &nodeType,
                                           const std::string &nodeParams) {
  if (regions_.find(name) != regions_.end())
    NTA_THROW << "Region with name '" << name << "' already exists in network";
  std::shared_ptr<Region> r = std::make_shared<Region>(name, nodeType, nodeParams, this);
  regions_[name] = r;
  initialized_ = false;


  setDefaultPhase_(r.get());
  return r;
}

std::shared_ptr<Region> Network::addRegion(const std::string &name, 
                                           const std::string &nodeType,
                                           ValueMap& vm) {
  if (regions_.find(name) != regions_.end())
    NTA_THROW << "Region with name '" << name << "' already exists in network";
  std::shared_ptr<Region> r = std::make_shared<Region>(name, nodeType, vm, this);
  regions_[name] = r;
  initialized_ = false;

  setDefaultPhase_(r.get());
  return r;
}


std::shared_ptr<Region> Network::addRegion(std::shared_ptr<Region>& r) {
  NTA_CHECK(r != nullptr);
  r->network_ = this;
  regions_[r->getName()] = r;
  
  // If a region is added, initially set the phase to the default phase.
  // The phase can be changed later.
  setDefaultPhase_(r.get());

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
      // add the new phase(s_
      phaseInfo_[i].insert(r);
    }
  }


  resetEnabledPhases_();
}

void Network::resetEnabledPhases_() {
  // min/max enabled phases based on what is in the network
  minEnabledPhase_ = getMinPhase();
  maxEnabledPhase_ = getMaxPhase();
}

void Network::setPhases(const std::string &name, std::set<UInt32> &phases) {
  auto itr = regions_.find(name);
  if (itr == regions_.end())
    NTA_THROW << "setPhases -- no region exists with name '" << name << "'";

  std::shared_ptr<Region>& r = itr->second;
  setPhases_(r.get(), phases);
}

std::set<UInt32> Network::getPhases(const std::string &name) const {
  auto itr = regions_.find(name);
  if (itr == regions_.end())
    NTA_THROW << "setPhases -- no region exists with name '" << name << "'";

  const std::shared_ptr<Region> r = itr->second;

  std::set<UInt32> phases;
  // construct the set of phases enabled for this region
  for (UInt32 i = 0; i < phaseInfo_.size(); i++) {
    if (phaseInfo_[i].find(r.get()) != phaseInfo_[i].end()) {
      phases.insert(i);
    }
  }
  return phases;
}

void Network::removeRegion(const std::string &name) {
  auto itr = regions_.find(name);
  if (itr == regions_.end())
    NTA_THROW << "removeRegion: no region named '" << name << "'";

  const std::shared_ptr<Region>& r = itr->second;
  if (r->hasOutgoingLinks())
    NTA_THROW << "Unable to remove region '" << name
              << "' because it has one or more outgoing links";

  // Network does not have to be uninitialized -- removing a region
  // has no effect on the network as long as it has no outgoing links,
  // which we have already checked.

  // Must uninitialize the region prior to removing incoming links
  // The incoming links are removed when the Input object is deleted.
  r->uninitialize();
  r->clearInputs();


  auto phase = phaseInfo_.begin();
  for (; phase != phaseInfo_.end(); phase++) {
    auto toremove = phase->find(r.get());
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

  // Region is deleted when the Shared_ptr goes out of scope.
  regions_.erase(itr);
  return;
}

std::shared_ptr<Link> Network::link(const std::string &srcRegionName,
                   const std::string &destRegionName,
                   const std::string &linkType, const std::string &linkParams,
                   const std::string &srcOutputName,
                   const std::string &destInputName,
                   const size_t propagationDelay) {

  // Find the regions
  auto itrSrc = regions_.find(srcRegionName);
  if (itrSrc == regions_.end()) {
    if (srcRegionName != "INPUT")
      NTA_THROW << "Network::link -- source region '" << srcRegionName << "' does not exist";
    // Our special InputRegion does not exist. But we are using it so we need to create it.
    ValueMap vm;  // Empty parameters.
    Network::addRegion(srcRegionName, "RawInput", vm);  // Note: will have no Outputs defined.
  }
  std::shared_ptr<Region> srcRegion = regions_[srcRegionName];

  auto itrDest = regions_.find(destRegionName);
  if (itrDest == regions_.end())
    NTA_THROW << "Network::link -- dest region '" << destRegionName
              << "' does not exist";
  std::shared_ptr<Region> destRegion = regions_[destRegionName];

  // Find the inputs/outputs
  // get the destination Input object
  std::string inputName = destInputName;
  if (inputName == "") {
    const std::shared_ptr<Spec> &destSpec = destRegion->getSpec();
    inputName = destSpec->getDefaultInputName();
  }

  std::shared_ptr<Input> destInput = destRegion->getInput(inputName);
  if (destInput == nullptr) {
    NTA_THROW << "Network::link -- input '" << inputName << " does not exist on region " << destRegionName;
  }

  // get the source Output object
  std::string outputName = srcOutputName;
  if (outputName == "") {
    const std::shared_ptr<Spec>& srcSpec = srcRegion->getSpec();
    outputName = srcSpec->getDefaultOutputName();
  }

  std::shared_ptr<Output> srcOutput = srcRegion->getOutput(outputName);
  if (srcOutput == nullptr) {
    if (srcRegionName == "INPUT") {
      // This is our special source region for manually setting inputs using a link
      // Get the data type from the destination Input object and create an output.
      NTA_BasicType type = destInput->getDataType();
      srcOutput = std::make_shared<Output>(srcRegion.get(), outputName, type);

      Dimensions dim;
      std::string params = Path::trim(linkParams);
      if (!params.empty()) {
        Value v;
        v.parse(linkParams);
        if (v.isMap() && v.contains("dim")) {
          Value v1 = v["dim"];
          if (v1.isSequence())
            dim = Dimensions(v1.asVector<UInt>());
          else if (v1.isScalar())
            dim = Dimensions(v1.as<UInt>());
        }
      }
      if (dim.empty())
        NTA_THROW << "Link declared with Special \"INPUT\" source requires dimensions in link parameters. Something like "
                     "{dim: 100} or {dim: [10,20]}";

      srcOutput->setDimensions(dim);
      srcOutput->initialize();  // allocates the buffer
      srcRegion->outputs_[outputName] = srcOutput;
    } else {
      NTA_THROW << "Network::link -- output " << outputName << " does not exist on region " << srcRegionName;
    }
  }

  // Create the link itself
  auto link = std::make_shared<Link>(linkType, linkParams, srcOutput, destInput, propagationDelay);
  destInput->addLink(link, srcOutput);
  return link;
}

void Network::removeLink(const std::string &srcRegionName,
                         const std::string &destRegionName,
                         const std::string &srcOutputName,
                         const std::string &destInputName) {
  // Find the regions
  auto itrSrc = regions_.find(srcRegionName);
  if (itrSrc == regions_.end())
    NTA_THROW << "Network::unlink -- source region '" << srcRegionName
              << "' does not exist";
  std::shared_ptr<Region> srcRegion = getRegion(srcRegionName);

  auto itrDest = regions_.find(destRegionName);
  if (itrDest == regions_.end())
    NTA_THROW << "Network::unlink -- dest region '" << destRegionName
              << "' does not exist";
  std::shared_ptr<Region> destRegion = getRegion(destRegionName);

  // Find the inputs
  const std::shared_ptr<Spec>& srcSpec = srcRegion->getSpec();
  const std::shared_ptr<Spec>& destSpec = destRegion->getSpec();
  std::string inputName;
  if (destInputName == "")
    inputName = destSpec->getDefaultInputName();
  else
    inputName = destInputName;

  std::shared_ptr<Input> destInput = destRegion->getInput(inputName);
  if (destInput == nullptr) {
    NTA_THROW << "Network::unlink -- input '" << inputName
              << " does not exist on region " << destRegionName;
  }

  std::string outputName = srcOutputName;
  if (outputName == "")
    outputName = srcSpec->getDefaultOutputName();
  std::shared_ptr<Link> link = destInput->findLink(srcRegionName, outputName);

  if (!link)
    NTA_THROW << "Network::unlink -- no link exists from region "
              << srcRegionName << " output " << outputName << " to region "
              << destRegionName << " input " << destInput->getName();

  // Finally, remove the link
  destInput->removeLink(link);
}



/**
 * Set the source data for a Link identified with the source as "INPUT" and <sourceName>.
 */
void Network::setInputData(const std::string& sourceName, const Array& data) {
  // The placeholder region "INPUT" with an output of <sourceName> should already exist if the link was defined.
  std::shared_ptr<Region> region = getRegion("INPUT"); 
  Array &a = region->getOutput(sourceName)->getData(); // we actually populate an output buffer that will be moved to input.
  NTA_CHECK(a.getCount() == data.getCount())
      << "setInputData: Number of elements in buffer ( " << a.getCount() << " ) do not match target dimensions.";
  if (a.getType() == data.getType()) {
    a = data;  // assign the buffer without copy
  }  else {
    data.convertInto(a);  // copy the data with conversion.
  }
}

void Network::setInputData(const std::string &sourceName, const Value& vm) {
  // The placeholder region "INPUT" with an output of <sourceName> should already exist if the link was defined.
  std::shared_ptr<Region> region = getRegion("INPUT");
  Array &a =  region->getOutput(sourceName)->getData(); // populate this output buffer that will be moved to the input.
  NTA_BasicType type = a.getType();

  NTA_CHECK(vm.contains("data"))
      << "Unexpected YAML or JSON format. Expecting something like {data: [1,0,1]}";

  NTA_CHECK(vm["data"].isSequence())
      << "Unexpected YAML or JSON format. Expecting something like {data: [1,0,1]}";

  if (type == NTA_BasicType_SDR) {
    NTA_CHECK(a.getCount() >= vm.size())
        << "setInputData: Number of elements in buffer ( " << a.getCount() << " ) do not match target dimensions.";
  } else {
    NTA_CHECK(a.getCount() == vm.size())
        << "setInputData: Number of elements in buffer ( " << a.getCount() << " ) do not match target dimensions.";
  }

  a.fromValue(vm);
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
    for (auto p: regions_) {
      const std::shared_ptr<Region> r = p.second;

      for (const auto &inputTuple : r->getInputs()) {
        for (const auto &pLink : inputTuple.second->getLinks()) {
          pLink->shiftBufferedData();
        }
      }
    }

  } // End of outer run-loop

  return;
}

void Network::initialize() {

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
   * 1. Calculate all Input/Output dimensions by evaluating links.
   */
  for (auto phase : phaseInfo_) { // evaluate regions in execution order
    for (auto r : phase) {
      r->evaluateLinks();
    }
  }


  /*
   * 2. initialize region/impl
   */
  for (auto p: regions_) {
    std::shared_ptr<Region> r = p.second;
    r->initialize();
  }

  /*
   * 3. Enable all phases in the network
   */
  resetEnabledPhases_();

  /*
   * Mark network as initialized.
   */
  initialized_ = true;
}

const Collection<std::shared_ptr<Region>> Network::getRegions() const { 
  Collection<std::shared_ptr<Region>> regions;
  for(auto r: regions_) {
    regions.add(r.first, r.second);
  }
  return regions; 
}

std::shared_ptr<Region> Network::getRegion(const std::string& name) const {
  auto itr = regions_.find(name);
  if (itr == regions_.end())
    NTA_THROW << "Network::getRegion; '" << name << "' does not exist";
  return itr->second;
}

std::string Network::getSpecJSON(const std::string &type) {
  RegionImplFactory &factory = RegionImplFactory::getInstance();
  const std::shared_ptr<Spec> spec = factory.getSpec(type);
  return spec->toString();
}



std::vector<std::shared_ptr<Link>> Network::getLinks() const {
  std::vector<std::shared_ptr<Link>> links;

  for (auto p : regions_) {
    for (auto &input : p.second->getInputs()) {
      for (auto &link : input.second->getLinks()) {
        links.push_back(link);
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

  return (UInt32)(phaseInfo_.size() - 1);
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



void Network::post_load(std::vector<std::shared_ptr<Link>>& links) {
    for(auto alink: links) {
      auto l = link( alink->getSrcRegionName(),
                     alink->getDestRegionName(),
                     "", "",
                     alink->getSrcOutputName(),
                     alink->getDestInputName(),
                     alink->getPropagationDelay());
      l->propagationDelayBuffer_ = alink->propagationDelayBuffer_;
    }
    post_load();
}

void Network::post_load() {
  // Post Load operations
  for(auto p: regions_) {
    std::shared_ptr<Region>& r = p.second;
    r->network_ = this;
    r->evaluateLinks();      // Create the input buffers.
  }


  // Note: When serialized, the output buffers are saved
  //       by each Region.  After restore we need to
  //       copy restored outputs to connected inputs.
  //
  //       Input buffers are not saved, they are restored by
  //       copying from their source output buffers via links.
  //       If an input is manually set then the input would be
  //       lost after restore.
  
  for (auto p: regions_) {
    std::shared_ptr<Region>&  r = p.second;

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
      for (const auto &pLink : inputTuple.second->getLinks()) {
        pLink->shiftBufferedData();
      }
    }
  }

  // If we made it this far without an exception, we are good to go.
  initialized_ = true;

}

std::string Network::phasesToString() const {
  std::stringstream ss;
  ss << "{";
  ss << "minEnabledPhase_: " << minEnabledPhase_ << ", ";
  ss << "maxEnabledPhase_: " << maxEnabledPhase_ << ", ";
  ss << "info: [";
  for (auto phase : phaseInfo_) {
    ss << "[";
    for (auto region : phase) {
      ss << region->getName() << ", ";
    }
    ss << "]";
  }
  ss << "]}";
  return ss.str();
}
void Network::phasesFromString(const std::string& phaseString) {
  std::string content = phaseString;
  content.erase(std::remove(content.begin(), content.end(), ','), content.end());
  std::stringstream ss(content);
  std::string tag;
  std::set<Region *> phase;
  
  NTA_CHECK(ss.peek() == '{') << "Invalid phase deserialization";
  ss.ignore(1);
  ss >> tag;
  NTA_CHECK(tag == "minEnabledPhase_:");
  ss >> minEnabledPhase_;
  ss >> tag;
  NTA_CHECK(tag == "maxEnabledPhase_:");
  ss >> maxEnabledPhase_;
  ss >> tag;
  NTA_CHECK(tag == "info:") << "Invalid phase deserialization";
  ss >> std::ws;
  NTA_CHECK(ss.peek() == '[') << "Invalid phase deserialization";
  ss.ignore(1);
  ss >> std::ws;
  while (ss.peek() != ']') {
    ss >> std::ws;
    if (ss.peek() == '[') {
      ss.ignore(1);
      ss >> std::ws;
      while (ss.peek() != ']') {
        ss >> tag;
        auto it = regions_.find(tag);
        NTA_CHECK(it != regions_.end()) << "Region '" << tag << "' not found while decoding phase.";
        phase.insert(it->second.get());
        ss >> std::ws;
      }
      ss.ignore(1); // ']'
      phaseInfo_.push_back(phase);
      phase.clear();
    }
  }
  ss >> std::ws;
  ss.ignore(1); // ']'
}


void Network::enableProfiling() {
  for (auto p: regions_) {
    std::shared_ptr<Region> r = p.second;
    r->enableProfiling();
  }
}

void Network::disableProfiling() {
  for (auto p: regions_) {
    std::shared_ptr<Region> r = p.second;
    r->disableProfiling();
  }
}

void Network::resetProfiling() {
  for (auto p: regions_) {
    std::shared_ptr<Region>  r = p.second;
    r->resetProfiling();
  }
}

  /*
   * Adds a region to the RegionImplFactory's list of packages
   */
void Network::registerRegion(const std::string name, RegisteredRegionImpl *wrapper) {
	RegionImplFactory::registerRegion(name, wrapper);
}
  /*
   * Removes a region from RegionImplFactory's list of packages
   */
void Network::unregisterRegion(const std::string name) {
	RegionImplFactory::unregisterRegion(name);
}

std::string Network::getRegistrations() {
  return RegionImplFactory::getRegistrations();
}

void Network::cleanup() {
    RegionImplFactory::cleanup();
}

bool Network::operator==(const Network &o) const {

  if (initialized_ != o.initialized_ || iteration_ != o.iteration_ ||
      minEnabledPhase_ != o.minEnabledPhase_ ||
      maxEnabledPhase_ != o.maxEnabledPhase_ ||
      regions_.size() != o.regions_.size()) {
    return false;
  }

  for(auto iter = regions_.cbegin(); iter != regions_.cend(); ++iter){
    std::shared_ptr<Region> r1 = iter->second;
    std::string name = r1->getName();
    auto itr = o.regions_.find(name);
    if (itr == o.regions_.end()) return false;
    std::shared_ptr<Region> r2 = itr->second;
    if (*(r1.get()) != *(r2.get())) {
      return false;
    }
  }
  return true;
}

std::ostream &operator<<(std::ostream &f, const Network &n) {
  // Display Network, Region, Links

  f << "Network: {\n";
  f << "iteration: " << n.iteration_ << "\n";
  f << "Regions: " << "[\n";

  for(auto iter = n.regions_.cbegin(); iter != n.regions_.cend(); ++iter) {
      std::shared_ptr<Region>  r = iter->second;
      f << (*r.get());
  }
  f << "]\n"; // end of regions

  // Display the Links
  f << "Links: [\n";
  for(auto iter = n.regions_.cbegin(); iter != n.regions_.cend(); ++iter) {
    std::shared_ptr<Region> r = iter->second;
    const std::map<std::string, std::shared_ptr<Input>> inputs = r->getInputs();
    for (const auto & inputs_input : inputs)
    {
      const std::vector<std::shared_ptr<Link>>& links = inputs_input.second->getLinks();
      for (const auto & links_link : links)
      {
        auto l = links_link;
        f << (*l.get());
      }

    }
  }
  f << "]\n"; // end of links

  f << "}\n"; // end of network
  f << std::endl;
  return f;
}



} // namespace htm
