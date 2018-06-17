/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2018, Numenta, Inc.  Unless you have an agreement
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
 
 * Author: David Keeney, April, 2018
 * ---------------------------------------------------------------------
 */
 
/*---------------------------------------------------------------------
  * This is a test of the SPRegion module.  It does not check the SpactialPooler itself
  * but rather just the plug-in mechanisom to call the SpactialPooler.
  *
  * For those not familiar with GTest:
  *     ASSERT_TRUE(value)   -- Fatal assertion that the value is true.  Test terminates if false.
  *     ASSERT_FALSE(value)   -- Fatal assertion that the value is false. Test terminates if true.
  *     ASSERT_STREQ(str1, str2)   -- Fatal assertion that the strings are equal. Test terminates if false.
  *
  *     EXPECT_TRUE(value)   -- Nonfatal assertion that the value is true.  Test continues if false.
  *     EXPECT_FALSE(value)   -- Nonfatal assertion that the value is false. Test continues if true.
  *     EXPECT_STREQ(str1, str2)   -- Nonfatal assertion that the strings are equal. Test continues if false.
  *
  *     EXPECT_THROW(statement, exception_type) -- nonfatal exception, cought and continues.
  *---------------------------------------------------------------------
  */


#include <nupic/engine/NuPIC.hpp>
#include <nupic/engine/Network.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/engine/Input.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Link.hpp>
#include <nupic/engine/RegisteredRegionImpl.hpp>
#include <nupic/ntypes/Array.hpp>
#include <nupic/ntypes/ArrayRef.hpp>
#include <nupic/types/Exception.hpp>
#include <nupic/os/OS.hpp> // memory leak detection
#include <nupic/os/Env.hpp>
#include <nupic/os/Path.hpp>
#include <nupic/os/Timer.hpp>
#include <nupic/os/Directory.hpp>
#include <nupic/engine/YAMLUtils.hpp>
#include <nupic/regions/SPRegion.hpp>


#include <string>
#include <vector>
#include <cmath> // fabs/abs
#include <cstdlib> // exit
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <streambuf>


#include "yaml-cpp/yaml.h"
#include "gtest/gtest.h"

#define VERBOSE if(verbose)std::cerr << "[          ] "
static bool verbose = true;  // turn this on to print extra stuff for debugging the test.

// The following string should contain a valid expected Spec - manually verified. 
#define EXPECTED_SPEC_COUNT  28  // The number of parameters expected in the SPRegion Spec

using namespace nupic;
namespace testing 
{


  ::testing::AssertionResult compareRegionArrays(Region_Ptr_t region1, Region_Ptr_t region2, std::string parameter, NTA_BasicType type);
	::testing::AssertionResult captureParameters(Region_Ptr_t region1, std::map<std::string, std::string>& parameters);
	::testing::AssertionResult compareParameters(Region_Ptr_t region1, std::map<std::string, std::string>& parameters);


  // Verify that all parameters are working.
  // Assumes that the default value in the Spec is the same as the default when creating a region with default constructor.
  // Will not work if region is initialized because the SpatialPooler enforces specific ranges for some values.
  TEST(SPRegionTest, testSpecAndParameters)
  {
    Network net;

    // create an SP region with default parameters
    Region_Ptr_t region1 = net.addRegion("region1", "SPRegion", "");  // use default configuration

    // check spec cache
    const Spec* ns = region1->getSpec();
    ASSERT_TRUE(ns == region1->getSpec()) << " First and second fetches of the spec gives different addresses. It was not cached.";


    // Make sure the number of parameters matches.
    size_t expectedSpecCount = EXPECTED_SPEC_COUNT;
    size_t specCount = ns->parameters.getCount();
    ASSERT_TRUE(specCount == expectedSpecCount) << "Unexpected number of parameters. Expected=" << expectedSpecCount << ", Actual=" << specCount;

    // Look through the Spec and look for every instance of a parameter.  get/set/verify each parameter
    for (size_t i = 0; i < specCount; i++) {
      std::pair<string, ParameterSpec> p = ns->parameters.getByIndex(i);
      string name = p.first;

      VERBOSE << "Parameter \"" << name << "\"" << std::endl;
      try {
        if (p.second.count == 1) {
          switch (p.second.dataType) {

          case NTA_BasicType_UInt32:
          {
            EXPECT_THROW(region1->getParameterInt32(name), nupic::Exception); // trying wrong data type

            // check the getter.
            UInt32 v = region1->getParameterUInt32(name);
            if (!p.second.defaultValue.empty()) {
              UInt32 d = std::stoul(p.second.defaultValue, nullptr, 0);
              EXPECT_TRUE(v == d)
                << "Failure: Parameter \"" << name << "\" Actual value does not match default. Expected=" << d << ", Actual=" << v;
            }
            // check the setter.
            if (p.second.accessMode == ParameterSpec::ReadWriteAccess) {
              if (p.second.constraints == "bool") {
                UInt32 d = (v) ? 0u : 1u;
                region1->setParameterUInt32(name, d);  // change value
                UInt32 s = region1->getParameterUInt32(name);
                EXPECT_TRUE(s == d)
                  << "Parameter \"" << name << "\" Actual value does not match changed value. Expected=" << (!v) << ", Actual=" << s;
              }
              else {
                region1->setParameterUInt32(name, 0xFFFFFFFF);  // set max value
                UInt32 s = region1->getParameterUInt32(name);
                EXPECT_TRUE(s == 0xFFFFFFFF)
                  << "Failure: Parameter \"" << name << "\" Actual value does not match UInt32 max. Expected=" << 0xFFFFFFFF << ", Actual=" << s;
              }
              region1->setParameterUInt32(name, v);  // return to original value.
            }
            break;
          }

          case NTA_BasicType_Int32:
          {
            EXPECT_THROW(region1->getParameterUInt32(name), nupic::Exception);

            // check the getter.
            Int32 v = region1->getParameterInt32(name);
            if (!p.second.defaultValue.empty()) {
              Int32 d = std::stoul(p.second.defaultValue, nullptr, 0);
              EXPECT_TRUE(v == d)
                << "Failure: Parameter \"" << name << "\" Actual value does not match default. Expected=" << d << ", Actual=" << v;
            }
            // check the setter.
            if (p.second.accessMode == ParameterSpec::ReadWriteAccess) {
              region1->setParameterInt32(name, 0x7FFFFFFF);  // set max value
              Int32 s = region1->getParameterInt32(name);
              EXPECT_TRUE(s == 0x7FFFFFFF)
                << "Parameter \"" << name << "\" Actual value does not match Int32 max. Expected=" << 0x7FFFFFFF << ", Actual=" << s;
              region1->setParameterInt32(name, v);  // return to original value.
            }
            break;
          }

          case NTA_BasicType_Real32:
          {
            EXPECT_THROW(region1->getParameterUInt32(name), nupic::Exception);

            // check the getter.
            Real32 v = region1->getParameterReal32(name);
            if (!p.second.defaultValue.empty()) {
              Real32 d = std::strtof(p.second.defaultValue.c_str(), nullptr);
              EXPECT_TRUE(v == d)
                << "Parameter \"" << name << "\" Actual value does not match default. Expected=" << d << ", Actual=" << v;
            }
            // check the setter.
            if (p.second.accessMode == ParameterSpec::ReadWriteAccess) {
              region1->setParameterReal32(name, FLT_MAX);  // set max value
              Real32 s = region1->getParameterReal32(name);
              EXPECT_TRUE(s == FLT_MAX)
                << "Parameter \"" << name << "\" Actual value does not match float max. Expected=" << FLT_MAX << ", Actual=" << s;
              region1->setParameterReal32(name, v);  // return to original value.
            }
            break;
          }

          case NTA_BasicType_Bool:
          {
            EXPECT_THROW(region1->getParameterUInt32(name), nupic::Exception);

            // check the getter.
            bool v = region1->getParameterBool(name);
            if (!p.second.defaultValue.empty()) {
              bool d = (p.second.defaultValue == "true");
              EXPECT_TRUE(v == d)
                << "Parameter \"" << name << "\" Actual value does not match default. Expected=" << d << ", Actual=" << v;
            }
            // check the setter.
            if (p.second.accessMode == ParameterSpec::ReadWriteAccess) {
              region1->setParameterBool(name, !v);  // change value
              bool s = region1->getParameterBool(name);
              EXPECT_TRUE(s == !v)
                << "Parameter \"" << name << "\" Actual value does not match changed value. Expected=" << (!v) << ", Actual=" << s;
              region1->setParameterBool(name, v);  // return to original value.
            }
            break;
          }

          default:
            FAIL() << "Parameter \"" << name << "\" Invalid data type.  found " << p.second.dataType;
            break;
          } // end switch
        }
        else {



          // Array types
          switch (p.second.dataType) {
          case NTA_BasicType_Byte:
          {
            std::string v = region1->getParameterString(name);
            if (!p.second.defaultValue.empty()) {
              std::string d = p.second.defaultValue;
              EXPECT_STREQ(v.c_str(), d.c_str())
                << "Parameter \"" << name << "\" Actual value does not match default. Expected=" << d << ", Actual=" << v;
            }
            // check the setter.
            if (p.second.accessMode == ParameterSpec::ReadWriteAccess) {
              region1->setParameterString(name, "XXX");  // change value
              std::string s = region1->getParameterString(name);
              EXPECT_STREQ(s.c_str(), "XXX")
                << "Parameter \"" << name << "\" Actual value does not match changed value. Expected=" << "XXX" << ", Actual=" << s;
              region1->setParameterString(name, v);  // return to original value.
            }
            break;

          }
          case NTA_BasicType_UInt32:
          case NTA_BasicType_Real32:
            break;

          default:
            ASSERT_TRUE(false) << "Parameter \"" << name << "\" Invalid data type.  found " << BasicType::getName(p.second.dataType);
            break;
          } // end switch
        }
      }
      catch (nupic::Exception& ex) {
        FAIL() << "Exception while processing parameter "
          << name << ":  " << ex.getFilename() << "(" << ex.getLineNumber() << ") " << ex.getMessage();
      }
      catch (std::exception& e) {
        FAIL() << "Exception while processing parameter " << name << ":  " << e.what() << "" << std::endl;
      }
    } // end loop


      // --- input/output access for region (C++ SPRegion) ---
    ASSERT_TRUE(ns->outputs.getByName("bottomUpOut").dataType == NTA_BasicType_Real32)
      << "Output type for \"bottomUpOut\" Invalid data type. Should be Real32, found "
      << ns->outputs.getByName("bottomUpOut").dataType;
    ASSERT_TRUE(ns->inputs.getByName("bottomUpIn").dataType == NTA_BasicType_Real32)
      << "Input type for \"bottomUpIn\" Invalid data type. Should be Real32, found " << ns->inputs.getByName("bottomUpIn").dataType;


  }



	TEST(SPRegionTest, initialization_with_builtin_impl)
	{
	  VERBOSE << "Creating network..." << std::endl;
	  Network net;

	  size_t regionCntBefore = net.getRegions().getCount();

	  VERBOSE << "Adding a built-in SPRegion region..." << std::endl;
	  Region_Ptr_t region1 = net.addRegion("region1", "SPRegion", "");
	  size_t regionCntAfter = net.getRegions().getCount();
	  ASSERT_TRUE(regionCntBefore + 1 == regionCntAfter) << " Expected number of regions to increase by one.  ";
	  ASSERT_TRUE(region1->getType() == "SPRegion") << " Expected type for region1 to be \"SPRegion\" but type is: " << region1->getType();


    EXPECT_THROW(region1->getOutputData("doesnotexist"), std::exception);
    EXPECT_THROW(region1->getInputData("doesnotexist"), std::exception);


	  // run and compute() should fail because network has not been initialized
	  EXPECT_THROW(net.run(1), std::exception);
	  EXPECT_THROW(region1->compute(), std::exception);

    EXPECT_THROW(net.initialize(), std::exception) << "Should be an exception saying Input dimentions not set.";
	}


  TEST(SPRegionTest, initialization_with_custom_impl)
  {
    VERBOSE << "Creating network..." << std::endl;
    Network net;

    size_t regionCntBefore = net.getRegions().getCount();

    // make sure the custom region registration works for CPP.
    // We will just use the same SPRegion class but it could be a subclass or some different custom class.
    // While we are at it, make sure we can initialize the dimensions and parameters from here too.
    // The parameter names and data types must match those of the spec.
    // Explicit parameters:  (Yaml format...but since YAML is a superset of JSON, you can use JSON format as well)
    std::string nodeParams = "{inputWidth: 946, columnCount: 2048, potentialRadius: 16}";

    VERBOSE << "Adding a custom-built SPRegion region..." << std::endl;
    net.registerCPPRegion("SPRegionCustom", new RegisteredRegionImpl<SPRegion>());
    Region_Ptr_t region2 = net.addRegion("region2", "SPRegionCustom", nodeParams);
    size_t regionCntAfter = net.getRegions().getCount();
    ASSERT_TRUE(regionCntBefore + 1 == regionCntAfter) << "  Expected number of regions to increase by one.  ";
    ASSERT_TRUE(region2->getType() == "SPRegionCustom") << " Expected type for region2 to be \"SPRegionCustom\" but type is: " << region2->getType();

    EXPECT_EQ(region2->getParameterUInt32("inputWidth"), 946);
    EXPECT_EQ(region2->getParameterUInt32("columnCount"), 2048);
    EXPECT_EQ(region2->getParameterUInt32("potentialRadius"), 16);

    // compute() should fail because network has not been initialized
    EXPECT_THROW(net.run(1), std::exception);
    EXPECT_THROW(region2->compute(), std::exception);

    EXPECT_THROW(net.initialize(), std::exception) << "Exception should say region2 has unspecified dimensions. ";
  }




	TEST(SPRegionTest, testLinking)
	{
    // This is a minimal end-to-end test containing an SPRegion region.
    // To make sure we can feed data from some other region to our SPRegion
    // this test will hook up the VectorFileSensor to our SPRegion and then
    // connect our SPRegion to a VectorFileEffector to capture the results.
    //
    std::string test_input_file = "TestOutputDir/SPRegionTestInput.csv";
    std::string test_output_file = "TestOutputDir/SPRegionTestOutput.csv";


    // make a place to put test data.
    if (!Directory::exists("TestOutputDir")) Directory::create("TestOutputDir", false, true); 
    if (Path::exists(test_input_file)) Path::remove(test_input_file);
    if (Path::exists(test_output_file)) Path::remove(test_output_file);

    // Create a csv file to use as input.
    // The SDR data we will feed it will be a matrix with 1's on the diagonal
    // and we will feed it one row at a time, for 10 rows.
    size_t dataWidth = 10;
    size_t dataRows = 10;
    std::ofstream  f(test_input_file.c_str());
    for (size_t i = 0; i < 10; i++) {
      for (size_t j = 0; j < dataWidth; j++) {
        if ((j % dataRows) == i) f << "1.0,";
        else f << "0.0,";
      }
      f << std::endl;
    }
    f.close();

    VERBOSE << "Setup Network; add 3 regions and 2 links." << std::endl;
	  Network net;

    // Explicit parameters:  (Yaml format...but since YAML is a superset of JSON, you can use JSON format as well)

	  Region_Ptr_t region1 = net.addRegion("region1", "VectorFileSensor", "{activeOutputCount: "+std::to_string(dataWidth) +"}");
    Region_Ptr_t region2 = net.addRegion("region2", "SPRegion", "{columnCount: 100}");
    Region_Ptr_t region3 = net.addRegion("region3", "VectorFileEffector", "{outputFile: '"+ test_output_file + "'}");


    net.link("region1", "region2", "UniformLink", "", "dataOut", "bottomUpIn");
    net.link("region2", "region3", "UniformLink", "", "bottomUpOut", "dataIn");

    VERBOSE << "Load Data." << std::endl;
    region1->executeCommand({ "loadFile", test_input_file });


    VERBOSE << "Initialize." << std::endl;
    net.initialize();



	  // check actual dimensions
    ASSERT_EQ(region2->getParameterUInt32("columnCount"), 100);
    ASSERT_EQ(region2->getParameterUInt32("inputWidth"), (UInt32)dataWidth);

    VERBOSE << "Execute once." << std::endl;
    net.run(1);

	  VERBOSE << "Checking data after first iteration..." << std::endl;
    VERBOSE << "  VectorFileSensor Output" << std::endl;
    Array r1OutputArray = region1->getOutputData("dataOut");
    EXPECT_EQ(r1OutputArray.getCount(), dataWidth);
    EXPECT_TRUE(r1OutputArray.getType() == NTA_BasicType_Real32);
    Real32 *buffer1 = (Real32*) r1OutputArray.getBuffer();
	  //for (size_t i = 0; i < r1OutputArray.getCount(); i++)
	  //{
		//VERBOSE << "  [" << i << "]=    " << buffer1[i] << "" << std::endl;
	  //}

    VERBOSE << "  SPRegion input" << std::endl;
    ArrayRef r2InputArray = region2->getInputData("bottomUpIn");
	  ASSERT_TRUE (r1OutputArray.getCount() == r2InputArray.getCount()) 
		<< "Buffer length different. Output from VectorFileSensor is " << r1OutputArray.getCount() << ", input to SPRegion is " << r2InputArray.getCount();
    EXPECT_TRUE(r2InputArray.getType() == NTA_BasicType_Real32);
		Real32 *buffer2 = (Real32*)r2InputArray.getBuffer(); 
		for (size_t i = 0; i < r2InputArray.getCount(); i++)
		{
		  //VERBOSE << "  [" << i << "]=    " << buffer2[i] << "" << std::endl;
		  ASSERT_TRUE(buffer2[i] == buffer1[i]) 
			<< " Buffer content different. Element " << i << " of Output from encoder is " << buffer1[i] << ", input to SPRegion is " << buffer2[i];
      ASSERT_TRUE(buffer2[i] == 1.0f || buffer2[i] == 0.0f) << " Value[" << i << "] is not a 0 or 1." << std::endl;
		}

	  // execute SPRegion several more times and check that it has output.
    VERBOSE << "Execute 9 times." << std::endl;
    net.run(9);

    VERBOSE << "Checking Output Data." << std::endl;
    VERBOSE << "  SPRegion output" << std::endl;
    UInt32 columnCount = region2->getParameterUInt32("columnCount");
	  Array r2OutputArray = region2->getOutputData("bottomUpOut");
	  ASSERT_TRUE(r2OutputArray.getCount() == columnCount)
		 << "Buffer length different. Output from SPRegion is " << r2OutputArray.getCount() << ", should be " << columnCount;
    Real32 *buffer3 = (Real32*)r2OutputArray.getBuffer();
    for (size_t i = 0; i < r2OutputArray.getCount(); i++)
    {
      //VERBOSE << "  [" << i << "]=    " << buffer3[i] << "" << std::endl;
      ASSERT_TRUE(buffer3[i] == 0.0f || buffer3[i] == 1.0f)
        << " Element " << i << " of Output from SPRegion is not 0.0 or 1.0; it is " << buffer3[i];
    }


    VERBOSE << "  VectorFileEffector input" << std::endl;
    ArrayRef r3InputArray = region3->getInputData("dataIn");
    ASSERT_TRUE(r3InputArray.getCount() == columnCount);
    Real32 *buffer4 = (Real32*)r3InputArray.getBuffer();
    for (size_t i = 0; i < r3InputArray.getCount(); i++)
    {
      //VERBOSE << "  [" << i << "]=    " << buffer4[i] << "" << std::endl;
      ASSERT_TRUE(buffer3[i] == buffer4[i])
        << " Buffer content different. Element[" << i << "] from SPRegion out is " << buffer3[i] << ", input to VectorFileEffector is " << buffer4[i];
    }

    // cleanup
    region3->executeCommand({ "closeFile" });

	}


	TEST(SPRegionTest, testSerialization)
	{
	  // use default parameters the first time
	  Network* net1 = new Network();
	  Network* net2 = nullptr;
	  Network* net3 = nullptr;

	  try {

		  VERBOSE << "Setup first network and save it" << std::endl;
      Region_Ptr_t n1region1 = net1->addRegion("region1", "ScalarSensor", "{n: 100,w: 10,minValue: 0,maxValue: 10}");
      Region_Ptr_t n1region2 = net1->addRegion("region2", "SPRegion", "{columnCount: 2048}");
      net1->link("region1", "region2", "UniformLink", "", "encoded", "bottomUpIn");
      net1->initialize();

      n1region1->setParameterReal64("sensedValue", 5.5);
      n1region1->prepareInputs();
		  n1region1->compute();

      n1region2->prepareInputs();
      n1region2->compute();

      // take a snapshot of everything in SPRegion at this point
      std::map<std::string, std::string> parameterMap;
      EXPECT_TRUE(captureParameters(n1region2, parameterMap)) << "Capturing parameters before save.";

      Directory::removeTree("TestOutputDir", true);
		  net1->save("TestOutputDir/spRegionTest.nta");

		  VERBOSE << "Restore into a second network and compare." << std::endl;
		  net2 = new Network("TestOutputDir/spRegionTest.nta");

		  Region_Ptr_t n2region2 = net2->getRegions().getByName("region2");
		  ASSERT_TRUE (n2region2->getType() == "SPRegion") 
		    << " Restored SPRegion region does not have the right type.  Expected SPRegion, found " << n2region2->getType();

      EXPECT_TRUE(compareParameters(n2region2, parameterMap)) << "Conflict when comparing SPRegion parameters after restore with before save.";

      EXPECT_TRUE(compareRegionArrays(n1region2, n2region2, "spInputNonZeros", NTA_BasicType_UInt32)) << " comparing NZ in arrays after restore with before save.";
      EXPECT_TRUE(compareRegionArrays(n1region2, n2region2, "spOutputNonZeros", NTA_BasicType_UInt32)) << " comparing NZ out arrays after restore with before save.";


		  // can we continue with execution?  See if we get any exceptions.
      n1region1->setParameterReal64("sensedValue", 5.5);
      n1region1->prepareInputs();
      n1region1->compute();

      n2region2->prepareInputs();
      n2region2->compute();

		  // Change some parameters and see if they are retained after a restore.
      n2region2->setParameterBool("globalInhibition", true);
      n2region2->setParameterUInt32("numActiveColumnsPerInhArea", 40);
      n2region2->setParameterReal32("potentialPct", 0.85f);
      n2region2->setParameterReal32("synPermConnected", 0.1f);
      n2region2->setParameterReal32("synPermActiveInc", 0.04f);
      n2region2->setParameterReal32("synPermInactiveDec", 0.005f);
      n2region2->setParameterReal32("boostStrength", 3.0f);
      n2region2->compute();

      parameterMap.clear();
      EXPECT_TRUE(captureParameters(n2region2, parameterMap)) << "Capturing parameters before second save.";
		  net2->save("spRegionTest.nta");

		  VERBOSE << "Restore into a third network and compare changed parameters." << std::endl;
		  net3 = new Network("spRegionTest.nta");
		  Region_Ptr_t n3region2 = net3->getRegions().getByName("region2");
      EXPECT_TRUE(n3region2->getType() == "SPRegion")
          << "Failure: Restored region does not have the right type. "
              " Expected \"SPRegion\", found \""
          << n3region2->getType() << "\".";

      EXPECT_TRUE(compareParameters(n3region2, parameterMap))
          << "Comparing parameters after second restore with before save.";


	  }
	  catch (nupic::Exception& ex) {
		  FAIL() << "Failure: Exception: " << ex.getFilename() << "(" << ex.getLineNumber() << ") " << ex.getMessage() << "" << std::endl;
	  }
	  catch (std::exception& e) {
		  FAIL() << "Failure: Exception: " << e.what() << "" << std::endl;
	  }


    // cleanup
    if (net1 != nullptr) {
      delete net1;
    }
	  if (net2 != nullptr) { delete net2; }
	  if (net3 != nullptr) { delete net3; }
    Directory::removeTree("TestOutputDir", true);

	}


	// a utility function to compare two parameter arrays
  ::testing::AssertionResult compareRegionArrays(Region_Ptr_t region1, Region_Ptr_t region2, std::string parameter, NTA_BasicType type)
	{
	  UInt32 * buf1;
	  UInt32 * buf2;
	  Real32 * buf3;
	  Real32 * buf4;
	  Array array1(type);
	  Array array2(type);
	  region1->getParameterArray(parameter, array1);
	  region2->getParameterArray(parameter, array2);

    if (type != array1.getType())
      return ::testing::AssertionFailure()  << "Failure: Original Array for parameter '" << parameter  << "' is not the expected type.";
    if (type != array2.getType())
      return ::testing::AssertionFailure()  << "Failure: Restored Array for parameter '" << parameter << "' is not the expected type.";

	  size_t len1 = array1.getCount();
	  if (len1 != array2.getCount()) {
      return ::testing::AssertionFailure() << "Failure: Arrays for parameter '" << parameter << "' are not the same length after restore.";
	  }
	  switch (type)
	  {
	  case NTA_BasicType_UInt32:
		buf1 = (UInt32*)array1.getBuffer();
		buf2 = (UInt32*)array2.getBuffer();
		for (int i = 0; i < len1; i++) {
		  if (buf1[i] != buf2[i]) {
        return ::testing::AssertionFailure() << "Failure: Array element for parameter '" << parameter << "[" << i << "]' is not the same after restore.";
		  }
		}
		break;

	  case NTA_BasicType_Real32:
		buf3 = (Real32*)array1.getBuffer();
		buf4 = (Real32*)array2.getBuffer();
		for (int i = 0; i < len1; i++) {
		  if (buf3[i] != buf4[i]) {
        return ::testing::AssertionFailure() << "Failure: Array element for parameter '" << parameter << "[" << i << "]' is not the same after restore.";
		  }
		}
		break;
	  } // end switch
    return ::testing::AssertionSuccess();
  }

	// uses the Spec to capture the non-array parameters and write them into the provided map.
	::testing::AssertionResult captureParameters(Region_Ptr_t region, std::map<std::string, std::string>& parameters)
	{
    parameters.clear();
	  const Spec* ns = region->getSpec();
	  size_t specCount = ns->parameters.getCount();
	  // Look through the Spec and look for every instance of a parameter.  
    // get each parameter, convert to a string and store in the parameters map.
    for (size_t i = 0; i < specCount; i++) {
      std::pair<string, ParameterSpec> p = ns->parameters.getByIndex(i);
      string name = p.first;

      try {
        if (p.second.count == 1) {
          switch (p.second.dataType) {
          case NTA_BasicType_Bool: { parameters[name] = (region->getParameterBool(name) ? "1" : "0"); break;  }
          case NTA_BasicType_UInt32: { parameters[name] = to_string(region->getParameterUInt32(name)); break; }
          case NTA_BasicType_Int32: { parameters[name] = to_string(region->getParameterInt32(name)); break; }
          case NTA_BasicType_Real32: { parameters[name] = to_string(region->getParameterReal32(name)); break; }
          case NTA_BasicType_Byte: { parameters[name] = region->getParameterString(name); break; }
          default:
            break;
          } // end switch
        }
      }
      catch (nupic::Exception& ex) {
        return ::testing::AssertionFailure() << "nupic::Exception while processing parameter "
          << name << ":  " << ex.getFilename() << "(" << ex.getLineNumber() << ") " << ex.getMessage();
      }
      catch (std::exception& e) {
        return ::testing::AssertionFailure() << "Exception while processing parameter " << name << ":  " << e.what() << "" << std::endl;
      }
    }// end for
    return ::testing::AssertionSuccess();
	}



	// uses the Spec to find non-array parameters in the region and compare them to contents of the provided map.
	::testing::AssertionResult compareParameters(Region_Ptr_t region, std::map<std::string, std::string>& parameters)
	{
	  const Spec* ns = region->getSpec();
    size_t specCount = ns->parameters.getCount();
    // Look through the Spec and look for every instance of a single parameter.  
    // get each parameter, convert to a string and compare with what is stored in the parameters map.
    for (size_t i = 0; i < specCount; i++) {
      std::pair<string, ParameterSpec> p = ns->parameters.getByIndex(i);
      string name = p.first;

      try {
        if (p.second.count == 1) {
          switch (p.second.dataType) {
          case NTA_BasicType_UInt32: {
            if (parameters[name] != to_string(region->getParameterUInt32(name))) {
              return ::testing::AssertionFailure() << "Parameter "
                << name << " Does not match.  Expected " << parameters[name]
                << " found " << to_string(region->getParameterUInt32(name)) << "";
            }
          }
          break;

          case NTA_BasicType_Int32: {
            if (parameters[name] != to_string(region->getParameterInt32(name))) {
              return ::testing::AssertionFailure() << "Parameter "
                << name << " Does not match.  Expected " << parameters[name]
                << " found " << to_string(region->getParameterInt32(name)) << "";
            }
          }
          break;

          case NTA_BasicType_Real32: {
            if (parameters[name] != to_string(region->getParameterReal32(name))) {
              return ::testing::AssertionFailure() << "Parameter "
                << name << " Does not match.  Expected " << parameters[name]
                << " found " << to_string(region->getParameterReal32(name)) << "";
            }
          }
          break;

          case NTA_BasicType_Bool: {
            if (parameters[name] !=  (region->getParameterBool(name) ? "1" : "0")) {
              return ::testing::AssertionFailure()
                     << "Parameter " << name << " Does not match.  Expected "
                     << parameters[name] << " found "
                     << (region->getParameterBool(name) ? "1" : "0") << "";
            }
          }
          break;

          case NTA_BasicType_Byte: {
            if (parameters[name] != region->getParameterString(name)) {
              return ::testing::AssertionFailure() << "Parameter "
                << name << " Does not match.  Expected " << parameters[name]
                << " found " << region->getParameterString(name) << "";
            }
          }
          break;

          default:
            break;
          } // end switch
        }
      }
      catch (nupic::Exception& ex) {
        return ::testing::AssertionFailure() << "nupic::Exception while processing parameter "
          << name << ":  " << ex.getFilename() << "(" << ex.getLineNumber() << ") " << ex.getMessage();
      }
      catch (std::exception& e) {
        return ::testing::AssertionFailure() << "Exception while processing parameter " << name << ":  " << e.what() << "" << std::endl;
      }
    }// end for
    return ::testing::AssertionSuccess();
  }
} // namespace

