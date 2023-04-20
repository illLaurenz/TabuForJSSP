#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "jssp.h"
#include "ts.h"
#include "mem.h"

namespace py = pybind11;

PYBIND11_MODULE(metaheuristic, m) {
    m.doc() = R"pbdoc(
        Metaheuristic plugin
        --------------------

        .. currentmodule:: metaheuristic

        .. autosummary::
            :toctree: _generate

            JSSPInstance
            BMResult
            TabuSearch
            MemeticAlgorithm

    )pbdoc";
    py::class_<JSSPInstance>(m, "JSSPInstance")
            .def(py::init<std::string &>())
            .def(py::init<std::string &, int>())
            .def("calcMakespan", &JSSPInstance::calcMakespan)
            .def("generateRandomSolution", &JSSPInstance::generateRandomSolution)
            .def("getSeed",&JSSPInstance::getSeed);
    py::class_<BMResult>(m, "BMResult")
            .def(py::init<std::vector<std::vector<int>>, int>())
            .def_readwrite("solution", &BMResult::solution)
            .def_readwrite("makespan", &BMResult::makespan)
            .def_readwrite("history", &BMResult::history);
    py::class_<Solution>(m, "Solution")
            .def(py::init<std::vector<std::vector<int>>, int>())
            .def_readwrite("solution", &Solution::solution)
            .def_readwrite("makespan", &Solution::makespan);
    py::class_<TabuSearch>(m,"TabuSearch")
            .def(py::init<JSSPInstance &>())
            .def("optimize", &TabuSearch::optimize);
    py::class_<MemeticAlgorithm>(m,"MemeticAlgorithm")
            .def(py::init<JSSPInstance &, int, int>())
            .def("optimize", &MemeticAlgorithm::optimize_pop)
            .def("optimize", &MemeticAlgorithm::optimize);
}
