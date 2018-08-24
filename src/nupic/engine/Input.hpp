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
 * Interface for the internal Input class.
 *
 * @note This class is internal, and is not wrapped.
 */

#ifndef NTA_INPUT_HPP
#define NTA_INPUT_HPP

#include <vector>

#include <nupic/types/ptr_types.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/ntypes/Array.hpp>

namespace nupic
{
  class Link;
  class Region;
  class Output;

  /**
   * Represents a named input to a Region. (e.g. bottomUpIn)
   *
   * @note Input is not available in the public API, but is visible by
   * the RegionImpl.
   *
   * @todo identify methods that may be called by RegionImpl -- this
   * is the internal "public interface"
   */
  class Input
  {
  public:

    /**
     * Constructor.
     *
     * @param region
     *        The region that the input belongs to.
     * @param type
     *        The type of the input, i.e. TODO
     */
    Input(Region& region, NTA_BasicType type);

    /**
     *
     * Destructor.
     *
     */
    ~Input();

    /**
     *
     * Set the name for the input.
     *
     * Inputs need to know their own name for error messages.
     *
     * @param name
     *        The name of the input
     *
     */
    void setName(const std::string& name);

    /**
     * Get the name of the input.
     *
     * @return
     *        The name of the input
     */
    const std::string& getName() const;

    /**
     * Add the given link to this input and to the list of links on the output
     *
     * @param link
     *        The link to add.
     * @param srcOutput
     *        The output of previous Region, which is also the source of the input
     */
    void
    addLink(Link_Ptr_t link, Output* srcOutput);

    /**
     * Locate an existing Link to the input.
     *
     * It's called by Network.removeLink() and internally when adding a link
     *
     * @param srcRegionName
     *            The name of the source Region
     * @param srcOutputName
     *            The name of the source Output
     *
     * @returns
     *     The link if found or @c NULL if no such link exists
     */
    Link_Ptr_t
    findLink(const std::string& srcRegionName,
             const std::string& srcOutputName);

    /**
     * Removing an existing link from the input.
     *
     * It's called in four cases:
     *
     * 1. Network.removeLink()
     * 2. Network.removeRegion() when given @a srcRegion
     * 3. Network.removeRegion() when given @a destRegion
     * 4. Network.~Network()
     *
     * It is an error to call this if our containing region
     * is uninitialized.
     *
     * @note This method will set the Link pointer to NULL on return (to avoid
     * a dangling reference)
     *
     * @param link
     *        The Link to remove, possibly retrieved by findLink(), note that
     *        it is a reference to the pointer, not the pointer itself.
     */
    void
    removeLink(Link_Ptr_t& link);

    /**
     * Make input data available.
     *
     * Called by Region.prepareInputs()
     */
    void
    prepare();

    /**
     *
     * Get the data of the input.
     *
     * @returns
     *         A reference to the data of the input as an @c Array
     */
    Array& getData();

    /**
     *
     * Get the Region that the input belongs to.
     *
     * @returns
     *         The mutable reference to the Region that the input belongs to
     */
    Region& getRegion();

    /**
     *
     * Get all the Link objects added to the input.
     *
     * @returns
     *         All the Link objects added to the input
     */
    const std::vector<Link_Ptr_t>& getLinks();




    /**
     * Initialize the Input .
     *
     * After the input has all the information it needs, it is initialized by
     * this method. Volatile data structures (e.g. the input buffer) are set up。
     */
    void
    initialize();

    /**
     * Tells whether the Input is initialized.
     *
     * @returns
     *         Whether the Input is initialized
     */
    bool
    isInitialized();



  private:
    Region& region_;
    // buffer is concatenation of input buffers (after prepare), or,
    // if zeroCopyEnabled it points to the connected output
    bool isRegionLevel_;


    // Use a vector of links because order is important.
    std::vector<Link_Ptr_t> links_;

    // volatile (non-serialized) state
    bool initialized_;
    Array data_;

    // Useful for us to know our own name
    std::string name_;

    // Internal methods

    /*
     * uninitialize is called by removeLink
     * and in our destructor. It is an error
     * to call it if our region is initialized.
     * It frees the input buffer and the splitter map
     * but does not affect the links.
     */
    void
    uninitialize();



  };

}

#endif // NTA_INPUT_HPP
