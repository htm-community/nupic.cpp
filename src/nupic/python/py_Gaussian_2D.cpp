
#include <fstream>

#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "py_utils.hpp"

#include <nupic/math/Math.hpp>

namespace py = pybind11;

using namespace nupic;

namespace nupic_ext
{
    void init_Gaussian_2D(py::module& m)
    {
        typedef Gaussian2D<Real32> Gaussian2D_t;

        py::class_<Gaussian2D_t> py_Gaussian2D(m, "Gaussian2D");

        // T c_x_, T c_y_, T s00_, T s01_, T s10_, T s11_
        py_Gaussian2D.def(py::init<Real32, Real32, Real32, Real32, Real32, Real32>()
            , py::arg("c_x_"), py::arg("c_y_"), py::arg("s00_"), py::arg("s01_"), py::arg("s10_"), py::arg("s11_"));

        // inline nupic::Real32 eval(nupic::Real32 x, nupic::Real32 y) const
        py_Gaussian2D.def("construct", [](const Gaussian2D_t& self, Real32 x, Real32 y)
        {
            return self.operator()(x, y);
        });
    }
} // namespace nupix_ext
