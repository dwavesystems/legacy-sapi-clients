//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <Python.h>

#include <limits>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <sapi-local/sapi-local.hpp>

#include "python-exception.hpp"

using std::numeric_limits;
using std::vector;

using boost::numeric_cast;

using sapilocal::MatrixEntry;
using sapilocal::makeMatrixEntry;
using sapilocal::SparseMatrix;
using sapilocal::OrangStructure;
using sapilocal::OrangSampleParams;
using sapilocal::OrangOptimizeParams;
using sapilocal::OrangHeuristicParams;

namespace {

namespace keys {
auto numVars = "num_vars";
auto activeVars = "active_vars";
auto activeVarPairs = "active_var_pairs";
auto varOrder = "var_order";
auto numReads = "num_reads";
auto maxAnswers = "max_answers";
auto answerHistogram = "answer_histogram";
auto beta = "beta";
auto randomSeed = "random_seed";

auto iterationLimit = "iteration_limit";
auto maxBitFlipProb = "max_bit_flip_prob";
auto maxComplexity = "max_local_complexity";
auto minBitFlipProb = "min_bit_flip_prob";
auto noProgressLimit = "local_stuck_limit";
auto numPerturbedCopies = "num_perturbed_copies";
auto numVariables = "num_variables";
auto timeLimitSeconds = "time_limit_seconds";

auto energies = "energies";
auto solutions = "solutions";
auto numOccurrences = "num_occurrences";
} // namespace keys

class NewPyRef {
private:
  PyObject *obj_;
  NewPyRef(const NewPyRef& other);
  NewPyRef& operator=(const NewPyRef& other);

public:
  explicit NewPyRef(PyObject* obj, bool allowNull=false) : obj_(obj) {
    if (!obj_ && !allowNull && PyErr_Occurred()) throw PythonException();
  }

  NewPyRef(NewPyRef&& other) : obj_(other.obj_) { other.obj_ = 0; }

  NewPyRef& operator=(NewPyRef&& other) {
    if (&other != this) {
      Py_XDECREF(obj_);
      obj_ = other.obj_;
      other.obj_ = 0;
    }
    return *this;
  }

  ~NewPyRef() { Py_XDECREF(obj_); }

  PyObject* get() { return obj_; }
  operator PyObject*() { return get(); }

  PyObject* release() { auto t = obj_; obj_ = 0; return t; }
};

NewPyRef getKey(PyObject* dict, const char* key) {
  return NewPyRef(PyMapping_GetItemString(dict, const_cast<char*>(key)));
}

int getInt(PyObject* obj) {
  auto x = PyLong_AsLong(obj);
  if (x == -1 && PyErr_Occurred()) throw PythonException();
  if (x < numeric_limits<int>::min() || x > numeric_limits<int>::max()) {
    PyErr_SetString(PyExc_OverflowError, "");
    throw PythonException();
  }
  return static_cast<int>(x);
}

double getDouble(PyObject* obj) {
  auto x = PyFloat_AsDouble(obj);
  if (x == -1.0 && PyErr_Occurred()) throw PythonException();
  return x;
}

int getVarIndex(PyObject* obj, Py_ssize_t i) {
  return getInt(NewPyRef(PySequence_GetItem(obj, i))); // new ref
}

int getIntKey(PyObject* obj, const char* key) {
  return getInt(getKey(obj, key)); // new ref;
}

MatrixEntry matrixEntry(PyObject* key, PyObject* value) {
  auto len = PySequence_Length(key);
  if (len == -1) throw PythonException();
  if (len != 2) {
    PyErr_SetString(PyExc_ValueError, "problem keys must have length 2");
    throw PythonException();
  }
  auto e = makeMatrixEntry(getVarIndex(key, 0), getVarIndex(key, 1), PyFloat_AsDouble(value));
  if (e.value == -1.0 && PyErr_Occurred()) throw PythonException();
  return e;
}

OrangStructure::VarVector convertVarVector(PyObject* obj) {
  OrangStructure::VarVector vec;
  auto iter = NewPyRef(PyObject_GetIter(obj)); // new ref

  while (auto item = NewPyRef(PyIter_Next(iter), true)) { // new ref
    vec.push_back(getInt(item));
  }

  return vec;
}

OrangStructure::VarPairVector convertVarPairVector(PyObject* obj) {
  OrangStructure::VarPairVector vec;
  auto iter = NewPyRef(PyObject_GetIter(obj)); // new ref

  while (auto item = NewPyRef(PyIter_Next(iter))) { // new ref
    auto len = PySequence_Length(item);
    if (len == -1) throw PythonException();
    if (len != 2) {
      PyErr_SetString(PyExc_ValueError, "variable pair must have length 2");
      throw PythonException();
    }

    vec.push_back(OrangStructure::VarPair(getVarIndex(item, 0), getVarIndex(item, 1)));
  }

  return vec;
}

OrangStructure orangStructure(PyObject* obj) {
  OrangStructure s;
  s.numVars = getInt(getKey(obj, keys::numVars));
  s.activeVars = convertVarVector(getKey(obj, keys::activeVars));
  s.activeVarPairs = convertVarPairVector(getKey(obj, keys::activeVarPairs));
  s.varOrder = convertVarVector(getKey(obj, keys::varOrder));
  return s;
}

} // namespace {anonymous}

namespace typemaps {

SparseMatrix inSparseMatrix(PyObject* obj) {
  if (!PyDict_Check(obj)) {
    PyErr_SetString(PyExc_TypeError, "problem must be a dictionary");
    throw PythonException();
  }

  SparseMatrix problem;
  PyObject* key;
  PyObject* value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(obj, &pos, &key, &value)) problem.push_back(matrixEntry(key, value));

  return problem;
}

OrangSampleParams inOrangSampleParams(PyObject* obj) {
  if (!PyDict_Check(obj)) {
    PyErr_SetString(PyExc_TypeError, "parameters must be a dictionary");
    throw PythonException();
  }

  OrangSampleParams params;
  params.s = orangStructure(obj);
  params.numReads = getInt(getKey(obj, keys::numReads));
  params.maxAnswers = getInt(getKey(obj, keys::maxAnswers));
  params.answerHistogram = PyObject_IsTrue(getKey(obj, keys::answerHistogram));
  params.beta = PyFloat_AsDouble(getKey(obj, keys::beta));
  if (params.beta == -1.0 && PyErr_Occurred()) throw PythonException();

  auto randomSeedObj = NewPyRef(PyUnicode_FromString(keys::randomSeed));
  switch (PyDict_Contains(obj, randomSeedObj)) {
    case 0:
      params.useSeed = false;
      break;

    case 1:
    {
      auto seedObj = getKey(obj, keys::randomSeed);
      if (seedObj == Py_None) {
        params.useSeed = false;
      } else {
        params.useSeed = true;
        auto x = PyLong_AsLong(getKey(obj, keys::randomSeed));
        if (x == -1 && PyErr_Occurred()) throw PythonException();
        params.randomSeed = static_cast<unsigned int>(x);
      }
      break;
    }

    default:
      throw PythonException();
  }

  return params;
}

OrangOptimizeParams inOrangOptimizeParams(PyObject* obj) {
  if (!PyDict_Check(obj)) {
    PyErr_SetString(PyExc_TypeError, "parameters must be a dictionary");
    throw PythonException();
  }

  OrangOptimizeParams params;
  params.s = orangStructure(obj);
  params.numReads = getInt(getKey(obj, keys::numReads));
  params.maxAnswers = getInt(getKey(obj, keys::maxAnswers));
  params.answerHistogram = PyObject_IsTrue(getKey(obj, keys::answerHistogram));
  return params;
}

OrangHeuristicParams inOrangHeuristicParams(PyObject* obj) {
  if (!PyDict_Check(obj)) {
    PyErr_SetString(PyExc_TypeError, "parameters must be a dictionary");
    throw PythonException();
  }

  OrangHeuristicParams params;
  params.iterationLimit = getInt(getKey(obj, keys::iterationLimit));
  params.maxBitFlipProb = getDouble(getKey(obj, keys::maxBitFlipProb));
  params.maxComplexity = getInt(getKey(obj, keys::maxComplexity));
  params.minBitFlipProb = getDouble(getKey(obj, keys::minBitFlipProb));
  params.noProgressLimit = getInt(getKey(obj, keys::noProgressLimit));
  params.numPerturbedCopies = getInt(getKey(obj, keys::numPerturbedCopies));
  params.numVariables = getInt(getKey(obj, keys::numVariables));

  auto randomSeedObj = NewPyRef(PyUnicode_FromString(keys::randomSeed));
  switch (PyDict_Contains(obj, randomSeedObj)) {
    case 0:
      params.useSeed = false;
      break;

    case 1:
    {
      auto seedObj = getKey(obj, keys::randomSeed);
      if (seedObj == Py_None) {
        params.useSeed = false;
      } else {
        params.useSeed = true;
        auto x = PyLong_AsLong(getKey(obj, keys::randomSeed));
        if (x == -1 && PyErr_Occurred()) throw PythonException();
		params.rngSeed = static_cast<unsigned int>(x);
      }
      break;
    }

    default:
      throw PythonException();
  }

  params.timeLimitSeconds = getDouble(getKey(obj, keys::timeLimitSeconds));

  return params;
}

PyObject* outIsingResult(const sapilocal::IsingResult& result) {
  try {
    auto rObj = NewPyRef(PyDict_New());

    auto numSolutions = numeric_cast<Py_ssize_t>(result.energies.size());
    auto solutionSize = numeric_cast<Py_ssize_t>(result.solutions.size()) / numSolutions;
    if (numSolutions * solutionSize != result.solutions.size()) {
      PyErr_SetString(PyExc_RuntimeError, "Internal error! Inconsistent result sizes");
      return 0;
    }

    auto energiesObj = NewPyRef(PyList_New(numSolutions));
    for (auto i = 0; i < numSolutions; ++i) {
      auto eObj = NewPyRef(PyFloat_FromDouble(result.energies[i]));
      PyList_SET_ITEM(energiesObj.get(), i, eObj.release()); // steals ref
    }
    PyDict_SetItemString(rObj, keys::energies, energiesObj);

    if (result.numOccurrences.size() == result.energies.size()) {
      auto numOccObj = NewPyRef(PyList_New(numSolutions));
      for (auto i = 0; i < numSolutions; ++i) {
        auto nObj = NewPyRef(PyLong_FromLong(result.numOccurrences[i]));
        PyList_SET_ITEM(numOccObj.get(), i, nObj.release()); // steals ref
      }
      PyDict_SetItemString(rObj, keys::numOccurrences, numOccObj);
    } else if (!result.numOccurrences.empty()) {
      PyErr_SetString(PyExc_RuntimeError, "Internal error! Inconsistent result sizes");
      return 0;
    }

    auto solutionsObj = NewPyRef(PyList_New(numSolutions));
    auto si = 0;
    for (auto i = 0; i < numSolutions; ++i) {
      auto solutionObj = NewPyRef(PyList_New(solutionSize));
      for (auto j = 0; j < solutionSize; ++j) {
        auto nObj = NewPyRef(PyLong_FromLong(result.solutions[si++]));
        PyList_SET_ITEM(solutionObj.get(), j, nObj.release()); // steals ref
      }
      PyList_SET_ITEM(solutionsObj.get(), i, solutionObj.release()); // steals ref
    }
    PyDict_SetItemString(rObj, keys::solutions, solutionsObj);

    return rObj.release();

  } catch (PythonException&) {
    return 0;
  }
}

} // namespace typemaps
