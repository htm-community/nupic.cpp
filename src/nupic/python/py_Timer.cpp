#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/os/Timer.hpp>

namespace py = pybind11;

using namespace nupic;

namespace nupic_ext {


    void init_Timer(py::module& m)
    {
        py::class_<Timer> py_Timer(m, "Timer");

        py_Timer.def(py::init<bool>(), py::arg("startme") = false);

        py_Timer.def("start", &Timer::start);
        py_Timer.def("stop", &Timer::stop);
        py_Timer.def("elapsed", &Timer::getElapsed);
        py_Timer.def("reset", &Timer::reset);
        py_Timer.def("startCount", &Timer::getStartCount);
        py_Timer.def("isStarted", &Timer::isStarted);
        py_Timer.def("toString", &Timer::toString);

        py_Timer.def("__str__", &Timer::toString);

    }

} // namespace nupic_ext