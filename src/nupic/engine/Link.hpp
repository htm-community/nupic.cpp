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
 * Interface for the Link class
 */

#ifndef NTA_LINK_HPP
#define NTA_LINK_HPP

#include <string>
#include <deque>

//#include <yaml-cpp/yaml.h>

#include <nupic/engine/Input.hpp> // needed for splitter map
#include <nupic/ntypes/Array.hpp>
#include <nupic/types/Types.hpp>

namespace nupic {

class Output;
class Input;

/**
 *
 * Represents a link between regions in a Network.
 *
 * @nosubgrouping
 *
 */
class Link {
public:
  /**
   * @name Initialization
   *
   * @{
   *
   * Links have four-phase initialization.
   *
   * 1. construct with link type, params, names of regions and inputs/outputs
   * 2. wire in to network (setting src and dest Output/Input pointers)
   * 3. set source and destination dimensions
   * 4. initialize -- sets the offset in the destination Input (not known
   * earlier)
   *
   * De-serializing is the same as phase 1.
   *
   * In phase 3, NuPIC will set and/or get source and/or destination
   * dimensions until both are set. Normally we will only set the src
   * dimensions, and the dest dimensions will be induced. It is possible to go
   * the other way, though.
   *
   * The @a linkType and @a linkParams parameters are given to
   * the LinkPolicyFactory to create a link policy
   *
   * @todo Should LinkPolicyFactory be documented?
   *
   */

  /**
   * Initialization Phase 1: setting parameters of the link.
   *
   * @param linkType
   *            The type of the link
   * @param linkParams
   *            The parameters of the link
   * @param srcRegionName
   *            The name of the source Region
   * @param destRegionName
   *            The name of the destination Region
   * @param srcOutputName
   *            The name of the source Output
   * @param destInputName
   *            The name of the destination Input
   * @param propagationDelay
   *            Propagation delay of the link as number of network run
   *            iterations involving the link as input; the delay vectors, if
   *            any, are initially populated with 0's. Defaults to 0=no delay.
   *            Per design, data on no-delay links is to become available to
   *            destination inputs within the same time step, while data on
   *            delayed links (propagationDelay > 0) is to be updated
   *            "atomically" between time steps.
   *
   * @internal
   *
   * @todo It seems this constructor should be deprecated in favor of the other,
   * which is less redundant. This constructor is being used for unit testing
   * and unit testing links and for deserializing networks.
   *
   * See comments below commonConstructorInit_()
   *
   * @endinternal
   *
   */
  Link(const std::string &linkType, const std::string &linkParams,
       const std::string &srcRegionName, const std::string &destRegionName,
       const std::string &srcOutputName = "",
       const std::string &destInputName = "",
       const size_t propagationDelay = 0);

  /**
   * De-serialization use case. Creates a "blank" link. The caller must follow
   * up with Link::deserialize() and Link::connectToNetwork
   *
   */
  Link();

  /**
   * Initialization Phase 2: connecting inputs/outputs to
   * the Network.
   *
   * @param src
   *            The source Output of the link
   * @param dest
   *            The destination Input of the link
   */
  void connectToNetwork(Output *src, Input *dest);

  /*
   * Initialization Phase 1 and 2.
   *
   * @param linkType
   *            The type of the link
   * @param linkParams
   *            The parameters of the link
   * @param srcOutput
   *            The source Output of the link
   * @param destInput
   *            The destination Input of the link
   * @param propagationDelay
   *            Propagation delay of the link as number of network run
   *            iterations involving the link as input; the delay vectors, if
   *            any, are initially populated with 0's. Defaults to 0=no delay
   */
  Link(const std::string &linkType, const std::string &linkParams,
       Output *srcOutput, Input *destInput, size_t propagationDelay = 0);


  /**
   * Destructor
   */
  ~Link();

    /**
   * Initialization Phase 4: sets the offset in the destination Input .
   *
   * @param destinationOffset
   *            The offset in the destination Input, i.e. TODO
   *
   */
  void initialize(size_t destinationOffset, bool zeroCopy);

  bool isInitialized() { return initialized_; }

  /**
   * Copy data from source to destination.
   *
   * Nodes request input data from their input objects. The input objects,
   * in turn, request links to copy data into the inputs.
   *
   * @note This method must be called on a fully initialized link(all 4 phases).
   *
   */
  void compute();

  /*
   * No-op for links without delay; for delayed links, remove head element of
   * the propagation delay buffer and push back the current value from source.
   *
   * NOTE It's intended that this method be called exactly once on all links
   * within a network at the end of every time step. Network::run calls it
   * automatically on all links at the end of each time step.
   */
  void shiftBufferedData();

  /**
   * @}
   *
   * @name Parameter getters of the link
   *
   * @{
   *
   */

  /**
   * Get the type of the link.
   *
   * @returns
   *         The type of the link
   */
  const std::string &getLinkType() const;

  /**
   * Get the parameters of the link.
   *
   * @returns
   *         The parameters of the link
   */
  const std::string &getLinkParams() const;

  /**
   * Get the name of the source Region
   *
   * @returns
   *         The name of the source Region
   */
  const std::string &getSrcRegionName() const;

  /**
   * Get the name of the source Output.
   *
   * @returns
   *         The name of the source Output
   */
  const std::string &getSrcOutputName() const;

  /**
   * Get the name of the destination Region.
   *
   * @returns
   *         The name of the destination Region
   *
   */
  const std::string &getDestRegionName() const;

  /**
   * Get the name of the destination Input.
   *
   * @returns
   *         The name of the destination Input
   */
  const std::string &getDestInputName() const;

  /**
   * Get the propogation Delay.
   *
   * @returns
   *         The propogation Delay.
   */
  const size_t getPropagationDelay() const { return propagationDelay_; }

  /**
   * @}
   *
   * @name Misc
   *
   * @{
   */

  // The methods below only work on connected links (after phase 2)

  /**
   *
   * Get a generated name of the link in the form
   * RegName.outName --> RegName.inName for debug logging purposes only.
   */
  std::string getMoniker() const;

  /**
   *
   * Get the source Output of the link.
   *
   * @returns
   *         The source Output of the link
   */
  Output &getSrc() const;

  /**
   *
   * Get the destination Input of the link.
   *
   * @returns
   *         The destination Input of the link
   */
  Input &getDest() const;


  /**
   * Convert the Link to a human-readable string.
   *
   * @returns
   *     The human-readable string describing the Link
   */
  const std::string toString() const;

  /**
   * Serialize the link to text.
   *
   * @param f
   *            The output stream being serialized to stream
   * @param link
   *            The Link being serialized
   */
  friend std::ostream &operator<<(std::ostream &f, const Link &link);

  /**
   * Serialize the link to YAML.
   *
   * @param out
   *            The YAML Emitter to encode into (from Network.cpp)
   */
#if defined YAML_SERIALIZATION
  void serialize(YAML::Emitter &out);
#else
  void serialize(std::ostream &f);
#endif // YAML_SERIALIZATION

  /**
   * Deserialize the link from YAML.
   *
   * @param link
   *            The YAML Node to decode from (from Network.cpp)
   *
   * @note After deserializing the link, caller must now call
   *    newLink->connectToNetwork(srcOutput, destInput);
   *
   * After everything is deserialized caller must call
   *    net.initialize()
   */
#if defined YAML_SERIALIZATION
  void deserialize(const YAML::Node &link);
#else
  void deserialize(std::istream &f);
#endif // YAML_SERIALIZATION


private:
  // common initialization for the two constructors.
  void commonConstructorInit_(const std::string &linkType,
                              const std::string &linkParams,
                              const std::string &srcRegionName,
                              const std::string &destRegionName,
                              const std::string &srcOutputName,
                              const std::string &destInputName,
                              const size_t propagationDelay);



  // TODO: The strings with src/dest names are redundant with
  // the src_ and dest_ objects. For unit testing links,
  // and for deserializing networks, we need to be able to create
  // a link object without a network. and for deserializing, we
  // need to be able to instantiate a link before we have instantiated
  // all the regions. (Maybe this isn't true? Re-evaluate when
  // more infrastructure is in place).

  std::string srcRegionName_;
  std::string destRegionName_;
  std::string srcOutputName_;
  std::string destInputName_;

  // We store the values given to use. Use these for
  // serialization instead of serializing the LinkPolicy
  // itself.
  std::string linkType_;
  std::string linkParams_;

  Output *src_;
  Input *dest_;

  // Each link contributes a contiguous chunk of the destination
  // input. The link needs to know its offset within the destination
  // input. This value is set at initialization time.
  size_t destOffset_;

  // Queue buffer for delayed source data buffering
  std::deque<Array> propagationDelayBuffer_;
  // Number of delay slots
  size_t propagationDelay_;

  // link must be initialized before it can compute()
  bool initialized_;

  // true if this link does not need to copy into the Input buffer
  // Just pass the shared_ptr.
  bool zeroCopy_;
};

} // namespace nupic

#endif // NTA_LINK_HPP
