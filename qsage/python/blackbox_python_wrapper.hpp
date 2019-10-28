//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef BLACKBOX_PYTHON_WRAPPER_HPP_INCLUDED
#define BLACKBOX_PYTHON_WRAPPER_HPP_INCLUDED

#include <Python.h>
#include "blackbox.hpp"

PyObject* solve_qsage_impl(PyObject* objective_function, int num_vars, PyObject* ising_solver, PyObject* params);

#endif  // BLACKBOX_PYTHON_WRAPPER_HPP_INCLUDED
