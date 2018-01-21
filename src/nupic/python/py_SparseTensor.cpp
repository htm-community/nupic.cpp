#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/math/SparseTensor.hpp>

#include "SparseTensorIndex.hpp"
#include "py_utils.hpp"

namespace py = pybind11;
using namespace nupic;

namespace nupic_ext
{
    void init_TensorIndex(py::module& m)
    {
        py::class_<PyBindTensorIndex> py_TensorIndex(m, "TensorIndex");

    }

    void init_SparseTensor(py::module& m)
    {
        typedef nupic::SparseTensor<PyBindTensorIndex, nupic::Real> Tensor_t;

        py::class_<Tensor_t> py_SparseTensor(m, "SparseTensor");

        py_SparseTensor.def(py::init([](const std::string& state)
        {
            size_t rank = 0;
            {
                std::stringstream forRank(state);
                forRank.exceptions(std::ios::failbit | std::ios::badbit);
                forRank >> rank;
            };

            PyBindTensorIndex index(rank, (const size_t *)0);
            for (size_t i = 0; i<rank; ++i) {
                index[i] = 1;
            }
            Tensor_t tensor(index);
            std::stringstream toRead(state);
            tensor.fromStream(toRead);

            return tensor;
        }));
    }

} // namespace nupix_ext
