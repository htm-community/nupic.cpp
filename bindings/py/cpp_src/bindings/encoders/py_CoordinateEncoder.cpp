/* ----------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2019, David McDougall
 * The following terms and conditions apply:
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
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------- */

#include <bindings/suppress_register.hpp>  //include before pybind11.h
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <nupic/encoders/CoordinateEncoder.hpp>

namespace py = pybind11;

using namespace std;
using namespace nupic;
using namespace nupic::encoders;
using nupic::sdr::SDR;

namespace nupic_ext
{
  void init_CoordinateEncoder(py::module& m)
  {
    py::class_<CoordinateEncoderParameters, RDSE_Parameters>
                            py_CoordEncParams(m, "CoordinateEncoderParameters",
R"(TODO: DOCUMENTATION)");

    py_CoordEncParams.def(py::init<>());

    py_CoordEncParams.def_readwrite("numDimensions", &CoordinateEncoderParameters::numDimensions,
R"(TODO: DOCUMENTATION)");

    py::class_<CoordinateEncoder> py_CoordinateEncoder(m, "CoordinateEncoder",
R"(TODO: DOCSTRINGS!)");

    py_CoordinateEncoder.def(py::init<CoordinateEncoderParameters&>());

    py_CoordinateEncoder.def_property_readonly("dimensions",
        [](const CoordinateEncoder &self) { return self.dimensions; });
    py_CoordinateEncoder.def_property_readonly("size",
        [](const CoordinateEncoder &self) { return self.size; });
    py_CoordinateEncoder.def_property_readonly("parameters",
        [](const CoordinateEncoder &self) { return self.parameters; },
R"(Contains the parameter structure which this encoder uses internally. All
fields are filled in automatically.)");

    py_CoordinateEncoder.def("encode", &CoordinateEncoder::encode);

    py_CoordinateEncoder.def("encode", []
        (CoordinateEncoder &self, vector<Real64> value) {
            auto sdr = new SDR({ self.size });
            self.encode( value, *sdr );
            return sdr;
    });
  }
}
