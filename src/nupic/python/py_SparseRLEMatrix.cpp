
#include <fstream>

#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/math/SparseRLEMatrix.hpp>

#include "py_utils.hpp"

namespace py = pybind11;

namespace nupic_ext
{
    void init_SparseRLEMatrix(py::module& m)
    {
        // ////////////////////////////
        // nupic::SparseRLEMatrix<nupic::UInt16, unsigned char>
        // ////////////////////////////
        // def __str__(self):
        // 
        // def __setstate__(self, inString):
        // 
        // inline PyObject* __getstate__() const
        // 
        // inline void readState(PyObject* str)
        // 
        // inline void appendRow(PyObject* py_x)
        // 
        // inline PyObject* getRowToDense(nupic::UInt32 row) const
        // 
        // inline nupic::UInt16 firstRowCloserThan(PyObject* py_x, nupic::Real32 d) const
        // 
        // inline void fromDense(PyObject* py_m)
        // 
        // inline PyObject* toDense() const
        // 
        // inline PyObject* toCSR() const
        // 
        // inline void fromCSR(PyObject* str)
        // 
        // inline void CSRSaveToFile(const std::string& filename) const
        // 
        // inline void CSRLoadFromFile(const std::string& filename)
        // 
        // 
        // 
        // ////////////////////////////
        // nupic::SparseRLEMatrix<nupic::UInt16, nupic::UInt16>
        // ////////////////////////////
        // def __str__(self):
        // 
        // def __setstate__(self, inString):
        // 
        // inline PyObject* __getstate__() const
        // 
        // inline void readState(PyObject* str)
        // 
        // inline void appendRow(PyObject* py_x)
        // 
        // inline PyObject* getRowToDense(nupic::UInt32 row) const
        // 
        // inline nupic::UInt16 firstRowCloserThan(PyObject* py_x, nupic::Real32 d) const
        // 
        // inline void fromDense(PyObject* py_m)
        // 
        // inline PyObject* toDense() const
        // 
        // inline PyObject* toCSR() const
        // 
        // inline void fromCSR(PyObject* str)
        // 
        // inline void CSRSaveToFile(const std::string& filename) const
        // 
        // inline void CSRLoadFromFile(const std::string& filename)
        // 
        // 
        // ////////////////////////////  
        // nupic::SparseRLEMatrix<nupic::UInt32, nupic::Real32>
        // ////////////////////////////
        // 
        // def __str__(self):
        // return self.toDense().__str__()
        // 
        // def __setstate__(self, inString):
        // 
        // inline PyObject* __getstate__() const
        // 
        // inline void readState(PyObject* str)
        // 
        // inline void appendRow(PyObject* py_x)
        // 
        // inline PyObject* getRowToDense(nupic::UInt32 row) const
        // 
        // inline nupic::UInt32 firstRowCloserThan(PyObject* py_x, nupic::Real32 d) const
        // 
        // inline void fromDense(PyObject* py_m)
        // 
        // inline PyObject* toDense() const
        // 
        // inline PyObject* toCSR() const
        // 
        // inline void fromCSR(PyObject* str)
        // 
        // inline void CSRSaveToFile(const std::string& filename) const
        // 
        // inline void CSRLoadFromFile(const std::string& filename)    
    }
} // namespace nupix_ext
