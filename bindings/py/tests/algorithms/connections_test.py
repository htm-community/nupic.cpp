# ----------------------------------------------------------------------
# HTM Community Edition of NuPIC
# Copyright (C) 2019, David McDougall
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
import sys

from htm.bindings.sdr import SDR
from htm.bindings.math import Random
from htm.bindings.algorithms import Connections

import numpy as np

import pickle

NUM_CELLS = 4096

class ConnectionsTest(unittest.TestCase):
  
  def _getPresynapticCells(self, connections, segment, threshold):
    """
    Return a set of presynaptic cells that have synapses to segment.
    """
    return set([connections.presynapticCellForSynapse(synapse) for synapse in connections.synapsesForSegment(segment) 
                if connections.permanenceForSynapse(synapse) >= threshold])

  def testAdaptShouldNotRemoveSegments(self):
    """
    Test that connections are generated on predefined segments.
    """
    random = Random(1981)
    active_cells = np.array(random.sample(np.arange(0, NUM_CELLS, 1, dtype="uint32"), 40), dtype="uint32")
    active_cells.sort()
    
    presynaptic_input = list(range(0, 10))
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input
    
    connections = Connections(NUM_CELLS, 0.51) 
    for i in range(NUM_CELLS):
      seg = connections.createSegment(i, 1)
    
    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        self.assertEqual(len(segments), 1, "Segments were destroyed.")
        segment = segments[0]
        connections.adaptSegment(segment, inputSDR, 0.1, 0.001, False)

        segments = connections.segmentsForCell(cell)
        self.assertEqual(len(segments), 1, "Segments were destroyed.")
        segment = segments[0]

  def testAdaptShouldRemoveSegments(self):
    """
    Test that connections are generated on predefined segments.
    """
    random = Random(1981)
    active_cells = np.array(random.sample(np.arange(0, NUM_CELLS, 1, dtype="uint32"), 40), dtype="uint32")
    active_cells.sort()
    
    presynaptic_input = list(range(0, 10))
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input
    
    connections = Connections(NUM_CELLS, 0.51) 
    for i in range(NUM_CELLS):
      seg = connections.createSegment(i, 2)
      seg = connections.createSegment(i, 2) #create 2 segments on each cell
    
    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        self.assertEqual(len(segments), 2, "Segments were prematurely destroyed.")
        segment = segments[0]
        numSynapsesOnSegment = len(segments)
        connections.adaptSegment(segment, inputSDR, 0.1, 0.001, pruneZeroSynapses=True, segmentThreshold=1) #set to =1 so that segments get always deleted in this test
        segments = connections.segmentsForCell(cell)
        self.assertEqual(len(segments), 1, "Segments were not destroyed.")

  def testAdaptShouldIncrementSynapses(self):
    """
    Test that connections are generated on predefined segments.
    """
    random = Random(1981)
    active_cells = np.array(random.sample(np.arange(0, NUM_CELLS, 1, dtype="uint32"), 40), dtype="uint32")
    active_cells.sort()
    
    presynaptic_input = list(range(0, 10))
    presynaptic_input_set = set(presynaptic_input)
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input
    
    connections = Connections(NUM_CELLS, 0.51) 
    for i in range(NUM_CELLS):
      seg = connections.createSegment(i, 1)
    
    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]
        for c in presynaptic_input:
          connections.createSynapse(segment, c, 0.1)          
        connections.adaptSegment(segment, inputSDR, 0.1, 0.001, True)

        presynamptic_cells = self._getPresynapticCells(connections, segment, 0.2)
        self.assertEqual(presynamptic_cells, presynaptic_input_set, "Missing synapses")

        presynamptic_cells = self._getPresynapticCells(connections, segment, 0.3)
        self.assertEqual(presynamptic_cells, set(), "Too many synapses")

  def testAdaptShouldDecrementSynapses(self):
    """
    Test that connections are generated on predefined segments.
    """
    random = Random(1981)
    active_cells = np.array(random.sample(np.arange(0, NUM_CELLS, 1, dtype="uint32"), 40), dtype="uint32")
    active_cells.sort()
    
    presynaptic_input = list(range(0, 10))
    presynaptic_input_set = set(presynaptic_input)
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input
    
    connections = Connections(NUM_CELLS, 0.51) 
    for i in range(NUM_CELLS):
      seg = connections.createSegment(i, 1)
    
    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]
        for c in presynaptic_input:
          connections.createSynapse(segment, c, 0.1)
          
        connections.adaptSegment(segment, inputSDR, 0.1, 0.0, False)
    
        presynamptic_cells = self._getPresynapticCells(connections, segment, 0.2)
        self.assertEqual(presynamptic_cells, presynaptic_input_set, "Missing synapses")

    presynaptic_input1 = list(range(0, 5))
    presynaptic_input_set1 = set(presynaptic_input1)
    inputSDR.sparse = presynaptic_input1
    
    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]
        connections.adaptSegment(segment, inputSDR, 0.0, 0.1, False)
    

        presynamptic_cells = self._getPresynapticCells(connections, segment, 0.2)
        self.assertEqual(presynamptic_cells, presynaptic_input_set1, "Too many synapses")

        presynamptic_cells = self._getPresynapticCells(connections, segment, 0.1)
        self.assertEqual(presynamptic_cells, presynaptic_input_set, "Missing synapses")



  def testCreateSynapse(self):
    # empty connections, create segment and a synapse
    co = Connections(NUM_CELLS, 0.51)
    self.assertEqual(co.numSynapses(), 0)
    self.assertEqual(co.numSegments(), 0)

    # 1st, create a segment
    seg = co.createSegment(NUM_CELLS-1, 1)
    self.assertEqual(co.numSegments(), 1)

    #1. create a synapse on that segment
    syn1 = co.createSynapse(seg, NUM_CELLS-1, 0.52)
    self.assertEqual(pytest.approx(co.permanenceForSynapse(syn1)), 0.52)
    self.assertEqual(co.numSynapses(), 1)

    #2. creating a duplicit synapse should not crash!
    syn2 = co.createSynapse(seg, NUM_CELLS-1, 0.52)
    self.assertEqual( syn1,  syn2, "creating duplicate synapses should return the same")
    self.assertEqual(co.numSynapses(), 1, "Duplicate synapse, number should not increase")

    #3. create a different synapse
    syn3 = co.createSynapse(seg, 1, 0.52)
    self.assertNotEqual( syn1,  syn3, "creating a different synapse must create a new one")
    self.assertEqual(co.numSynapses(), 2, "New synapse should increase the number")

    #4. create existing synapse with a new value -> should update permanence
    #4.a lower permanence -> keep max()
    syn4 = co.createSynapse(seg, NUM_CELLS-1, 0.11) #all the same just permanence is a lower val
    self.assertEqual( syn1,  syn4, "just updating existing syn")
    self.assertEqual(co.numSynapses(), 2, "Duplicate synapse, number should not increase")
    self.assertEqual(pytest.approx(co.permanenceForSynapse(syn1)), 0.52, "update keeps the larger value")

    #4.b higher permanence -> update
    syn5 = co.createSynapse(seg, NUM_CELLS-1, 0.99) #all the same just permanence is a higher val
    self.assertEqual( syn1,  syn5, "just updating existing syn")
    self.assertEqual(co.numSynapses(), 2, "Duplicate synapse, number should not increase")
    self.assertEqual(pytest.approx(co.permanenceForSynapse(syn1)), 0.99, "updated to the larger permanence value")



  def testDestroySynapse(self):
    # empty connections, create segment seg and a synapse syn
    co = Connections(NUM_CELLS, 0.51)
    seg = co.createSegment(NUM_CELLS-1, 1)
    syn1 = co.createSynapse(seg, NUM_CELLS-1, 0.52)

    # destroy the synapse
    co.destroySynapse(syn1)
    self.assertEqual(co.numSynapses(), 0)

    with pytest.raises(RuntimeError): # NTA_CHECK, data for removed synapse must not be accessible!
      permRemoved = co.permanenceForSynapse(syn1)
      assert permRemoved == perm1

    # double remove should be just ignored
    co.destroySynapse(syn1)
    


  def testNumSynapses(self):
    """
    Test that connections are generated on predefined segments.
    """
    random = Random(1981)
    active_cells = np.array(random.sample(np.arange(0, NUM_CELLS, 1, dtype="uint32"), 40), dtype="uint32")
    active_cells.sort()
    
    presynaptic_input = list(range(0, 10))
    presynaptic_input_set = set(presynaptic_input)
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input
    
    connections = Connections(NUM_CELLS, 0.3) 
    for i in range(NUM_CELLS):
      seg = connections.createSegment(i, 1)
      
    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]
        for c in presynaptic_input:
          connections.createSynapse(segment, c, 0.1)
          
        connections.adaptSegment(segment, inputSDR, 0.1, 0.0, False)
    
        num_synapses = connections.numSynapses(segment)
        self.assertEqual(num_synapses, len(presynaptic_input), "Missing synapses")
        
    self.assertEqual(connections.numSynapses(), len(presynaptic_input) * 40, "Missing synapses")
    

  def testNumConnectedSynapses(self):
    """
    Test that connections are generated on predefined segments.
    """
    random = Random(1981)
    active_cells = np.array(random.sample(np.arange(0, NUM_CELLS, 1, dtype="uint32"), 40), dtype="uint32")
    active_cells.sort()
    
    presynaptic_input = list(range(0, 10))
    presynaptic_input_set = set(presynaptic_input)
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input
    
    connections = Connections(NUM_CELLS, 0.2) 
    for i in range(NUM_CELLS):
      seg = connections.createSegment(i, 1)
    
    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]
        for c in presynaptic_input:
          connections.createSynapse(segment, c, 0.1)
          
        connections.adaptSegment(segment, inputSDR, 0.1, 0.0, False)
    
        connected_synapses = connections.numConnectedSynapses(segment)
        self.assertEqual(connected_synapses, len(presynaptic_input), "Missing synapses")

    presynaptic_input1 = list(range(0, 5))
    presynaptic_input_set1 = set(presynaptic_input1)
    inputSDR.sparse = presynaptic_input1
    
    total_connected = 0

    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]
        connections.adaptSegment(segment, inputSDR, 0.0, 0.1, False)
    
        connected_synapses = connections.numConnectedSynapses(segment)
        self.assertEqual(connected_synapses, len(presynaptic_input1), "Missing synapses")
        
        total_connected += connected_synapses

        connected_synapses = connections.numSynapses(segment)
        self.assertEqual(connected_synapses, len(presynaptic_input), "Missing synapses")

    self.assertEqual(total_connected, len(presynaptic_input1) * 40, "Missing synapses")

  def testComputeActivity(self):
    """
    Test that connections are generated on predefined segments.
    """
    random = Random(1981)
    active_cells = np.array(random.sample(np.arange(0, NUM_CELLS, 1, dtype="uint32"), 40), dtype="uint32")
    active_cells.sort()
    
    presynaptic_input = list(range(0, 10))
    presynaptic_input_set = set(presynaptic_input)
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input
    l = len(presynaptic_input)
    
    connections = Connections(NUM_CELLS, 0.51, False) 
    for i in range(NUM_CELLS):
      seg = connections.createSegment(i, 1)
    
    numActiveConnectedSynapsesForSegment = connections.computeActivity(inputSDR, False)
    for count in numActiveConnectedSynapsesForSegment:
      self.assertEqual(count, 0, "Segment should not be active")

    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]
        for c in presynaptic_input:
          connections.createSynapse(segment, c, 0.1)
        
    numActiveConnectedSynapsesForSegment = connections.computeActivity(inputSDR, False)
    for count in numActiveConnectedSynapsesForSegment:
      self.assertEqual(count, 0, "Segment should not be active")

    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]        
        connections.adaptSegment(segment, inputSDR, 0.5, 0.0, False)
        
    active_cells_set = set(active_cells)
    numActiveConnectedSynapsesForSegment = connections.computeActivity(inputSDR, False)
    for cell, count in enumerate(numActiveConnectedSynapsesForSegment):
      if cell in active_cells_set:
        self.assertEqual(count, l, "Segment should be active")
      else:
        self.assertEqual(count, 0, "Segment should not be active")
        
  def _learn(self, connections, active_cells, presynaptic_input):
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input

    for cell in active_cells:
      segments = connections.segmentsForCell(cell)
      segment = segments[0]
      for c in presynaptic_input:
        connections.createSynapse(segment, c, 0.1)
        
    for cell in active_cells:
        segments = connections.segmentsForCell(cell)
        segment = segments[0]
        connections.adaptSegment(segment, inputSDR, 0.5, 0.0, False)

  def testComputeActivityUnion(self):
    """
    Test that connections are generated on predefined segments.
    """
    random = Random(1981)
    active_cells = np.array(random.sample(np.arange(0, NUM_CELLS, 1, dtype="uint32"), 40), dtype="uint32")
    active_cells.sort()
    
    presynaptic_input1 = list(range(0, 10))
    presynaptic_input1_set = set(presynaptic_input1)
    presynaptic_input2 = list(range(10, 20))
    presynaptic_input2_set = set(presynaptic_input1)
    
    connections = Connections(NUM_CELLS, 0.51, False) 
    for i in range(NUM_CELLS):
      seg = connections.createSegment(i, 1)
    
    self._learn(connections, active_cells, presynaptic_input1)
    self._learn(connections, active_cells, presynaptic_input2)
    
    numSynapses = connections.numSynapses()
    self.assertNotEqual(numSynapses, 40, "There should be a synapse for each presynaptic cell")
    
    active_cells_set = set(active_cells)
    inputSDR = SDR(1024)
    inputSDR.sparse = presynaptic_input1
    
    numActiveConnectedSynapsesForSegment = connections.computeActivity(inputSDR, False)
    for cell, count in enumerate(numActiveConnectedSynapsesForSegment):
      if cell in active_cells_set:
        self.assertNotEqual(count, 0, "Segment should be active")

    inputSDR.sparse = presynaptic_input2
    numActiveConnectedSynapsesForSegment = connections.computeActivity(inputSDR, False)
    for cell, count in enumerate(numActiveConnectedSynapsesForSegment):
      if cell in active_cells_set:
        self.assertNotEqual(count, 0, "Segment should be active")


  def testConnectedThreshold(self):
    conn = Connections(1024, 0.123, False)
    self.assertAlmostEqual(conn.connectedThreshold,  0.123, places=4)

  def testCreateSegment(self):
    co = Connections(NUM_CELLS, 0.51)
    self.assertEqual(co.numSegments(), 0, "there are zero segments yet")

    # create segment
    co.createSegment(NUM_CELLS-1, 20)
    self.assertEqual(co.numSegments(), 1, "created 1 new segment")

    # wrong param
    with pytest.raises(RuntimeError):
      co.createSegment(1, 0) #wrong param maxSegmentsPerCell "0"

    # wrong param - OOB cell
    with pytest.raises(RuntimeError):
      co.createSegment(NUM_CELLS+22, 1)

    # another segment
    co.createSegment(NUM_CELLS-1, 20)
    self.assertEqual(co.numSegments(), 2)

    # segment pruning -> reduce to 1 seg per cell
    co.createSegment(NUM_CELLS-1, 1)
    self.assertEqual(co.numSegments(), 1)


  def testDestroySegment(self):
    co = Connections(NUM_CELLS, 0.51)
    self.assertEqual(co.numSegments(), 0, "there are zero segments yet")

    # removing while no segments exist
    co.destroySegment(1)

    # successfully remove
    seg = co.createSegment(1, 20)
    self.assertEqual(co.numSegments(), 1)
    n = co.numConnectedSynapses(seg) #uses dataForSegment()
    co.destroySegment(seg)
    self.assertEqual(co.numSegments(), 0, "segment should have been removed")
    with pytest.raises(RuntimeError):
      n2 = co.numConnectedSynapses(seg)
    



if __name__ == "__main__":
  unittest.main()
