/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013-2020, Numenta, Inc.
 *
 * author: David Keeney, dkeeney@gmail.com, Feb 2020
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
//
// Test the REST API facility.
// This test will start up a server in a new thread and then use the original
// thread to act as the client.
//

#include "gtest/gtest.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

#include <examples/rest/server_core.hpp>

namespace testing {

using namespace htm;


static std::string host = "127.0.0.1";
static int port = 8050;
static bool verbose = false; // turn this on to print extra stuff for debugging the test.



#define VERBOSE  if (verbose)  std::cerr << "[          ] "
#define EPOCHS 3


static std::string dump_headers(const Headers &headers) {
  std::string s;
  char buf[BUFSIZ];

  for (const auto &x : headers) {
    snprintf(buf, sizeof(buf), "  %s: %s\n", x.first.c_str(), x.second.c_str());
    s += buf;
  }

  return s;
}

static std::string log(const Request &req, const Response &res) {
  std::string s;
  char buf[BUFSIZ];
  s += "================================\n";
  snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(), req.version.c_str(), req.path.c_str());
  s += buf;
  std::string query;
  for (auto it = req.params.begin(); it != req.params.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%c%s=%s", (it == req.params.begin()) ? '?' : '&', x.first.c_str(), x.second.c_str());
    query += buf;
  }
  s += query + "\n";
  s += dump_headers(req.headers);
  if (!req.body.empty())
    s += "body: " + req.body + "\n";
  s += "--------------------------------\n";
  snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
  s += buf;
  s += dump_headers(res.headers);
  if (!res.body.empty()) {
    s += "body: " + std::string(res.body);
  }
  s += "\n";
  return s;
}




class RESTapiTest : public ::testing::Test {

protected:
  std::shared_ptr<httplib::Client> client;
  std::thread serverThreadObj;
  RESTserver server;
  RESTapiTest() {}

  virtual ~RESTapiTest() {}

  void serverThread() { // This function is ran in the server thread.
    // This starts the NetworkAPI REST server
    std::string net_interface = "127.0.0.1";
    try {
      if (verbose)
        server.set_logger([](const Request &req, const Response &res) { std::cout << log(req, res); });
      server.listen(port, net_interface.c_str());
      // will exit listen loop when the "GET /stop" message is received.
    } catch (Exception &e) {
      ASSERT_TRUE(false) << "REST Server Error: " << e.getMessage();
    }
  }


  virtual void SetUp() {
    // start the server in a separate thread
    serverThreadObj = std::thread (&RESTapiTest::serverThread, this); // start REST server
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield to give server time to start
    client.reset(new httplib::Client(host, port));
    //client->set_timeout_sec(30);
  }

  virtual void TearDown() { 
    // Note:  The server should stop if sent a "/stop" message.
    //        If the server thread did not stop for some reason
    //        we need to shut it down by calling server.stop( ).
    // Normally you don't need to do the check to see if the server is running, etc.
    // We do this here just to test that the server does shut down.

    client->Get("/stop");                                        // stop the server.
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // yield to give server time to stop

    if (server.is_running()) {
      ASSERT_TRUE(false) << "The server did not shut down with the /stop command within 500ms.";
      server.stop();  
    }
    serverThreadObj.join(); // wait until server thread has stopped.

  }
};

TEST_F(RESTapiTest, helloWorld) {
  // Client thread.
  Value vm;
  //client->set_timeout_sec(30);

  // request "Hello World" to see if we are able to connect to the server.
  auto res = client->Get("/hi");
  ASSERT_TRUE(res) << "No response from server.";
  ASSERT_EQ(res->status, 200) << "Unexpected status returned: " << res->status << std::endl;
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  EXPECT_STREQ(vm["result"].c_str(), "Hello World!") << "Response to GET /hi request";
}

#ifdef NDEBUG //FIXME cpp-httplib started segfaulting in Debug, see https://github.com/htm-community/htm.core/issues/884  
TEST_F(RESTapiTest, example) {
  // A test similar to the Client Example.

  // Client thread.
  const httplib::Params noParams;
  char message[1000];
  Value vm;

  //client->set_timeout_sec(30);

  // Configure a NetworkAPI example
  // See Network.configure() for syntax.
  //     Simple situation    Encoder  ==>  SP  ==>  TM
  //     Compare this to the napi_sine example.
  std::string config = R"(
   {network: [
       {addRegion: {name: "encoder", type: "RDSEEncoderRegion", params: {size: 1000, sparsity: 0.2, radius: 0.03, seed: 2019, noise: 0.01}}},
       {addRegion: {name: "sp", type: "SPRegion", params: {dim: [2,1024], globalInhibition: true}}},
       {addRegion: {name: "tm", type: "TMRegion", params: {cellsPerColumn: 8, orColumnOutputs: true}}},
       {addLink:   {src: "encoder.encoded", dest: "sp.bottomUpIn"}},
       {addLink:   {src: "sp.bottomUpOut", dest: "tm.bottomUpIn"}}
    ]})";

  // create the network object
  auto res = client->Post("/network", config, "application/json");
  ASSERT_TRUE(res && res->status/100 == 2) << "Failed Response to POST /network request.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  std::string id = vm["result"].str();
  ASSERT_STREQ(id.c_str(), "1");

  // Send GET parameter message to retreive "tm.cellsPerColumn" parameter from the tm region.
  snprintf(message, sizeof(message), "/network/%s/region/tm/param/cellsPerColumn", id.c_str());
  res = client->Get(message);
  ASSERT_TRUE(res && res->status/100 == 2) << " GET param message failed.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  EXPECT_STREQ(vm["result"].c_str(), "8") << "Response to GET param request";


  // execute
  float x = 0.00f;
  for (size_t e = 0; e < EPOCHS; e++) {
    // -- sine wave, 0.01 radians per iteration   (Note: first iteration is for x=0.01, not 0)
    x += 0.01f; // step size for fn(x)
    double s = std::sin(x);

    // Send set parameter message to feed "sensedValue" parameter data into RDSE encoder for this iteration.
    snprintf(message, sizeof(message), "/network/%s/region/encoder/param/sensedValue?data=%.02f", id.c_str() , s);
    res = client->Put(message, noParams);
    ASSERT_TRUE(res && res->status / 100 == 2) << " PUT param message failed.";
    vm.parse(res->body);
    ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
    EXPECT_STREQ(vm["result"].c_str(), "OK") << "Response to PUT param request";

    // Execute an iteration
    snprintf(message, sizeof(message), "/network/%s/run", id.c_str());
    res = client->Get(message);
    vm.parse(res->body);
    ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
    EXPECT_STREQ(vm["result"].c_str(), "OK") << "Response to GET run";
  }

  // Retreive the final anomaly score from the TM object, 'tm.anomaly'.
  snprintf(message, sizeof(message), "/network/%s/region/tm/output/anomaly", id.c_str());
  res = client->Get(message);
  ASSERT_TRUE(res && res->status / 100 == 2) << " GET output message failed.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  EXPECT_STREQ(vm["result"][0].c_str(), "1") << "Response to GET output request (The Anomaly Score)";
  EXPECT_STREQ(vm["type"].c_str(), "Real32") << "response to GET output request did not have the right type.";
  std::vector<UInt> dim = vm["dim"].asVector<UInt>();
  EXPECT_EQ(dim.size(), 1u) << "response to GET output request did not have the correct number of dimensions.";
  EXPECT_EQ(dim[0], 1u) << "response to GET output request did not have the correct dimensions.";


  // Get the output of the SP
  snprintf(message, sizeof(message), "/network/%s/region/sp/output/bottomUpOut", id.c_str());
  res = client->Get(message);
  ASSERT_TRUE(res && res->status / 100 == 2) << " GET output message failed.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  EXPECT_STREQ(vm["type"].c_str(), "SDR") << "Type of response to GET region sp output bottomUpOut";
  dim = vm["dim"].asVector<UInt>();
  EXPECT_EQ(dim.size(), 2u) << " number of dimensions.";
  EXPECT_EQ(dim[0], 2u) << "First dim.";
  EXPECT_EQ(dim[1], 1024u) << "second dim.";


}
#endif


TEST_F(RESTapiTest, test_delete) {

  // Client thread.
  char message[1000];
  Value vm;


  // Configure a NetworkAPI example
  // See Network.configure() for syntax.
  //     Simple situation    Encoder  ==>  SP  ==>  TM
  //     Compare this to the napi_sine example.
  std::string config = R"(
   {network: [
       {addRegion: {name: "encoder", type: "RDSEEncoderRegion", params: {size: 1000, sparsity: 0.2, radius: 0.03, seed: 2019, noise: 0.01}}},
       {addRegion: {name: "sp", type: "SPRegion", params: {columnCount: 2048, globalInhibition: true}}},
       {addRegion: {name: "tm", type: "TMRegion", params: {cellsPerColumn: 8, orColumnOutputs: true}}},
       {addLink:   {src: "encoder.encoded", dest: "sp.bottomUpIn"}},
       {addLink:   {src: "sp.bottomUpOut", dest: "tm.bottomUpIn"}}
    ]})";

  // create the network object
  auto res = client->Post("/network", config, "application/json");
  ASSERT_TRUE(res && res->status / 100 == 2) << "Failed Response to POST /network request.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  std::string id = vm["result"].str();

  // Now delete the second link.
  snprintf(message, sizeof(message), "/network/%s/link/sp.bottomUpOut/tm.bottomUpIn", id.c_str());
  res = client->Delete(message);
  ASSERT_TRUE(res && res->status / 100 == 2) << " DELETE link message failed.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  EXPECT_STREQ(vm["result"].c_str(), "OK") << "Response to DELETE Link request";

  // Delete a region
  snprintf(message, sizeof(message), "/network/%s/region/tm", id.c_str());
  res = client->Delete(message);
  ASSERT_TRUE(res && res->status / 100 == 2) << " DELETE region message failed.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  EXPECT_STREQ(vm["result"].c_str(), "OK") << "Response to DELETE Link request";
  
  // Delete a Network
  snprintf(message, sizeof(message), "/network/%s/ALL", id.c_str());
  res = client->Delete(message);
  ASSERT_TRUE(res && res->status / 100 == 2) << " DELETE region message failed.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  EXPECT_STREQ(vm["result"].c_str(), "OK") << "Response to DELETE Link request";
  
  
}


TEST_F(RESTapiTest, alternative_ids) {

  // Client thread.
  Value vm;

  // See Network.configure() for syntax.
  //     Simple situation    Just the encoder Encoder, nothing else 
  std::string config = R"(
   {network: [
       {addRegion: {name: "encoder", type: "RDSEEncoderRegion", params: {size: 1000, sparsity: 0.2, radius: 0.03, seed: 2019, noise: 0.01}}},
    ]})";

  // create a Network object using a numeric id as a URL parameter
  auto res = client->Post("/network?id=123", config, "application/json");
  ASSERT_TRUE(res && res->status/100 == 2 && res->body.size() > 0) << "Failed Response to POST /network?id=123 request.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  std::string id = vm["result"].str();
  EXPECT_STREQ(id.c_str(), "123");
  
  // create a Network object using a numeric id as a URL field
  res = client->Post("/network/456", config, "application/json");
  ASSERT_TRUE(res && res->status/100 == 2 && res->body.size() > 0) << "Failed Response to POST /network/456 request.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  id = vm["result"].str();
  EXPECT_STREQ(id.c_str(), "456");

  // create a Network object using a non-numeric id as a URL field
  res = client->Post("/network/TestObj", config, "application/json");
  ASSERT_TRUE(res && res->status/100 == 2 && res->body.size() > 0) << "Failed Response to POST /network/TestObj request.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  id = vm["result"].str();
  EXPECT_STREQ(id.c_str(), "TestObj");

  // create a Network object using a non-numeric id as a URL parameter that contains URL encoding.
  res = client->Post("/network?id=%20abc", config, "application/json");
  ASSERT_TRUE(res && res->status/100 == 2 && res->body.size() > 0) << "Failed Response to POST /network?id=%20abc request.";
  vm.parse(res->body);
  ASSERT_FALSE(vm.contains("err")) << "An error returned. " << vm["err"].str();
  id = vm["result"].str();
  EXPECT_STREQ(id.c_str(), "%20abc");
}


} // namespace testing
