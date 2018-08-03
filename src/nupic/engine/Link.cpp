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
 * Implementation of the Link class
 */
#include <cstring> // memcpy,memset
#include <nupic/engine/Input.hpp>
#include <nupic/engine/Link.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/ntypes/Array.hpp>
#include <nupic/types/BasicType.hpp>
#include <nupic/utils/Log.hpp>

// Set this to true when debugging to enable handy debug-level logging of data
// moving through the links, including the delayed link transitions.
#define _LINK_DEBUG false

namespace nupic {

Link::Link(const std::string &linkType, const std::string &linkParams,
           const std::string &srcRegionName, const std::string &destRegionName,
           const std::string &srcOutputName, const std::string &destInputName,
           const size_t propagationDelay) {
  commonConstructorInit_(linkType, linkParams, srcRegionName, destRegionName,
                         srcOutputName, destInputName, propagationDelay);
}

Link::Link(const std::string &linkType, const std::string &linkParams,
           Output *srcOutput, Input *destInput, const size_t propagationDelay) {
  commonConstructorInit_(linkType, linkParams, srcOutput->getRegion().getName(),
                         destInput->getRegion().getName(), srcOutput->getName(),
                         destInput->getName(), propagationDelay);

  connectToNetwork(srcOutput, destInput);
  // Note -- link is not usable until we set the destOffset, which happens at
  // initialization time
}

Link::Link() {  // needed for deserialization
  destOffset_ = 0;
  src_ = nullptr;
  dest_ = nullptr;
  zeroCopy_ = false;
  initialized_ = false;
}

void Link::commonConstructorInit_(const std::string &linkType,
                                  const std::string &linkParams,
                                  const std::string &srcRegionName,
                                  const std::string &destRegionName,
                                  const std::string &srcOutputName,
                                  const std::string &destInputName,
                                  const size_t propagationDelay) {
  linkType_ = linkType;
  linkParams_ = linkParams;
  srcRegionName_ = srcRegionName;
  srcOutputName_ = srcOutputName;
  destRegionName_ = destRegionName;
  destInputName_ = destInputName;
  propagationDelay_ = propagationDelay;
  destOffset_ = 0;
  src_ = nullptr;
  dest_ = nullptr;
  initialized_ = false;
  zeroCopy_ = false;

}

Link::~Link() {}


void Link::initialize(size_t destinationOffset, bool zeroCopy) {
  // Make sure all information is specified and
  // consistent. Unless there is a NuPIC implementation
  // error, all these checks are guaranteed to pass
  // because of the way the network is constructed
  // and initialized.

  // Make sure we have been attached to a real network
  NTA_CHECK(src_ != nullptr)
      << "Link::initialize() and src_ Output object not set.";
  NTA_CHECK(dest_ != nullptr)
      << "Link::initialize() and dest_ Input object not set.";

  // We do not need to copy the buffer during propogation if:
  //  1) There is no fanIn (no more than one link into the same Input buffer)
  //  2) A data conversion is not required (Output and Input data types same)
  // Note that if there is propagation delay, the buffer is copied to the queue
  //      but a copy from the queue to the destination Input buffer can be avoided
  //      if there is no fanIn or conversion.
  zeroCopy_ = zeroCopy;
  destOffset_ = destinationOffset;

  // ---
  // Initialize the propagation delay buffer
  // But skip it if it already has something in it from deserialize().
  // ---
  if (propagationDelay_ > 0 && propagationDelayBuffer_.empty()) {
    // Initialize delay data elements.  This must be done during initialize()
    // because the buffer size is not known prior to then.
    // front of queue will be the next value to be copied to the dest Input buffer.
    // back of queue will be the same as the current contents of source Output.
    NTA_BasicType dataElementType = src_->getData().getType();
    size_t dataElementCount = src_->getData().getCount();
    for (size_t i = 0; i < (propagationDelay_); i++) {
      Array arrayTemplate(dataElementType);
      arrayTemplate.allocateBuffer(dataElementCount);
      arrayTemplate.zeroBuffer();
      propagationDelayBuffer_.push_back(arrayTemplate);
    }
  }

  initialized_ = true;
}

void Link::compute() {
  NTA_CHECK(initialized_);
  Array* from = &src_->getData();
  Array* to = &dest_->getData();

// Debugging displays
//if (destRegionName_ == "region2" && destInputName_ == "bottomUpIn") {
//  std::cout << "Link: source " << src_->getData().nonZero() << std::endl;
//}
  if (propagationDelay_) {
    // A delayed link's queue buffer size should always be number of delays.
    NTA_CHECK(propagationDelayBuffer_.size() == (propagationDelay_));
    from = &propagationDelayBuffer_.front();
  }
  if (zeroCopy_) {
    // This does a shallow copy so that the buffer does NOT get copied
    // but the type, count, capacity, and shared_ptr are copied.
    // The buffer is a smart shared pointer so its contents will be deleted 
    // when the last reference is deleted.
    from->zeroCopy(*to);
  } else {
    NTA_CHECK(from->getCount() + destOffset_ <= to->getCapacity())
        << "Not enough room in buffer to propogate to " << destRegionName_
        << " " << destInputName_ << ". ";
    // This does a deep copy of the buffer, and if needed it also does a type
    // conversion at the same time. It is copied into the destination Input
    // buffer at the specified offset so an Input with multiple incoming links
    // has the Output buffers appended into a single large Input buffer.
    from->convertInto(*to, destOffset_);
  }
// Debugging code
//if (destRegionName_ == "region2" && destInputName_ == "bottomUpIn") {
//  std::cout << "Link: Input: " << to->nonZero() << std::endl;
//}
}

void Link::shiftBufferedData() 
{
  if (propagationDelay_) {
    Array *from = &src_->getData();
    NTA_CHECK(propagationDelayBuffer_.size() == (propagationDelay_));

    // push a copy of the source Output buffer on the back of the queue.
    // This must be a deep copy.
    Array a = from->copy();
    propagationDelayBuffer_.push_back(a);

    // Pop the head of the queue
    // The top of the queue now becomes the value to copy to destination.
    propagationDelayBuffer_.pop_front();
  }
}


// Return constructor params
const std::string &Link::getLinkType() const { return linkType_; }

const std::string &Link::getLinkParams() const { return linkParams_; }

const std::string &Link::getSrcRegionName() const { return srcRegionName_; }

const std::string &Link::getSrcOutputName() const { return srcOutputName_; }

const std::string &Link::getDestRegionName() const { return destRegionName_; }

const std::string &Link::getDestInputName() const { return destInputName_; }

std::string Link::getMoniker() const {
  std::stringstream ss;
  ss << getSrcRegionName() << "." << getSrcOutputName() << "-->"
     << getDestRegionName() << "." << getDestInputName();
  return ss.str();
}

const std::string Link::toString() const {
  std::stringstream ss;
  ss << "[" << getSrcRegionName() << "." << getSrcOutputName();
  ss << " to " << getDestRegionName() << "." << getDestInputName();
  ss << "]";
  return ss.str();
}

void Link::connectToNetwork(Output *src, Input *dest) {
  NTA_CHECK(src != nullptr);
  NTA_CHECK(dest != nullptr);

  src_ = src;
  dest_ = dest;
}

// The methods below only work on connected links.
Output &Link::getSrc() const

{
  NTA_CHECK(src_ != nullptr)
      << "Link::getSrc() can only be called on a connected link";
  return *src_;
}

Input &Link::getDest() const {
  NTA_CHECK(dest_ != nullptr)
      << "Link::getDest() can only be called on a connected link";
  return *dest_;
}

void Link::serialize(YAML::Emitter &out) {
  size_t srcCount = ((!src_) ? (size_t)0 : src_->getData().getCount());
  NTA_BasicType srcType =
      ((!src_) ? BasicType::parse(getLinkType()) : src_->getData().getType());
  out << YAML::BeginMap;
  out << YAML::Key << "linkType" << YAML::Value << getLinkType();
  out << YAML::Key << "params" << YAML::Value << getLinkParams();
  out << YAML::Key << "srcRegion" << YAML::Value << getSrcRegionName();
  out << YAML::Key << "srcOutput" << YAML::Value << getSrcOutputName();
  out << YAML::Key << "destRegion" << YAML::Value << getDestRegionName();
  out << YAML::Key << "destInput" << YAML::Value << getDestInputName();
  out << YAML::Key << "propagationDelay" << YAML::Value << propagationDelay_;
  out << YAML::Key << "propagationDelayBuffer" << YAML::Value << YAML::BeginSeq;
  if (propagationDelay_ > 0) {
    // we need to capture the propagationDelayBuffer_ used for propagationDelay
    // Do not copy the last entry.  It is the same as the output buffer.

    // The current contents of the Destination Input buffer also needs
    // to be captured as if it were the top value of the propagationDelayBuffer.
    // When restored, it will be copied to the dest input buffer and popped off
    // before the next execution. If there is an offset, we only
    // want to capture the amount of the input buffer contributed by
    // this link.
    Array a = dest_->getData().subset(destOffset_, srcCount); 
    a.serialize(out); // our part of the current Dest Input buffer.

    std::deque<Array>::iterator itr;
    for (auto itr = propagationDelayBuffer_.begin();
         itr != propagationDelayBuffer_.end(); itr++) {
      if (itr + 1 == propagationDelayBuffer_.end())
        break; // skip the last buffer. Its the current output.
      Array &buf = *itr;
      buf.serialize(out);
    } // end for
  }
  out << YAML::EndSeq;

  out << YAML::EndMap;
}

void Link::deserialize(const YAML::Node &link) {
  // Each link is a map -- extract the 9 values in the map
  // The "circularBuffer" element is a two dimentional array only present if
  // propogationDelay > 0.
  NTA_CHECK(link.Type() == YAML::NodeType::Map)
      << "Invalid network structure file -- bad link (not a map)";
  NTA_CHECK(link.size() == 8)
      << "Invalid network structure file -- bad link (wrong size)";

  YAML::Node node;

  // 1. type
  node = link["linkType"];
  NTA_CHECK(node.IsScalar()) << "Invalid network structure file -- link does "
                                "not have a 'linkType' field.";
  std::string linkType = node.as<std::string>();

  // 2. params
  node = link["params"];
  NTA_CHECK(node.IsScalar()) << "Invalid network structure file -- link does "
                                "not have a 'params' field.";
  std::string linkParams = node.as<std::string>();

  // 3. srcRegion (name)
  node = link["srcRegion"];
  NTA_CHECK(node.IsScalar()) << "Invalid network structure file -- link does "
                                "not have a 'srcRegion' field.";
  std::string srcRegionName = node.as<std::string>();

  // 4. srcOutput
  node = link["srcOutput"];
  NTA_CHECK(node.IsScalar()) << "Invalid network structure file -- link does "
                                "not have a 'srcOutput' field";
  std::string srcOutputName = node.as<std::string>();

  // 5. destRegion
  node = link["destRegion"];
  NTA_CHECK(node.IsScalar()) << "Invalid network structure file -- link does "
                                "not have a 'destRegion' field.";
  std::string destRegionName = node.as<std::string>();

  // 6. destInput
  node = link["destInput"];
  NTA_CHECK(node.IsScalar()) << "Invalid network structure file -- link does "
                                "not have a 'destInput' field.";
  std::string destInputName = node.as<std::string>();

  // 7. propagationDelay (number of cycles to delay propagation)
  node = link["propagationDelay"];
  NTA_CHECK(node.IsScalar()) << "Invalid network structure file -- link does "
                                "not have a 'propagationDelay' field.";
  size_t propagationDelay = node.as<size_t>();

  // fill in the data for the Link object
  commonConstructorInit_(linkType, linkParams, srcRegionName, destRegionName,
                         srcOutputName, destInputName, propagationDelay);

  // 8. propagationDelayBuffer
  node = link["propagationDelayBuffer"];
  NTA_CHECK(node.IsSequence()) << " Invalid network structure file-- link does "
                                  "not have a 'propagationDelayBuffer' field.";
  // if no propagationDelay (value = 0) then there should be an empty sequence.
  size_t idx = 0;
  for (const auto &valiter : node) {
    NTA_CHECK(idx < propagationDelay_) << "Invalid network structure file -- "
                                          "link has too many buffers in "
                                          "'propagationDelayBuffer'.";
    Array a;
    propagationDelayBuffer_.push_back(a);
    propagationDelayBuffer_.back().deserialize(valiter);
    idx++;
  }
  NTA_CHECK(idx == propagationDelay_) << "Invalid network structure file -- "
                                         "link has too few buffers in "
                                         "'propagationDelayBuffer'.";
  // To complete the restore, call r->prepareInputs()
  // and then shiftBufferedData();
}

std::ostream &operator<<(std::ostream &f, const Link &link) {
  f << "<Link>\n";
  f << "  <type>" << link.getLinkType() << "</type>\n";
  f << "  <params>" << link.getLinkParams() << "</params>\n";
  f << "  <srcRegion>" << link.getSrcRegionName() << "</srcRegion>\n";
  f << "  <destRegion>" << link.getDestRegionName() << "</destRegion>\n";
  f << "  <srcOutput>" << link.getSrcOutputName() << "</srcOutput>\n";
  f << "  <destInput>" << link.getDestInputName() << "</destInput>\n";
  f << "  <propagationDelay>" << link.getPropagationDelay()
    << "</propagationDelay>\n";
  f << "</Link>\n";
  return f;
}

} // namespace nupic
