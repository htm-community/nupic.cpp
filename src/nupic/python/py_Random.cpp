#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/utils/Random.hpp>

#include "py_utils.hpp"

namespace py = pybind11;

namespace nupic_ext {

    void init_Random(py::module& m)
    {
        typedef nupic::Random Random_t;

        py::class_<Random_t> Random(m, "Random");

        Random.def(py::init<nupic::UInt64>(), py::arg("seed") = 0)
            .def("getUInt32", &Random_t::getUInt32, py::arg("max") = Random_t::MAX32)
            .def("getUInt64", &Random_t::getUInt64, py::arg("max") = Random_t::MAX64)
            .def("getReal64", &Random_t::getReal64)
            .def("max", &Random_t::max)
            .def("min", &Random_t::min)
            .def_readonly_static("MAX32", &Random_t::MAX32)
            .def_readonly_static("MAX64", &Random_t::MAX64);


        //////////////////
        // sample
        /////////////////

        Random.def("sample", &Random_t::sample<UINT32>);

        Random.def("sample",
            [](Random_t r, py::array_t<nupic::UInt32>& population, py::array_t<nupic::UInt32>& choices)
        {
            if (population.ndim() != 1 || choices.ndim() != 1)
            {
                throw std::runtime_error("Number of dimensions must be one.");
            }

            r.sample(get_it(population), population.size(), get_it(choices), choices.size());

            return choices;
        });

        //////////////////
        // shuffle
        /////////////////

        Random.def("shuffle",
            [](Random_t r, py::array_t<nupic::UInt32>& a)
        {
            //py::scoped_ostream_redirect stream(
            //    std::cout,                               // std::ostream&
            //    py::module::import("sys").attr("stdout") // Python output
            //);

            if (a.ndim() != 1)
            {
                throw std::runtime_error("Number of dimensions must be one.");
            }

            r.shuffle(get_it(a), get_end(a));
        });

        ////////////////////

        Random.def("initializeUInt32Array", [](Random_t& self, py::array_t<nupic::UInt32>& a, nupic::UInt32 max_value)
        {
            auto array_data = get_it(a);

            for (nupic::UInt32 i = 0; i != a.size(); ++i)
                array_data[i] = self.getUInt32() % max_value;

        });


        //////////////////
        // pickle
        /////////////////
        Random.def(py::pickle(
            [](const Random_t& r)
        {
            std::stringstream ss;
            ss << r;
            return ss.str();
        },
            [](const std::string& str)
        {
            if (str.empty())
            {
                throw std::runtime_error("Empty state");
            }

            std::stringstream ss(str);
            Random_t r;
            ss >> r;

            return r;
        }
        ));

    }

} // namespace nupic_ext