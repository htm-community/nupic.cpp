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
 * Definitions for the Scalar class
 *
 * A Scalar object is an instance of an NTA_BasicType -- essentially a union
 * It is used internally in the conversion of YAML strings to C++ objects.
 */

#ifndef NTA_SCALAR_HPP
#define NTA_SCALAR_HPP

#include <nupic/types/Serializable.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/utils/Log.hpp> // temporary, while implementation is in hpp
#include <string>

namespace nupic {
class Scalar : public Serializable {
public:
  Scalar(NTA_BasicType theTypeParam);

  NTA_BasicType getType();

  template <typename T> T getValue() const;

  union {
    Handle handle;
    Byte byte;
    Int16 int16;
    UInt16 uint16;
    Int32 int32;
    UInt32 uint32;
    Int64 int64;
    UInt64 uint64;
    Real32 real32;
    Real64 real64;
    bool boolean;
  } value;

      
  template<class Archive>
  void save_ar(Archive & ar) const {
      switch (theType_) {
      case NTA_BasicType_Byte:   ar(value.byte);    break;
	    case NTA_BasicType_Int16:  ar(value.int16);   break;
	    case NTA_BasicType_UInt16: ar(value.uint16);  break;
	    case NTA_BasicType_Int32:  ar(value.int32);   break;
	    case NTA_BasicType_UInt32: ar(value.uint32);  break;
	    case NTA_BasicType_Int64:  ar(value.int64);   break;
	    case NTA_BasicType_UInt64: ar(value.uint64);  break;
	    case NTA_BasicType_Real32: ar(value.real32);  break;
	    case NTA_BasicType_Real64: ar(value.real64);  break;
	    case NTA_BasicType_Bool:   ar(value.boolean); break;
	    default:
	      NTA_THROW << "Unexpected Element Type: " << theType_;
	      break;
	    }
  }
  template<class Archive>
  void load_ar(Archive & ar) {
      switch (theType_) {
      case NTA_BasicType_Byte:   ar(value.byte);    break;
	    case NTA_BasicType_Int16:  ar(value.int16);   break;
	    case NTA_BasicType_UInt16: ar(value.uint16);  break;
	    case NTA_BasicType_Int32:  ar(value.int32);   break;
	    case NTA_BasicType_UInt32: ar(value.uint32);  break;
	    case NTA_BasicType_Int64:  ar(value.int64);   break;
	    case NTA_BasicType_UInt64: ar(value.uint64);  break;
	    case NTA_BasicType_Real32: ar(value.real32);  break;
	    case NTA_BasicType_Real64: ar(value.real64);  break;
	    case NTA_BasicType_Bool:   ar(value.boolean); break;
	    default:
	      NTA_THROW << "Unexpected Element Type: " << theType_;
	      break;
	    }
  }

  void save(std::ostream &stream) const override { };  // will be removed later
  void load(std::istream &stream) override { };


private:
  NTA_BasicType theType_;
};

} // namespace nupic

#endif // NTA_SCALAR_HPP
