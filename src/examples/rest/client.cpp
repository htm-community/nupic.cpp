//
//  client.cpp
//
//  Taken from https://github.com/yhirose/cpp-httplib/blob/master/example/client.cc
//  Copyright (c) 2019 Yuji Hirose. All rights reserved.
//  MIT License
//

// A client for the NetworkAPI REST interface.
// Before running this client example, start the example server in the background.
// USAGE:  client [host [port]] 
//         The default host is "127.0.0.1", default port is 8050.
//         (Note: on Windows, a host of "localhost" may cause delays in resolving ip address).
//
// What should happen:
//  1) client sends a "/hi" message to the server.  The server replys with "Hello World\n".
//     This is just to confirm that we are connected to the server.
//  2) client sends a "config" message to create and configure an instance of a 
//     NetworkAPI Network object. It returns with a resource id code that needs to be 
//     passed to the server for all subsequent messages that act on this object.
//     In this case we are creating a Network with regions connected like this:
//          encoder -> SP -> TM 
//
//  In a loop of EPOCH iterations:
//  3) client sends a PUT param message for "encoder.sensedValue" parameter to 
//     pass data to the encoder. In this case we are passing a sine of one radian 
//     per iteration. Reply should be "OK\n" 
//  4) client send a GET run message to execute the NetworkAPI object once.
//     Reply should be "OK\n" if successful.
//
//  After the loop
//  5) client send a GET output message to obtain the "TM.anomaly" output.
//     Reply should be a JSON encoded Array object obtained from the output.
//     In this case it should be one element array of value 1.0.
//
// Setting the verbose variable to true will show messages coming and going.
//


#include <iostream>
#include <cstring>

// save diagnostic state
#pragma GCC diagnostic push
// turn off the specific warning. Can also use "-Wall"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <httplib.h>
// turn back on the compiler warnings
#pragma GCC diagnostic pop

//#define CA_CERT_FILE "./ca-bundle.crt"
#define DEFAULT_PORT 8050
#define DEFAULT_HOST "127.0.0.1"
#define EPOCHS 5  // The number of iterations

static bool verbose = true; // turn this on to print extra stuff for debugging.
#define VERBOSE  if (verbose) std::cout 

std::string trim(const std::string &s) {
  size_t i, j;
  for (i = 0; i < s.length(); i++)
    if (!std::isspace(s[i]))
      break;
  for (j = s.length(); j > i; j--)
    if (!std::isspace(s[j - 1]))
      break;
  return s.substr(i, j - i);
}


using namespace std;

int main(int argc, char **argv) {
  char message[1000];
  const httplib::Params noParams;
  int port = DEFAULT_PORT;
  std::string serverHost = DEFAULT_HOST;

  if (argc == 2) {
    serverHost = std::stoi(argv[1]);
  } else if (argc == 3) {
    serverHost = std::stoi(argv[1]);
    port = std::stoi(argv[2]);
  }

  VERBOSE << "Connecting to server: " + serverHost + " port: " << port << std::endl;
  httplib::Client client(serverHost.c_str(), port);
  //client.set_timeout_sec(30);  // The time it waits for a network connection.

  // request "Hello World" to see if we are able to connect to the server.
  VERBOSE << "GET /hi" << std::endl;
  auto res = client.Get("/hi");
  if (!res) {
    std::cerr << "Connection to server failed." << std::endl;
    return 1;
  }
  VERBOSE << res->body << endl;
  if (res->body.substr(0, 6) == "ERROR:") {
    return 1;
  }

  // Configure a NetworkAPI example
  // See Network.configure() for syntax.
  //     Simple situation    Encoder  ==>  SP  ==>  TM
  //     Compare this to the napi_sine example.
  std::string config = R"(
   {network: [
       {addRegion: {name: "encoder", type: "RDSERegion", params: {size: 1000, sparsity: 0.2, radius: 0.03, seed: 2019, noise: 0.01}}},
       {addRegion: {name: "sp", type: "SPRegion", params: {columnCount: 2048, globalInhibition: true}}},
       {addRegion: {name: "tm", type: "TMRegion", params: {cellsPerColumn: 8, orColumnOutputs: true}}},
       {addLink:   {src: "encoder.encoded", dest: "sp.bottomUpIn"}},
       {addLink:   {src: "sp.bottomUpOut", dest: "tm.bottomUpIn"}}
    ]})";
  VERBOSE << "Post /network\n  body: " + config << std::endl;
  res = client.Post("/network", config, "application/json");
  if (!res || res->status / 100 != 2) {
    std::cerr << "Network configuration failed." << std::endl;
    return 1;
  }
  if (res->body.substr(0,6) == "ERROR:") {
    std::cerr << res->body << std::endl;
    return 1;
  }
  std::string id = trim(res->body);
  VERBOSE << "Resource ID: " << id << std::endl;


  // execute 
  float x = 0.00f;
  for (size_t e = 0; e < EPOCHS; e++) {
    // -- sine wave, 0.01 radians per iteration   (Note: first iteration is for x=0.01, not 0)
    x += 0.01f; // step size for fn(x)
    double s = std::sin(x);

    // Send set parameter message to feed "sensedValue" parameter data into RDSE encoder for this iteration.
    std::snprintf(message, sizeof(message), "/network/%s/region/encoder/param/sensedValue?data=%-5.02f", id.c_str(), s);
    VERBOSE << "PUT " << message << std::endl;
    res = client.Put(message, noParams);
    if (!res) {
      std::cerr << "Error setting parameter.\n";
      return 1;
    }
    if (res->status/100 != 2 || trim(res->body) != "OK") {
      std::cerr << res->body << std::endl;
      return 1;
    }

    // Execute an iteration
    std::snprintf(message, sizeof(message), "/network/%s/run", id.c_str());
    VERBOSE << "GET " << message << std::endl;
    res = client.Get(message);
    if (!res || trim(res->body) != "OK") {
      std::cerr << "Run failed.\n";
      if (res) std::cerr << res->body << std::endl;
      return 1;
    }
    VERBOSE << res->body << std::endl;
  }

  // Retreive the final anomaly score from the TM object's 'anomaly' output.
  std::snprintf(message, sizeof(message), "/network/%s/region/tm/output/anomaly", id.c_str());
  VERBOSE << "GET " << message << std::endl;
  res = client.Get(message);
  if (!res || res->body.substr(0, 6) == "ERROR:") {
    std::cerr << "Run failed.\n";
    if (res) std::cerr << res->body << std::endl;
    return 1;
  }
  VERBOSE << "Anomaly Score: " << res->body << std::endl;
  return 0;
}
