# ----------------------------------------------------------------------
# HTM Community Edition of NuPIC
# Copyright (C) 2014-2015, Numenta, Inc.
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

import unittest
import pytest
import pickle
import sys
import os

from htm.bindings.sdr import SDR
from htm.algorithms import TemporalMemory as TM
import numpy as np

debugPrint = False

parameters1 = {

 'sp': {'boostStrength': 3.0,
        'columnCount': 1638,
        'localAreaDensity': 0.04395604395604396,
        'potentialPct': 0.85,
        'synPermActiveInc': 0.04,
        'synPermConnected': 0.13999999999999999,
        'synPermInactiveDec': 0.006},
 'tm': {'activationThreshold': 17,
        'cellsPerColumn': 13,
        'initialPerm': 0.21,
        'maxSegmentsPerCell': 128,
        'maxSynapsesPerSegment': 64,
        'minThreshold': 10,
        'newSynapseCount': 32,
        'permanenceDec': 0.1,
        'permanenceInc': 0.1},
}

class TemporalMemoryBindingsTest(unittest.TestCase):

  def testCompute(self):
    """ Check that there are no errors in call to compute. """
    inputs = SDR( 100 ).randomize( .05 )

    tm = TM( inputs.dimensions)
    tm.compute( inputs, True )

    active = tm.getActiveCells()
    self.assertTrue( active.getSum() > 0 )

  @pytest.mark.skipif(sys.version_info < (3, 6), reason="Fails for python2 with segmentation fault")
  def testNupicTemporalMemoryPickling(self):
    """Test pickling / unpickling of NuPIC TemporalMemory."""

    # Simple test: make sure that dumping / loading works...
    inputs = SDR( 100 ).randomize( .05 )
    tm = TM( inputs.dimensions)
    for _ in range(10):
      tm.compute( inputs, True)

    pickledTm = pickle.dumps(tm, 2)
    tm2 = pickle.loads(pickledTm)

    self.assertEqual(tm.numberOfCells(), tm2.numberOfCells(),
                     "Simple NuPIC TemporalMemory pickle/unpickle failed.")


  @pytest.mark.skip(reason="Fails with rapidjson internal assertion -- indicates a bad serialization")
  def testNupicTemporalMemorySavingToString(self):
    """Test writing to and reading from TemporalMemory."""
    inputs = SDR( 100 ).randomize( .05 )
    tm = TM( inputs.dimensions)
    for _ in range(10):
      tm.compute( inputs, True)

    # Simple test: make sure that writing/reading works...
    s = tm.writeToString()

    tm2 = TM()
    tm2.loadFromString(s)

    self.assertEqual(str(tm), str(tm),
                     "TemporalMemory write to/read from string failed.")

  def testNupicTemporalMemorySerialization(self):
     # Test serializing with each type of interface.
    inputs = SDR( 100 ).randomize( .05 )
    tm = TM( inputs.dimensions)
    for _ in range(10):
      tm.compute( inputs, True)

    #print(str(tm))

    # The TM now has some data in it, try serialization.
    file = "temporalMemory_test_save2.bin"
    tm.saveToFile(file)
    tm3 = TM()
    tm3.loadFromFile(file)
    self.assertEqual(str(tm), str(tm3), "TemporalMemory serialization (using saveToFile/loadFromFile) failed.")
    os.remove(file)

  def testPredictiveCells(self):
    """
    This tests that we don't get empty predicitve cells
    """

    tm = TM(
        columnDimensions=(parameters1["sp"]["columnCount"],),
        cellsPerColumn=parameters1["tm"]["cellsPerColumn"],
        activationThreshold=parameters1["tm"]["activationThreshold"],
        initialPermanence=parameters1["tm"]["initialPerm"],
        connectedPermanence=parameters1["sp"]["synPermConnected"],
        minThreshold=parameters1["tm"]["minThreshold"],
        maxNewSynapseCount=parameters1["tm"]["newSynapseCount"],
        permanenceIncrement=parameters1["tm"]["permanenceInc"],
        permanenceDecrement=parameters1["tm"]["permanenceDec"],
        predictedSegmentDecrement=0.0,
        maxSegmentsPerCell=parameters1["tm"]["maxSegmentsPerCell"],
        maxSynapsesPerSegment=parameters1["tm"]["maxSynapsesPerSegment"],
    )

    activeColumnsA = SDR(parameters1["sp"]["columnCount"])
    activeColumnsB = SDR(parameters1["sp"]["columnCount"])

    activeColumnsA.randomize(sparsity=0.4,seed=1)
    activeColumnsB.randomize(sparsity=0.4,seed=1)

    # give pattern A - bursting
    # give pattern B - bursting
    # give pattern A - should be predicting

    tm.activateDendrites(True)
    self.assertTrue(tm.getPredictiveCells().getSum() == 0)
    predictiveCellsSDR = tm.getPredictiveCells()
    tm.activateCells(activeColumnsA,True)

    _print("\nColumnsA")
    _print("activeCols:"+str(len(activeColumnsA.sparse)))
    _print("activeCells:"+str(len(tm.getActiveCells().sparse)))
    _print("predictiveCells:"+str(len(predictiveCellsSDR.sparse)))



    tm.activateDendrites(True)
    self.assertTrue(tm.getPredictiveCells().getSum() == 0)
    predictiveCellsSDR = tm.getPredictiveCells()
    tm.activateCells(activeColumnsB,True)

    _print("\nColumnsB")
    _print("activeCols:"+str(len(activeColumnsB.sparse)))
    _print("activeCells:"+str(len(tm.getActiveCells().sparse)))
    _print("predictiveCells:"+str(len(predictiveCellsSDR.sparse)))

    tm.activateDendrites(True)
    self.assertTrue(tm.getPredictiveCells().getSum() > 0)
    predictiveCellsSDR = tm.getPredictiveCells()
    tm.activateCells(activeColumnsA,True)

    _print("\nColumnsA")
    _print("activeCols:"+str(len(activeColumnsA.sparse)))
    _print("activeCells:"+str(len(tm.getActiveCells().sparse)))
    _print("predictiveCells:"+str(len(predictiveCellsSDR.sparse)))

  def testTMexposesConnections(self):
    """TM exposes internal connections as read-only object"""
    tm = TM(columnDimensions=[2048], connectedPermanence=0.42)
    self.assertAlmostEqual(tm.connections.connectedThreshold, 0.42, places=3)

  def testGetMethods(self):
    """check if the get-Methodes return correct values"""
    # first create instance of Tm with some parameters
    tm = TM(
        columnDimensions=(parameters1["sp"]["columnCount"],),
        cellsPerColumn=parameters1["tm"]["cellsPerColumn"],
        activationThreshold=parameters1["tm"]["activationThreshold"],
        initialPermanence=parameters1["tm"]["initialPerm"],
        connectedPermanence=parameters1["sp"]["synPermConnected"],
        minThreshold=parameters1["tm"]["minThreshold"],
        maxNewSynapseCount=parameters1["tm"]["newSynapseCount"],
        permanenceIncrement=parameters1["tm"]["permanenceInc"],
        permanenceDecrement=parameters1["tm"]["permanenceDec"],
        predictedSegmentDecrement=0.0,
        maxSegmentsPerCell=parameters1["tm"]["maxSegmentsPerCell"],
        maxSynapsesPerSegment=parameters1["tm"]["maxSynapsesPerSegment"],
        checkInputs = True
    )

    # second call each function to get the values
    columnDimension = tm.getColumnDimensions()
    cellsPerColumn = tm.getCellsPerColumn()
    activationThreshold = tm.getActivationThreshold()
    initialPermanence = tm.getInitialPermanence()
    connectedPermanence = tm.getConnectedPermanence()
    minThreshold = tm.getMinThreshold()
    maxNewSynapseCount = tm.getMaxNewSynapseCount()
    permanenceIncrement = tm.getPermanenceIncrement()
    permanenceDecrement = tm.getPermanenceDecrement()
    predictedSegmentDecrement = tm.getPredictedSegmentDecrement()
    maxSegmentsPerCell = tm.getMaxSegmentsPerCell()
    maxSynapsesPerSegment = tm.getMaxSynapsesPerSegment()
    checkInputs = tm.getCheckInputs()

    # third and final, compare the input parameters with the parameters from the get-Methods
    # floating point numbers maybe not 100 % equal...
    self.assertEqual([parameters1["sp"]["columnCount"]], columnDimension, "using method (getColumnDimension) failed")
    self.assertEqual(parameters1["tm"]["cellsPerColumn"], cellsPerColumn, "using method (getCellsPerColumn) failed")
    self.assertEqual(parameters1["tm"]["activationThreshold"], activationThreshold, "using (getActivationThreshold) failed")
    self.assertAlmostEqual(parameters1["sp"]["synPermConnected"], connectedPermanence, msg="using method (getConnectedPermanence) failed")
    self.assertEqual(parameters1["tm"]["minThreshold"], minThreshold, "using method (getMinThreshold) failed")
    self.assertEqual(parameters1["tm"]["newSynapseCount"], maxNewSynapseCount, "using method (getMaxNewSynapseCount) failed")
    self.assertAlmostEqual(parameters1["tm"]["permanenceInc"], permanenceIncrement, msg="using method (getPermanenceIncrement) failed")
    self.assertAlmostEqual(parameters1["tm"]["permanenceDec"], permanenceDecrement, msg="using method (getPermanenceDecrement) failed")
    self.assertAlmostEqual(0.0, predictedSegmentDecrement, msg="using Methode (getPredictedSegmentDecrement) failed")
    self.assertEqual(parameters1["tm"]["maxSegmentsPerCell"], maxSegmentsPerCell, "using Method (getMaxSegmentsPerCell) failed")
    self.assertEqual(parameters1["tm"]["maxSynapsesPerSegment"], maxSynapsesPerSegment, "using Method (getMaxSynapsesPerSegment) failed")
    self.assertEqual(True, checkInputs, "using Method (getCheckInputs) failed")

def _print(txt):
    if debugPrint:
        print(txt)

if __name__ == "__main__":
  unittest.main()
