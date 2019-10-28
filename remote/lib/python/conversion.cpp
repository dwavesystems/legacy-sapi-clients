//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <Python.h>

#include <limits>
#include <string>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/foreach.hpp>

#include <json.hpp>
#include <coding.hpp>
#include <types.hpp>

#include "conversion.hpp"
#include "python-exception.hpp"
#include "refholder.hpp"

using std::numeric_limits;
using std::string;

using boost::numeric_cast;

using sapiremote::QpProblem;
using sapiremote::QpProblemEntry;
using sapiremote::QpAnswer;
using sapiremote::toString;

namespace submittedstates = sapiremote::submittedstates;
namespace remotestatuses = sapiremote::remotestatuses;
namespace errortypes = sapiremote::errortypes;

namespace {

namespace answerkeys {
const auto format = "format";
const auto activeVars = "active_variables";
const auto numVars = "num_variables";
const auto solutions = "solutions";
const auto energies = "energies";
const auto numOccurrences = "num_occurrences";
} // namespace {anonymous}::answerkeys

namespace infokeys {
const auto problemId = "problem_id";
const auto state = "state";
const auto lastGoodState = "last_good_state";
const auto remoteStatus = "remote_status";
const auto errorType = "error_type";
const auto errorMessage = "error_message";
const auto submittedOn = "submitted_on";
const auto solvedOn = "solved_on";
} // namespace {anonymous}::infokeys

class JsonToPythonVisitor : public boost::static_visitor<PyObject*> {
public:
  PyObject* operator()(const json::Null&) const {
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject* operator()(bool x) const {
    auto ret = x ? Py_True : Py_False;
    Py_INCREF(ret);
    return ret;
  }

  PyObject* operator()(json::Integer x) const {
    if (x >= numeric_limits<long>::min() && x <= numeric_limits<long>::max()) {
      auto ret = PyInt_FromLong(static_cast<long>(x)); // new ref
      if (!ret) throw PythonException();
      return ret;
    } else {
      auto ret = PyLong_FromLongLong(x); // new ref
      if (!ret) throw PythonException();
      return ret;
    }
  }

  PyObject* operator()(double x) const {
    auto ret = PyFloat_FromDouble(x); // new ref
    if (!ret) throw PythonException();
    return ret;
  }

  PyObject* operator()(const string& x) const {
    auto ret = PyString_FromString(x.c_str()); // new ref
    if (!ret) throw PythonException();
    return ret;
  }

  PyObject* operator()(const json::Array& x) const {
    auto size = x.size() <= static_cast<size_t>(numeric_limits<Py_ssize_t>::max())
        ? static_cast<Py_ssize_t>(x.size()) : -1;
    auto pylist = PyList_New(size);
    if (!pylist) throw PythonException();
    RefHolder lrh(pylist);

    for (auto i = 0; i < size; ++i) {
      PyList_SET_ITEM(pylist, i, x[i].variant().apply_visitor(*this)); // steals ref
    }
    lrh.release();
    return pylist;
  }

  PyObject* operator()(const json::Object& x) const {
    auto pydict = PyDict_New();
    if (!pydict) throw PythonException();
    RefHolder drh(pydict);

    BOOST_FOREACH(const auto& v, x) {
      auto pyvalue = v.second.variant().apply_visitor(*this);
      RefHolder vrh(pyvalue);
      if (PyDict_SetItemString(pydict, v.first.c_str(), pyvalue) == -1) throw PythonException();
    }

    drh.release();
    return pydict;
  }
};

void addEnergiesToDict(const QpAnswer& qpAnswer, PyObject* d) {
  auto size = qpAnswer.energies.size() <= static_cast<size_t>(numeric_limits<Py_ssize_t>::max())
      ? static_cast<Py_ssize_t>(qpAnswer.energies.size()) : -1;
  auto pyEnergies = PyList_New(size);
  if (!pyEnergies) throw PythonException();
  RefHolder rh(pyEnergies);

  for (auto i = 0; i < size; ++i) {
    auto e = PyFloat_FromDouble(qpAnswer.energies[i]);
    if (!e) throw PythonException();
    PyList_SET_ITEM(pyEnergies, i, e); // steals ref
  }

  if (PyDict_SetItemString(d, answerkeys::energies, pyEnergies) == -1) throw PythonException();
}

void addNumOccToDict(const QpAnswer& qpAnswer, PyObject* d) {
  auto size = qpAnswer.numOccurrences.size() <= static_cast<size_t>(numeric_limits<Py_ssize_t>::max())
      ? static_cast<Py_ssize_t>(qpAnswer.numOccurrences.size()) : -1;
  auto pyNumOcc = PyList_New(size);
  if (!pyNumOcc) throw PythonException();
  RefHolder rh(pyNumOcc);

  for (auto i = 0; i < size; ++i) {
    auto n = PyInt_FromLong(qpAnswer.numOccurrences[i]);
    if (!n) throw PythonException();
    PyList_SET_ITEM(pyNumOcc, i, n); // steals ref
  }

  if (PyDict_SetItemString(d, answerkeys::numOccurrences, pyNumOcc) == -1) throw PythonException();
}

void addSolutionsToDict(const QpAnswer& qpAnswer, PyObject* d) {
  auto numSols = qpAnswer.energies.size();
  auto sNumSols = numSols <= static_cast<size_t>(numeric_limits<Py_ssize_t>::max())
      ? static_cast<Py_ssize_t>(numSols) : -1;
  auto solSize = numSols > 0 ? qpAnswer.solutions.size() / numSols : 0;
  auto sSolSize = solSize <= static_cast<size_t>(numeric_limits<Py_ssize_t>::max())
      ? static_cast<Py_ssize_t>(solSize) : -1;

  auto pySols = PyList_New(sNumSols);
  if (!pySols) throw PythonException();
  RefHolder rh(pySols);

  auto solIter = qpAnswer.solutions.begin();
  for (auto i = 0; i < sNumSols; ++i) {
    auto pySol = PyList_New(sSolSize);
    if (!pySol) throw PythonException();
    RefHolder rhi(pySol);

    for (auto j = 0; j < sSolSize; ++j) {
      auto x = PyInt_FromLong(*solIter);
      if (!x) throw PythonException();
      PyList_SET_ITEM(pySol, j, x); // steals ref
      ++solIter;
    }

    PyList_SET_ITEM(pySols, i, pySol); // steals ref
    rhi.release();
  }

  if (PyDict_SetItemString(d, answerkeys::solutions, pySols) == -1) throw PythonException();
}

void addStringToDict(const char* key, const string& val, PyObject* d) {
  auto valObj = PyString_FromString(val.c_str()); // new ref
  if (!valObj) throw PythonException();
  RefHolder vrh(valObj);
  if (PyDict_SetItemString(d, key, valObj) == -1) throw PythonException();
}

} // namespace {anonymous}

json::Value pythonToJson(PyObject* pyobj) {
  if (pyobj == Py_None) {
    return json::Null();

  } else if (PyBool_Check(pyobj)) {
    return pyobj == Py_True;

  } else if (PyInt_Check(pyobj)) {
    auto val = PyInt_AsLong(pyobj);
    if (val == -1 && PyErr_Occurred()) throw PythonException();
    return val;

  } else if (PyFloat_Check(pyobj)) {
    auto val = PyFloat_AsDouble(pyobj);
    if (val == -1.0 && PyErr_Occurred()) throw PythonException();
    return val;

  } else if (PyLong_Check(pyobj)) {
    auto llval = PyLong_AsLongLong(pyobj);
    if (llval == -1 && PyErr_Occurred()) throw PythonException();

    auto dval = PyLong_AsDouble(pyobj);
    return dval;
    if (dval == -1.0 && PyErr_Occurred()) throw PythonException();

  } else if (PyString_Check(pyobj)) {
    return PyString_AS_STRING(pyobj);

  } else if (PyUnicode_Check(pyobj)) {
    auto strobj = PyUnicode_AsUTF8String(pyobj);
    if (!strobj) throw PythonException();
    RefHolder r(strobj);
    return pythonToJson(strobj);

  } else if (PyList_Check(pyobj)) {
    json::Array array;
    auto size = PyList_Size(pyobj);
    for (auto i = 0; i < size; ++i) {
      array.push_back(pythonToJson(PyList_GET_ITEM(pyobj, i)));
    }
    return array;

  } else if (PyTuple_Check(pyobj)) {
    json::Array array;
    auto size = PyTuple_Size(pyobj);
    for (auto i = 0; i < size; ++i) {
      array.push_back(pythonToJson(PyTuple_GET_ITEM(pyobj, i)));
    }
    return array;

  } else if (PyDict_Check(pyobj)) {
    PyObject* key = 0;
    PyObject* value = 0;
    Py_ssize_t ppos = 0;
    json::Object object;
    while (PyDict_Next(pyobj, &ppos, &key, &value)) {
      if (PyString_Check(key)) {
        object[PyString_AS_STRING(key)] = pythonToJson(value);
      } else if (PyUnicode_Check(key)) {
        auto strobj = PyUnicode_AsUTF8String(key);
        if (!strobj) throw PythonException();
        RefHolder r(strobj);
        object[PyString_AS_STRING(strobj)] = pythonToJson(value);
      } else {
        throw json::ValueException();
      }
    }
    return object;

  } else {
    throw json::ValueException();
  }
}

PyObject* jsonToPython(const json::Value& v) {
  JsonToPythonVisitor visitor;
  return v.variant().apply_visitor(visitor);
}

PyObject* jsonToPython(const json::Object& o) {
  JsonToPythonVisitor visitor;
  return visitor(o);
}

QpProblem pythonToQpProblem(PyObject* pyobj) {
  try {
    if (!PyDict_Check(pyobj)) {
      PyErr_SetString(PyExc_TypeError, "problem must be a dictionary");
      throw PythonException();
    }

    QpProblem problem;
    problem.reserve(PyDict_Size(pyobj));

    PyObject* dkey = 0;
    PyObject* dvalue = 0;
    Py_ssize_t ppos = 0;
    json::Object object;
    while (PyDict_Next(pyobj, &ppos, &dkey, &dvalue)) {
      auto value = PyFloat_AsDouble(dvalue);
      if (value == -1.0 && PyErr_Occurred()) throw PythonException();

      if (!PyTuple_Check(dkey) || PyTuple_Size(dkey) != 2) {
        PyErr_SetString(PyExc_TypeError, "problem keys must be 2-element tuples");
        throw PythonException();
      }

      auto i = PyInt_AsLong(PyTuple_GET_ITEM(dkey, 0));
      if (i == -1 && PyErr_Occurred()) throw PythonException();
      auto j = PyInt_AsLong(PyTuple_GET_ITEM(dkey, 1));
      if (j == -1 && PyErr_Occurred()) throw PythonException();

      problem.push_back(QpProblemEntry{numeric_cast<int>(i), numeric_cast<int>(j), value});
    }
    return problem;

  } catch (boost::bad_numeric_cast&) {
    PyErr_SetString(PyExc_ValueError, "variable index out of range");
    throw PythonException();
  }
}


PyObject* qpAnswerToPython(json::Object answer, const QpAnswer& qpAnswer) {
  auto hasNumOcc = answer.find(answerkeys::numOccurrences) != answer.end();
  answer.erase(answerkeys::format);
  answer.erase(answerkeys::activeVars);
  answer.erase(answerkeys::numVars);
  answer.erase(answerkeys::solutions);
  answer.erase(answerkeys::energies);
  answer.erase(answerkeys::numOccurrences);

  auto pyAnswer = jsonToPython(answer);
  RefHolder arh(pyAnswer);
  addEnergiesToDict(qpAnswer, pyAnswer);
  addSolutionsToDict(qpAnswer, pyAnswer);
  if (hasNumOcc) addNumOccToDict(qpAnswer, pyAnswer);
  arh.release();
  return pyAnswer;
}


PyObject* submittedProblemInfoToPython(const sapiremote::SubmittedProblemInfo& status) {
  auto pydict = PyDict_New();
  if (!pydict) throw PythonException();
  RefHolder drh(pydict);

  addStringToDict(infokeys::problemId, status.problemId, pydict);
  addStringToDict(infokeys::state, toString(status.state), pydict);
  addStringToDict(infokeys::remoteStatus, toString(status.remoteStatus), pydict);

  if (status.state == submittedstates::FAILED || status.state == submittedstates::RETRYING) {
    addStringToDict(infokeys::lastGoodState, toString(status.lastGoodState), pydict);
  }

  if (status.state == submittedstates::FAILED || status.state == submittedstates::RETRYING
      || (status.state == submittedstates::DONE && status.remoteStatus != remotestatuses::COMPLETED)) {
    addStringToDict(infokeys::errorType, toString(status.error.type), pydict);
    addStringToDict(infokeys::errorMessage, status.error.message, pydict);
  }

  if (!status.submittedOn.empty()) addStringToDict(infokeys::submittedOn, status.submittedOn, pydict);
  if (!status.solvedOn.empty()) addStringToDict(infokeys::solvedOn, status.solvedOn, pydict);

  drh.release();
  return pydict;
}
