#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/math/SparseMatrix.hpp>
#include <nupic/math/SegmentMatrixAdapter.hpp>

#include "py_utils.hpp"

namespace py = pybind11;

namespace nupic_ext
{
    void init_SegmentSparseMatrix(py::module& m)
    {
        typedef nupic::SegmentMatrixAdapter<
            nupic::SparseMatrix<nupic::UInt32, nupic::Real32, nupic::Int32, nupic::Real64, nupic::DistanceToZero<nupic::Real32>>> SSM_t;

        py::class_<SSM_t> smc(m, "SegmentSparseMatrix");

        smc.def(py::init<nupic::UInt32, nupic::UInt32>(), py::arg("nCells"), py::arg("ncols"));

        smc.def_readwrite("matrix", &SSM_t::matrix);

        smc.def("createSegment", &SSM_t::createSegment);

        smc.def("createSegments", [](SSM_t& self, py::array_t<nupic::UInt32>& py_cells)
        {
            if (py_cells.ndim() != 1) { throw std::runtime_error("Number of dimensions must be one."); }

            py::array_t<nupic::UInt32> segmentsOut(py_cells.size());

            self.createSegments(get_it(py_cells), get_end(py_cells), get_it(segmentsOut));

            return segmentsOut;
        });

        smc.def("destroySegments", [](SSM_t& self, py::array_t<nupic::UInt32>& py_segments)
        {
            if (py_segments.ndim() != 1) { throw std::runtime_error("Number of dimensions must be one."); }

            self.destroySegments(get_it(py_segments), get_end(py_segments));
        });

        smc.def("getSegmentCounts", [](const SSM_t& self, py::array_t<nupic::UInt32>& py_cells)
        {
            if (py_cells.ndim() != 1) { throw std::runtime_error("Number of dimensions must be one."); }

            py::array_t<nupic::Int32> segmentsOut(py_cells.size());

            self.getSegmentCounts(get_it(py_cells), get_end(py_cells), get_it(segmentsOut));

            return segmentsOut;
        });

        smc.def("getSegmentsForCell", [](const SSM_t& self, nupic::UInt32 cell)
        {
            auto segments = self.getSegmentsForCell(cell);
            
            py::array_t<nupic::UInt32> npSegments({ segments.size() }, segments.data());

            return npSegments;
        });

        smc.def("sortSegmentsByCell", [](const SSM_t& self, py::array_t<nupic::UInt32>& segments)
        {
            if (segments.ndim() != 1) { throw std::runtime_error("Number of dimensions must be one."); }

            self.sortSegmentsByCell(get_it(segments), get_end(segments));
        });

        smc.def("filterSegmentsByCell", [](const SSM_t& self
            , py::array_t<nupic::UInt32>& py_segments
            , py::array_t<nupic::UInt32>& py_cells
            , bool assumeSorted)
        {
            if (py_segments.ndim() != 1) { throw std::runtime_error("Number of dimensions must be one."); }
            if (py_cells.ndim() != 1) { throw std::runtime_error("Number of dimensions must be one."); }

            if (assumeSorted == false)
            {
                std::vector<nupic::UInt32> segments_copy(get_it(py_segments), get_end(py_segments));
                self.sortSegmentsByCell(segments_copy.begin(), segments_copy.end());

                std::vector<nupic::UInt32> cells_copy(get_it(py_cells), get_end(py_cells));
                sort(cells_copy.begin(), cells_copy.end());

                auto filtered = self.filterSegmentsByCell(segments_copy.begin(), segments_copy.end(), cells_copy.begin(), cells_copy.end());

                py::array_t<nupic::UInt32> npFiltered({ filtered.size() }, filtered.data());

                return npFiltered;
            }
            else
            {
                auto filtered = self.filterSegmentsByCell(get_it(py_segments), get_end(py_segments), get_it(py_cells), get_end(py_cells));

                py::array_t<nupic::UInt32> npFiltered({ filtered.size() }, filtered.data());

                return npFiltered;
            }
        }, "Return the subset of segments that are on the provided cells."
            , py::arg("py_segments"), py::arg("py_cells"), py::arg("assumeSorted") = false);


        smc.def("mapSegmentsToCells", [](const SSM_t& self, py::array_t<nupic::UInt32>& segments)
        {
            if (segments.ndim() != 1) { throw std::runtime_error("Number of dimensions must be one."); }

            py::array_t<nupic::UInt32> cellsOut(segments.size());

            self.mapSegmentsToCells(get_it(segments), get_end(segments), get_it(cellsOut));

            return cellsOut;
        });

    }

} // namespace nupix_ext
