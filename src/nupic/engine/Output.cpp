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
 * Implementation of Output class
 *
*/

#include <cstring> // memset
#include <nupic/types/BasicType.hpp>
#include <nupic/ntypes/Array.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/Link.hpp> // temporary


namespace nupic
{

Output::Output(Region& region, NTA_BasicType type) :
  region_(region),  name_("Unnamed"), nodeOutputElementCount_(0)
{
  data_ = Array(type);
}

Output::~Output() 
{
}

// allocate output buffer
void
Output::initialize(size_t count)
{
  // reinitialization is ok
  // might happen if initial initialization failed with an
  // exception (elsewhere) and was retried.
  // If just restored from serialization it will already have a buffer.

  if (data_.getBuffer() == nullptr) {
    // no buffer, so create one and zero it.
    nodeOutputElementCount_ = count;
    if (nodeOutputElementCount_ != 0)
    {
      data_.allocateBuffer(nodeOutputElementCount_);
      // Zero the buffer because unitialized outputs can screw up inspectors,
      // which look at the output before compute(). NPC-60
      data_.zeroBuffer();
    }
  }
}

void
Output::addLink(Link_Ptr_t link)
{
  // Make sure we don't add the same link twice
  // It is a logic error if we add the same link twice here, since
  // this method should only be called from Input::addLink
  auto linkIter = links_.find(link);
  NTA_CHECK(linkIter == links_.end());

  links_.insert(link);
}

void
Output::removeLink(Link_Ptr_t link)
{
  auto linkIter = links_.find(link);
  // Should only be called internally. Logic error if link not found
  NTA_CHECK(linkIter != links_.end());
  // Output::removeLink is only called from Input::removeLink so we don't
  // have to worry about removing it on the Input side
  links_.erase(linkIter);
}





bool
Output::hasOutgoingLinks()
{
  return (!links_.empty());
}

}

