#!/usr/bin/python
# ----------------------------------------------------------------------
# HTM Community Edition of NuPIC
# Copyright (C) 2020, Numenta, Inc.
#
# author: David Keeney, dkeeney@gmail.com, March 2020
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero Public License for more details.
#
# You should have received a copy of the GNU Affero Public License
# along with this program.  If not, see http://www.gnu.org/licenses.
# ----------------------------------------------------------------------
# REQUIREMENTS:
#     pip install requests
#
# USAGE:
#     First, in a seperate window or machine, start the server.  See examples/rest/README.md
#     client.py [host [port]]
#        host default: localhost
#        port default: 8050
#

import sys
import math

import htm_rest_api

verbose = True

def main(argv):
  EPOCHS = 5                   # The number of iterations
  host = '127.0.0.1' # Get local machine IP, the default.
  port = 8050                 # The default port
  if len(argv) >= 2:
    host = argv[1]
  if len(argv) >= 3:
    port = argv[2]
  URL = 'http://'+host+':'+str(port)

  # See docs/NetworkAPI_REST.md for Network configuration string.
  config = """
    {network: [
       {addRegion: {name: "encoder", type: "RDSEEncoderRegion", params: {size: 1000, sparsity: 0.2, radius: 0.03, seed: 2019, noise: 0.01}}},
       {addRegion: {name: "sp", type: "SPRegion", params: {columnCount: 2048, globalInhibition: true}}},
       {addRegion: {name: "tm", type: "TMRegion", params: {cellsPerColumn: 8, orColumnOutputs: true}}},
       {addLink:   {src: "encoder.encoded", dest: "sp.bottomUpIn"}},
       {addLink:   {src: "sp.bottomUpOut", dest: "tm.bottomUpIn"}}
    ]}"""

  net = htm_rest_api.NetworkRESTBase(config, host = URL, verbose=verbose)

  # Send the config string and obtain a token for the created Network object instance.
  net.create()

  # iterate EPOCHS times
  x = 0.00
  for e in range(EPOCHS):
    # Data is a sine wave,    (Note: first iteration is for x=0.01, not 0)
    x += 0.01 # advance one step, 0.01 radians
    s = math.sin(x)

    # Send set parameter message to feed "sensedValue" parameter data into RDSE encoder for this iteration.
    net.put_region_param('encoder', 'sensedValue', '{:f}'.format(s))

    # Execute one iteration of the Network object
    net.run(100)

  # Retreive the final anomaly score from the TM object's 'anomaly' output.
  score = net.get_region_output('tm', 'anomaly')[0]
  
  print('Anomaly Score: ' + str(score) )
  #Note: Anomaly score will be 1 until there have been enough iterations to 'learn' the pattern.


if __name__ == "__main__":
   main(sys.argv)
      