#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/os/OS.hpp>

namespace py = pybind11;

using namespace nupic;

namespace nupic_ext {

    void init_OS(py::module& m)
    {
        py::class_<OS> py_OS(m, "OS");
        py_OS.def_static("getProcessMemoryUsage", &nupic::OS::getProcessMemoryUsage);
    }

} // namespace nupic_ext