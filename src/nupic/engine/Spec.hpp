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
Definition of Spec data structures
*/

#ifndef NTA_SPEC_HPP
#define NTA_SPEC_HPP

#include <map>
#include <nupic/types/Serializable.hpp>
#include <nupic/ntypes/Collection.hpp>
#include <nupic/types/Types.hpp>
#include <string>

namespace nupic {
class InputSpec : public Serializable {
public:
  InputSpec() {}
  InputSpec(std::string description,
            NTA_BasicType dataType,
            UInt32 count = 0,
            bool required = false,
            bool regionLevel = true,
            bool isDefaultInput = false);
    bool operator==(const InputSpec &other) const;
    inline bool operator!=(const InputSpec &other) const {
    return !operator==(other);
  }
  std::string description;   // description of input
	
  NTA_BasicType dataType;    // declare type of input

  // width of buffer if fixed. 0 means variable.
  // If non-zero positive value it means this region was developed
	// to accept a fixed sized 1D array only.
  UInt32 count;             
	
  bool required;             // true if input must be connected.
	
  bool regionLevel;          // if true, this means this input can propagate its 
                             // dimensions to/from the region's dimensions.
	
  bool isDefaultInput;       // if True, assume this if input name not given 
	                           // in functions involving inputs of a region.

        
  template<class Archive>
  void save_ar(Archive & ar) const {
    ar(cereal::make_nvp("description",    description), 
       cereal::make_nvp("dataType",       dataType), 
       cereal::make_nvp("count",          count), 
       cereal::make_nvp("required",       required), 
       cereal::make_nvp("regionLevel",    regionLevel), 
       cereal::make_nvp("isDefaultInput", isDefaultInput));
  }
  template<class Archive>
  void load_ar(Archive & ar) {
    ar(cereal::make_nvp("description",    description), 
       cereal::make_nvp("dataType",       dataType), 
       cereal::make_nvp("count",          count), 
       cereal::make_nvp("required",       required), 
       cereal::make_nvp("regionLevel",    regionLevel), 
       cereal::make_nvp("isDefaultInput", isDefaultInput));
  }
  void save(std::ostream &stream) const override { };  // will be removed later
  void load(std::istream &stream) override { };

};
std::ostream &operator<<(std::ostream &f, const InputSpec &s);
std::istream &operator>>(std::istream &f, InputSpec &s);


class OutputSpec : public Serializable  {
public:
  OutputSpec() {}
  OutputSpec(std::string description,
             const NTA_BasicType dataType,
             size_t count = 0,              // set size of buffer, 0 means unknown size.
             bool regionLevel = true,
             bool isDefaultOutput = false);
    bool operator==(const OutputSpec &other) const;
    inline bool operator!=(const OutputSpec &other) const {
    return !operator==(other);
  }
  std::string description;   // description of output
	
	NTA_BasicType dataType;    // The type of the output buffer.

  size_t count;              // Size, in number of elements. If size is fixed.  
	                           // If non-zero value it means this region 
														 // was developed to output a fixed sized 1D array only.
                             // if 0, call askImplForOutputDimensions() to get dimensions.

  bool regionLevel;          // If true, this output is can get its dimensions from
                             // the region dimensions.

  bool isDefaultOutput;      // if true, use this output for region if output name not given
	                           // in functions involving outputs on a region.
        
  template<class Archive>
  void save_ar(Archive & ar) const {
    ar(cereal::make_nvp("description",    description), 
       cereal::make_nvp("dataType",       dataType), 
       cereal::make_nvp("count",          count), 
       cereal::make_nvp("regionLevel",    regionLevel), 
       cereal::make_nvp("isDefaultOutput", isDefaultOutput));
  }
  template<class Archive>
  void load_ar(Archive & ar) {
    ar(cereal::make_nvp("description",    description), 
       cereal::make_nvp("dataType",       dataType), 
       cereal::make_nvp("count",          count), 
       cereal::make_nvp("regionLevel",    regionLevel), 
       cereal::make_nvp("isDefaultOutput", isDefaultOutput));
  }

  void save(std::ostream &stream) const override { };  // will be removed later
  void load(std::istream &stream) override { };
};
std::ostream &operator<<(std::ostream &f, const OutputSpec &s);
std::istream &operator>>(std::istream &f, OutputSpec &s);

class CommandSpec : public Serializable  {
public:
  CommandSpec() {}
  CommandSpec(std::string description);
  bool operator==(const CommandSpec &other) const;
  inline bool operator!=(const CommandSpec &other) const {
    return !operator==(other);
  }
  std::string description;

  template<class Archive>
  void save_ar(Archive & ar) const {
    ar(cereal::make_nvp("description",    description));
  }
  template<class Archive>
  void load_ar(Archive & ar) {
    ar(cereal::make_nvp("description",    description));
  }
  void save(std::ostream &stream) const override { };  // will be removed later
  void load(std::istream &stream) override { };
};
std::ostream &operator<<(std::ostream &f, const CommandSpec &s);
std::istream &operator>>(std::istream &f, CommandSpec &s);

class ParameterSpec : public Serializable  {
public:
  typedef enum { CreateAccess, ReadOnlyAccess, ReadWriteAccess } AccessMode;

  ParameterSpec() {}
  /**
   * @param defaultValue -- a JSON-encoded value
   */
  ParameterSpec(std::string description, NTA_BasicType dataType, size_t count,
                std::string constraints, std::string defaultValue,
                AccessMode accessMode);
  bool operator==(const ParameterSpec &other) const;
  inline bool operator!=(const ParameterSpec &other) const {
    return !operator==(other);
  }
  std::string description;

  // [open: current basic types are bytes/{u}int16/32/64, real32/64, BytePtr. Is
  // this the right list? Should we have std::string, jsonstd::string?]
  NTA_BasicType dataType;
  // 1 = scalar; > 1 = array o fixed sized; 0 = array of unknown size
  // TODO: should be size_t? Serialization issues?
  size_t count;
  std::string constraints;
  std::string defaultValue; // JSON representation; empty std::string means
                            // parameter is required
  AccessMode accessMode;

  template<class Archive>
  void save_ar(Archive & ar) const {
    ar(cereal::make_nvp("description",    description), 
       cereal::make_nvp("dataType",       dataType), 
       cereal::make_nvp("count",          count), 
       cereal::make_nvp("constraints",    constraints), 
       cereal::make_nvp("defaultValue",   defaultValue),
       cereal::make_nvp("accessMode",     accessMode));
  }
  template<class Archive>
  void load_ar(Archive & ar) {
    ar(cereal::make_nvp("description",    description), 
       cereal::make_nvp("dataType",       dataType), 
       cereal::make_nvp("count",          count), 
       cereal::make_nvp("constraints",    constraints), 
       cereal::make_nvp("defaultValue",   defaultValue),
       cereal::make_nvp("accessMode",     accessMode));
  }
  void save(std::ostream &stream) const override { };  // will be removed later
  void load(std::istream &stream) override { };
};
std::ostream &operator<<(std::ostream &f, const ParameterSpec &s);
std::istream &operator>>(std::istream &f, ParameterSpec &s);

class Spec  : public Serializable {
public:
  // Constructor
  Spec();

  // Return a printable string with Spec information
  // TODO: should this be in the base API or layered? In the API right
  // now since we do not build layered libraries.
  std::string toString() const;
  bool operator==(const Spec &other) const;
  inline bool operator!=(const Spec &other) const { return !operator==(other); }
  // Some RegionImpls support only a single node in a region.
  // Such regions always have dimension [1]
  bool singleNodeOnly;

  // Description of the node as a whole
  std::string description;

  // containers for the components of the spec.
  Collection<InputSpec> inputs;
  Collection<OutputSpec> outputs;
  Collection<CommandSpec> commands;
  Collection<ParameterSpec> parameters;


  std::string getDefaultOutputName() const;
  std::string getDefaultInputName() const;

  // a value that applys to the count field in inputs, outputs, parameters.
  // It means that the field is an array and its size is not fixed.
  static const int VARIABLE = 0; 

  // a value that applys to the count field in inputs, outputs, parameters.
  // It means that the field not an array and has a single scaler value.
  static const int SCALER = 1; 

      
  template<class Archive>
  void save_ar(Archive & ar) const {
    ar(cereal::make_nvp("singleNodeOnly", singleNodeOnly), 
       cereal::make_nvp("description",    description), 
       cereal::make_nvp("inputs",         inputs), 
       cereal::make_nvp("outputs",        outputs), 
       cereal::make_nvp("commands",       commands), 
       cereal::make_nvp("parameters",     parameters));
  }
  template<class Archive>
  void load_ar(Archive & ar) {
    ar(cereal::make_nvp("singleNodeOnly", singleNodeOnly), 
       cereal::make_nvp("description",    description), 
       cereal::make_nvp("inputs",         inputs), 
       cereal::make_nvp("outputs",        outputs), 
       cereal::make_nvp("commands",       commands), 
       cereal::make_nvp("parameters",     parameters));
  }
  void save(std::ostream &stream) const override { };  // will be removed later
  void load(std::istream &stream) override { };

};
std::ostream &operator<<(std::ostream &f, const Spec &s);
std::istream &operator>>(std::istream &f, Spec &s);

} // namespace nupic

#endif // NTA_SPEC_HPP
