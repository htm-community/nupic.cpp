#include <codecvt>
#include <locale>

#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/types/BasicType.hpp>
#include <nupic/algorithms/Svm.hpp>

#include "py_utils.hpp"

namespace py = pybind11;

namespace nupic_ext
{
    void init_SVM(py::module& m)
    {
        /////////////////////
        // svm_problem
        /////////////////////
        typedef nupic::algorithms::svm::svm_problem svm_problem_t;

        py::class_<svm_problem_t> py_svm_problem(m, "svm_problem");

        py_svm_problem.def(py::init<int, bool, float>(), py::arg("n_dims"), py::arg("recover"), py::arg("float") = 0);
        py_svm_problem.def(py::init<int, int, bool, float>(), py::arg("n_dims"), py::arg("size"), py::arg("recover"), py::arg("float") = 0);

        py_svm_problem.def("size", &svm_problem_t::size);
        py_svm_problem.def("n_dims", &svm_problem_t::n_dims);
        py_svm_problem.def("nnz", &svm_problem_t::nnz);

        py_svm_problem.def("resize", &svm_problem_t::resize);

        py_svm_problem.def("add_sample", [](svm_problem_t& self, float y_val, py::array_t<float>& x_vector)
        {
            self.add_sample(y_val, get_it(x_vector));
        });


        /////////////////////
        // svm_problem01
        /////////////////////
        typedef nupic::algorithms::svm::svm_problem01 svm_problem01_t;
        
        py::class_<svm_problem01_t> py_svm_problem01(m, "svm_problem01");

        py_svm_problem01.def(py::init<int, bool, float>(), py::arg("n_dims"), py::arg("recover"), py::arg("threshold") = .9);
        py_svm_problem01.def(py::init<int, int, bool, float>(), py::arg("n_dims"), py::arg("size"), py::arg("recover"), py::arg("threshold") = .9);

        py_svm_problem01.def("size", &svm_problem01_t::size);
        py_svm_problem01.def("n_dims", &svm_problem01_t::n_dims);
        py_svm_problem01.def("nnz", &svm_problem01_t::nnz);

        py_svm_problem01.def("resize", &svm_problem01_t::resize);

        py_svm_problem01.def("add_sample", [](svm_problem01_t& self, float y_val, py::array_t<float>& x_vector)
        {
            self.add_sample(y_val, get_it(x_vector));
        });


        /////////////////////
        // svm_dense
        /////////////////////
        typedef nupic::algorithms::svm::svm_dense svm_dense_t;

        py::class_<svm_dense_t> py_svm_dense(m, "svm_dense");

        py_svm_dense.def(py::init<int, int, float, int, int, bool, int>()
            , py::arg("kernel") = 0
            , py::arg("n_dims") = 0
            , py::arg("threshold") = 0.9
            , py::arg("cache_size") = 100
            , py::arg("shrinking") = 1
            , py::arg("probability") = false
            , py::arg("seed") = -1
        );

        py_svm_dense.def("persistent_size", &svm_dense_t::persistent_size);

        py_svm_dense.def("add_sample", [](svm_dense_t& self, float y_val, py::array_t<float>& x_vector)
        {
            self.add_sample(y_val, get_it(x_vector));
        });

        py_svm_dense.def("predict", [](svm_dense_t& self, py::array_t<float>& x_vector)
        {
            self.predict(get_it(x_vector));
        });

        py_svm_dense.def("predict_probability", [](svm_dense_t& self, py::array_t<float>& x_vector, py::array_t<float>& proba_vector)
        {
            self.predict_probability(get_it(x_vector), get_it(proba_vector));
        });


        //py_svm_dense.def("cross_validate", [](svm_dense_t& self, float gamma, float C, float eps)
        //{
        //    // http://pybind11.readthedocs.io/en/master/advanced/misc.html#global-interpreter-lock-gil
        //    py::gil_scoped_release release;

        //    self.train(gamma, C, eps);
        //}, "", py::arg("gamma"), py::arg("C"), py::arg("eps"));

        py_svm_dense.def("cross_validate", [](svm_dense_t& self, int n_fold, float gamma, float C, float eps)
        {
            // http://pybind11.readthedocs.io/en/master/advanced/misc.html#global-interpreter-lock-gil
            py::gil_scoped_release release;

            auto accuracy = self.cross_validation(n_fold, gamma, C, eps);

            return accuracy;
        }, "", py::arg("n_fold"), py::arg("gamma"), py::arg("C"), py::arg("eps"));

        py_svm_dense.def("train", &svm_dense_t::train, "", py::arg("gamma"), py::arg("C"), py::arg("eps"));


        // pickle
        // when using python 3 python returns:
        // "UnicodeDecodeError: 'utf-8' codec can't decode byte 0x80 in position 68: invalid start byte"
        // @todo So, I'm trying to fix that with wstring, not sure that's correct
        //    better would be to fix svm_dense_t::save() not to create non-ascii characters

        // @todo there is no copy constructor in svm_dense_t. Does it make sense to add move constuctor?
#if PY_MAJOR_VERSION >= 3
        py_svm_dense.def(py::pickle(
            [](const svm_dense_t& self) -> std::wstring
        {
            // __getstate__
            std::ostringstream os;
            self.save(os);

            auto s1 = os.str();

            std::wstring ws(s1.begin(), s1.end());

            return ws;
        },
            [](const std::wstring& wstr)
        {
            // __setstate__
            if (wstr.empty())
            {
                throw std::runtime_error("Empty state");
            }

            using convert_typeX = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            auto str = converterX.to_bytes(wstr);

            std::istringstream is(str);

            auto a = std::make_unique<svm_dense_t>();
            a->load(is);

            return a;
        }
        ));
#else
        py_svm_dense.def(py::pickle(
            [](const svm_dense_t& self) -> std::string
        {
            // __getstate__
            std::ostringstream os;
            self.save(os);

            return os.str();
        },
            [](const std::string& str) -> std::unique_ptr<svm_dense_t>
        {
            // __setstate__
            if (str.empty())
            {
                throw std::runtime_error("Empty state");
            }

            std::istringstream is(str);

            auto a = std::make_unique<svm_dense_t>();
            a->load(is);

            return a;
        }
        ));
#endif

        /////////////////////
        // svm_dense
        /////////////////////
        typedef nupic::algorithms::svm::svm_01 svm_01_t;

        py::class_<svm_01_t> py_svm_01(m, "svm_01");

        py_svm_01.def(py::init<int, int, float, int, int, bool, int>()
            , py::arg("kernel") = 0
            , py::arg("n_dims") = 0
            , py::arg("threshold") = 0.9
            , py::arg("cache_size") = 100
            , py::arg("shrinking") = 1
            , py::arg("probability") = false
            , py::arg("seed") = -1
        );

        py_svm_01.def("persistent_size", &svm_01_t::persistent_size);

        py_svm_01.def("add_sample", [](svm_01_t& self, float y_val, py::array_t<float>& x_vector)
        {
            self.add_sample(y_val, get_it(x_vector));
        });

        py_svm_01.def("predict", [](svm_01_t& self, py::array_t<float>& x_vector)
        {
            self.predict(get_it(x_vector));
        });

        py_svm_01.def("predict_probability", [](svm_01_t& self, py::array_t<float>& x_vector, py::array_t<float>& proba_vector)
        {
            self.predict_probability(get_it(x_vector), get_it(proba_vector));
        });

        py_svm_01.def("cross_validate", [](svm_01_t& self, int n_fold, float gamma, float C, float eps)
        {
            // http://pybind11.readthedocs.io/en/master/advanced/misc.html#global-interpreter-lock-gil
            py::gil_scoped_release release;

            auto accuracy = self.cross_validation(n_fold, gamma, C, eps);

            return accuracy;
        }, "", py::arg("n_fold"), py::arg("gamma"), py::arg("C"), py::arg("eps"));

        py_svm_01.def("train", &svm_01_t::train, "", py::arg("gamma"), py::arg("C"), py::arg("eps"));




#if PY_MAJOR_VERSION >= 3
        py_svm_01.def(py::pickle(
            [](const svm_01_t& self) -> std::wstring
        {
            // __getstate__
            std::ostringstream os;
            self.save(os);

            auto s1 = os.str();

            std::wstring ws(s1.begin(), s1.end());

            return ws;
        },
            [](const std::wstring& wstr) -> std::unique_ptr<svm_01_t>
        {
            // __setstate__
            if (wstr.empty())
            {
                throw std::runtime_error("Empty state");
            }

            using convert_typeX = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            auto str = converterX.to_bytes(wstr);

            std::istringstream is(str);

            auto a = std::make_unique<svm_01_t>();
            a->load(is);

            return a;
        }
        ));
#else
        py_svm_01.def(py::pickle(
            [](const svm_01_t& self) -> std::string
        {
            // __getstate__
            std::ostringstream os;
            self.save(os);

            return os.str();
        },
            [](const std::string& str) -> std::unique_ptr<svm_01_t>
        {
            // __setstate__
            if (str.empty())
            {
                throw std::runtime_error("Empty state");
            }

            std::istringstream is(str);

            auto a = std::make_unique<svm_01_t>();
            a->load(is);

            return a;
        }
        ));
#endif

    }

} // namespace nupix_ext
