#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/engine/PyBindRegion.hpp>

#include <nupic/engine/TestNode.hpp>

namespace py = pybind11;
using namespace nupic;

namespace nupic_ext
{
    void init_Regions(py::module& m)
    {
        ///////////////////
        // Test Node
        ///////////////////
        typedef nupic::PyBindRegion Region_t;
        py::class_<Region_t> py_Region(m, "PyRegion");

        py_Region.def("initialize", &Region_t::initialize)
            .def("compute", &Region_t::compute);

        py_Region.def("getSpec", &Region_t::getSpec);


        ///////////////////
        // Test Node
        ///////////////////
        py::class_<TestNode> py_TestNode(m, "TestNode");
        py_TestNode.def("initialize", &TestNode::initialize)
            .def("compute", &TestNode::compute)
            .def("getName", [](const TestNode& self) { return "Hello";  });

    }

} // namespace nupix_ext
