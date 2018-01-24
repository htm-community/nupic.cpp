#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>


#include <nupic/os/OS.hpp>
#include <nupic/os/Timer.hpp>

#include <nupic/engine/Link.hpp>
#include <nupic/engine/Network.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/Spec.hpp>

namespace py = pybind11;
using namespace nupic;

namespace nupic_ext
{
    void init_Engine(py::module& m)
    {
        ///////////////////
        // Dimensions
        ///////////////////

        py::class_<Dimensions> py_Dimensions(m, "Dimensions");

        // constructors
        py_Dimensions.def(py::init<>())
            .def(py::init<std::vector<size_t>>())
            .def(py::init<size_t>())
            .def(py::init<size_t, size_t>())
            .def(py::init<size_t, size_t, size_t>());
            
        // members
        py_Dimensions.def("getCount", &Dimensions::getCount)
            .def("getDimensionCount", &Dimensions::getDimensionCount)
            .def("getDimension", &Dimensions::getDimension)
            .def("isUnspecified", &Dimensions::isUnspecified)
            .def("isDontcare", &Dimensions::isDontcare)
            .def("isSpecified", &Dimensions::isSpecified)
            .def("isOnes", &Dimensions::isOnes)
            .def("isValid", &Dimensions::isValid)
            .def("getIndex", &Dimensions::getIndex)
            .def("getCoordinate", &Dimensions::getCoordinate)
            .def("toString", &Dimensions::toString, "", py::arg("humanReadable") = true)
            .def("promote", &Dimensions::promote)
            ;

        // operator overloading
        py_Dimensions.def(py::self == py::self)
            .def(py::self != py::self);

        // python slots
        py_Dimensions.def("__str__", &Dimensions::toString)
            .def("__repr__", &Dimensions::toString);

        ///////////////////
        // Link
        ///////////////////
        py::class_<Link, std::shared_ptr<Link>> py_Link(m, "Link");

        // constructors
        py_Link.def(py::init<>())
            .def(py::init<const std::string&, const std::string&
                , const std::string&, const std::string&
                , const std::string&, const std::string&
                , size_t>()
                , ""
                , py::arg("linkType"), py::arg("linkParams")
                , py::arg("srcRegionName"), py::arg("destRegionName")
                , py::arg("srcOutputName") = "", py::arg("destInputName") = ""
                , py::arg("propagationDelay") = 0);




        ///////////////////
        // Spec
        ///////////////////
        py::class_<Spec> py_Spec(m, "Spec");

        ///////////////////
        // Region
        ///////////////////

        py::class_<Region, std::shared_ptr<Region>> py_Region(m, "Region");
        
        py_Region.def("getName", &Region::getName)
            .def("getType", &Region::getType)
            .def("getDimensions", &Region::getDimensions)
            .def("setDimensions", &Region::setDimensions);
        
        /*
        
        getSpec

        static member
        getSpecFromType
        */


        ///////////////////
        // Network
        ///////////////////

        py::class_<Network> py_Network(m, "Network");
        
        // constructors
        py_Network.def(py::init<>())
            .def(py::init<std::string>());

        py_Network.def("addRegion", &nupic::Network::addRegion)
            .def("getRegions", &nupic::Network::getRegions)
            .def("getLinks", &nupic::Network::getLinks)
            .def("getMinPhase", &nupic::Network::getMinPhase)
            .def("getMaxPhase", &nupic::Network::getMaxPhase)
            .def("setMinEnabledPhase", &nupic::Network::getMinPhase)
            .def("setMaxEnabledPhase", &nupic::Network::getMaxPhase)
            .def("getMinEnabledPhase", &nupic::Network::getMinPhase)
            .def("getMaxEnabledPhase", &nupic::Network::getMaxPhase);

        py_Network.def("initialize", &nupic::Network::initialize);

        py_Network.def("link", &nupic::Network::link
            , ""
            , py::arg("srcName"), py::arg("destName")
            , py::arg("linkType"), py::arg("linkParams")
            , py::arg("srcOutput") = "", py::arg("destInput") = "", py::arg("propagationDelay") = 0);


        
        py_Network.def_static("registerPyRegion", &nupic::Network::registerPyBindRegion);


        py_Network.def_static("unregisterPyRegion", &nupic::Network::unregisterPyBindRegion);



        ///////////////////
        // Collection
        ///////////////////

        // Regions
        typedef Collection<std::shared_ptr<Region>> Region_Collection_t;
        py::class_<Region_Collection_t> py_RegionCollection(m, "RegionCollection");
        py_RegionCollection.def("getByName", &Region_Collection_t::getByName);
        py_RegionCollection.def("contains", &Region_Collection_t::contains);
        py_RegionCollection.def("getCount", &Region_Collection_t::getCount);

        // bare bone sequence protocol
        py_RegionCollection.def("__len__", &Region_Collection_t::getCount);
        py_RegionCollection.def("__getitem__", [](Region_Collection_t& coll, size_t i) 
        { 
            if (i >= coll.getCount())
            {
                throw py::index_error();
            }

            return coll.getByIndex(i); 
        });
        
        // Links
        typedef Collection<std::shared_ptr<Link>> Link_Collection_t;
        py::class_<Link_Collection_t> py_LinkCollection(m, "Link_Collection_t");
        py_LinkCollection.def("getByName", &Link_Collection_t::getByName);
        py_LinkCollection.def("contains", &Link_Collection_t::contains);
        py_LinkCollection.def("getCount", &Link_Collection_t::getCount);

        // bare bone sequence protocol
        py_LinkCollection.def("__len__", &Link_Collection_t::getCount);
        py_LinkCollection.def("__getitem__", [](Link_Collection_t& coll, size_t i)
        {
            if (i >= coll.getCount())
            {
                throw py::index_error();
            }

            return coll.getByIndex(i);
        });

        // not sure we need __iter__
        //py_LinkCollection.def("__iter__", [](Link_Collection_t& coll) { return py::make_iterator(coll.begin(), coll.end()); }, py::keep_alive<0, 1>());
    }

} // namespace nupix_ext
