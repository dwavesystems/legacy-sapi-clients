//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

%module(docstring="D-Wave Solver API communication library", threads="1") sapiremote
%{
#include <new>
#include <stdexcept>
#include <tuple>
#include <utility>

#include <exceptions.hpp>
#include <coding.hpp>

#include "python-api.hpp"
#include "conversion.hpp"
#include "python-exception.hpp"
#include "refholder.hpp"

#define CATCH_ALL \
  catch (PythonException&) { \
    SWIG_fail; \
  } catch (std::runtime_error& e) { \
    SWIG_exception(SWIG_RuntimeError, e.what()); \
  } catch (std::bad_alloc&) { \
    SWIG_exception(SWIG_MemoryError, "Out of memory"); \
  } catch (...) { \
    SWIG_exception(SWIG_UnknownError, "Unknown error"); \
  }

%}
%nothread;

%exception {
  try {
    $action
  } catch (sapiremote::NetworkException& e) {
    SWIG_exception(SWIG_IOError, e.what());
  } CATCH_ALL
}

%typemap(in) json::Value& (json::Value value_val) {
  try {
    value_val = pythonToJson($input);
    $1 = &value_val;
  } catch (json::ValueException&) {
    SWIG_exception(SWIG_TypeError, "Unable to convert Python object to JSON");
  } CATCH_ALL
}

%typemap(in) json::Object& (json::Object object_val) {
  try {
    object_val = std::move(pythonToJson($input).getObject());
    $1 = &object_val;
  } catch (json::ValueException&) {
    SWIG_exception(SWIG_TypeError, "Unable to convert Python object to JSON");
  } catch (json::TypeException&) {
    SWIG_exception(SWIG_TypeError, "Unable to convert Python object to JSON object");
  } CATCH_ALL
}

%typemap(in) sapiremote::QpProblem& (sapiremote::QpProblem prob_val) {
  try {
    prob_val = pythonToQpProblem($input);
    $1 = &prob_val;
  } catch (sapiremote::EncodingException& e) {
    SWIG_exception(SWIG_ValueError, e.what());
  } CATCH_ALL
}

%typemap(in) sapiremote::http::Proxy {
  if ($input != Py_None) {
    auto s = PyString_AsString($input);
    if (!s) SWIG_fail;
    $1 = sapiremote::http::Proxy(s);
  }
}

%typecheck(SWIG_TYPECHECK_STRING) sapiremote::http::Proxy {
  $1 = $input == Py_None || PyString_Check($input) || PyUnicode_Check($input) ? 1 : 0;
}

%apply long long { sapiremote::AnswerManager::AnswerId }

%typemap(out) json::Value {
  try {
    $result = jsonToPython($1);
  } CATCH_ALL
}

%typemap(out) const json::Object& {
  $result = jsonToPython(*$1);
}

%typemap(out) std::tuple<std::string, json::Value> {
  try {
    $result = PyList_New(2);
    if (!$result) throw PythonException();
    RefHolder rrh($result);

    auto type = PyString_FromString(std::get<0>($1).c_str());
    if (!type) throw PythonException();
    PyList_SET_ITEM($result, 0, type);

    PyList_SET_ITEM($result, 1, jsonToPython(std::get<1>($1)));

    rrh.release();
  } CATCH_ALL
}

%typemap(out) std::tuple<json::Object, sapiremote::QpAnswer> {
  try {
    $result = qpAnswerToPython(std::move(std::get<0>($1)), std::get<1>($1));
  } CATCH_ALL
}

%typemap(out) sapiremote::SubmittedProblemInfo {
  try {
    $result = submittedProblemInfoToPython($1);
  } CATCH_ALL
}

%include exception.i
%include std_string.i
%typemap(in) std::string& = const std::string&;

%include std_vector.i
%template() std::vector<bool>;
%template() std::vector<SubmittedProblem>;

%include std_map.i
%template() std::map<std::string, Solver>;

// Docstrings -----------------------------------------------------------------------------------------
%feature("docstring") SubmittedProblem "A problem submitted to a SAPI solver"

%feature("docstring") SubmittedProblem::cancel "cancel(self)

Cancel the problem.  This is only a request; it is possible to
receive an answer or error after trying to cancel a problem."

%feature("docstring") SubmittedProblem::problem_id "problem_id(self) -> str

Retrieve the problem ID.  If the problem ID is not available,
an empty string is returned."

%feature("docstring") SubmittedProblem::answer "answer(self) -> ?

Retrieve the answer to the problem.  The answer format is solver
and problem dependent.  If the problem is not complete or failed
for any reason, an exception will be raised."

%feature("docstring") SubmittedProblem::done "done(self) -> bool

Indicates whether or not the problem is done, that is, if calling
the answer method will return (or raise an exception) immediately."

%feature("docstring") Solver "A remote SAPI solver"

%feature("docstring") Solver::properties "properties(self) -> dict

Returns the solver properties as a dictionary with string keys.
The format and meaning of the values is solver-dependent."

%feature("docstring") Solver::submit "submit(self, type, problem, params -> SubmittedProblem

Submit a problem to a solver.  Arguments:
    type: problem type (string)
    problem: problem data.  This is solver dependent.
        Any combination of None, bools, numbers, strings, lists,
        tuples, and dicts with string keys is acceptable to this
        method (but not necessarily to the solver).
    params: problem parameters.  Must be a dict with string keys;
        allowed values are the same as for problem data but again,
        the solver will have its own requirements.

Returns a SubmittedProblem instance."

%feature("docstring") Solver::solve "solve(self, type, problem, params) -> ?

Solve a problem and return the answer.  Identical to calling the
submit method and then waiting arbitrarily long for the answer."

%feature("docstring") Connection "Provides access to remote SAPI solvers"

%feature("docstring") Connection::Connection "__init__(self, url, token, proxy=None)

Establish a connection to a remote SAPI server.  Arguments:
    url: server URL (string).  Ask your system administrator.
    token: authentication token (string).
    proxy: proxy URL (string or None).  Passing None will cause the
        http_proxy or https_proxy and all_proxy environment
        variables to be examined for proxy information.  Pass an
        empty string to force the use of no proxy."

%feature("docstring") Connection::solvers "solvers(self) -> dict

Returns the available SAPI solvers as a dict mapping solver name
to Solver object."

%feature("docstring") Connection::add_problem "add_problem(self, id) -> SubmittedProblem

Access an existing problem on a SAPI server by its problem ID."
// ----------------------------------------------------------------------------------------------------

%ignore Solver::Solver;
%ignore Solver::solver;
%ignore SubmittedProblem::SubmittedProblem;
%ignore SubmittedProblem::sp;
%thread await_completion;
%include "python-api.hpp"
