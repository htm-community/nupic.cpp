# ------------------------------------------------------------------------------
# HTM Community Edition of NuPIC
# Copyright (C) 2019-2020, Li Meng Jun
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero Public License version 3 as published by the Free
# Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
#
# You should have received a copy of the GNU Affero Public License along with
# this program.  If not, see http://www.gnu.org/licenses.
# ------------------------------------------------------------------------------
from urllib.parse import urlencode
import requests
import json
import re


class NetworkRESTError(Exception):
  pass

# returns the decoded JSON response from the server.
def request(method, url, data=None, verbose=False):
  if verbose:
    if data:
      print('{} {}; body: {}'.format(method, url, data))
    else:
      print('{} {}'.format(method, url))
  rsp = requests.request(method, url, data=data)
  if rsp.status_code != requests.codes.ok:
    raise NetworkRESTError('HTTP Error')

  # all responses are JSON encoded.  
  # Expecting {"err": message} or {"result": result value}
  result = json.loads( rsp.text.strip() ); # removes trailing \n and parses JSON.

  if result.get('err'):
    raise NetworkRESTError(result['err'])

  return result['result']


class NetworkRESTBase(object):
  def __init__(self,
               config,
               id=None,
               host='http://127.0.0.1:8050',
               verbose=False):

    if isinstance(config, NetworkConfig):
      config.set_net(self)

    self.config = config
    self.id = id
    self.host = host
    self.verbose = verbose

  def api(self, uri, query=None):
    if query:
      return "{}/network{}?{}".format(self.host, uri, urlencode(query))

    return "{}/network{}".format(self.host, uri)

  def api1(self, uri, query=None):
    return self.api('/{}{}'.format(self.id, uri), query)

  def create(self):
    query = {}
    if self.id:
      query['id'] = self.id
    url = self.api('', query)
    config = self.config
    if isinstance(config, NetworkConfig):
      config = str(config)
    id = str(request('POST', url, config, verbose=self.verbose))

    if self.verbose:
      print('Resource ID: ' + id)

    if self.id and self.id != id:
      raise NetworkRESTError('Network id not match')
    else:
      self.id = id

  def put_region_param(self, region_name, param_name, data):
    url = self.api1('/region/{}/param/{}'.format(region_name, param_name),
                    {'data': data})
    return request('PUT', url, verbose=self.verbose)

  def get_region_param(self, region_name, param_name):
    url = self.api1('/region/{}/param/{}'.format(region_name, param_name))
    return request('GET', url, verbose=self.verbose)

  def input(self, input_name, data):
    url = self.api1('/input/{}'.format(input_name))
    if not isinstance(data, list):
      data = [data]
    data = {'data': data}
    return request('PUT', url, verbose=self.verbose, data=json.dumps(data))

  def get_region_input(self, region_name, input_name):
    url = self.api1('/region/{}/input/{}'.format(region_name, input_name))
    return request('GET', url, verbose=self.verbose)

  def get_region_output(self, region_name, output_name):
    url = self.api1('/region/{}/output/{}'.format(region_name, output_name))
    return request('GET', url, verbose=self.verbose)

  def delete_region(self, region_name):
    url = self.api1('/region/{}'.format(region_name))
    return request('DELETE', url, verbose=self.verbose)

  def delete_link(self, source_name, dest_name):
    url = self.api1('/link/{}/{}'.format(source_name, dest_name))
    return request('DELETE', url, verbose=self.verbose)

  def delete_all(self):
    url = self.api1('/ALL')
    return request('DELETE', url, verbose=self.verbose)

  def run(self, iterations=None):
    query = None if iterations is None else {'iterations': iterations}
    url = self.api1('/run', query)
    return request('GET', url, verbose=self.verbose)

  def execute(self, region_name, command):
    url = self.api1('/region/{}/command'.format(region_name),
                    {'data': command})
    return request('GET', url, verbose=self.verbose)


def get_classifer_predict(net, region_name):
  pred = net.get_region_output(region_name, 'predicted')
  if not pred:
    return {}
  titles = net.get_region_output(region_name, 'titles')
  pdf = net.get_region_output(region_name, 'pdf')

  return {'title': titles[pred[0]], 'prob': pdf[pred[0]]}


class RegionREST(object):
  def __init__(self, name, type, params={}):
    self.name = name
    self.type = type
    self.params = params
    self.net = None

  def set_net(self, net):
    self.net = net

  def input(self, input_name):
    return self.net.get_region_input(self.name, input_name)

  def param(self, param_name, data=None):
    if data is None:
      return self.net.get_region_param(self.name, param_name)

    return self.net.put_region_param(self.name, param_name, data)

  def output(self, output_name):
    return self.net.get_region_output(self.name, output_name)

  def execute(self, command):
    return self.net.execute(self.name, output_name)

  def destory(self):
    return self.net.delete_region(self.name)


INPUT = RegionREST('INPUT', 'RawInput')


class LinkREST(object):
  def __init__(self, source_name, dest_name, source_output, dest_input, dim = None, delay = None):
    self.source_name = source_name
    self.dest_name = dest_name
    self.source_output = source_output
    self.dest_input = dest_input
    self.net = None
    self.dim = dim
    self.delay = delay

  def set_net(self, net):
    self.net = net

  @property
  def src(self):
    return '{}.{}'.format(self.source_name, self.source_output)

  @property
  def dest(self):
    return '{}.{}'.format(self.dest_name, self.dest_input)

  def destory(self):
    return self.net.delete_link(self.src, self.dest)


class NetworkConfig(object):
  def __init__(self):
    self.regions = []
    self.links = []
    self.net = None

  def set_net(self, net):
    self.net = net

    for region in self.regions:
      region.set_net(net)

    for link in self.links:
      link.set_net(net)

  def has_region(self, name):
    for region in self.regions:
      if region.name == name:
        return True

    return False

  def add_region(self, name, type, params={}):
    if self.has_region(name):
      raise NetworkRESTError('Region {} is already exists.'.format(name))

    region = RegionREST(name, type, params)
    region.set_net(self.net)

    self.regions.append(region)

    return region

  def add_link(self, source_region, dest_region, source_output, dest_input, dim = None, delay = None):
    if not self.has_region(source_region.name) and source_region.name != INPUT.name:
      raise NetworkRESTError('Region {} is not found.'.format(
        source_region.name))
    if not self.has_region(dest_region.name):
      raise NetworkRESTError('Region {} is not found.'.format(
        dest_region.name))

    link = LinkREST(source_region.name, dest_region.name, source_output,
                    dest_input, dim, delay)
    link.set_net(self.net)
    self.links.append(link)

    return link

  def __str__(self):
    network = []

    for region in self.regions:
      network.append({
        'addRegion': {
          'name': region.name,
          'type': region.type,
          'params': region.params
        }
      })
    for link in self.links:
      params = {'src': link.src, 'dest': link.dest}
      if link.delay is not None:
        params['delay'] = link.delay
      if link.dim is not None:
        params['dim'] = link.dim
      network.append({'addLink': params})

    return json.dumps({'network': network}, indent=2)


class NetworkREST(NetworkRESTBase, NetworkConfig):
  def __init__(self, id=None, host='http://127.0.0.1:8050', verbose=False):
    NetworkConfig.__init__(self)
    NetworkRESTBase.__init__(self, self, id, host, verbose)

  def __str__(self):
    return NetworkConfig.__str__(self)
