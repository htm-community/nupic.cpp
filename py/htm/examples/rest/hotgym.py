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

import csv
import datetime
import os
import numpy as np
import math

from htm_rest_api import NetworkConfig, NetworkREST, get_classifer_predict, INPUT

_EXAMPLE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_INPUT_FILE_PATH = os.path.join(_EXAMPLE_DIR, "gymdata.csv")

default_parameters = {
  # there are 2 (3) encoders: "value" (RDSE) & "time" (DateTime weekend, timeOfDay)
  'enc': {
    "value": {
      'resolution': 0.88,
      'size': 700,
      'sparsity': 0.02
    },
    "time": {
      'timeOfDay': (30, 1),
      'weekend': 21
    }
  },
  'sp': {
    'boostStrength': 3.0,
    'columnCount': 1638,
    'localAreaDensity': 0.04395604395604396,
    'potentialPct': 0.85,
    'synPermActiveInc': 0.04,
    'synPermConnected': 0.13999999999999999,
    'synPermInactiveDec': 0.006
  },
  'tm': {
    'activationThreshold': 17,
    'cellsPerColumn': 13,
    'initialPerm': 0.21,
    'maxSegmentsPerCell': 128,
    'maxSynapsesPerSegment': 64,
    'minThreshold': 10,
    'newSynapseCount': 32,
    'permanenceDec': 0.1,
    'permanenceInc': 0.1
  }
}


def main(parameters=default_parameters, argv=None, verbose=True):
  if verbose:
    import pprint
    print("Parameters:")
    pprint.pprint(parameters, indent=4)
    print("")

  # Read the input file.
  records = []
  with open(_INPUT_FILE_PATH, "r") as fin:
    reader = csv.reader(fin)
    headers = next(reader)
    next(reader)
    next(reader)
    for record in reader:
      records.append(record)

  net = NetworkREST(verbose=verbose)
  # Make the Encoders.  These will convert input data into binary representations.
  dateRegion = net.add_region(
    'dateEncoder', 'DateEncoderRegion',
    dict(timeOfDay_width=parameters["enc"]["time"]["timeOfDay"][0],
         timeOfDay_radius=parameters["enc"]["time"]["timeOfDay"][1],
         weekend_width=parameters["enc"]["time"]["weekend"]))

  scalarRegion = net.add_region(
    'scalarEncoder', 'RDSEEncoderRegion',
    dict(size=parameters["enc"]["value"]["size"],
         sparsity=parameters["enc"]["value"]["sparsity"],
         resolution=parameters["enc"]["value"]["resolution"]))

  # Make the HTM.  SpatialPooler & TemporalMemory & associated tools.
  spParams = parameters["sp"]
  spRegion = net.add_region(
    'sp',
    'SPRegion',
    dict(
      columnCount=spParams['columnCount'],
      potentialPct=spParams["potentialPct"],
      potentialRadius=0,  # 0 is auto assign as inputWith
      globalInhibition=True,
      localAreaDensity=spParams["localAreaDensity"],
      synPermInactiveDec=spParams["synPermInactiveDec"],
      synPermActiveInc=spParams["synPermActiveInc"],
      synPermConnected=spParams["synPermConnected"],
      boostStrength=spParams["boostStrength"],
      wrapAround=True))

  tmParams = parameters["tm"]
  tmRegion = net.add_region(
    'tm', 'TMRegion',
    dict(columnCount=spParams['columnCount'],
         cellsPerColumn=tmParams["cellsPerColumn"],
         activationThreshold=tmParams["activationThreshold"],
         initialPermanence=tmParams["initialPerm"],
         connectedPermanence=spParams["synPermConnected"],
         minThreshold=tmParams["minThreshold"],
         maxNewSynapseCount=tmParams["newSynapseCount"],
         permanenceIncrement=tmParams["permanenceInc"],
         permanenceDecrement=tmParams["permanenceDec"],
         predictedSegmentDecrement=0.0,
         maxSegmentsPerCell=tmParams["maxSegmentsPerCell"],
         maxSynapsesPerSegment=tmParams["maxSynapsesPerSegment"]))

  clsrRegion = net.add_region('clsr', 'ClassifierRegion', {'learn': True})

  net.add_link(dateRegion, spRegion, 'encoded', 'bottomUpIn')
  net.add_link(scalarRegion, spRegion, 'encoded', 'bottomUpIn')
  net.add_link(spRegion, tmRegion, 'bottomUpOut', 'bottomUpIn')
  net.add_link(tmRegion, clsrRegion, 'bottomUpOut', 'pattern')
  net.add_link(INPUT, clsrRegion, 'clsr_bucket', 'bucket', 1)

  net.create()

  # Iterate through every datum in the dataset, record the inputs & outputs.
  inputs = []
  anomaly = []
  anomalyProb = []
  predictions = []
  for count, record in enumerate(records):

    # Convert date string into Python date object.
    dateString = datetime.datetime.strptime(record[0], "%m/%d/%y %H:%M")
    # Convert data value string into float.
    consumption = float(record[1])
    inputs.append(consumption)

    # Call the encoders to create bit representations for each value.  These are SDR objects.
    dateRegion.param('sensedTime', int(dateString.timestamp()))
    scalarRegion.param('sensedValue', consumption)
    net.input('clsr_bucket', consumption)

    # Predict what will happen, and then train the predictor based on what just happened.
    net.run()
    pred = get_classifer_predict(net, clsrRegion.name)
    pred['anomaly'] = tmRegion.output('anomaly')[0]
    if pred.get('title'):
      predictions.append(pred['title'])
    else:
      predictions.append(float('nan'))

    anomaly.append(pred['anomaly'])

  ## # Calculate the predictive accuracy, Root-Mean-Squared
  accuracy = 0
  accuracy_samples = 0

  for idx, inp in enumerate(inputs):
    val = predictions[idx]
    if not math.isnan(val):
      accuracy += (inp - val)**2
      accuracy_samples += 1

  accuracy = (accuracy / accuracy_samples)**.5
  print("Predictive Error (RMS):", accuracy)

  # Show info about the anomaly (mean & std)
  print("Anomaly Mean", np.mean(anomaly))
  print("Anomaly Std ", np.std(anomaly))

  # Plot the Predictions and Anomalies.
  if verbose:
    try:
      import matplotlib.pyplot as plt
    except:
      print("WARNING: failed to import matplotlib, plots cannot be shown.")
      return -accuracy

    plt.subplot(2, 1, 1)
    plt.title("Predictions")
    plt.xlabel("Time")
    plt.ylabel("Power Consumption")
    plt.plot(np.arange(len(inputs)), inputs, 'red', np.arange(len(inputs)),
             predictions, 'blue')
    plt.legend(labels=('Input', '1 Step Prediction, Shifted 1 step'))

    plt.subplot(2, 1, 2)
    plt.title("Anomaly Score")
    plt.xlabel("Time")
    plt.ylabel("Power Consumption")
    inputs = np.array(inputs) / max(inputs)
    plt.plot(np.arange(len(inputs)), inputs, 'red', np.arange(len(inputs)),
             anomaly, 'blue')
    plt.legend(labels=('Input', 'Anomaly Score'))
    plt.show()

  return -accuracy


if __name__ == "__main__":
  main()
