#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/algorithms/SpatialPooler.hpp>

#include "py_utils.hpp"

namespace py = pybind11;
using namespace nupic;
using namespace nupic::algorithms::spatial_pooler;

namespace nupic_ext
{
    void init_Spatial_Pooler(py::module& m)
    {
        py::class_<SpatialPooler> py_SpatialPooler(m, "SpatialPooler");

        // default constructor
        py_SpatialPooler.def(py::init<>([]() 
        {
            vector<UInt> inputDimensions = { 32, 32 };
            vector<UInt> columnDimensions = { 64, 64 };

            return SpatialPooler(inputDimensions, columnDimensions);
        }));

        py_SpatialPooler.def(
            py::init<vector<UInt>
            , vector<UInt>
            , UInt
            , Real
            , bool
            , Real
            , UInt
            , UInt
            , Real
            , Real
            , Real
            , Real
            , UInt
            , Real
            , Int
            , UInt
            , bool>()
            , py::arg("inputDimensions")
            , py::arg("columnDimensions")
            , py::arg("potentialRadius") = 16
            , py::arg("potentialPct") = 0.5
            , py::arg("globalInhibition") = false
            , py::arg("localAreaDensity") = -1.0
            , py::arg("numActiveColumnsPerInhArea") = 10.0
            , py::arg("stimulusThreshold") = 0
            , py::arg("synPermInactiveDec") = 0.01
            , py::arg("synPermActiveInc") = 0.1
            , py::arg("synPermConnected") = 0.1
            , py::arg("minPctOverlapDutyCycles") = 0.001
            , py::arg("dutyCyclePeriod") = 1000
            , py::arg("boostStrength") = 0.0
            , py::arg("seed") = -1
            , py::arg("spVerbosity") = 0
            , py::arg("wrapAround") = true
        );

        // loadFromString
        py_SpatialPooler.def("loadFromString", [](SpatialPooler& self, const std::string& inString)
        {
            std::istringstream inStream(inString);
            self.load(inStream);
        });

        // writeToString
        py_SpatialPooler.def("writeToString", [](const SpatialPooler& self)
        {
            std::ostringstream os;
            os.flags(ios::scientific);
            os.precision(numeric_limits<double>::digits10 + 1);

            self.save(os);

            return os.str();
        });

        // compute
        py_SpatialPooler.def("compute", [](SpatialPooler& self, py::array_t<UInt>& x, bool learn, py::array_t<UInt>& y)
        {
            self.compute(get_it(x), learn, get_it(y));
        });

        // stripUnlearnedColumns
        py_SpatialPooler.def("stripUnlearnedColumns", [](SpatialPooler& self, py::array_t<UInt>& x)
        {
            self.stripUnlearnedColumns(get_it(x));
        });

        // setBoostFactors
        py_SpatialPooler.def("setBoostFactors", [](SpatialPooler& self, py::array_t<Real>& x)
        {
            self.setBoostFactors(get_it(x));
        });

        // getBoostFactors
        py_SpatialPooler.def("getBoostFactors", [](const SpatialPooler& self, py::array_t<Real>& x)
        {
            self.getBoostFactors(get_it(x));
        });

        // setOverlapDutyCycles
        py_SpatialPooler.def("setOverlapDutyCycles", [](SpatialPooler& self, py::array_t<Real>& x)
        {
            self.setOverlapDutyCycles(get_it(x));
        });

        // getOverlapDutyCycles
        py_SpatialPooler.def("getOverlapDutyCycles", [](const SpatialPooler& self, py::array_t<Real>& x)
        {
            self.getOverlapDutyCycles(get_it(x));
        });

        // setActiveDutyCycles
        py_SpatialPooler.def("setActiveDutyCycles", [](SpatialPooler& self, py::array_t<Real>& x)
        {
            self.setActiveDutyCycles(get_it(x));
        });

        // getActiveDutyCycles
        py_SpatialPooler.def("getActiveDutyCycles", [](const SpatialPooler& self, py::array_t<Real>& x)
        {
            self.getActiveDutyCycles(get_it(x));
        });

        // setMinOverlapDutyCycles
        py_SpatialPooler.def("setMinOverlapDutyCycles", [](SpatialPooler& self, py::array_t<Real>& x)
        {
            self.setMinOverlapDutyCycles(get_it(x));
        });

        // getMinOverlapDutyCycles
        py_SpatialPooler.def("getMinOverlapDutyCycles", [](const SpatialPooler& self, py::array_t<Real>& x)
        {
            self.getMinOverlapDutyCycles(get_it(x));
        });

        // setPotential
        py_SpatialPooler.def("setPotential", [](SpatialPooler& self, UInt column, py::array_t<UInt>& x)
        {
            self.setPotential(column, get_it(x));
        });

        // getPotential
        py_SpatialPooler.def("getPotential", [](const SpatialPooler& self, UInt column, py::array_t<UInt>& x)
        {
            self.getPotential(column, get_it(x));
        });

        // setPermanence
        py_SpatialPooler.def("setPermanence", [](SpatialPooler& self, UInt column, py::array_t<Real>& x)
        {
            self.setPermanence(column, get_it(x));
        });

        // getPermanence
        py_SpatialPooler.def("getPermanence", [](const SpatialPooler& self, UInt column, py::array_t<Real>& x)
        {
            self.getPermanence(column, get_it(x));
        });

        // getConnectedSynapses
        py_SpatialPooler.def("getConnectedSynapses", [](const SpatialPooler& self, UInt column, py::array_t<UInt>& x)
        {
            self.getConnectedSynapses(column, get_it(x));
        });

        // getConnectedCounts
        py_SpatialPooler.def("getConnectedCounts", [](const SpatialPooler& self, py::array_t<UInt>& x)
        {
            self.getConnectedCounts(get_it(x));
        });

        // getOverlaps
        py_SpatialPooler.def("getOverlaps", [](SpatialPooler& self)
        {
            auto overlaps = self.getOverlaps();

            return py::array_t<UInt>({ overlaps.size() }, overlaps.data());
        });

        // getBoostedOverlaps
        py_SpatialPooler.def("getBoostedOverlaps", [](SpatialPooler& self)
        {
            auto overlaps = self.getBoostedOverlaps();

            return py::array_t<Real>({ overlaps.size() }, overlaps.data());
        });

        // calculateOverlap_
        py_SpatialPooler.def("calculateOverlap_", [](SpatialPooler& self, py::array_t<UInt>& input)
        {
            std::vector<nupic::UInt> overlapVector;

            self.calculateOverlap_(get_it(input), overlapVector);

            return py::array_t<UInt>({ overlapVector.size() }, overlapVector.data());
        });

        // inhibitColumns_
        py_SpatialPooler.def("inhibitColumns_", [](SpatialPooler& self, py::array_t<Real>& overlaps)
        {
            std::vector<nupic::Real> overlapsVector(get_it(overlaps), get_end(overlaps));

            std::vector<nupic::UInt> activeColumnsVector;

            self.inhibitColumns_(overlapsVector, activeColumnsVector);

            return py::array_t<UInt>({ activeColumnsVector.size() }, activeColumnsVector.data());
        });

        // updatePermanencesForColumn_
        py_SpatialPooler.def("updatePermanencesForColumn_", [](SpatialPooler& self, py::array_t<Real>& perm, UInt column, bool raisePerm)
        {
            std::vector<nupic::Real> permVector(get_it(perm), get_end(perm));

            self.updatePermanencesForColumn_(permVector, column, raisePerm);
        });

        // updateDutyCycles_
        py_SpatialPooler.def("updateDutyCycles_", [](SpatialPooler& self, py::array_t<UInt>& overlaps, py::array_t<UInt>& activeArray)
        {
            std::vector<nupic::UInt> overlapsVector(get_it(overlaps), get_end(overlaps));

            self.updateDutyCycles_(overlapsVector, get_it(activeArray));
        });

        // getIterationLearnNum
        py_SpatialPooler.def("getIterationLearnNum", &SpatialPooler::getIterationLearnNum);

    }
} // namespace nupix_ext
