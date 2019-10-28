//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef FIX_VARIABLES_PYTHON_WRAPPER_HPP_INCLUDED
#define FIX_VARIABLES_PYTHON_WRAPPER_HPP_INCLUDED

#include <Python.h>
#include "fix_variables.hpp"

PyObject* fix_variables_impl(PyObject* Q, PyObject* method=Py_None);

#endif  // FIX_VARIABLES_PYTHON_WRAPPER_HPP_INCLUDED
