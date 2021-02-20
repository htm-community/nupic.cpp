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
 * REST Interface for the NetworkAPI
 *
 * NOTE: The REST protocol is basically stateless.  Our application is not
 *       stateless because it stores the Network class as a resource and
 *       operations cause changes to the Network class.  But the protocol
 *       is stateless which means that each message contains everything
 *       needed for the server to carry out the operation.  We use the
 *       'id' field as the identifier for the Network class instance that
 *       is to be operated on.
 *
 *       Each "network create" message will create a Network object.  Each subsequent "run"
 *       message from the same client will use the same Network object to perform operations.
 *       Multiple clients can be using the same server at the same time.
 *
 *       For this to happen we need to know which Network object the "run" 
 *       message should apply to. Since the protocol is stateless, the "run" 
 *       messages must carry their own state or context. That context is a 
 *       resouce id that was returned from a previous "network create"  message 
 *       that created a Network context and stashed it in a resouces map
 *       indexed by id.
 *
 *       There is a maximum of 9999 active Network class resources available
 *       if you allow REST to assign the id's.  Otherwise the program imposes no limits.
 *
 *       The methods in the class are called from examples/rest/server_core.hpp
 *       which is compiled with the rest server.  An application can use the server
 *       AS-IS or replace the server and server_core.hpp to sute its needs.
 *
 * LIMITATIONS:
 *       1) Only built-in C++ regions can be used.  There are plans to
 *          eventually allow connecting to Python regions and dynamically 
 *          loaded C++ regions.
 *       2) Save and Load operations via REST on the Network object have not 
 *          yet been implemented.
 */

#ifndef NTA_REST_API_HPP
#define NTA_REST_API_HPP


#include <htm/engine/Network.hpp>

namespace htm {

class RESTapi
{
public:
  static RESTapi* getInstance();

  /**
   *
   * Constructor for the RESTapi class
   *
   */
  RESTapi();
  ~RESTapi();

  /**
  * @b Description
  * Handler for a "create network resource" request message.
  * This will create a new Network object for this context and configure it.
  *
  * @param id  The id to use for this new Network object.  If this is an empty string
  *         the next available id will be used.  All subsequet calls
  *         of any message type should use the id returned by this function.
  *         NOTE: if there is already a Network object corresponding to a specified id
  *               that Network object will be deleted and a new one created.
  *
  * @param conf  A YAML or JSON string containing the configuration for the Network class.
  *              See Network::configure() for syntax.
  *
  * @retval On success it returns the id (JSON encoded) to use with this resource context. 
  *         Otherwise it returns the JSON encoded error message starting with "ERROR: ".
  */
  std::string create_network_request(const std::string &id, const std::string &conf);

  /**
   * @b Description:
   * Handler for a PUT "input" request message.
   * This will pass the attached data to the specified region's input.
   *
   * @param input_name  The name of the input that is to receive the data.
   *                    Give the name of a input as defined by the link source
   *                    name you are configuring.
   *
   * @param data        The serialized data itself in JSON format. The following is expected:
   *                      { data: [ <array of elements comma seprated> ]}
   *                    The data portion must be a sequence, even if only one element.
   *                    If blank, no data is set.
   *
   * @param id          Identifier for the resource context (a Network class instance).
   *                    Subsequent calls related to this Network object should use this id.
   *                    Note: the id parameter can be an empty string.  If empty the program
   *                          will generate and use the next available id; a four digit string
   *                          between "0001" and "9999".  If the id exceeds "9999" it will
   *                          wrap and re-use id's for which Network objects have been deleted.
   *
   *                          Otherwise it will use and return the specified id. The specified id
   *                          does not have to be numeric.  However, if it is not compatible with
   *                          URL syntax the returned id will be a URLencoded copy of the requested id.
   *
   *                          If a Network object is already associated with the specified id the
   *                          program will remove the existing Network object and create a new one.
   *
   *
   * @retval            If successful it returns "OK".
   *                    Otherwise returns JSON encoded error message starting with "ERROR: ".
   */
  std::string put_input_request(const std::string &id,
                                const std::string &input_name,
                                const std::string &data);


  /**
   * @b Description:
   * Handler for a GET "input" request message.
   * This will return the data from the specified region's input.
   *
   * @param region_name The name of the region for the parameter that is to get the data from.
   *                    For example if "encoder" is the name assigned to your ScalarSensor region in
   *                    the Network's configuration, enter "encoder".
   *
   * @param input_name  The name of the input that is to receive the data.
   *                    Give the name of a input as defined by the region Spec for
   *                    the region you are configuring.
   *                    For example;  For a ScalarSensor region, such a parameter is "values".
   *
   * @param id          Identifier for the resource context (a Network class instance).
   *                    Client should pass the id returned by the previous "configure"
   *                    request message.
   *
   * @retval            If success returns JSON encoded sequence containing the input array.
   *                    Otherwise returns a JSON encoded error message starting with "ERROR: ".
   */
  std::string get_input_request(const std::string &id, 
                                const std::string &region_name, 
                                const std::string &input_name);



  /**
   * @b Description:
   * Handler for a GET "output" request message.
   * This will capture the specified output values and return a response.
   *
   * @param id  Identifier for the resource context (a Network class instance).
   *            Client should pass the id returned by the previous "configure"
   *            request message.
   *
   *
   * @param region_name The name of the region for the parameter that is to receive the data.
   *                    For example if "tm" is the name assigned to your TMRegion region in
   *                    the Network's configuration, enter "tm".
   *
   * @param output_name The name of the output that is to get data from.
   *                    Give the name of an output as defined by the region Spec for
   *                    the region you are configuring.
   *                    For example;  For a TMRegion region, such a parameter is "activeCells".
   *
   * @retval            If success returns JSON encoded sequence containing the output array.
   *                    Otherwise returns a JSON encoded error message starting with "ERROR: ".
   */
  std::string get_output_request(const std::string &id, 
                                 const std::string &region_name, 
                                 const std::string &output_name);


  /**
   * @b Description:
   * Handler for a PUT "param" request message.
   * This will pass the attached data to the specified region's parameter.
   *
   * @param id          Identifier for the resource context (a Network class instance).
   *                    Client should pass the id returned by the previous "configure"
   *                    request message.
   *
   * @param region_name The name of the region for the parameter that is to receive the data.  
   *                    For example if "sp" the name assigned to your SPRegion region in
   *                    the configuration, enter "sp".
   * @param param_name  The name of the parameter that is to receive the data.
   *                    Give the name of a parameter as defined by the region Spec for
   *                    the region you are configuring. Must be a ReadWrite parameter.
   *                    For example;  For an SPRegion region, such a parameter is "potentialRadius".
   *
   * @retval            If success returns JSON encoded sequence containing the parameter data.
   *                    Otherwise returns a JSON encoded error message starting with "ERROR: ".
   *                    The data portion must be a sequence, even if only one element.
   *                    If blank, it returns JSON null.
   *
   * @retval            If success returns "OK".
   *                    Otherwise returns error message starting with "ERROR: ".
   */
  std::string put_param_request(const std::string &id, 
                                const std::string &region_name, 
                                const std::string &param_name,
                                const std::string &data);


  /**
   * @b Description:
   * Handler for a GET "param" request message.
   * This will capture the specified parameter values and return a response.
   *
   * @param id  Identifier for the resource context (a Network class instance).
   *            Client should pass the id returned by the previous "configure"
   *            request message.
   *
   *
   * @param region_name The name of the region for the parameter that is to receive the data.
   *                    For example if "sp" the name assigned to your SPRegion region in
   *                    the configuration, enter "sp".
   * @param param_name  The name of the parameter that is to receive the data.
   *                    Give the name of a parameter as defined by the region Spec for
   *                    the region you are configuring. Can be a ReadWrite, ReadOnly, or Create parameter.
   *                    For example;  For an TMRegion region, such a parameter is "cellsPerColumn".
   *
   * @retval            If success returns the requested data in JSON format.
   *                    Otherwise returns error message starting with "ERROR: ".
   */
  std::string get_param_request(const std::string &id, 
                                const std::string &region_name, 
                                const std::string &param_name);


  /**
   * @b Description:
   * Handler for a DELETE "link" request message.
   * This will remove the link with matching source and destination names.
   *
   * @param id  Identifier for the resource context (a Network class instance).
   *            Client should pass the id returned by the previous "configure"
   *            request message.
   *
   *
   * @param source_name The name of the region that is the source of data.
   *
   * @param dest_name   The name of the region that is destination of data.
   *
   * @retval            If success returns Ok.
   *                    Otherwise returns error message starting with "ERROR: ".
   */
  std::string delete_link_request(const std::string &id, 
                                  const std::string &source_name, 
                                  const std::string &dest_name);


  /**
   * @b Description:
   * Handler for a DELETE "region" request message.
   * This will remove the region with matching name.
   *
   * @param id  Identifier for the resource context (a Network class instance).
   *            Client should pass the id returned by the previous "configure"
   *            request message.
   *
   *
   * @param region_name The name of the region that is to be removed.
   *
   * @retval            If success returns Ok.
   *                    Otherwise returns error message starting with "ERROR: ".
   */
  std::string delete_region_request(const std::string &id, const std::string &region_name);

    /**
   * @b Description:
   * Handler for a DELETE "network" request message.
   * This will remove the entire Network object.
   *
   * @param id  Identifier for the resource context (a Network class instance).
   *            Client should pass the id returned by the previous "configure"
   *            request message.
   *
   *
   * @retval            If success returns Ok.
   *                    Otherwise returns error message starting with "ERROR: ".
   */
  std::string delete_network_request(const std::string &id);


  /**
   * @b Description:
   * Handler for a "run" request message.
   * This will initialize the Network object if needed and execute the 
   * Network class with the indicated number of iterations. 
   *
   * @param id  Identifier for the resource context (a Network class instance). 
   *            Client should pass the id returned by the previous "configure" 
   *            request message.
   *
   * @param iterations  The number of iterations to run.  Normally this is 1, the default. 
   *
   * @retval            If success returns "OK".
   *                    Otherwise returns error message starting with "ERROR: ".
   */
  std::string run_request(const std::string &id, 
                          const std::string &iterations);

  /**
   * @b Description:
   * Execute a command on a region.
   * Only commands defined in the Spec may be executed.
   *
   * @param id  Identifier for the resource context (a Network class instance).
   *            Client should pass the id returned by the previous "configure"
   *            request message.
   *
   * @param region_name  The name of the region.
   *
   * @param command   The command string.
   *
   * @retval            Result of the command execution.
   *                    Otherwise returns error message starting with "ERROR: ".
   */
  std::string command_request(const std::string &id, const std::string &region_name, const std::string& command);



private:
  struct ResourceContext {
    std::string id;               // id for the resource
    time_t t;                     // last access time
    std::shared_ptr<Network> net; // context for this resource instance
  };

  // A map of open resources. 
  std::map<std::string, ResourceContext> resource_;
  std::string get_new_id_(); 
};

} // namespace htm

#endif // NTA_REST_API_HPP
