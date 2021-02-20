//
//  Copyright (c) 2019 Yuji Hirose. All rights reserved.
//  MIT License
//

// Example taken from https://github.com/yhirose/cpp-httplib/blob/master/example/server.cc
// with minor mods to call NetworkAPI by David Keeney, Feb 2020

// This is written as an header-only library so that the server can be constructed
// as a stand-alone REST web server executable or incorporated in another program
// (such as a unit test) running under a separate thread.
//
// The key structure for the resources are as follows:
//   /network/<id>
//   /network/<id>/region/<name>
//   /network/<id>/region/<name>/param/<name>
//   /network/<id>/region/<name>/input/<name>
//   /network/<id>/region/<name>/output/<name>
//   /network/<id>/link/<source_name>/<dest_name>
//      Note: source_name syntax is "region_name.output_name"
//            dest_name   syntax is "region_name.input_name"
//
// The expected protocol for the NetworkAPI application is as follows:
// All responces are JSON encoded except Errors responses which are text
// with a prefix of "ERROR: ".
//
//  POST /network
//     or
//       /network/<id>
//     or
//       /network?id=<id>
//       Create a new Network class object as a resource identified by id.
//       The id field is optional. If the id is given the new network object will be
//       assigned to this id.  Otherwise it will be assigned to the next available id.
//       The body of the POST is JSON formatted configuration string
//       Returns the id for the created resouce or an Error message.
//  PUT  /network/<id>/region/<region name/param/<param name>?data=<JSON encoded data>
//       Set the value of a region's parameter. The <data> could also be in the body.
//  GET  /network/<id>/region/<region name>/param/<param name>
//       Get the value of a region's parameter.
//  PUT  /network/<id>/input/<input name>?data=<JSON encoded array>
//       Set the value of a region's input. The <data> could also be in the body.
//  GET  /network/<id>/region/<region name>/input/<input name>
//       Get the value of a region's input. Returns a JSON encoded array.
//  GET  /network/<id>/region/<region name>/output/<output name>
//       Get the value of a region's output. Returns a JSON encoded array.
//  DELETE /network/<id>/region/<region name>
//       Deletes a region. Must not be in any links.
//  DELETE /network/<id>/link/<source_name>/<dest_name>
//       Deletes a link.
//  DELETE /network/<id>/ALL
//       Deletes the entire Network object
//  GET  /network/<id>/run?iterations=<iterations>
//       Execute all regions in phase order. Repeat <iterations> times.
//  GET  /network/<id>/region/<region name>/command?data=<command>
//       Execute a predefined command on a region. <command> must start with the
//       command name followed by the arguments.
//       The data could also be in the body.
//
//  GET  /hi
//       Respond with "Hello World\n" as a way to check client to server connection.
//  GET  /stop
//       Stop the server.  All resources are released.

#include <cctype>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

// save diagnostic state
#pragma GCC diagnostic push
// turn off the specific warning. Can also use "-Wall"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <httplib.h>
// turn back on the compiler warnings
#pragma GCC diagnostic pop

#include <htm/engine/Network.hpp>
#include <htm/engine/RESTapi.hpp>


#define SERVER_CERT_FILE "./cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./key.pem"
#define DEFAULT_PORT 8050 // default port to listen on
#define DEFAULT_INTERFACE "127.0.0.1"

using namespace httplib;
using namespace htm;

class RESTserver {

public:
  RESTserver() {

    /*** Register all of the handlers ***/

    //  GET  /hi    ==>  "Hello World!"\n
    svr.Get("/hi", [](const Request & /*req*/, Response &res) { res.set_content("{\"result\": \"Hello World!\"}\n", "application/json"); });
    if (!svr.is_valid()) {
      NTA_THROW << "server could not be created...\n";
    }
    

    //  POST /network?id=<specified id>
    // or
    //  POST /network/<specified id>
    // or
    //  POST /network
    //  Create and configure a Network resource indexed by an id.
    //  The configuration string is passed in the POST body, JSON encoded.
    //  Returns the id used (URLencoded).  Subsequent calls should use this id.
    //  Note: the id parameter is optional.  If not provided it will use the next
    //        available id.  Otherwise it will use and return the specified id.
    //        If a Network object is already associated with the specified id
    //        the program will remove the existing Network object and create a new one.
    //
    //        The specified id does not have to be numeric.  However, if it is
    //        not compatible with URL syntax the returned id will be a URLencoded
    //        copy of the id.
    //
    svr.Post("/network", [&](const Request &req, Response &res) {
      std::string id;
      auto itr = req.params.find("id");
      if (itr != req.params.end())
        id = url_encode(itr->second); // the returned parameter value gets url decoded so must re-encode it.

      std::string data = req.body;
      auto ix = req.params.find("data"); // The body could optionally be encoded in a parameter
      if (ix != req.params.end())
        data = ix->second;

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->create_network_request(id, data);
      res.set_content(result + "\n", "application/json");
    });
    svr.Post("/network/[^/]*", [&](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];

      std::string data = req.body;
      auto ix = req.params.find("data");
      if (ix != req.params.end())
        data = ix->second;

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->create_network_request(id, req.body);
      res.set_content(result + "\n", "application/json");
    });

    //  PUT  /network/<id>/region/<region name>/param/<param name>?data=<JSON encoded data>
    // Set the value of a ReadWrite Parameter on a Region.  Alternatively, the data could be in the body.
    svr.Put("/network/.*/region/.*/param/.*", [&](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string region_name = flds[4];
      std::string param_name = flds[6];
      std::string data = req.body;
      auto ix = req.params.find("data");
      if (ix != req.params.end())
        data = ix->second;

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->put_param_request(id, region_name, param_name, data);
      res.set_content(result + "\n", "application/json");
    });

    //  GET  /network/<id>/region/<region name>/param/<param name>
    //     Get the value of a Parameter on a Region.
    svr.Get("/network/.*/region/.*/param/.*", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string region_name = flds[4];
      std::string param_name = flds[6];

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->get_param_request(id, region_name, param_name);
      res.set_content(result + "\n", "application/json");
    });

    //  PUT  /network/<id>/input/<input name>?data=<url encoded JSON data>
    //       Set the value of the network's input. The <data> could also be in the body.
    //  The data is a JSON encoded Array object which includes the type specifier;
    svr.Put("/network/.*/input/.*", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string input_name = flds[4];
      std::string data = req.body;
      auto ix = req.params.find("data");
      if (ix != req.params.end())
        data = ix->second;

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->put_input_request(id, input_name, data);
      res.set_content(result + "\n", "application/json");
    });

    //  GET  /network/<id>/region/<region name>/input/<input name>
    //       Get the value of a region's input. Returns a JSON ecoded Array object.
    svr.Get("/network/.*/region/.*/input/.*", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string region_name = flds[4];
      std::string input_name = flds[6];

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->get_input_request(id, region_name, input_name);
      res.set_content(result + "\n", "application/json");
    });

    //  GET  /network/<id>/region/<region name>/output/<output name>
    // Get a specific Output of a region. Returns a JSON encoded Array object.
    svr.Get("/network/.*/region/.*/output/.*", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string region_name = flds[4];
      std::string output_name = flds[6];

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->get_output_request(id, region_name, output_name);
      res.set_content(result + "\n", "application/json");
    });

    //  DELETE /network/<id>/region/<region name>
    //       Deletes a region. Must not be in any links.
    svr.Delete("/network/.*/region/.*", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string region_name = flds[4];

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->delete_region_request(id, region_name);
      res.set_content(result + "\n", "application/json");
    });

    //  DELETE /network/<id>/link/<name>
    //       Deletes a link.
    svr.Delete("/network/.*/link/.*/.*", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string source_name = flds[4];
      std::string dest_name = flds[5];

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->delete_link_request(id, source_name, dest_name);
      res.set_content(result + "\n", "application/json");
    });

    //  DELETE /network/<id>/ALL
    //       Deletes the entire Network object
    svr.Delete("/network/.*/ALL", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->delete_network_request(id);
      res.set_content(result + "\n", "application/json");
    });

    // GET /network/<id>/run?iterations=<iterations>
    //    Execute the NetworkAPI <iterations> times.
    //           iterations are optional; defaults to 1.
    svr.Get("/network/.*/run", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string iterations = "1";
      auto ix = req.params.find("iterations");
      if (ix != req.params.end())
        iterations = ix->second;

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->run_request(id, iterations);
      res.set_content(result + "\n", "application/json");
    });

    //  GET  /network/<id>/region/<region name>/command?data=<command>
    //       Execute a predefined command on a region. <command> must start with the
    //       command name followed by the arguments.
    //       The data could also be in the body.
    svr.Get("/network/.*/region/.*/command", [](const Request &req, Response &res) {
      std::vector<std::string> flds = Path::split(req.path, '/');
      std::string id = flds[2];
      std::string region_name = flds[4];
      std::string command = req.body;
      auto ix = req.params.find("data");
      if (ix != req.params.end())
        command = ix->second;

      RESTapi *interface = RESTapi::getInstance();
      std::string result = interface->command_request(id, region_name, command);
      res.set_content(result + "\n", "application/json");
    });

    //  GET /stop
    //    Halt the server.
    svr.Get("/stop", [&](const Request & /*req*/, Response & /*res*/) { svr.stop(); });

    // What to do if there is an error in the server.
    svr.set_error_handler([](const Request & /*req*/, Response &res) {
      const char *fmt = "ERROR: Status %d";
      char buf[BUFSIZ];
      snprintf(buf, sizeof(buf), fmt, res.status);
      res.set_content(buf, "application/json");
    });
  }

  // How to perform logging.
  inline void set_logger(httplib::Logger logger) { svr.set_logger(logger); }

  inline void listen(int port, std::string net_interface) {
    // Enter the server listen loop.
    svr.listen(net_interface.c_str(), port);
  }

  inline std::string url_encode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
      std::string::value_type c = (*i);

      // Keep alphanumeric and other accepted characters intact
      if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
        escaped << c;
        continue;
      }

      // Any other characters are percent-encoded
      escaped << std::uppercase;
      escaped << '%' << std::setw(2) << int((unsigned char)c);
      escaped << std::nouppercase;
    }

    return escaped.str();
  }

  inline bool is_running() { return svr.is_running(); }
  inline void stop() { svr.stop(); }

private:
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  SSLServer svr(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);
#else
  Server svr;
#endif
};
