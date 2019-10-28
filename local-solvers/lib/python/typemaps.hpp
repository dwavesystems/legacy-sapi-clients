//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPILOCAL_TYPEMAPS_HPP_INCLUDED
#define SAPILOCAL_TYPEMAPS_HPP_INCLUDED

#include <Python.h>
#include <sapi-local/sapi-local.hpp>

namespace typemaps {

sapilocal::SparseMatrix inSparseMatrix(PyObject* obj);
sapilocal::OrangSampleParams inOrangSampleParams(PyObject* obj);
sapilocal::OrangOptimizeParams inOrangOptimizeParams(PyObject* obj);
sapilocal::OrangHeuristicParams inOrangHeuristicParams(PyObject* obj);

PyObject* outIsingResult(const sapilocal::IsingResult& result);

} // namespace typemaps

#endif
