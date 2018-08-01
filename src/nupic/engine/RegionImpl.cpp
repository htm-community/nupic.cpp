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

#include <iostream>

#include <nupic/engine/Region.hpp>
#include <nupic/engine/RegionImpl.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/ntypes/Array.hpp>
#include <nupic/ntypes/Buffer.hpp>
#include <nupic/ntypes/BundleIO.hpp>
#include <nupic/os/FStream.hpp>
#include <nupic/types/BasicType.hpp>

namespace nupic
{

  RegionImpl::RegionImpl(Region *region) :
    region_(region)
  {
  }

  RegionImpl::~RegionImpl()
  {
  }

  // convenience method
  const std::string& RegionImpl::getType() const
  {
    return region_->getType();
  }

  const std::string& RegionImpl::getName() const
  {
    return region_->getName();
  }




  /* ------------- Parameter support --------------- */
  // Normally a RegionImpl subclass would override these functions
  // if they support a parameter of that type and handle parameters
  // with matching names. if the name does not match, come here.
  //
  // Here we check the spec to see if that parameter name is defined
  // and for which type.  If the name and type are ok, then call
  // getParameterFromBuffer() in case it is implemented using the 
  // old style.  If not found there, then issue an error.


  Int32 RegionImpl::getParameterInt32(const std::string& name, Int64 index) {
    return getParameter<Int32>(name, index, NTA_BasicType_Int32);
  }
  UInt32 RegionImpl::getParameterUInt32(const std::string& name, Int64 index) {
    return getParameter<UInt32>(name, index, NTA_BasicType_UInt32);
  }
  Int64 RegionImpl::getParameterInt64(const std::string& name, Int64 index) {
    return getParameter<Int64>(name, index, NTA_BasicType_Int64);
  }
  UInt64 RegionImpl::getParameterUInt64(const std::string& name, Int64 index) {
    return getParameter<UInt64>(name, index, NTA_BasicType_UInt64);
  }
  Real32 RegionImpl::getParameterReal32(const std::string& name, Int64 index) {
    return getParameter<Real32>(name, index, NTA_BasicType_Real32);
  }
  Real64 RegionImpl::getParameterReal64(const std::string& name, Int64 index) {
    return getParameter<Real64>(name, index, NTA_BasicType_Real64);
  }
  bool RegionImpl::getParameterBool(const std::string& name, Int64 index) {
    return getParameter<bool>(name, index, NTA_BasicType_Bool);
  }


  void RegionImpl::setParameterInt32(const std::string& name, Int64 index, Int32 value) {
    setParameter<Int32>(name, index, value, NTA_BasicType_Int32);
  }
  void RegionImpl::setParameterUInt32(const std::string& name, Int64 index, UInt32 value) {
    setParameter<UInt32>(name, index, value, NTA_BasicType_UInt32);
  }
  void RegionImpl::setParameterInt64(const std::string& name, Int64 index, Int64 value) {
    setParameter<Int64>(name, index, value, NTA_BasicType_Int64);
  }
  void RegionImpl::setParameterUInt64(const std::string& name, Int64 index, UInt64 value) {
    setParameter<UInt64>(name, index, value, NTA_BasicType_UInt64);
  }
  void RegionImpl::setParameterReal32(const std::string& name, Int64 index, Real32 value) {
    setParameter<Real32>(name, index, value, NTA_BasicType_Real32);
  }
  void RegionImpl::setParameterReal64(const std::string& name, Int64 index, Real64 value) {
    setParameter<Real64>(name, index, value, NTA_BasicType_Real64);
  }
  void RegionImpl::setParameterBool(const std::string& name, Int64 index, bool value) {
    setParameter<bool>(name, index, value, NTA_BasicType_Bool);
  }

  // Only used for internal C++ testing
  void RegionImpl::setParameterHandle(const std::string& name, Int64 index, void * value) {
    // Note: We are using an integer value to hold a pointer...not a good idea.
    setParameter<UInt64>(name, index, (UInt64)value, NTA_BasicType_Handle);
  }

  
  template <typename T>
  T RegionImpl::getParameter(const std::string& name, Int64 index, NTA_BasicType type) {
    if (! region_->getSpec()->parameters.contains(name))   
      NTA_THROW << ": parameter \"" 
                << name << "\" does not exist in nodespec for " << getType();
    ParameterSpec p = region_->getSpec()->parameters.getByName(name); 
    if (p.dataType != type)
      NTA_THROW << " parameter \"" << name << "\" is of type "
                << BasicType::getName(p.dataType) << " not "
                << BasicType::getName(type)
                << " for " << getType();
    WriteBuffer wb; 
    getParameterFromBuffer(name, index, wb); 
    ReadBuffer rb(wb.getData(), wb.getSize(), false /* copy */); 
    T val; 
    int rc = rb.read(val);
    if (rc != 0)
    {
      NTA_THROW << "getParameter" << BasicType::getName(type) 
                << " -- failure to get parameter '"
                << name << "' on " << getType(); 
    } 
    return val;

  }

  template <typename T>
  void RegionImpl::setParameter(const std::string& name, Int64 index, T value, NTA_BasicType type) {
    if (! region_->getSpec()->parameters.contains(name))   
      NTA_THROW << ": parameter \"" 
                << name << "\" does not exist in nodespec for " << getType();
    ParameterSpec p = region_->getSpec()->parameters.getByName(name); 
    if (p.dataType != type)
      NTA_THROW << " parameter \"" << name << "\" is of type "
                << BasicType::getName(p.dataType) << " not "
                << BasicType::getName(type)
                << " for " << getType();

    WriteBuffer wb; 
    wb.write((T)value); 
    ReadBuffer rb(wb.getData(), wb.getSize(), false /* copy */); 
    setParameterFromBuffer(name, index, rb); 
  }




  void RegionImpl::getParameterArray(const std::string& name, Int64 index, Array & array)
  {
    WriteBuffer wb;
    getParameterFromBuffer(name, index, wb);
    ReadBuffer rb(wb.getData(), wb.getSize(), false /* copy */);
    size_t count = array.getCount();
    if (count == 0) {
      // If no space, allocate the space.
      // If the caller wants to own the buffer it must pre-allocate the space.
      count = getParameterArrayCount(name, index);
      array.allocateBuffer(count);
    }
    void *buffer = array.getBuffer();

    for (size_t i = 0; i < count; i++)
    {
      int rc;
      switch (array.getType())
      {

      case NTA_BasicType_Byte:
        rc = rb.read(((Byte*)buffer)[i]);
        break;
      case NTA_BasicType_Int32:
        rc = rb.read(((Int32*)buffer)[i]);
        break;
      case NTA_BasicType_UInt32:
        rc = rb.read(((UInt32*)buffer)[i]);
        break;
      case NTA_BasicType_Int64:
        rc = rb.read(((Int64*)buffer)[i]);
        break;
      case NTA_BasicType_UInt64:
        rc = rb.read(((UInt64*)buffer)[i]);
        break;
      case NTA_BasicType_Real32:
        rc = rb.read(((Real32*)buffer)[i]);
        break;
      case NTA_BasicType_Real64:
        rc = rb.read(((Real64*)buffer)[i]);
        break;
      default:
        NTA_THROW << "Unsupported basic type " << BasicType::getName(array.getType())
                  << " in getParameterArray for parameter " << name;
        break;
      }

      if (rc != 0)
      {
        NTA_THROW << "getParameterArray -- failure to get parameter '"
                  << name << "' on node of type " << getType();
      }
    }
    return;
  }


  void RegionImpl::setParameterArray(const std::string& name, Int64 index,const Array & array)
  {
    WriteBuffer wb;
    size_t count = array.getCount();
    void *buffer = array.getBuffer();
    for (size_t i = 0; i < count; i++)
    {
      int rc;
      switch (array.getType())
      {

      case NTA_BasicType_Byte:
        rc = wb.write(((Byte*)buffer)[i]);
        break;
      case NTA_BasicType_Int32:
        rc = wb.write(((Int32*)buffer)[i]);
        break;
      case NTA_BasicType_UInt32:
        rc = wb.write(((UInt32*)buffer)[i]);
        break;
      case NTA_BasicType_Int64:
        rc = wb.write(((Int64*)buffer)[i]);
        break;
      case NTA_BasicType_UInt64:
        rc = wb.write(((UInt64*)buffer)[i]);
        break;
      case NTA_BasicType_Real32:
        rc = wb.write(((Real32*)buffer)[i]);
        break;
      case NTA_BasicType_Real64:
        rc = wb.write(((Real64*)buffer)[i]);
        break;
      default:
        NTA_THROW << "Unsupported basic type " << BasicType::getName(array.getType())
                  << " in setParameterArray for parameter " << name;
        break;
      }

      if (rc != 0)
      {
        NTA_THROW << "setParameterArray - failure to set parameter '" << name <<
          "' on node of type " << getType();
      }
    }

    ReadBuffer rb(wb.getData(), wb.getSize(), false);
    setParameterFromBuffer(name, index, rb);
  }


  void RegionImpl::setParameterString(const std::string& name, Int64 index, const std::string& s)
  {
    ReadBuffer rb(s.c_str(), s.size(), false);
    setParameterFromBuffer(name, index, rb);
  }

  std::string RegionImpl::getParameterString(const std::string& name, Int64 index)
  {
    WriteBuffer wb;
    getParameterFromBuffer(name, index, wb);
    return std::string(wb.getData(), wb.getSize());
  }


  // Must be overridden by subclasses
  bool RegionImpl::isParameterShared(const std::string& name)
  {
    NTA_THROW << "RegionImpl::isParameterShared was not overridden in node type " << getType();
  }

  // To be compatable with existing Version 1 RegionImpl subclasses
  void RegionImpl::getParameterFromBuffer(const std::string& name,
                               Int64 index,
                               IWriteBuffer& value) 
  {
    NTA_THROW << "RegionImpl::getParameterFromBuffer  -- unknown name " << name;
  }

  // To be compatable with existing Version 1 RegionImpl subclasses
  void RegionImpl::setParameterFromBuffer(const std::string& name,
                            Int64 index,
                            IReadBuffer& value)
  {
    NTA_THROW << "RegionImpl::setParameterFromBuffer  -- unknown name " << name;
  }



  size_t RegionImpl::getParameterArrayCount(const std::string& name, Int64 index)
  {
    // Default implementation for RegionImpls with no array parameters
    // that have a dynamic length.
    //std::map<std::string, ParameterSpec*>::iterator i = nodespec_->parameters.find(name);
    //if (i == nodespec_->parameters.end())


    if (!region_->getSpec()->parameters.contains(name))
    {
      NTA_THROW << "getParameterArrayCount -- no parameter named '"
                << name << "' in node of type " << getType();
    }
    size_t count = region_->getSpec()->parameters.getByName(name).count;
    if (count == 0)
    {
      NTA_THROW << "Internal Error -- unknown Array parameter element count for "
                << "node type '" << getType() << "' parameter '" << name << "'. The RegionImpl "
                << "implementation should override this method.";
    }

    return count;
  }


  // Provide data access for subclasses

  Input* RegionImpl::getInput(const std::string& name) const
  {
    return region_->getInput(name);
  }

  Output* RegionImpl::getOutput(const std::string& name) const
  {
    return region_->getOutput(name);
  }


} // namespace
