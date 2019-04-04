/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2019, David McDougall
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
 * --------------------------------------------------------------------- */

/** @file
 * PyBind11 bindings for Connections class
 */

#include <bindings/suppress_register.hpp>  //include before pybind11.h
#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/algorithms/Connections.hpp>

namespace py = pybind11;
using namespace nupic;
using namespace nupic::algorithms::connections;
using nupic::sdr::SDR;

namespace nupic_ext
{
  void init_Connections(py::module& m)
  {
    py::class_<Connections> py_Connections(m, "Connections");
    py_Connections.def(py::init<>());
    py_Connections.def(py::init<UInt, Permanence>(),
        py::arg("numCells"),
        py::arg("connectedThreshold"));

    py_Connections.def("createSegment", &Connections::createSegment,
        py::arg("cell"));

    py_Connections.def("destroySegment", &Connections::destroySegment);

    py_Connections.def("createSynapse", &Connections::createSynapse,
        py::arg("segment"),
        py::arg("presynaticCell"),
        py::arg("permanence"));

    py_Connections.def("destroySynapse", &Connections::destroySynapse);

    py_Connections.def("updateSynapsePermanence", &Connections::updateSynapsePermanence,
        py::arg("synapse"),
        py::arg("permanence"));

    py_Connections.def("segmentsForCell", &Connections::segmentsForCell);

    py_Connections.def("synapsesForSegment", &Connections::synapsesForSegment);

    py_Connections.def("cellForSegment", &Connections::cellForSegment);

    // idxOnCellForSegment
    // mapSegmentsToCells
    // segmentForSynapse
    // dataForSegment
    // dataForSynapse
    // getSegment(CellIdx cell, SegmentIdx idx)
    // segmentFlatListLength
    // synapsesForPresynapticCell
    // computeActivity
    // computeActivity
    // adaptSegment
    // raisePermanencesToThreshold
    // bumpSegment

    py_Connections.def("numCells", &Connections::numCells);
    py_Connections.def("numSegments",
        [](Connections &self) { return self.numSegments(); });
    py_Connections.def("numSegments",
        [](Connections &self, CellIdx cell) { return self.numSegments(cell); });

    py_Connections.def("numSynapses",
        [](Connections &self) { return self.numSynapses(); });
    py_Connections.def("numSynapses",
        [](Connections &self, Segment seg) { return self.numSynapses(seg); });

  } // End init_Connections
}   // End namespace nupic_ext
