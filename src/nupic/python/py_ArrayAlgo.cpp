
#include <fstream>

#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/math/SparseMatrix.hpp>

#include "py_utils.hpp"

namespace py = pybind11;

typedef nupic::SparseMatrix<nupic::UInt32, nupic::Real32, nupic::Int32, nupic::Real64, nupic::DistanceToZero<nupic::Real32>> SparseMatrix32_t;

typedef nupic::SparseMatrix<nupic::UInt32, nupic::Real64, nupic::Int32, nupic::Real64, nupic::DistanceToZero<nupic::Real64>> _SparseMatrix64;

namespace nupic_ext
{
    void init_array_algo(py::module& m)
    {
        // count_gt
        m.def("count_gt", [](py::array_t<nupic::Real32>& x, nupic::Real32 threshold)
        {
            return nupic::count_gt(get_it(x), get_end(x), threshold);
        });

        // count_gte
        m.def("count_gte", [](py::array_t<nupic::Real32>& x, nupic::Real32 threshold)
        {
            return nupic::count_gte(get_it(x), get_end(x), threshold);
        });

        // count_lt
        m.def("count_lt", [](py::array_t<nupic::Real32>& x, nupic::Real32 threshold)
        {
            return nupic::count_lt(get_it(x), get_end(x), threshold);
        });

        // partialArgsort
        m.def("partialArgsort", [](size_t k, py::array_t<nupic::Real32>& x, py::array_t<nupic::UInt32>& r, int direction)
        {
            return nupic::partial_argsort(k, get_it(x), get_end(x), get_it(r), get_end(r), direction);
        }, "", py::arg("k"), py::arg("x"), py::arg("r"), py::arg("direction") = -1);

        // positiveLearningPartialArgsort
        m.def("positiveLearningPartialArgsort", [](size_t k, py::array_t<nupic::Real32>& x, py::array_t<nupic::UInt32>& r, nupic::Random& rng, bool real_random)
        {
            return nupic::partial_argsort_rnd_tie_break(k, get_it(x), get_end(x), get_it(r), get_end(r), rng, real_random);
        }, "", py::arg("k"), py::arg("x"), py::arg("r"), py::arg("rng"), py::arg("real_random") = false);

        // logicalAnd
        m.def("logicalAnd", [](py::array_t<nupic::Real32>& x, py::array_t<nupic::Real32>& y)
        {
            py::array_t<nupic::Real32> z(x.size());

            nupic::logical_and(get_it(x), get_end(x), get_it(y), get_end(y), get_it(z), get_end(z));

            return z;
        });

        // logicalAnd2
        m.def("logicalAnd2", [](py::array_t<nupic::Real32>& x, py::array_t<nupic::Real32>& y)
        {
            nupic::in_place_logical_and(get_it(x), get_end(x), get_it(y), get_end(y));
        });

        //inline PyObject* binarize_with_threshold(nupic::Real32 threshold, PyObject* py_x)
        m.def("binarize_with_threshold", [](nupic::Real32 threshold, py::array_t<nupic::Real32>& x)
        {
            py::array_t<nupic::Real32> y(x.size());

            nupic::UInt32 c = nupic::binarize_with_threshold(threshold, get_it(x), get_end(x), get_it(y), get_end(y));

            return py::make_tuple(c, y);
        }, "A function that binarizes a dense vector.");
    }
} // namespace nupix_ext
