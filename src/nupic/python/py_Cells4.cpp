#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/algorithms/Cell.hpp>
#include <nupic/algorithms/Cells4.hpp>
#include <nupic/algorithms/Segment.hpp>
#include <nupic/algorithms/SegmentUpdate.hpp>

#include "py_utils.hpp"

namespace py = pybind11;

namespace nupic_ext
{
    void init_Cells4(py::module& m)
    {
        //////////////////
        // Cell
        //////////////////
        typedef nupic::algorithms::Cells4::Cell Cell_t;

        py::class_<Cell_t> py_cell(m, "Cell");
        py_cell.def(py::init<>());

        py_cell.def("getSegment", &Cell_t::getSegment);


        //////////////////
        // Segment
        //////////////////
        typedef nupic::algorithms::Cells4::Segment Segment_t;

        py::class_<Segment_t> py_segment(m, "Segment");

        py_segment.def(py::init<>());

        py_segment.def("size", &Segment_t::size);
        py_segment.def("getSrcCellIdx", &Segment_t::getSrcCellIdx);
        py_segment.def("getPermanence", &Segment_t::getPermanence);
        

        //////////////////
        // Cells4
        //////////////////
        typedef nupic::algorithms::Cells4::Cells4 Cells4_t;

        py::class_<Cells4_t> py_cells4(m, "Cells4");

        py_cells4.def(py::init<UInt   // nColumns =0, 
            , UInt // nCellsPerCol =0,
            , UInt // activationThreshold =1,
            , UInt // minThreshold =1,
            , UInt // newSynapseCount =1,
            , UInt // segUpdateValidDuration =1,
            , Real // permInitial =.5,
            , Real // permConnected =.8,
            , Real // permMax =1,
            , Real // permDec =.1,
            , Real // permInc =.1,
            , Real // globalDecay =0,
            , bool // doPooling =false,
            , int  // seed =-1,
            , bool // initFromCpp =false,
            , bool // checkSynapseConsistency =false
        >()
            , py::arg("nColumns") = 0
            , py::arg("nCellsPerCol") = 0
            , py::arg("activationThreshold") = 1
            , py::arg("minThreshold") = 1
            , py::arg("newSynapseCount") = 1
            , py::arg("segUpdateValidDuration") = 1
            , py::arg("permInitial") = .5
            , py::arg("permConnected") = .8
            , py::arg("permMax") = 1
            , py::arg("permDec") = .1
            , py::arg("permInc") = .1
            , py::arg("globalDecay") = 0
            , py::arg("doPooling") = false
            , py::arg("seed") = -1
            , py::arg("initFromCpp") = false
            , py::arg("checkSynapseConsistency") = false);

        py_cells4.def("version", &Cells4_t::version);

        py_cells4.def("nSegments", &Cells4_t::nSegments);
        py_cells4.def("nCells", &Cells4_t::nCells);
        py_cells4.def("nColumns", &Cells4_t::nColumns);
        py_cells4.def("nCellsPerCol", &Cells4_t::nCellsPerCol);
        py_cells4.def("getMinThreshold", &Cells4_t::getMinThreshold);
        py_cells4.def("getPermConnected", &Cells4_t::getPermConnected);
        py_cells4.def("getVerbosity", &Cells4_t::getVerbosity);
        py_cells4.def("getMaxAge", &Cells4_t::getMaxAge);
        py_cells4.def("getPamLength", &Cells4_t::getPamLength);
        py_cells4.def("getMaxInfBacktrack", &Cells4_t::getMaxInfBacktrack);
        py_cells4.def("getMaxLrnBacktrack", &Cells4_t::getMaxLrnBacktrack);
        py_cells4.def("getPamCounter", &Cells4_t::getPamCounter);
        py_cells4.def("getMaxSeqLength", &Cells4_t::getMaxSeqLength);
        py_cells4.def("getAvgLearnedSeqLength", &Cells4_t::getAvgLearnedSeqLength);
        py_cells4.def("getNLrnIterations", &Cells4_t::getNLrnIterations);
        py_cells4.def("getMaxSegmentsPerCell", &Cells4_t::getMaxSegmentsPerCell);
        py_cells4.def("getMaxSynapsesPerSegment", &Cells4_t::getMaxSynapsesPerSegment);
        py_cells4.def("getCheckSynapseConsistency", &Cells4_t::getCheckSynapseConsistency);

        py_cells4.def("nSegmentsOnCell", &Cells4_t::nSegmentsOnCell, "Returns the number of segments currently in use on the given cell.");
        py_cells4.def("nSynapses", &Cells4_t::nSynapses);

        py_cells4.def("setMaxInfBacktrack", &Cells4_t::setMaxInfBacktrack);
        py_cells4.def("setMaxLrnBacktrack", &Cells4_t::setMaxLrnBacktrack);
        py_cells4.def("setVerbosity", &Cells4_t::setVerbosity);
        py_cells4.def("setMaxAge", &Cells4_t::setMaxAge);
        py_cells4.def("setMaxSeqLength", &Cells4_t::setMaxSeqLength);
        py_cells4.def("setCheckSynapseConsistency", &Cells4_t::setCheckSynapseConsistency);

        py_cells4.def("setMaxSegmentsPerCell", &Cells4_t::setMaxSegmentsPerCell);
        py_cells4.def("setMaxSynapsesPerCell", &Cells4_t::setMaxSynapsesPerCell);
        py_cells4.def("setPamLength", &Cells4_t::setPamLength);

        py_cells4.def("nSynapsesInCell", &Cells4_t::nSynapsesInCell, "Total number of synapses in a given cell (at at given point, changes all the time).");


        /////////////////


        py_cells4.def(py::pickle(
            [](const Cells4_t& self) -> std::string
        {
            // __getstate__
            std::ostringstream os;
            self.save(os);

            return os.str();
        },
            [](const std::string& str) -> Cells4_t
        {
            // __setstate__
            if (str.empty())
            {
                throw std::runtime_error("Empty state");
            }

            std::istringstream is(str);

            Cells4_t a;
            a.load(is);

            return a;
        }
        ));

        py_cells4.def("setStatePointers", [](Cells4_t& self
            , py::array_t<nupic::UInt32>& py_infActiveStateT
            , py::array_t<nupic::UInt32>& py_infActiveStateT1
            , py::array_t<nupic::UInt32>& py_infPredictedStateT
            , py::array_t<nupic::UInt32>& py_infPredictedStateT1
            , py::array_t<nupic::Real32>& py_colConfidenceT
            , py::array_t<nupic::Real32>& py_colConfidenceT1
            , py::array_t<nupic::Real32>& py_cellConfidenceT
            , py::array_t<nupic::Real32>& py_cellConfidenceT1
            )
        {
            self.setStatePointers((nupic::Byte*)get_it(py_infActiveStateT)
                , (nupic::Byte*)get_it(py_infActiveStateT1)
                , (nupic::Byte*)get_it(py_infPredictedStateT)
                , (nupic::Byte*)get_it(py_infPredictedStateT1)
                , (nupic::Real32*)get_it(py_colConfidenceT)
                , (nupic::Real32*)get_it(py_colConfidenceT1)
                , (nupic::Real32*)get_it(py_cellConfidenceT)
                , (nupic::Real32*)get_it(py_cellConfidenceT1)
            );
        });

        py_cells4.def("setStatePointers", [](const Cells4_t& self)
        {
            nupic::UInt32 nCells = self.nCells();
            nupic::UInt32 nColumns = self.nColumns();

            nupic::Byte* cpp_activeT, *cpp_activeT1;
            nupic::Byte* cpp_predT, *cpp_predT1;
            nupic::Real* cpp_colConfidenceT, *cpp_colConfidenceT1;
            nupic::Real* cpp_confidenceT, *cpp_confidenceT1;

            self.getStatePointers(cpp_activeT, cpp_activeT1,
                cpp_predT, cpp_predT1,
                cpp_colConfidenceT, cpp_colConfidenceT1,
                cpp_confidenceT, cpp_confidenceT1);

            return py::make_tuple(
                py::array_t<nupic::UInt32>({ nCells }, (nupic::UInt32*) cpp_activeT)
                , py::array_t<nupic::UInt32>({ nCells }, (nupic::UInt32*) cpp_activeT1)
                , py::array_t<nupic::UInt32>({ nCells }, (nupic::UInt32*) cpp_predT)
                , py::array_t<nupic::UInt32>({ nCells }, (nupic::UInt32*) cpp_predT1)

                , py::array_t<nupic::Real>({ nColumns }, cpp_colConfidenceT)
                , py::array_t<nupic::Real>({ nColumns }, cpp_colConfidenceT1)
                , py::array_t<nupic::Real>({ nColumns }, cpp_confidenceT)
                , py::array_t<nupic::Real>({ nColumns }, cpp_confidenceT1)
            );
        });

        py_cells4.def("getLearnStates", [](const Cells4_t& self)
        {
            nupic::UInt32 nCells = self.nCells();

            nupic::Byte* cpp_activeT, *cpp_activeT1;
            nupic::Byte* cpp_predT, *cpp_predT1;

            self.getLearnStatePointers(cpp_activeT, cpp_activeT1, cpp_predT, cpp_predT1);


            return py::make_tuple(py::array_t<nupic::UInt32>({ nCells }, (nupic::UInt32*) cpp_activeT)
                , py::array_t<nupic::UInt32>({ nCells }, (nupic::UInt32*) cpp_activeT1)
                , py::array_t<nupic::UInt32>({ nCells }, (nupic::UInt32*) cpp_predT)
                , py::array_t<nupic::UInt32>({ nCells }, (nupic::UInt32*) cpp_predT1)
            );
        });

        py_cells4.def("compute", [](Cells4_t& self, py::array_t<nupic::Real> x, bool doInference, bool doLearning)
        {
            py::array_t<nupic::Real> y(self.nCells());
            
            self.compute(get_it(x), get_it(y), doInference, doLearning);

            return y;
        });


        py_cells4.def("reset", &Cells4_t::reset);

        py_cells4.def("addNewSegment", &Cells4_t::addNewSegment);
        py_cells4.def("updateSegment", &Cells4_t::updateSegment);

        py_cells4.def("rebuildOutSynapses", &Cells4_t::rebuildOutSynapses);

        py_cells4.def("saveToFile", &Cells4_t::saveToFile);
        py_cells4.def("loadFromFile", &Cells4_t::loadFromFile);

        py_cells4.def("getCell", &Cells4_t::getCell);

        py_cells4.def("setCellSegmentOrder", &Cells4_t::setCellSegmentOrder);
    }

} // namespace nupix_ext
