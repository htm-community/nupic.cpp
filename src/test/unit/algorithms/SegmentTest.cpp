/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2016, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
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
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

/** @file
 * Implementation of unit tests for Segment
 */

#include <gtest/gtest.h>
#include <nupic/algorithms/Segment.hpp>
#include <set>

using namespace nupic::algorithms::Cells4;
using namespace std;

void setUpSegment(Segment &segment, vector<UInt> &inactiveSegmentIndices,
                  vector<UInt> &activeSegmentIndices,
                  vector<UInt> &activeSynapseIndices,
                  vector<UInt> &inactiveSynapseIndices) {
  vector<float> permanences = {0.2f, 0.9f, 0.9f, 0.7f, 0.4f,  // active synapses
                               0.8f, 0.1f, 0.2f, 0.3f, 0.2f}; // inactive synapses

  set<UInt> srcCells;
  for (Size i = 0; i < permanences.size(); i++) {
    srcCells.clear();
    srcCells.insert(i);

    segment.addSynapses(srcCells, permanences[i], 0.5f);

    if (i < 5u) {
      inactiveSegmentIndices.push_back(i);
      inactiveSynapseIndices.push_back(0);
    } else {
      activeSegmentIndices.push_back(i);
      activeSynapseIndices.push_back(0);
    }
  }
}

/*
 * Test that synapses are removed from inactive first even when there
 * are active synapses with lower permanence.
 */
TEST(SegmentTest, freeNSynapsesInactiveFirst) {
  Segment segment;

  vector<UInt> inactiveSegmentIndices;
  vector<UInt> activeSegmentIndices;
  vector<UInt> activeSynapseIndices;
  vector<UInt> inactiveSynapseIndices;
  vector<UInt> removed;

  setUpSegment(segment, inactiveSegmentIndices, activeSegmentIndices,
               activeSynapseIndices, inactiveSynapseIndices);

  ASSERT_EQ(segment.size(), 10u);

  segment.freeNSynapses(2, inactiveSynapseIndices, inactiveSegmentIndices,
                        activeSynapseIndices, activeSegmentIndices, removed, 0,
                        10, 1.0f);

  ASSERT_EQ(segment.size(), 8u);

  vector<UInt> removed_expected = {0u, 4u};
  sort(removed.begin(), removed.end());
  ASSERT_EQ(removed, removed_expected);
}

/*
 * Test that active synapses are removed once all inactive synapses are
 * exhausted.
 */
TEST(SegmentTest, freeNSynapsesActiveFallback) {
  Segment segment;

  vector<UInt> inactiveSegmentIndices;
  vector<UInt> activeSegmentIndices;

  vector<UInt> activeSynapseIndices;
  vector<UInt> inactiveSynapseIndices;
  vector<UInt> removed;

  setUpSegment(segment, inactiveSegmentIndices, activeSegmentIndices,
               activeSynapseIndices, inactiveSynapseIndices);

  ASSERT_EQ(segment.size(), 10u);

  segment.freeNSynapses(6, inactiveSynapseIndices, inactiveSegmentIndices,
                        activeSynapseIndices, activeSegmentIndices, removed, 0,
                        10, 1.0);

  vector<UInt> removed_expected = {0u, 1u, 2u, 3u, 4u, 6u};
  sort(removed.begin(), removed.end());
  ASSERT_EQ(removed, removed_expected);
}

/*
 * Test that removal respects insertion order (stable sort of permanences).
 */
TEST(SegmentTest, freeNSynapsesStableSort) {
  Segment segment;

  vector<UInt> inactiveSegmentIndices;
  vector<UInt> activeSegmentIndices;

  vector<UInt> activeSynapseIndices;
  vector<UInt> inactiveSynapseIndices;
  vector<UInt> removed;

  setUpSegment(segment, inactiveSegmentIndices, activeSegmentIndices,
               activeSynapseIndices, inactiveSynapseIndices);

  ASSERT_EQ(segment.size(), 10u);

  segment.freeNSynapses(7, inactiveSynapseIndices, inactiveSegmentIndices,
                        activeSynapseIndices, activeSegmentIndices, removed, 0,
                        10, 1.0f);

  vector<UInt> removed_expected = {0u, 1u, 2u, 3u, 4u, 6u, 7u};
  sort(removed.begin(), removed.end());
  ASSERT_EQ(removed, removed_expected);
}

/**
 * Test operator '=='
 */
TEST(SegmentTest, testEqualsOperator) {
  Segment segment1;
  Segment segment2;

  vector<UInt> inactiveSegmentIndices;
  vector<UInt> activeSegmentIndices;
  vector<UInt> activeSynapseIndices;
  vector<UInt> inactiveSynapseIndices;

  setUpSegment(segment1, inactiveSegmentIndices, activeSegmentIndices,
               activeSynapseIndices, inactiveSynapseIndices);
  ASSERT_TRUE(segment1 != segment2);
  setUpSegment(segment2, inactiveSegmentIndices, activeSegmentIndices,
               activeSynapseIndices, inactiveSynapseIndices);
  ASSERT_TRUE(segment1 == segment2);
}