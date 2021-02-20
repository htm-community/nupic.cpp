/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013-2020, Numenta, Inc.
 *
 * Author: David Keeney, dkeeney@gmail.com   Feb 2020
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
Implementation of the RESTapi class
*/
#include <htm/engine/RESTapi.hpp>
#include <htm/engine/Network.hpp>
#include <htm/engine/Spec.hpp>

const size_t ID_MAX = 9999; // maximum number of generated ids  (this is arbitrary)

using namespace htm;

// Global values (singletons)
static RESTapi rest;
static unsigned int next_id = 1;

RESTapi::RESTapi() {}
RESTapi::~RESTapi() { }

RESTapi* RESTapi::getInstance() { return &rest; }

std::string RESTapi::get_new_id_() {
  // No id was provided so find the next available number.
  // Note: This will return a number between 1 and 9999,
  //       starting with "1" and incrementing on each use with wrap at "9999".
  //       This will never return "0"

  std::map<std::string, ResourceContext>::iterator itr;
  std::string id;
  while (rest.resource_.size() < ID_MAX) {  // limit the total number of generated resources
    unsigned int id_nbr = next_id++;
    if (id_nbr > ID_MAX)
      id_nbr = 1; // allow integer wrap of the id without using a "0" value.
    char buf[10];
    std::snprintf(buf, sizeof(buf), "%d", id_nbr);
    id = buf;

    // Make sure this new session id is not in use.
    itr = resource_.find(id);
    if (itr == resource_.end()) {
      // This is one we can use
      break;
    }
  }
  return id;
}



std::string RESTapi::create_network_request(const std::string &specified_id, const std::string &config) {
  try {
    std::string id = specified_id;
    if (id.empty()) id = get_new_id_();
    ResourceContext obj;
    obj.id = id;
    obj.t = time(0);
    obj.net.reset(new htm::Network);  // Allocate a Network object.

    obj.net->configure(config);
    resource_[id] = obj;              // assign the resource (deleting any previous value)

    return "{\"result\": " + Value::json_string(id) + "}";
  } catch (Exception& e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::put_input_request(const std::string &id,
                                       const std::string &input_name,
                                       const std::string &data) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);

    Value vm;
    vm.parse(data);

    itr->second.net->setInputData(input_name, vm);

    return "{\"result\": \"OK\"}";
  }
  catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::get_input_request(const std::string &id,
                                       const std::string &region_name,
                                       const std::string &input_name) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);
    auto region = itr->second.net->getRegion(region_name);
    const Array &b = region->getInputData(input_name);
    std::string data = b.toJSON();
    std::string type = BasicType::getName(b.getType());
    std::string dim = region->getInputDimensions(input_name).toString(false);

    return "{\"result\": " + data + ", \"type\": \"" + type + "\", \"dim\": " + dim +"}";

  } catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::get_output_request(const std::string &id,
                                        const std::string &region_name,
                                        const std::string &output_name) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);
    auto region = itr->second.net->getRegion(region_name);
    const Array &b = region->getOutputData(output_name);
    std::string data = b.toJSON();
    std::string type = BasicType::getName(b.getType());
    std::string dim = region->getOutputDimensions(output_name).toString(false);

    return "{\"result\": " + data + ", \"type\": \"" + type + "\", \"dim\": " + dim +"}";
  } catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::put_param_request(const std::string &id,
                                       const std::string &region_name,
                                       const std::string &param_name,
                                       const std::string &data) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);

    itr->second.net->getRegion(region_name)->setParameterJSON(param_name, data);

    return "{\"result\": \"OK\"}";
  } catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::get_param_request(const std::string &id,
                                       const std::string &region_name,
                                       const std::string &param_name) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);

    std::string response;
    response = "{\"result\": " + itr->second.net->getRegion(region_name)->getParameterJSON(param_name) + "}";

    return response;
  } catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::delete_region_request(const std::string &id, const std::string &region_name) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);

    itr->second.net->removeRegion(region_name);

    return "{\"result\": \"OK\"}";
  } catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::delete_link_request(const std::string &id,
                                         const std::string &source_name,
                                         const std::string &dest_name) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);

    std::vector<std::string> args;
    args = Path::split(source_name, '.');
    NTA_CHECK(args.size() == 2) << "Expected syntax <region>.<output> for source name. Found " << source_name;
    std::string source_region = args[0];
    std::string source_output = args[1];

    args = Path::split(dest_name, '.');
    NTA_CHECK(args.size() == 2) << "Expected syntax <region>.<input> for destination name. Found " << dest_name;
    std::string dest_region = args[0];
    std::string dest_input = args[1];

    itr->second.net->removeLink(source_region, dest_region, source_output, dest_input);

    return "{\"result\": \"OK\"}";
  } catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::delete_network_request(const std::string &id) {
  try {

    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";

    resource_.erase(itr);

    return "{\"result\": \"OK\"}";
  } catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}


std::string RESTapi::run_request(const std::string &id, const std::string &iterations) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);

    int iter = 1;
    if (!iterations.empty()) {
      iter = std::strtol(iterations.c_str(), nullptr, 10);
    }
    itr->second.net->run(iter);
    return "{\"result\": \"OK\"}";
  }
  catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}

std::string RESTapi::command_request(const std::string& id,
                                     const std::string& region_name,
                                     const std::string& command) {
  try {
    auto itr = resource_.find(id);
    NTA_CHECK(itr != resource_.end()) << "Context for resource '" + id + "' not found.";
    itr->second.t = time(0);

    std::string response;
    std::vector<std::string> args;
    args = Path::split(command, ' ');
    response = itr->second.net->getRegion(region_name)->executeCommand(args);

    return "{\"result\": " + response + "}";
  } catch (Exception &e) {
    return "{\"err\": " + Value::json_string(e.getMessage()) + "}";
  } catch (std::exception& e) {
    return "{\"err\": " + Value::json_string(e.what()) + "}";
  } catch (...) {
    return "{\"err\": " + Value::json_string("Unknown Exception.") + "}";
  }
}
