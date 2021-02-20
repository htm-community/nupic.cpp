/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013-2017, Numenta, Inc.
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
 * --------------------------------------------------------------------- */

/** @file
 * Implementation of Input test
 */

#include "gtest/gtest.h"
#include <htm/engine/Input.hpp>
#include <htm/engine/Network.hpp>
#include <htm/engine/Output.hpp>
#include <htm/engine/Region.hpp>
#include <htm/ntypes/Dimensions.hpp>
#include <htm/regions/TestNode.hpp>

namespace testing { 
    
static bool verbose = false;
#define VERBOSE                                                                \
  if (verbose)                                                                 \
  std::cerr << "[          ]"

using namespace htm;

TEST(InputTest, BasicNetworkConstruction) {
  Network net;
  std::shared_ptr<Region> r1 = net.addRegion("r1", "TestNode", "");
  std::shared_ptr<Region> r2 = net.addRegion("r2", "TestNode", "");

  // Test constructor
  std::shared_ptr<Input> x = r1->getInput("bottomUpIn");
  std::shared_ptr<Input> y = r2->getInput("bottomUpIn");

  // test getRegion()
  ASSERT_EQ(r1.get(), x->getRegion());
  ASSERT_EQ(r2.get(), y->getRegion());

  // test isInitialized()
  ASSERT_TRUE(!x->isInitialized());
  ASSERT_TRUE(!y->isInitialized());

  Dimensions d1;
  d1.push_back(8);
  d1.push_back(4);
  r1->setDimensions(d1);
  Dimensions d2;
  d2.push_back(2);
  d2.push_back(16);
  r2->setDimensions(d2);
  net.link("r1", "r2");

  net.initialize();

  VERBOSE << "Dimensions: \n";
  VERBOSE << " TestNode in       - " << r1->getInputDimensions("bottomUpIn")  <<"\n";
  VERBOSE << " TestNode out      - " << r1->getOutputDimensions("bottomUpOut")<<"\n";
  VERBOSE << " TestNode in       - " << r2->getInputDimensions("bottomUpIn")  <<"\n";
  VERBOSE << " TestNode out      - " << r2->getOutputDimensions("bottomUpOut")<<"\n";

  // test getData() with empty buffer
  const ArrayBase *pa = &(y->getData());
  ASSERT_EQ(32u, pa->getCount());
}


TEST(InputTest, LinkTwoRegionsOneInput1Dmatch) {
  Network net;
  VERBOSE << "Testing [4] + [4] = [8]\n";
  std::shared_ptr<Region> region1 = net.addRegion("region1", "TestNode", "{dim: [4]}");
  std::shared_ptr<Region> region2 = net.addRegion("region2", "TestNode", "{dim: [4]}");
  std::shared_ptr<Region> region3 = net.addRegion("region3", "TestNode", "");
  net.link("region1", "region3");
  net.link("region2", "region3");

  net.initialize();
  VERBOSE << "region1 output dims: " << region1->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region2 output dims: " << region2->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region3 region dims: " << region3->getDimensions() << "\n";
  VERBOSE << "region3 input  dims: " << region3->getInputDimensions("bottomUpIn") << "\n";

  Dimensions expected = {8};
  Dimensions d3 = region3->getDimensions();
  EXPECT_EQ(d3, expected) << "Expected region3 region dimensions " << expected;

  std::shared_ptr<Input> in3 = region3->getInput("bottomUpIn");
  d3 = in3->getDimensions();
  EXPECT_EQ(d3, expected) << "Expected region3 input dimensions " << expected;

  net.run(2);

  // test getData()
  std::vector<Real64> expectedData = {1.0, 0.0, 1.0, 2.0, 1.0, 0.0, 1.0, 2.0 };
  const Array *pa = &(in3->getData());
  VERBOSE << "region3 input data: " << *pa << std::endl;
  ASSERT_EQ(expectedData.size(), pa->getCount());
  ASSERT_EQ(expectedData, pa->asVector<Real64>());
}

TEST(InputTest, LinkTwoRegionsOneInput1Dnomatch) {
  Network net;
  VERBOSE << "Testing [4] + [3] = [7]\n";
  std::shared_ptr<Region> region1 = net.addRegion("region1", "TestNode", "{dim: [4]}");
  std::shared_ptr<Region> region2 = net.addRegion("region2", "TestNode", "{dim: [3]}");
  std::shared_ptr<Region> region3 = net.addRegion("region3", "TestNode", "");
  net.link("region1", "region3");
  net.link("region2", "region3");

  net.initialize();
  VERBOSE << "region1 output dims: " << region1->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region2 output dims: " << region2->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region3 region dims: " << region3->getDimensions() << "\n";
  VERBOSE << "region3 input  dims: " << region3->getInputDimensions("bottomUpIn") << "\n";

  Dimensions expected = {7};
  Dimensions d3 = region3->getDimensions();
  EXPECT_EQ(d3, expected) << "Expected region3 region dimensions " << expected;

  std::shared_ptr<Input> in3 = region3->getInput("bottomUpIn");
  d3 = in3->getDimensions();
  EXPECT_EQ(d3, expected) << "Expected region3 input dimensions " << expected;

  net.run(2);

  // test getData()
  std::vector<Real64> expectedData = {1.0, 0.0, 1.0, 2.0, 1.0, 0.0, 1.0 };
  const Array *pa = &(in3->getData());
  VERBOSE << "region3 input data: " << *pa << std::endl;
  ASSERT_EQ(expectedData.size(), pa->getCount());
  ASSERT_EQ(expectedData, pa->asVector<Real64>());
}

TEST(InputTest, LinkTwoRegionsOneInput4X3) {
  Network net;
  VERBOSE << "Testing [4,2] + [4] = [4,3]\n";
  std::shared_ptr<Region> region1 =
      net.addRegion("region1", "TestNode", "{dim: [4,2]}");
  std::shared_ptr<Region> region2 =
      net.addRegion("region2", "TestNode", "{dim: [4]}");
  std::shared_ptr<Region> region3 = net.addRegion("region3", "TestNode", "");
  net.link("region1", "region3");
  net.link("region2", "region3");

  net.initialize();
  VERBOSE << "region1 output dims: " << region1->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region2 output dims: " << region2->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region3 region dims: " << region3->getDimensions() << "\n";
  VERBOSE << "region3 input  dims: " << region3->getInputDimensions("bottomUpIn") << "\n";

  Dimensions expected = {4, 3};
  Dimensions d3 = region3->getDimensions();
  EXPECT_EQ(d3, expected) << "Expected region3 region dimensions " << expected;

  std::shared_ptr<Input> in3 = region3->getInput("bottomUpIn");
  d3 = in3->getDimensions();
  EXPECT_EQ(d3, expected) << "Expected region3 input dimensions " << expected;

  net.run(2);

  // test getData()
  std::vector<Real64> expectedData = {1.0, 0.0, 1.0, 2.0, 
                                      1.0, 1.0, 2.0, 3.0,
                                      1.0, 0.0, 1.0, 2.0};
  const Array *pa = &(in3->getData());
  VERBOSE << "region3 input data: " << *pa << std::endl;
  ASSERT_EQ(expectedData.size(), pa->getCount());
  ASSERT_EQ(expectedData, pa->asVector<Real64>());
}

TEST(InputTest, LinkTwoRegionsOneInput4X4) {
  Network net;
  VERBOSE << "Testing [4,2] + [4,2] = [4,4]\n";
  std::shared_ptr<Region> region1 = net.addRegion("region1", "TestNode", "");
  std::shared_ptr<Region> region2 = net.addRegion("region2", "TestNode", "");
  std::shared_ptr<Region> region3 = net.addRegion("region3", "TestNode", "");

  Dimensions d1;
  d1.push_back(4);
  d1.push_back(2);
  region1->setDimensions(d1);
  region2->setDimensions(d1);

  net.link("region1", "region3");
  net.link("region2", "region3");

  net.initialize();

  VERBOSE << "region1 output dims: " << region1->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region2 output dims: " << region2->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region3 region dims: " << region3->getDimensions() << "\n";
  VERBOSE << "region3 input  dims: " << region3->getInputDimensions("bottomUpIn") << "\n";

  Dimensions expected = {4, 4};
  Dimensions d3 = region3->getDimensions();
  VERBOSE << "region3 region dims: " << d3 << "\n";
  EXPECT_EQ(d3, expected) << "Expected region3 region dimensions " << expected;

  std::shared_ptr<Input> in3 = region3->getInput("bottomUpIn");
  d3 = in3->getDimensions();
  VERBOSE << "region3 input dims: " << in3->getDimensions() << "\n";
  EXPECT_EQ(d3, expected) << "Expected region3 input dimensions " << expected;

  net.run(2);

  // test getData()
  std::vector<Real64> expectedData = {
      1.0, 0.0, 1.0, 2.0, 
      1.0, 1.0, 2.0, 3.0,
      1.0, 0.0, 1.0, 2.0, 
      1.0, 1.0, 2.0, 3.0};
  const Array *pa = &(in3->getData());
  VERBOSE << "region3 input data: " << *pa << std::endl;
  ASSERT_EQ(expectedData.size(), pa->getCount());
  ASSERT_EQ(expectedData, pa->asVector<Real64>());
}


TEST(InputTest, LinkTwoRegionsOneInput3D1) {
  Network net;
  VERBOSE << "Testing [4,2] + [4,2,1] = [4,4,1]\n";
  std::shared_ptr<Region> region1 = net.addRegion("region1", "TestNode", "{dim: [4,2]}");
  std::shared_ptr<Region> region2 = net.addRegion("region2", "TestNode", "{dim: [4,2,1]}");
  std::shared_ptr<Region> region3 = net.addRegion("region3", "TestNode", "");

  net.link("region1", "region3");
  net.link("region2", "region3");

  net.initialize();

  VERBOSE << "region1 output dims: " << region1->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region2 output dims: " << region2->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region3 region dims: " << region3->getDimensions() << "\n";
  VERBOSE << "region3 input  dims: " << region3->getInputDimensions("bottomUpIn") << "\n";

  Dimensions expected = {4, 4};
  Dimensions d3 = region3->getDimensions();
  VERBOSE << "region3 region dims: " << d3 << "\n";
  EXPECT_EQ(d3, expected) << "Expected region3 region dimensions " << expected;

  std::shared_ptr<Input> in3 = region3->getInput("bottomUpIn");
  d3 = in3->getDimensions();
  VERBOSE << "region3 input dims: " << in3->getDimensions() << "\n";
  EXPECT_EQ(d3, expected) << "Expected region3 input dimensions " << expected;

  net.run(2);

  // test getData()
  std::vector<Real64> expectedData = {
      1.0, 0.0, 1.0, 2.0, 
      1.0, 1.0, 2.0, 3.0,
      1.0, 0.0, 1.0, 2.0, 
      1.0, 1.0, 2.0, 3.0};
  const Array *pa = &(in3->getData());
  VERBOSE << "region3 input data: " << *pa << std::endl;
  ASSERT_EQ(expectedData.size(), pa->getCount());
  ASSERT_EQ(expectedData, pa->asVector<Real64>());
}


TEST(InputTest, LinkTwoRegionsOneInput3D2) {
  Network net;
  VERBOSE << "Testing [4,2] + [4,2,2] = [4,4,1]\n";
  std::shared_ptr<Region> region1 = net.addRegion("region1", "TestNode", "{dim: [4,2]}");
  std::shared_ptr<Region> region2 = net.addRegion("region2", "TestNode", "{dim: [4,2,2]}");
  std::shared_ptr<Region> region3 = net.addRegion("region3", "TestNode", "");

  net.link("region1", "region3");
  net.link("region2", "region3");

  net.initialize();

  VERBOSE << "region1 output dims: " << region1->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region2 output dims: " << region2->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region3 region dims: " << region3->getDimensions() << "\n";
  VERBOSE << "region3 input  dims: " << region3->getInputDimensions("bottomUpIn") << "\n";

  Dimensions expected = {4, 2, 3};
  Dimensions d3 = region3->getDimensions();
  VERBOSE << "region3 region dims: " << d3 << "\n";
  EXPECT_EQ(d3, expected) << "Expected region3 region dimensions " << expected;

  std::shared_ptr<Input> in3 = region3->getInput("bottomUpIn");
  d3 = in3->getDimensions();
  VERBOSE << "region3 input dims: " << in3->getDimensions() << "\n";
  EXPECT_EQ(d3, expected) << "Expected region3 input dimensions " << expected;

  net.run(2);

  // test getData()
  std::vector<Real64> expectedData = {
      1.0, 0.0, 1.0, 2.0, 
      1.0, 1.0, 2.0, 3.0,
      1.0, 0.0, 1.0, 2.0, 
      1.0, 1.0, 2.0, 3.0,
      1.0, 2.0, 3.0, 4.0, 
      1.0, 3.0, 4.0, 5.0};
  const Array *pa = &(in3->getData());
  VERBOSE << "region3 input data: " << *pa << std::endl;
  ASSERT_EQ(expectedData.size(), pa->getCount());
  ASSERT_EQ(expectedData, pa->asVector<Real64>());
}

TEST(InputTest, LinkTwoRegionsOneInputFlatten) {
  Network net;
  VERBOSE << "Testing [4,2] + [3,2] = [14]\n";
  std::shared_ptr<Region> region1 = net.addRegion("region1", "TestNode", "{dim: [4,2]}");
  std::shared_ptr<Region> region2 = net.addRegion("region2", "TestNode", "{dim: [3,2]}");
  std::shared_ptr<Region> region3 = net.addRegion("region3", "TestNode", "");

  net.link("region1", "region3");
  net.link("region2", "region3");

  net.initialize();

  VERBOSE << "region1 output dims: " << region1->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region2 output dims: " << region2->getOutputDimensions("bottomUpOut") << "\n";
  VERBOSE << "region3 region dims: " << region3->getDimensions() << "\n";
  VERBOSE << "region3 input  dims: " << region3->getInputDimensions("bottomUpIn") << "\n";

  Dimensions expected = {14};
  Dimensions d3 = region3->getDimensions();
  VERBOSE << "region3 region dims: " << d3 << "\n";
  EXPECT_EQ(d3, expected) << "Expected region3 region dimensions " << expected;

  std::shared_ptr<Input> in3 = region3->getInput("bottomUpIn");
  d3 = in3->getDimensions();
  VERBOSE << "region3 input dims: " << in3->getDimensions() << "\n";
  EXPECT_EQ(d3, expected) << "Expected region3 input dimensions " << expected;

  net.run(2);

  // test getData()
  std::vector<Real64> expectedData = {1.0, 0.0, 1.0, 2.0, 1.0, 1.0, 2.0,
                                      3.0, 1.0, 0.0, 1.0, 1.0, 1.0, 2.0};
  const Array *pa = &(in3->getData());
  VERBOSE << "region3 input data: " << *pa << std::endl;
  ASSERT_EQ(expectedData.size(), pa->getCount());
  ASSERT_EQ(expectedData, pa->asVector<Real64>());
}



TEST(InputTest, LinkFromAppSimple) {
  Network net;
  VERBOSE << "With Input from an App\n";
  std::shared_ptr<Region> region1 = net.addRegion("region1", "SPRegion", "{dim: [1000]}");
  std::shared_ptr<Region> region2 = net.addRegion("region2", "TMRegion", "");

  net.link("region1", "region2");
  net.link("INPUT", "region1", "", "{dim: 10}", "app_source1", "bottomUpIn");  // accepts input from app

  net.initialize();

  // Check dimensions
  Dimensions expected_dim = {10};
  Dimensions d1 = region1->getInputDimensions("bottomUpIn");
  VERBOSE << "region1 input dims: " << d1 << "\n";
  EXPECT_EQ(d1, expected_dim) << "Expected region1 input dimensions " << expected_dim;

  // Check initial buffer
  SDR expectedData({10});   // starts out zero filled.
  ASSERT_EQ(expectedData.size, (UInt)region1->getInputData("bottomUpIn").getCount());
  ASSERT_TRUE(expectedData == region1->getInputData("bottomUpIn").getSDR());

  Array a(NTA_BasicType_Real32);   // send data as Real32 to confirm that conversion works.
  a.allocateBuffer(expected_dim.getCount());
  Real32 *ptr = (Real32*)a.getBuffer();

  for (size_t i = 0; i < 10; i++) {
    a.zeroBuffer();
    ptr[i] = 1.0;  // modifies the Array a to set the bit indexed by i;

    // send it to the link that references "INPUT" and "app_source1" as source.
    net.setInputData("app_source1", a);  

    net.run(1);  // processes it

    // check that the input data arrived, with type conversion.

    SDR_sparse_t s;     // We expect to find the same bit set
    s.push_back((UInt)i);
    expectedData.setSparse(s);
    VERBOSE << "Iteration " << i << " Input buffer=" << region1->getInputData("bottomUpIn").getSDR() 
            << " expecting=" << expectedData << "\n";
    EXPECT_TRUE(expectedData == region1->getInputData("bottomUpIn").getSDR());
  }
}


TEST(InputTest, LinkFromAppSDRFanIn) {
  Network net;
  VERBOSE << "With two SDR Inputs from an App Fan-In to one input.\n";
  // This is something like you might have if the App was implementing an encoder with two independent variables.
  // The two input streams will be appended and fed into the SP which turns it into a true SDR with width of 1000.
  const std::vector<std::vector<UInt>> testdata1 = {{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}};          // sparse data
  const std::vector<std::vector<UInt>> testdata2 = {{10, 25, 26, 75}, {11, 26, 27, 31}, {5, 10, 15, 80}}; // sparse data
  std::shared_ptr<Region> region1 = net.addRegion("region1", "SPRegion", "{dim: [1000]}");
  std::shared_ptr<Region> region2 = net.addRegion("region2", "TMRegion", "");

  net.link("region1", "region2");
  net.link("INPUT", "region1", "", "{dim: 20}",  "app_source1", "bottomUpIn"); // accepts input from app 'app_source1'
  net.link("INPUT", "region1", "", "{dim: 100}", "app_source2", "bottomUpIn"); // accepts input from app 'app_source2'

  net.initialize();

  // Check dimensions
  Dimensions expected_dim(120);  // 20 + 100
  Dimensions d1 = region1->getInputDimensions("bottomUpIn");
  VERBOSE << "region1 input dims: " << d1 << "\n";
  EXPECT_EQ(d1, expected_dim) << "Expected region1 input dimensions " << expected_dim;

  for (size_t i = 0; i < 3; i++) {
    // variable 1
    SDR data1({20});  // the first link, identified as 'app_source1' is expecing data with dim of [20].
    data1.setSparse(testdata1[i]);
    net.setInputData("app_source1", Array(data1));

    // variable 2
    SDR data2({100}); // the second link, identified as 'app_source2' is expecing data with dim of [100].
    data2.setSparse(testdata2[i]);
    net.setInputData("app_source2", Array(data2));

    net.run(1); // processes it

    // check that the input data arrived as the concatination of the two inputs.
    SDR expectedData(expected_dim.asVector());
    expectedData.concatenate(data1, data2);

    VERBOSE << "Iteration " << i << " Input buffer=" << region1->getInputData("bottomUpIn").getSDR()
            << " expectedData=" << expectedData;
    EXPECT_TRUE(expectedData == region1->getInputData("bottomUpIn").getSDR());
  }
}


} // namespace