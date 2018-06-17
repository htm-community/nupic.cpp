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
 * Implementation of Input class
 *
 */

#include <cstring> // memset

#include <nupic/types/ptr_types.hpp>

#include <nupic/ntypes/Array.hpp>
#include <nupic/engine/Input.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Link.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/types/BasicType.hpp>

namespace nupic
{

  Input::Input(Region& region, NTA_BasicType dataType) :
    region_(region), initialized_(false),  data_(dataType), name_("Unnamed")
  {
  }

  Input::~Input()
  {
    uninitialize();
    std::vector<Link_Ptr_t> linkscopy = links_;
    for (auto & elem : linkscopy)
    {
      removeLink(elem);
    }
  }


  void
  Input::addLink(Link_Ptr_t link, Output* srcOutput)
  {
    if (initialized_)
      NTA_THROW << "Attempt to add link to input " << name_
                << " on region " << region_.getName()
                << " when input is already initialized";

    // Make sure we don't already have a link to the same output
    for (std::vector<Link_Ptr_t>::const_iterator link = links_.begin();
         link != links_.end(); link++)
    {
      if (srcOutput == &((*link)->getSrc()))
      {
        NTA_THROW << "addLink -- link from region " << srcOutput->getRegion().getName()
                  << " output " << srcOutput->getName() << " to region "
                  << region_.getName() << " input " << getName() << " already exists";
      }
    }

    links_.push_back(link);

    srcOutput->addLink(link);
    // Note -- link is not usable until we set the destOffset, which
    // is calculated at initialization time
  }


  void
  Input::removeLink(Link_Ptr_t& link)
  {

    // removeLink should only be called internally -- if it
    // does not exist, it is a logic error
    auto linkiter = links_.begin();
    for(; linkiter!= links_.end(); linkiter++)
    {
      if (*linkiter == link)
        break;
    }

    NTA_CHECK(linkiter != links_.end());

    if (region_.isInitialized())
      NTA_THROW << "Cannot remove link " << link->toString()
                << " because destination region " << region_.getName()
                << " is initialized. Remove the region first.";

    // We may have been initialized even if our containing region
    // was not. If so, uninitialize.
    uninitialize();
    link->getSrc().removeLink(link);
    links_.erase(linkiter);

    link.reset();
   }

  Link_Ptr_t Input::findLink(const std::string& srcRegionName,
                        const std::string& srcOutputName)
  {
    std::vector<Link_Ptr_t>::const_iterator linkiter = links_.begin();
    for (; linkiter != links_.end(); linkiter++)
    {
      Output& output = (*linkiter)->getSrc();
      if (output.getName() == srcOutputName &&
          output.getRegion().getName() == srcRegionName)
      {
        return *linkiter;
      }
    }

    // Link not found
    return nullptr;
  }

  void
  Input::prepare()
  {
    // Each link copies data into its section of the overall input
    for (auto & elem : links_)
    {
      (elem)->compute();
    }
  }

  Array &
  Input::getData()
  {
    NTA_CHECK(initialized_) << "Attempt to access an Input object but it is not initialized.";
    return data_;
  }

  Region&
  Input::getRegion()
  {
    return region_;
  }

  const std::vector<Link_Ptr_t>&
  Input::getLinks()
  {
    return links_;
  }




  // Called after all links have been created and Output buffers have been allocated. 
  // Now we can calculate our input buffer size and offset and set up any data structures needed
  // for copying data over a link.

  void Input::initialize()
  {
    if (initialized_)
      return;

    bool fanIn = (links_.size() > 1);

    // Calculate our size and the offset into the Input buffer used by each link.
    // The first link starts at offset 0 
    // The next link starts at offset +  size of its source buffer.
    size_t offset = 0;
    for (std::vector<Link_Ptr_t>::const_iterator l = links_.begin(); l != links_.end(); l++)
    {
      // Setting the destination offset makes the link usable.
      // We can do zeroCopy if not fan-in on Input and buffer data types are the same.
      bool zeroCopy = !fanIn && (*l)->getSrc().getData().getType() == data_.getType();
      (*l)->initialize(offset, zeroCopy);
      offset += (*l)->getSrc().getData().getCount();  // the size of the link's source Output Buffer
    }
    size_t totalSize = offset;

    // Initially allocate and zero an input buffer even if we will be using zero copy
    // so that the inspectors will have something to look at before the first run.
    data_.allocateBuffer(totalSize);
    data_.zeroBuffer();

    initialized_ = true;
  }

  void Input::uninitialize()
  {
    if (!initialized_)
      return;

    NTA_CHECK(!region_.isInitialized());

    initialized_ = false;
    data_.releaseBuffer();
  }

  bool Input::isInitialized()
  {
    return(initialized_);
  }

  void Input::setName(const std::string& name)
  {
    name_ = name;
  }

  const std::string& Input::getName() const
  {
    return  name_;
  }


} // namespace

