#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/types/BasicType.hpp>

namespace py = pybind11;

namespace nupic_ext
{
    std::string GetNTARealType() { return "NTA_Real"; }

    std::string GetNumpyDataType(const std::string& type)
    {
#ifdef NTA_DOUBLE_PRECISION
        if (type == "NTA_Real")
        {
            return py::format_descriptor<double>::format();
        }
        else if (type == "NTA_Real32")
        {
            return py::format_descriptor<float>::format();
        }
        else if (type == "NTA_Real64")
        {
            return py::format_descriptor<double>::format();
        }

        throw std::runtime_error("Unsupported type name");
#else
        if (type == "NTA_Real")
        {
            return py::format_descriptor<float>::format();
        }
        else if (type == "NTA_Real32")
        {
            return py::format_descriptor<float>::format();
        }
        else if (type == "NTA_Real64")
        {
            return py::format_descriptor<double>::format();
        }

        throw std::runtime_error("Unsupported type name");
#endif // NTA_DOUBLE_PRECISION
    }

    void init_reals(py::module& m)
    {
        m.def("GetBasicTypeFromName", [](const std::string& type) { return nupic::BasicType::parse(type); });
        m.def("GetBasicTypeSize", [](const std::string& type) { return nupic::BasicType::getSize(nupic::BasicType::parse(type)); });

        m.def("GetNumpyDataType", &GetNumpyDataType);

        // GetNTARealType
        m.def("GetNTARealType", &GetNTARealType
            , "Gets the name of the NuPIC floating point base type, which is used for most internal calculations. This base type name can be used with GetBasicTypeFromName(),GetBasicTypeSize(), and GetNumpyDataType().");

        // GetNTAReal
        m.def("GetNTAReal", []()
        {
            return GetNumpyDataType(GetNTARealType());
        }, "Gets the numpy dtype of the NuPIC floating point base type, which is used for most internal calculations. The returned value can be used with numpy functions like numpy.array(..., dtype = dtype) and numpy.astype(..., dtype = dtype).");
    }

} // namespace nupix_ext
