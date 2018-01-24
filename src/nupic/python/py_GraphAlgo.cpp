
#include <fstream>

#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "py_utils.hpp"

#include <nupic/math/SparseMatrix.hpp>
#include <nupic/math/GraphAlgorithms.hpp>

namespace py = pybind11;

namespace nupic_ext
{
    void init_graph_algo(py::module& m)
    {
        typedef nupic::SparseMatrix<nupic::UInt32, nupic::Real32, nupic::Int32, nupic::Real64, nupic::DistanceToZero<nupic::Real32>> SparseMatrix32_t;

        
        // PyObject* enumerate_sequences(nupic::Real threshold,PyObject* g,int cr=0,int ns=0)
        m.def("enumerate_sequences", [](nupic::Real threshold, const SparseMatrix32_t& sm, int cr, int ns)
        {
            try
            {

                std::list<std::vector<nupic::UInt32>> sequences;

                nupic::EnumerateSequences(threshold, sm, sequences, ns);
                nupic::UInt32 N = static_cast<nupic::UInt32>(sequences.size());

                py::list toReturn(N);

                std::list<std::vector<nupic::UInt32> >::const_iterator it;
                nupic::UInt32 i = 0;

                for (it = sequences.begin(); it != sequences.end(); ++it, ++i)
                {
                    const std::vector<nupic::UInt32>& seq = *it;
                    nupic::UInt32 M = static_cast<nupic::UInt32>(it->size());

                    py::list py_seq(M);

                    for (nupic::UInt32 j = 0; j != M; ++j)
                    {
                        py_seq[j] = seq[j];
                    }

                    toReturn[i] = py_seq;
                }

                return toReturn;
            }
            catch (...)
            {
                throw std::runtime_error("Unknown error in enumerate_sequences");
            }

        }, "", py::arg("threshold"), py::arg("g"), py::arg("cr") = 0, py::arg("ns") = 0);

        // PyObject* find_connected_components(nupic::Real threshold,PyObject* g)
        m.def("find_connected_components", [](nupic::Real threshold, const SparseMatrix32_t& sm)
        {
            std::list<std::vector<nupic::UInt32> > comps;

            nupic::FindConnectedComponents(threshold, sm, comps);
            nupic::UInt32 N = static_cast<nupic::UInt32>(comps.size());

            py::list toReturn(N);

            std::list<std::vector<nupic::UInt32> >::const_iterator it;
            nupic::UInt32 i = 0;

            for (it = comps.begin(); it != comps.end(); ++it, ++i)
            {
                const std::vector<nupic::UInt32>& comp = *it;

                nupic::UInt32 M = static_cast<nupic::UInt32>(it->size());

                py::list py_comp(M);

                for (nupic::UInt32 j = 0; j != M; ++j)
                {
                    py_comp[j] = comp[j];
                }

                toReturn[i] = py_comp;
            }

            return toReturn;
        });
    }
} // namespace nupix_ext
