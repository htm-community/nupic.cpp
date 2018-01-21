#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

#include <nupic/os/Timer.hpp>

#include <nupic/engine/Network.hpp>
#include <nupic/engine/Region.hpp>


using namespace nupic;
namespace py = pybind11;

void test_PyBindRegion();

int main()
{
    // not needed since PyBindRegion is already initializing python interpreter, via initialize_interpreter
    ////py::scoped_interpreter guard{};
    //try
    //{
    //    test_PyBindRegion();
    //}
    //catch (py::error_already_set& e)
    //{
    //    std::cout << e.what() << std::endl;
    //}


    py::scoped_interpreter guard{};

    try
    {
        // Work around: AttributeError: module 'sys' has no attribute 'argv'
        py::exec(R"(
            import numpy;
            from nupic.bindings.math import *

            s = SM32(numpy.random.random((4,4)))
            t = SM32(numpy.random.random((4,4)))
            print('\ns / 3.1\n', s / 3)

            # calls py::kwargs
            #s = SparseMatrix(dtype='Float32')

            # calls py::args
            #s = SparseMatrix([[1,2],[3,4]])

            # calls py::args && py::kwargs
            #s = SparseMatrix([[1,2],[3,4]], dtype='Float32')

            s = SM32(numpy.random.random((4,4)))
            s.threshold(.5)
        )");

        //py::eval_file(R"(D:\nupic\src\python\python3\tests\unit\nupic\regions\record_sensor_region_test.py)");
    }
    catch (const py::error_already_set& e)
    {
        std::cout << e.what() << std::endl;
    }
    catch (const std::runtime_error& e)
    {
        std::cout << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unexpected error." << std::endl;
    }


        return 0;
}