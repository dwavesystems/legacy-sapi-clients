//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef FIND_EMBEDDING_PYTHON_WRAPPER_HPP_INCLUDED
#define FIND_EMBEDDING_PYTHON_WRAPPER_HPP_INCLUDED

#include <Python.h>
#include "find_embedding.hpp"

PyObject* find_embedding_impl(PyObject* Q, PyObject* A, PyObject* params);

#endif  // FIND_EMBEDDING_PYTHON_WRAPPER_HPP_INCLUDED
