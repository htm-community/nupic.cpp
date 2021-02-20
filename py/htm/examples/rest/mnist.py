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
""" An MNIST classifier using Spatial Pooler."""

import random
import numpy as np
import sys

# fetch datasets from www.openML.org/
from sklearn.datasets import fetch_openml

from htm_rest_api import NetworkREST, get_classifer_predict, INPUT


def load_ds(name, num_test, shape=None):
  """
    fetch dataset from openML.org and split to train/test
    @param name - ID on openML (eg. 'mnist_784')
    @param num_test - num. samples to take as test
    @param shape - new reshape of a single data point (ie data['data'][0]) as a list. Eg. [28,28] for MNIST
    """
  data = fetch_openml(name, version=1)
  sz = data['target'].shape[0]

  X = data['data']
  if shape is not None:
    new_shape = shape.insert(0, sz)
    X = np.reshape(X, shape)

  y = data['target'].astype(np.int32)
  # split to train/test data
  train_labels = y[:sz - num_test]
  train_images = X[:sz - num_test]
  test_labels = y[sz - num_test:]
  test_images = X[sz - num_test:]

  return train_labels, train_images, test_labels, test_images


def encode(data):
  """
    encode the (image) data
    @param data - raw data
    @param out  - return SDR with encoded data
    """
  dense = data >= np.mean(data)  # convert greyscale image to binary B/W.
  #TODO improve. have a look in htm.vision etc. For MNIST this is ok, for fashionMNIST in already loses too much information
  out = []
  i = 0
  for v0 in dense:
    for v1 in v0:
      if v1:
        out.append(i)
      i += 1
  return out


# These parameters can be improved using parameter optimization,
# see py/htm/optimization/ae.py
# For more explanation of relations between the parameters, see
# src/examples/mnist/MNIST_CPP.cpp
default_parameters = {
  'potentialRadius': 7,
  'boostStrength': 7.0,
  'columnDimensions': (79, 79),
  'dutyCyclePeriod': 1402,
  'localAreaDensity': 0.1,
  'minPctOverlapDutyCycle': 0.2,
  'potentialPct': 0.1,
  'stimulusThreshold': 1,
  'synPermActiveInc': 0.14,
  'synPermConnected': 0.5,
  'synPermInactiveDec': 0.02
}


def main(parameters=default_parameters, argv=None, verbose=True):

  # Load data.
  train_labels, train_images, test_labels, test_images = load_ds(
    'mnist_784', 10000, shape=[28, 28])  # HTM: ~95.6%
  #train_labels, train_images, test_labels, test_images = load_ds('Fashion-MNIST', 10000, shape=[28,28]) # HTM baseline: ~83%

  training_data = list(zip(train_images, train_labels))
  test_data = list(zip(test_images, test_labels))
  random.shuffle(training_data)

  dim = list(train_images[0].shape)

  # Setup the AI.
  net = NetworkREST(verbose=verbose)
  sp = net.add_region(
    'sp',
    'SPRegion',
    dict(
      columnCount=parameters['columnDimensions'][0] *
      parameters['columnDimensions'][1],
      potentialRadius=parameters['potentialRadius'],
      potentialPct=parameters['potentialPct'],
      globalInhibition=True,
      localAreaDensity=parameters['localAreaDensity'],
      stimulusThreshold=int(round(parameters['stimulusThreshold'])),
      synPermInactiveDec=parameters['synPermInactiveDec'],
      synPermActiveInc=parameters['synPermActiveInc'],
      synPermConnected=parameters['synPermConnected'],
      minPctOverlapDutyCycle=parameters['minPctOverlapDutyCycle'],
      dutyCyclePeriod=int(round(parameters['dutyCyclePeriod'])),
      boostStrength=parameters['boostStrength'],
      seed= 0,  # this is important, 0="random" seed which changes on each invocation
      spVerbosity=99,
      wrapAround=False))

  sdrc = net.add_region('clsr', 'ClassifierRegion', {'learn': True})
  net.add_link(sp, sdrc, 'bottomUpOut', 'pattern')
  net.add_link(INPUT, sp, 'sp_input', 'bottomUpIn', dim)
  net.add_link(INPUT, sdrc, 'sdrc_input', 'bucket', [1])

  net.create()

  # Training Loop
  for i in range(len(train_images)):
    if i % 1000 == 0:
      print('iter: ', i, '/', len(train_images))
      pred = get_classifer_predict(net, sdrc.name)
      print(pred)
    img, lbl = training_data[i]
    data = encode(img)
    net.input('sp_input', data)
    net.input('sdrc_input', int(lbl))

    net.run()

  # Testing Loop
  score = 0
  sp.param('learningMode', 0)
  sdrc.param('learn', 'false')
  for img, lbl in test_data:
    data = encode(img)
    net.input('sp_input', data)
    net.run()
    pred = get_classifer_predict(net, sdrc.name)
    if int(lbl) == pred['title']:
      score += 1
  score = score / len(test_data)

  print('Score:', 100 * score, '%')
  return score


# baseline: without SP (only Classifier = logistic regression): 90.1%
# kNN: ~97%
# human: ~98%
# state of the art: https://paperswithcode.com/sota/image-classification-on-mnist , ~99.9%
if __name__ == '__main__':
  sys.exit(main(verbose=False) < 0.95)
