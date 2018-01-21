#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/algorithms/Segment.hpp>

#include "py_utils.hpp"

namespace py = pybind11;

namespace nupic_ext
{
    void init_algorithms(py::module& m)
    {
        typedef nupic::algorithms::Cells4::Segment Segment_t;

        // defines some functions
        m.def("getSegmentActivityLevel", [](std::vector<Segment_t>& seg
            , py::array_t<nupic::Byte>& py_state
            , bool connectedSynapsesOnly
            , nupic::Real32 connectedPerm)
        {
            nupic::UInt32 activity = 0;

            auto state = get_it(py_state);
            auto stride0 = py_state.request().strides[0];

            // see algorithms.i[748]
            if (connectedSynapsesOnly)
            {
                for (int i = 0; seg.size(); ++i)
                {
                    auto p = seg[i][2].permanence();
                    if (p >= connectedPerm)
                    {
                        nupic::UInt32 c = seg[i][0].srcCellIdx();
                        nupic::UInt32 j = seg[i][1].srcCellIdx();

                        activity += state[c * stride0 + j];
                    }
                }
            }
            else
            {
                for (int i = 0; seg.size(); ++i)
                {
                    nupic::UInt32 c = seg[i][0].srcCellIdx();
                    nupic::UInt32 j = seg[i][1].srcCellIdx();

                    activity += state[c * stride0 + j];
                }
            }

            return activity;
        });

        m.def("isSegmentActive", [](std::vector<Segment_t>& seg, py::array_t<nupic::Byte>& py_state,
            nupic::Real32 connectedPerm,
            nupic::UInt32 activationThreshold)
        {
            throw std::runtime_error("Not implemented.");

            // see algorithms.i[785]


            return false;

        });


    }

} // namespace nupix_ext
