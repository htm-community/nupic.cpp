#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "py_utils.hpp"

#include <nupic/math/Set.hpp>
#include <nupic/types/Types.hpp>

namespace py = pybind11;

using namespace nupic;

namespace nupic_ext
{
    void init_Set(py::module& m)
    {
        typedef Set<nupic::UInt32> Set_t;

        py::class_<Set_t> py_Set(m, "Set");

        py_Set.def(py::init<>());

        // inline void construct(nupic::UInt32 m, PyObject* py_a)
        py_Set.def("construct", [](Set_t& self, nupic::UInt32 m, py::array_t<nupic::UInt32>& a)
        {
            self.construct(m, static_cast<UInt32>(a.shape(0)), get_it(a));
        });

        // 
        // inline nupic::UInt32 intersection(PyObject* py_s2, PyObject* py_r) const
        py_Set.def("intersection", [](const Set_t& self, py::array_t<nupic::UInt32>& s2, py::array_t<nupic::UInt32>& r)
        {
            return self.intersection(static_cast<UInt32>(s2.shape(0)), get_it(s2), get_it(r));
        });
    }
} // namespace nupix_ext
