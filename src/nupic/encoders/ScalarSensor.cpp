/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2016, Numenta, Inc.  Unless you have an agreement
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
 * Implementation of the ScalarSensor
 */

#include <string>

#include <nupic/encoders/ScalarSensor.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/utils/Log.hpp>
#include <nupic/ntypes/ObjectModel.hpp> // IWrite/ReadBuffer
#include <nupic/ntypes/Array.hpp>
#include <nupic/ntypes/Value.hpp>
#include <nupic/ntypes/BundleIO.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Input.hpp>

namespace nupic
{
  ScalarSensor::ScalarSensor(const ValueMap& params, Region *region)
    : RegionImpl(region)
  {
    n_ = params.getScalarT<UInt32>("n");
    w_ = params.getScalarT<UInt32>("w");
    resolution_ = params.getScalarT<Real64>("resolution");
    radius_ = params.getScalarT<Real64>("radius");
    minValue_ = params.getScalarT<Real64>("minValue");
    maxValue_ = params.getScalarT<Real64>("maxValue");
    periodic_ = params.getScalarT<bool>("periodic");
    clipInput_ = params.getScalarT<bool>("clipInput");
    if (periodic_)
    {
      encoder_ = new PeriodicScalarEncoder(w_, minValue_, maxValue_, n_, radius_,
                                           resolution_);
    }
    else
    {
      encoder_ = new ScalarEncoder(w_, minValue_, maxValue_, n_, radius_, resolution_,
                                   clipInput_);
    }

    sensedValue_ = params.getScalarT<Real64>("sensedValue");
  }

  ScalarSensor::ScalarSensor(BundleIO& bundle, Region* region) :
    RegionImpl(region)
  {
    deserialize(bundle);
  }

  ScalarSensor::~ScalarSensor()
  {
    delete encoder_;
  }

  void ScalarSensor::compute()
  {
    Real32* array = (Real32*)encodedOutput_->getData().getBuffer();
    const Int32 iBucket = encoder_->encodeIntoArray(sensedValue_, array);
    ((Int32*)bucketOutput_->getData().getBuffer())[0] = iBucket;
  }

  /* static */ Spec*
  ScalarSensor::createSpec()
  {
    auto ns = new Spec;

    ns->singleNodeOnly = true;

    /* ----- parameters ----- */
    ns->parameters.add(
      "sensedValue",
      ParameterSpec(
        "Scalar input",
        NTA_BasicType_Real64,
        1, // elementCount
        "", // constraints
        "-1", // defaultValue
        ParameterSpec::ReadWriteAccess));

    ns->parameters.add(
      "n",
      ParameterSpec(
        "The length of the encoding",
        NTA_BasicType_UInt32,
        1, // elementCount
        "", // constraints
        "0", // defaultValue
        ParameterSpec::CreateAccess));

    ns->parameters.add(
      "w",
      ParameterSpec(
        "The number of active bits in the encoding",
        NTA_BasicType_UInt32,
        1, // elementCount
        "", // constraints
        "0", // defaultValue
        ParameterSpec::CreateAccess));

    ns->parameters.add(
      "resolution",
      ParameterSpec(
        "The resolution for the encoder",
        NTA_BasicType_Real64,
        1, // elementCount
        "", // constraints
        "0", // defaultValue
        ParameterSpec::CreateAccess));

    ns->parameters.add(
      "radius",
      ParameterSpec(
        "The radius for the encoder",
        NTA_BasicType_Real64,
        1, // elementCount
        "", // constraints
        "0", // defaultValue
        ParameterSpec::CreateAccess));

    ns->parameters.add(
      "minValue",
      ParameterSpec(
        "The minimum value for the input",
        NTA_BasicType_Real64,
        1, // elementCount
        "", // constraints
        "-1", // defaultValue
        ParameterSpec::CreateAccess));

    ns->parameters.add(
      "maxValue",
      ParameterSpec(
        "The maximum value for the input",
        NTA_BasicType_Real64,
        1, // elementCount
        "", // constraints
        "-1", // defaultValue
        ParameterSpec::CreateAccess));

    ns->parameters.add(
      "periodic",
      ParameterSpec(
        "Whether the encoder is periodic",
        NTA_BasicType_Bool,
        1, // elementCount
        "bool", // constraints
        "false", // defaultValue
        ParameterSpec::CreateAccess));

    ns->parameters.add(
      "clipInput",
      ParameterSpec(
        "Whether to clip inputs if they're outside [minValue, maxValue]",
        NTA_BasicType_Bool,
        1, // elementCount
        "bool", // constraints
        "false", // defaultValue
        ParameterSpec::CreateAccess));

    /* ----- outputs ----- */

    ns->outputs.add(
      "encoded",
      OutputSpec(
        "Encoded value",
        NTA_BasicType_Real32,
        0, // elementCount
        true, // isRegionLevel
        true // isDefaultOutput
        ));

    ns->outputs.add(
      "bucket",
      OutputSpec(
        "Bucket number for this sensedValue",
        NTA_BasicType_Int32,
        0, // elementCount
        true, // isRegionLevel
        false // isDefaultOutput
        ));

    return ns;
  }

  ////////////////////////////////////////////////////////////////////////
  //           Parameters
  ////////////////////////////////////////////////////////////////////////
  Real64 ScalarSensor::getParameterReal64(const std::string &name, Int64 index) 
  {
      if (name == "sensedValue") {
        return sensedValue_;
      }
      if (name == "resolution") {
        return resolution_;
      }
      if (name == "radius") {
        return radius_;
      }
      if (name == "minValue") {
        return minValue_;
      }
      if (name == "maxValue") {
        return maxValue_;
      }
      return this->RegionImpl::getParameterUInt32(name, index); // default
  }
  UInt32 ScalarSensor::getParameterUInt32(const std::string &name, Int64 index) 
  {
      if (name == "n") {
        return (UInt32)encoder_->getOutputWidth();
      }
      if (name == "w") {
        return w_;
      }
      return this->RegionImpl::getParameterUInt32(name, index); // default
  }
  bool ScalarSensor::getParameterBool(const std::string &name, Int64 index) 
  {
      if (name == "periodic") {
        return periodic_;
      }
      if (name == "clipInput") {
        return clipInput_;
      }
      return this->RegionImpl::getParameterBool(name, index); // default
  }

  void ScalarSensor::setParameterReal64(const std::string &name, Int64 index, Real64 value) 
  {
    if (name == "sensedValue") {
      sensedValue_ = value;
      return;
    }
    RegionImpl::setParameterReal64(name, index, value);
  }


  void
  ScalarSensor::initialize()
  {
    encodedOutput_ = getOutput("encoded");
    bucketOutput_ = getOutput("bucket");
  }

  size_t
  ScalarSensor::getNodeOutputElementCount(const std::string& outputName)
  {
    if (outputName == "encoded")
    {
      return encoder_->getOutputWidth();
    }
    else if (outputName == "bucket")
    {
      return 1;
    }
    else
    {
      NTA_THROW << "ScalarSensor::getOutputSize -- unknown output " << outputName;
    }
  }

  std::string ScalarSensor::executeCommand(const std::vector<std::string>& args, Int64 index)
  {
    NTA_THROW << "ScalarSensor::executeCommand -- commands not supported";
  }

  void ScalarSensor::serialize(BundleIO& bundle)
  {
    std::ofstream &f = bundle.getOutputStream("scaler");
    f << "ScalarSensor" << " " 
      << n_ << " " << w_ << " " << resolution_ << " " << radius_ << " "
      << minValue_ << " " << maxValue_ << " " << clipInput_ << " " << periodic_ << " "
      << sensedValue_ << " ";
    f.close();
  }


  void ScalarSensor::deserialize(BundleIO& bundle)
  {
    std::ifstream &f = bundle.getInputStream("scaler");
    std::string signatureString;
    f >> signatureString;
    if (signatureString != "ScalarSensor") {
      NTA_THROW << "Bad serialization for region '" << region_->getName()
                << "' of type ScalarSensor. Serialization file must start "
                << "with \"ScalarSensor\" but instead it starts with '"
                << signatureString << "'";
    }

    f >> n_;
    f >> w_; 
    f >> resolution_;
    f >> radius_; 
    f >> minValue_;
    f >> maxValue_;
    f >> clipInput_;
    f >> periodic_;
    f >> sensedValue_;
    f.close();

    if (periodic_) {
      encoder_ = new PeriodicScalarEncoder(w_, minValue_, maxValue_, n_,
                                           radius_, resolution_);
    } else {
      encoder_ = new ScalarEncoder(w_, minValue_, maxValue_, n_, radius_,
                                   resolution_, clipInput_);
    }
    initialize();

    // create and prepopulate the output buffers
    encodedOutput_->initialize(getNodeOutputElementCount("encoded"));
    bucketOutput_->initialize(getNodeOutputElementCount("bucket"));
    compute();
  }

} // namespace
