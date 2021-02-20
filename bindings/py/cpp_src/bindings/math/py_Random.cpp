/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2018, Numenta, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * Author: @chhenning, 2018
 * --------------------------------------------------------------------- */

/** @file
PyBind11 bindings for Random class
*/

#include <bindings/suppress_register.hpp>  //include before pybind11.h
#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <htm/utils/Random.hpp>

#include "bindings/engine/py_utils.hpp"

namespace py = pybind11;

namespace htm_ext {

    void init_Random(py::module& m)
    {
        typedef htm::Random Random_t;

        py::class_<Random_t> Random(m, "Random");

        Random.def(py::init<htm::UInt64>(), py::arg("seed") = 0)
              .def("getUInt32", &Random_t::getUInt32, py::arg("max") = (htm::UInt32)-1l)
              .def("getReal64", &Random_t::getReal64)
	      .def("getSeed", &Random_t::getSeed)
              .def("max", &Random_t::max)
              .def("min", &Random_t::min)
              .def("__eq__", [](Random_t const & self, Random_t const & other) { return self == other; }, py::is_operator()); //operator==

        Random.def_property_readonly_static("MAX32", [](py::object) {
				return Random_t::MAX32;
			});

        //////////////////
        // sample
        /////////////////


        Random.def("sample", [](Random_t& r, py::array& population, const htm::UInt32 nSelect)
        {
            if (population.ndim() != 1 )
            {
                throw std::invalid_argument("Number of dimensions must be one.");
            }

	    std::vector<htm::UInt32> tmp_pop(get_it<htm::UInt32>(population), get_it<htm::UInt32>(population) + population.size()); //vector from numpy.array
            return r.sample(tmp_pop, nSelect);
        });

        //////////////////
        // shuffle
        /////////////////

        Random.def("shuffle",
            [](Random_t& r, py::array& a)
        {
            //py::scoped_ostream_redirect stream(
            //    std::cout,                               // std::ostream&
            //    py::module::import("sys").attr("stdout") // Python output
            //);

            if (a.ndim() != 1)
            {
                throw std::invalid_argument("Number of dimensions must be one.");
            }

            r.shuffle(get_it<htm::UInt32>(a), get_end<htm::UInt32>(a));
        });

        ////////////////////

        Random.def("initializeUInt32Array", [](Random_t& self, py::array& a, htm::UInt32 max_value)
        {
            auto array_data = get_it<htm::UInt32>(a);

            for (auto i = a.size()-1; i >= 0; --i)
                array_data[i] = self.getUInt32(max_value);

        });


        ////////////////////

        Random.def("initializeReal64Array", [](Random_t& self, py::array& a)
        {
            auto array_data = get_it<htm::Real64>(a);

            for (auto i = a.size()-1; i >=0; --i)
                array_data[i] = self.getReal64();

        });


        //////////////////
        // serialization
        /////////////////
				Random.def("saveToFile", [](Random_t& self, const std::string& name, int fmt) { 
				  htm::SerializableFormat fmt1;
				  switch(fmt) {                                             
			                case 0: fmt1 = htm::SerializableFormat::BINARY; break;
			                case 1: fmt1 = htm::SerializableFormat::PORTABLE; break;
					case 2: fmt1 = htm::SerializableFormat::JSON; break;
					case 3: fmt1 = htm::SerializableFormat::XML; break;
					default: NTA_THROW << "unknown serialization format.";
					} 
					self.saveToFile(name, fmt1); 
				}, "serialize to a File, using BINARY=0, PORTABLE=1, JSON=2, or XML=3 format.",
			  py::arg("name"), py::arg("fmt") = 0);
				
				Random.def("loadFromFile", [](Random_t& self, const std::string& name, int fmt) { 
				  htm::SerializableFormat fmt1;
				  switch(fmt) {                                             
			    case 0: fmt1 = htm::SerializableFormat::BINARY; break;
			    case 1: fmt1 = htm::SerializableFormat::PORTABLE; break;
					case 2: fmt1 = htm::SerializableFormat::JSON; break;
					case 3: fmt1 = htm::SerializableFormat::XML; break;
					default: NTA_THROW << "unknown serialization format.";
					} 
				  self.loadFromFile(name, fmt1);
				}, "load from a File, using BINARY, PORTABLE, JSON, or XML format.",
				py::arg("name"), py::arg("fmt") = 0);


        Random.def(py::pickle(
            [](const Random_t& r) //__getstate__
        {
            std::stringstream ss;
            r.save(ss); //save r's state to archive (stream) with cereal

	    /* The values in stringstream are binary so pickle will get confused
             * trying to treat it as utf8 if you just return ss.str().
             * So we must treat it as py::bytes.  Some characters could be null values.
             */
            return py::bytes( ss.str() );
        },

	   [](py::bytes &s)   // __setstate__
       {
           /* pybind11 will pass in the bytes array without conversion.
            * so we should be able to just create a string to initalize the stringstream.
            */
           std::stringstream ss( s.cast<std::string>() );
           std::unique_ptr<htm::Random> r(new htm::Random());
           r->load(ss);

           /*
            * The __setstate__ part of the py::pickle() is actually a py::init() with some options.
            * So the return value can be the object returned by value, by pointer,
            * or by container (meaning a unique_ptr). SP has a problem with the copy constructor
            * and pointers have problems knowing who the owner is so lets use unique_ptr.
            * See: https://pybind11.readthedocs.io/en/stable/advanced/classes.html#custom-constructors
            */
           return r;
        }
        ));

    }

} // namespace htm_ext
