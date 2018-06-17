/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013, Numenta, Inc.  Unless you have an agreement
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
Implementation of the Region class

Methods related to parameters are in Region_parameters.cpp
Methods related to inputs and outputs are in Region_io.cpp

*/

#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>

#include <nupic/engine/Input.hpp>
#include <nupic/engine/Link.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/RegionImpl.hpp>
#include <nupic/engine/RegionImplFactory.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/os/Timer.hpp>
#include <nupic/utils/Log.hpp>
#include <nupic/ntypes/BundleIO.hpp>

namespace nupic {

class GenericRegisteredRegionImpl;

// Create region from parameter spec
Region::Region(std::string name, const std::string &nodeType,
               const std::string &nodeParams, Network *network)
    : name_(std::move(name)), type_(nodeType), initialized_(false),
      network_(network), profilingEnabled_(false) 
{
  // Set region info before creating the RegionImpl so that the
  // Impl has access to the region info in its constructor.
  RegionImplFactory &factory = RegionImplFactory::getInstance();
  spec_ = factory.getSpec(nodeType);


  impl_ = factory.createRegionImpl(nodeType, nodeParams, this);
  createInputsAndOutputs_();
}

// Deserialize region
Region::Region(std::string name, const std::string &nodeType,
               BundleIO &bundle, Network *network)
    : name_(std::move(name)), type_(nodeType), initialized_(false),
      network_(network), profilingEnabled_(false) 
{
  // Set region info before creating the RegionImpl so that the
  // Impl has access to the region info in its constructor.
  RegionImplFactory &factory = RegionImplFactory::getInstance();
  spec_ = factory.getSpec(nodeType);

  createInputsAndOutputs_();
  impl_ = factory.deserializeRegionImpl(nodeType, bundle, this);

}

Network *Region::getNetwork() { return network_; }

void Region::createInputsAndOutputs_() {

  // Create all the outputs for this node type. By default outputs are zero size
  for (size_t i = 0; i < spec_->outputs.getCount(); ++i) {
    const std::pair<std::string, OutputSpec> &p = spec_->outputs.getByIndex(i);
    std::string outputName = p.first;
    const OutputSpec &os = p.second;
    auto output = new Output(*this, os.dataType);
    outputs_[outputName] = output;
    // keep track of name in the output also -- see note in Region.hpp
    output->setName(outputName);
  }

  // Create all the inputs for this node type.
  for (size_t i = 0; i < spec_->inputs.getCount(); ++i) {
    const std::pair<std::string, InputSpec> &p = spec_->inputs.getByIndex(i);
    std::string inputName = p.first;
    const InputSpec &is = p.second;

    auto input = new Input(*this, is.dataType);
    inputs_[inputName] = input;
    // keep track of name in the input also -- see note in Region.hpp
    input->setName(inputName);
  }
}

bool Region::hasOutgoingLinks() const {
  for (const auto &elem : outputs_) {
    if (elem.second->hasOutgoingLinks()) {
      return true;
    }
  }
  return false;
}

Region::~Region() {
  if (initialized_)
    uninitialize();

  // If there are any links connected to our outputs, this should fail.
  // We catch this error in the Network class and give the
  // user a good error message (regions may be removed either in
  // Network::removeRegion or Network::~Network())
  for (auto &elem : outputs_) {
    delete elem.second;
    elem.second = nullptr;
  }
  outputs_.clear();

  for (auto &elem : inputs_) {
    delete elem
        .second; // This is an Input object. Its destructor deletes the links.
    elem.second = nullptr;
  }
  inputs_.clear();

  delete impl_;
}

void Region::initialize() {

  if (initialized_)
    return;

  impl_->initialize();
  initialized_ = true;
}

bool Region::isInitialized() const { return initialized_; }

const std::string &Region::getName() const { return name_; }

const std::string &Region::getType() const { return type_; }

const Spec *Region::getSpec() const { return spec_; }

const Spec *Region::getSpecFromType(const std::string &nodeType) {
  RegionImplFactory &factory = RegionImplFactory::getInstance();
  return factory.getSpec(nodeType);
}

void Region::registerCPPRegion(const std::string name,
                               GenericRegisteredRegionImpl *wrapper) {
  RegionImplFactory::registerCPPRegion(name, wrapper);
}

void Region::unregisterCPPRegion(const std::string name) {
  RegionImplFactory::unregisterCPPRegion(name);
}

void Region::enable() {
  NTA_THROW << "Region::enable not implemented (region name: " << getName()
            << ")";
}

void Region::disable() {
  NTA_THROW << "Region::disable not implemented (region name: " << getName()
            << ")";
}

std::string Region::executeCommand(const std::vector<std::string> &args) {
  std::string retVal;
  if (args.size() < 1) {
    NTA_THROW << "Invalid empty command specified";
  }

  if (profilingEnabled_)
    executeTimer_.start();

  retVal = impl_->executeCommand(args, (UInt64)(-1));

  if (profilingEnabled_)
    executeTimer_.stop();

  return retVal;
}

void Region::compute() {
  if (!initialized_)
    NTA_THROW << "Region " << getName()
              << " unable to compute because not initialized";

  if (profilingEnabled_)
    computeTimer_.start();

  impl_->compute();

  if (profilingEnabled_)
    computeTimer_.stop();

  return;
}


size_t Region::getNodeOutputElementCount(const std::string &name) {
  // Use output count if specified in nodespec, otherwise
  // ask the region Impl what it expects to produce.
  NTA_CHECK(spec_->outputs.contains(name));
  size_t count = spec_->outputs.getByName(name).count;
  if (count == 0) {
    try {
      count = impl_->getNodeOutputElementCount(name);
    } catch (Exception &e) {
      NTA_THROW << "Internal error -- the size for the output " << name
                << "is unknown. : " << e.what();
    }
  }

  return count;
}

void Region::initOutputs() {
  // Some outputs are optional. These outputs will have 0 elementCount in the
  // node spec and also return 0 from impl->getNodeOutputElementCount(). These
  // outputs still appear in the output map, but with an array size of 0. All
  // other outputs we initialize to size determined by spec or by impl.

  for (auto &elem : outputs_) {
    const std::string &name = elem.first;

    size_t count = 0;
    try {
      count = getNodeOutputElementCount(name);
    } catch (nupic::Exception &e) {
      NTA_THROW << "Internal error -- unable to get size of output " << name
                << " : " << e.what();
    }
    elem.second->initialize(count);
  }
}

void Region::initInputs() const {
  auto i = inputs_.begin();
  for (; i != inputs_.end(); i++) {
    i->second->initialize();
  }
}



void Region::removeAllIncomingLinks() {
  InputMap::const_iterator i = inputs_.begin();
  for (; i != inputs_.end(); i++) {
    std::vector<Link_Ptr_t> links = i->second->getLinks();
    for (auto &links_link : links) {
      i->second->removeLink(links_link);
    }
  }
}

void Region::uninitialize() { initialized_ = false; }

void Region::setPhases(std::set<UInt32> &phases) { phases_ = phases; }

std::set<UInt32> &Region::getPhases() { return phases_; }

void Region::serializeImpl(BundleIO &bundle) { impl_->serialize(bundle); }

void Region::enableProfiling() { profilingEnabled_ = true; }

void Region::disableProfiling() { profilingEnabled_ = false; }

void Region::resetProfiling() {
  computeTimer_.reset();
  executeTimer_.reset();
}

const Timer &Region::getComputeTimer() const { return computeTimer_; }

const Timer &Region::getExecuteTimer() const { return executeTimer_; }


void Region::serializeOutput(YAML::Emitter &out) {

  out << YAML::BeginSeq;
  for (auto iter : outputs_) {
    Output *output = iter.second;
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << output->getName();
    out << YAML::Key << "data";
    output->serializeData(out);
    out << YAML::EndMap;
  }
  out << YAML::EndSeq;
}

void Region::deserializeOutput(const YAML::Node& doc) {
  for (const auto &valiter : doc) { // list of Outputs for this region.
    NTA_CHECK(valiter.IsMap()) << "Deserializing network structure file -- missing region output.";
    NTA_CHECK(valiter.size() == 2)  << "Deserializing network structure file -- wrong map size for region output.";

    YAML::Node node;
    node = valiter["name"];
    NTA_CHECK(node.IsScalar()) << "Deserializing network structure file -- missing region output name.";
    std::string name = node.as<std::string>();

    Output *output = getOutput(name);
    NTA_CHECK(output != nullptr) << "Deserializing network structure file -- No output with the name '" << name << "'.";

    node = valiter["data"];
    NTA_CHECK(node.IsMap()) << "Deserializing network structure file -- missing region output data.";
    output->deserializeData(node);

  }
}

} // namespace nupic
