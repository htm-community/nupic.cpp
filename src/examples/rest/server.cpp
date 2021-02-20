//
//  Copyright (c) 2019 Yuji Hirose. All rights reserved.
//  MIT License
//

// Example taken from https://github.com/yhirose/cpp-httplib/blob/master/example/server.cc
// with minor mods to call NetworkAPI by David Keeney, Feb 2020

// This runs as a REST web server executable
// To run as https, define SERVER_CERT_FILE and SERVER_PRIVATE_KEY_FILE
// which must point to a certification.
// See server_core.hpp for more details.


//#define SERVER_CERT_FILE "./cert.pem"
//#define SERVER_PRIVATE_KEY_FILE "./key.pem"
#define DEFAULT_PORT 8050  // default port to listen on
#define DEFAULT_INTERFACE "127.0.0.1"
static bool verbose = true; // turn this on to print extra stuff for debugging the test.
#define VERBOSE if (verbose) std::cout

#include <examples/rest/server_core.hpp>
using namespace htm;

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
    snprintf(buf, sizeof(buf), "%c%s=%s",
             (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
             x.second.c_str());
    query += buf;
  }
  snprintf(buf, sizeof(buf), "%s\n", query.c_str());
  s += buf;
  s += dump_headers(req.headers);
  s += "--------------------------------\n";
  snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
  s += buf;
  s += dump_headers(res.headers);
  s += "\n";
  if (!res.body.empty()) { s += res.body; }
  s += "\n";
  return s;
}





int main(int argc, char** argv) {
  std::string net_interface = DEFAULT_INTERFACE;
  int port = DEFAULT_PORT;

  if(argc == 2) {
    port = std::stoi(argv[1]);
  }
  else if(argc == 3) {
    port = std::stoi(argv[1]);
    net_interface = argv[2];
  }

  RESTserver  server;
 

  // How to perform logging.
  if (verbose) server.set_logger([](const Request &req, const Response &res) {
    printf("%s", log(req, res).c_str());
  });

  // Enter the server listen loop.
  // The server will not exit this loop until the /stop command is received
  // (or the program is stopped).
  VERBOSE << "Starting Server on port: " << port << std::endl;
  server.listen(port, net_interface.c_str());
  VERBOSE << "Server stopped." << std::endl;

  return 0;
}
