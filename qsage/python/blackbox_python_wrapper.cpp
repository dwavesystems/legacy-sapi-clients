//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "blackbox_python_wrapper.hpp"

#include "blackbox.hpp"

#include <map>
#include <utility>
#include <set>
#include <string>

namespace
{

int getPythonFunctionNumOfArguments(PyObject* callable)
{
  int count = -1;
  PyObject* fc = PyObject_GetAttrString(callable, "func_code");
    if (fc != NULL)
  {
    PyObject* ac = PyObject_GetAttrString(fc, "co_argcount");
    if (ac != NULL)
    {
      count = PyInt_AsLong(ac);
      Py_XDECREF(ac);
    }
    Py_XDECREF(fc);
  }

  return count;
}

class IsingSolverPython : public blackbox::IsingSolver
{
public:
  // // Here need to set numQubitsAttr_(NULL), qubitsAttr_(NULL), couplersAttr_(NULL), hRangeAttr_(NULL), hRangeAttr_(NULL), solveIsingMethod_(NULL)
  // any of these pointers may not be assigned any value in the program if it throws an exception, and this will cause problem
  // when the destructor calls Py_XDECREF(...);
  IsingSolverPython(PyObject* isingSolver) : isingSolver_(isingSolver), qubitsAttr_(NULL), couplersAttr_(NULL), hRangeAttr_(NULL), jRangeAttr_(NULL), solveIsingMethod_(NULL)
  {
    if (isingSolver_ == NULL || isingSolver_ == Py_None)
      throw blackbox::BlackBoxException("Python ising solver is invalid");

    PyObject* item;

    qubitsAttr_ = PyObject_GetAttrString(isingSolver_, "qubits"); // New reference
    if (qubitsAttr_ == NULL || qubitsAttr_ == Py_None || !PyList_Check(qubitsAttr_))
      throw blackbox::BlackBoxException("solver's qubits attribite must be a list containing non-negative integers");
    int numActiveQubits = static_cast<int>(PyList_Size(qubitsAttr_));
    qubits_.resize(numActiveQubits);
    for (int i = 0; i < numActiveQubits; ++i)
    {
      item = PyList_GetItem(qubitsAttr_, i);
      if (item == NULL || item == Py_None)
        throw blackbox::BlackBoxException("solver's qubits attribite must be a list containing non-negative integers");
      qubits_[i] = PyInt_AsLong(item);
      if (PyErr_Occurred())
        throw blackbox::BlackBoxException("solver's qubits attribite must be a list containing non-negative integers");
    }

    couplersAttr_ = PyObject_GetAttrString(isingSolver_, "couplers"); // New reference
    if (couplersAttr_ == NULL || couplersAttr_ == Py_None || !PyList_Check(couplersAttr_))
      throw blackbox::BlackBoxException("solver's couplers attribite must be a list of lists containing two non-negative integers");
    int numCouplers = static_cast<int>(PyList_Size(couplersAttr_));
    couplers_.resize(numCouplers);
    for (int i = 0; i < numCouplers; ++i)
    {
      item = PyList_GetItem(couplersAttr_, i);
      if (item == NULL || item == Py_None || !PyList_Check(item) || PyList_Size(item) != 2)
        throw blackbox::BlackBoxException("solver's couplers attribite must be a list of lists containing two non-negative integers");

      PyObject* tmp = PyList_GetItem(item, 0);
      if (tmp == NULL || tmp == Py_None)
        throw blackbox::BlackBoxException("solver's couplers attribite must be a list of lists containing two non-negative integers");
      couplers_[i].first = PyInt_AsLong(tmp);
      if (PyErr_Occurred())
        throw blackbox::BlackBoxException("solver's couplers attribite must be a list of lists containing two non-negative integers");

      tmp = PyList_GetItem(item, 1);
      if (tmp == NULL || tmp == Py_None)
        throw blackbox::BlackBoxException("solver's couplers attribite must be a list of lists containing two non-negative integers");
      couplers_[i].second = PyInt_AsLong(tmp);
      if (PyErr_Occurred())
        throw blackbox::BlackBoxException("solver's couplers attribite must be a list of lists containing two non-negative integers");
    }

    if (PyObject_HasAttrString(isingSolver_, "h_range"))
      hRangeAttr_ = PyObject_GetAttrString(isingSolver_, "h_range"); // New reference
    if (hRangeAttr_ != NULL && hRangeAttr_ != Py_None)
    {
      if (!PyList_Check(hRangeAttr_) || PyList_Size(hRangeAttr_) != 2)
        throw blackbox::BlackBoxException("solver's h_range attribite must be a list containing two float numbers");

      hMin_ = PyFloat_AsDouble(PyList_GetItem(hRangeAttr_, 0));
      if (PyErr_Occurred())
        throw blackbox::BlackBoxException("solver's h_range attribite must be a list containing two float numbers");

      hMax_ = PyFloat_AsDouble(PyList_GetItem(hRangeAttr_, 1));
      if (PyErr_Occurred())
        throw blackbox::BlackBoxException("solver's h_range attribite must be a list containing two float numbers");
    }

    if (PyObject_HasAttrString(isingSolver_, "j_range"))
      jRangeAttr_ = PyObject_GetAttrString(isingSolver_, "j_range"); // New reference
    if (jRangeAttr_ != NULL && jRangeAttr_ != Py_None)
    {
      if (!PyList_Check(jRangeAttr_) || PyList_Size(jRangeAttr_) != 2)
        throw blackbox::BlackBoxException("solver's j_range attribite must be a list containing two float numbers");

      jMin_ = PyFloat_AsDouble(PyList_GetItem(jRangeAttr_, 0));
      if (PyErr_Occurred())
        throw blackbox::BlackBoxException("solver's j_range attribite must be a list containing two float numbers");

      jMax_ = PyFloat_AsDouble(PyList_GetItem(jRangeAttr_, 1));
      if (PyErr_Occurred())
        throw blackbox::BlackBoxException("solver's j_range attribite must be a list containing two float numbers");
    }

    solveIsingMethod_ = PyObject_GetAttrString(isingSolver_, "solve_ising");  // New reference

    if (   solveIsingMethod_ == NULL || solveIsingMethod_ == Py_None || !PyCallable_Check(solveIsingMethod_)
      || getPythonFunctionNumOfArguments(solveIsingMethod_) != 3)
      throw blackbox::BlackBoxException("solver's solve_ising function is not callable or the number of arguments is not 3");
  }

  virtual ~IsingSolverPython()
  {
    Py_XDECREF(qubitsAttr_);
    Py_XDECREF(couplersAttr_);
    Py_XDECREF(hRangeAttr_);
    Py_XDECREF(jRangeAttr_);
    Py_XDECREF(solveIsingMethod_);
  }

private:
  virtual void solveIsingImpl(const std::vector<double>& h, const std::map<std::pair<int, int>, double>& J, std::vector<std::vector<int> >& solutions, std::vector<double>& energies, std::vector<int>& numOccurrences) const
  {
    PyObject* hPythonObject = PyList_New(h.size());
    for (int i = 0; i < h.size(); ++i)
      PyList_SetItem(hPythonObject, i, PyFloat_FromDouble(h[i]));

    PyObject* JPythonObject = PyDict_New();
    for (std::map<std::pair<int, int>, double>::const_iterator it = J.begin(), end = J.end(); it != end; ++it)
    {
      PyObject* key = PyTuple_New(2);
      PyTuple_SetItem(key, 0, PyInt_FromLong(it->first.first));
      PyTuple_SetItem(key, 1, PyInt_FromLong(it->first.second));

      PyObject* value = PyFloat_FromDouble(it->second);

      PyDict_SetItem(JPythonObject, key, value);

      // Putting something in a dictionary is "storing" it. Therefore PyDict_SetItem() INCREFs both the key and the val. So here it needs to
      // Py_DECREF both the key and the val to make its reference count back to 1
      Py_XDECREF(key);
      Py_XDECREF(value);
    }

    PyObject* args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, hPythonObject);
    PyTuple_SetItem(args, 1, JPythonObject);

    PyObject* pythonResult = PyObject_CallObject(solveIsingMethod_, args);

    if (PyErr_Occurred())
    {
      // pvalue contains error message, ptraceback contains stack snapshot and many other information (see python traceback structure)
      // PyErr_Fetch will clear the error indicator if it is set, so here no need to call PyErr_Clear
      // PyErr_Fetch: "Retrieve the error indicator into three variables whose addresses are passed. If the error indicator is not set, set all three
      // variables to NULL. If it is set, it will be cleared and you own a reference to each object retrieved. The value and traceback
      // object may be NULL even when the type object is not."
      PyObject *ptype, *pvalue, *ptraceback;
      PyErr_Fetch(&ptype, &pvalue, &ptraceback);

      PyObject* pvaluePythonObject = PyObject_Str(pvalue); //New reference
      std::string errorMessage = PyString_AsString(pvaluePythonObject);
      Py_XDECREF(pvaluePythonObject);

      Py_XDECREF(ptype);
      Py_XDECREF(pvalue);
      Py_XDECREF(ptraceback);

      Py_XDECREF(args);
      throw blackbox::BlackBoxException(errorMessage);
    }

    if (pythonResult == NULL || pythonResult == Py_None)
    {
      Py_XDECREF(args);
      throw blackbox::BlackBoxException("solver's solve_ising function return value is NULL or Py_None");
    }

    PyObject* solutionsPythonObject = PyDict_GetItemString(pythonResult, "solutions"); // Borrowed reference
    if (solutionsPythonObject == NULL || solutionsPythonObject == Py_None || !PyList_Check(solutionsPythonObject))
    {
      Py_XDECREF(args);
      Py_XDECREF(pythonResult);
      throw blackbox::BlackBoxException("solver's solve_ising function return value's solutions is not correct");
    }

    int numSolutions = static_cast<int>(PyList_Size(solutionsPythonObject));
    solutions.resize(numSolutions);
    for (int i = 0; i < numSolutions; ++i)
    {
      PyObject* item = PyList_GetItem(solutionsPythonObject, i);
      if (item == NULL || item == Py_None || !PyList_Check(item))
      {
        Py_XDECREF(args);
        Py_XDECREF(pythonResult);
        throw blackbox::BlackBoxException("solver's solve_ising function return value's solutions is not correct");
      }

      int numStates = static_cast<int>(PyList_Size(item));
      solutions[i].resize(numStates);
      for (int j = 0; j < numStates; ++j)
      {
        PyObject* tmp = PyList_GetItem(item, j);
          if (tmp == NULL || tmp == Py_None)
        {
          Py_XDECREF(args);
            Py_XDECREF(pythonResult);
          throw blackbox::BlackBoxException("solver's solve_ising function return value's solutions is not correct");
        }
        solutions[i][j] = PyInt_AsLong(tmp);
        if (PyErr_Occurred())
        {
          Py_XDECREF(args);
            Py_XDECREF(pythonResult);
          throw blackbox::BlackBoxException("solver's solve_ising function return value's solutions is not correct");
        }

        if (solutions[i][j] != -1 && solutions[i][j] != 1 && solutions[i][j] != 3)
        {
          Py_XDECREF(args);
            Py_XDECREF(pythonResult);
          throw blackbox::BlackBoxException("solver's solve_ising function return value's solutions is not correct");
        }
      }
    }

    PyObject* energiesPythonObject = PyDict_GetItemString(pythonResult, "energies"); // Borrowed reference
    if (energiesPythonObject == NULL || energiesPythonObject == Py_None || !PyList_Check(energiesPythonObject) || PyList_Size(energiesPythonObject) != numSolutions)
    {
      Py_XDECREF(args);
      Py_XDECREF(pythonResult);
      throw blackbox::BlackBoxException("solver's solve_ising function return value's energies is not correct");
    }

    energies.resize(numSolutions);
    for (int i = 0; i < numSolutions; ++i)
    {
      energies[i] = PyFloat_AsDouble(PyList_GetItem(energiesPythonObject, i));
      if (PyErr_Occurred())
      {
        Py_XDECREF(args);
          Py_XDECREF(pythonResult);
        throw blackbox::BlackBoxException("solver's solve_ising function return value's energies is not correct");
      }
    }

    PyObject* numOccurrencesPythonObject = PyDict_GetItemString(pythonResult, "num_occurrences"); // Borrowed reference
    if (numOccurrencesPythonObject != NULL && numOccurrencesPythonObject != Py_None)
    {
      if (!PyList_Check(numOccurrencesPythonObject) || PyList_Size(numOccurrencesPythonObject) != numSolutions)
      {
        Py_XDECREF(args);
        Py_XDECREF(pythonResult);
        throw blackbox::BlackBoxException("solver's solve_ising function return value's num_occurrences is not correct");
      }

      numOccurrences.resize(numSolutions);
      for (int i = 0; i < numSolutions; ++i)
      {
        numOccurrences[i] = PyInt_AsLong(PyList_GetItem(energiesPythonObject, i));
        if (PyErr_Occurred())
        {
          Py_XDECREF(args);
          Py_XDECREF(pythonResult);
          throw blackbox::BlackBoxException("solver's solve_ising function return value's num_occurrences is not correct");
        }
      }
    }

    Py_XDECREF(args);
    Py_XDECREF(pythonResult);
  }

  PyObject* isingSolver_;
  PyObject* qubitsAttr_;
  PyObject* couplersAttr_;
  PyObject* hRangeAttr_;
  PyObject* jRangeAttr_;
  PyObject* solveIsingMethod_;
};

class BlackBoxObjectiveFunctionPython : public blackbox::BlackBoxObjectiveFunction
{
public:
  BlackBoxObjectiveFunctionPython(PyObject* pythonObjectiveFunction, blackbox::LocalInteractionPtr localInteractionPtr) : BlackBoxObjectiveFunction(localInteractionPtr), pythonObjectiveFunction_(pythonObjectiveFunction)
  {
    if (pythonObjectiveFunction_ == NULL || pythonObjectiveFunction_ == Py_None || !PyCallable_Check(pythonObjectiveFunction_))
      throw blackbox::BlackBoxException("Python objective_function is invalid");
  }

  virtual ~BlackBoxObjectiveFunctionPython() {}

private:
  virtual std::vector<double> computeObjectValueImpl(const std::vector<std::vector<int> >& states) const
  {
    int numStates = static_cast<int>(states.size());
    int stateLen = static_cast<int>(states[0].size());
    PyObject* statesPython = PyTuple_New(numStates);
    for (int i = 0; i < numStates; ++i)
    {
      PyObject* statePython = PyTuple_New(stateLen);
      for (int j = 0; j < stateLen; ++j)
        PyTuple_SetItem(statePython, j, PyInt_FromLong(states[i][j]));
      PyTuple_SetItem(statesPython, i, statePython);
    }

    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, statesPython);

    PyObject* pythonResult = PyObject_CallObject(pythonObjectiveFunction_, args);

    if (PyErr_Occurred())
    {
      // pvalue contains error message, ptraceback contains stack snapshot and many other information (see python traceback structure)
      // PyErr_Fetch will clear the error indicator if it is set, so here no need to call PyErr_Clear
      // PyErr_Fetch: "Retrieve the error indicator into three variables whose addresses are passed. If the error indicator is not set, set all three
      // variables to NULL. If it is set, it will be cleared and you own a reference to each object retrieved. The value and traceback
      // object may be NULL even when the type object is not."
      PyObject *ptype, *pvalue, *ptraceback;
      PyErr_Fetch(&ptype, &pvalue, &ptraceback);

      PyObject* pvaluePythonObject = PyObject_Str(pvalue); //New reference
      std::string errorMessage = PyString_AsString(pvaluePythonObject);
      Py_XDECREF(pvaluePythonObject);

      Py_XDECREF(ptype);
      Py_XDECREF(pvalue);
      Py_XDECREF(ptraceback);

      Py_XDECREF(args);
      throw blackbox::BlackBoxException(errorMessage);
    }

    if (!pythonResult || pythonResult == Py_None)
    {
      Py_XDECREF(args);
      throw blackbox::BlackBoxException("objective function's return value is NULL or Py_None");
    }

    std::vector<double> ret;

    if (PyTuple_Check(pythonResult))
    {
      int m = static_cast<int>(PyTuple_Size(pythonResult));
      ret.resize(m);
      for (int i = 0; i < m; ++i)
      {
        PyObject* pythonResultPart = PyTuple_GetItem(pythonResult, i);
        ret[i] = PyFloat_AsDouble(pythonResultPart);
        if (PyErr_Occurred())
        {
          // pvalue contains error message, ptraceback contains stack snapshot and many other information (see python traceback structure)
          // PyErr_Fetch will clear the error indicator if it is set, so here no need to call PyErr_Clear
          // PyErr_Fetch: "Retrieve the error indicator into three variables whose addresses are passed. If the error indicator is not set, set all three
          // variables to NULL. If it is set, it will be cleared and you own a reference to each object retrieved. The value and traceback
          // object may be NULL even when the type object is not."
          PyObject *ptype, *pvalue, *ptraceback;
          PyErr_Fetch(&ptype, &pvalue, &ptraceback);

          PyObject* pvaluePythonObject = PyObject_Str(pvalue); //New reference
          std::string errorMessage = PyString_AsString(pvaluePythonObject);
          Py_XDECREF(pvaluePythonObject);

          Py_XDECREF(ptype);
          Py_XDECREF(pvalue);
          Py_XDECREF(ptraceback);

          Py_XDECREF(args);
          Py_XDECREF(pythonResult);
          throw blackbox::BlackBoxException(errorMessage);
        }
      }
    }
    else if (PyList_Check(pythonResult))
    {
      int m = static_cast<int>(PyList_Size(pythonResult));
      ret.resize(m);
      for (int i = 0; i < m; ++i)
      {
        PyObject* pythonResultPart = PyList_GetItem(pythonResult, i);
        ret[i] = PyFloat_AsDouble(pythonResultPart);
        if (PyErr_Occurred())
        {
          // pvalue contains error message, ptraceback contains stack snapshot and many other information (see python traceback structure)
          // PyErr_Fetch will clear the error indicator if it is set, so here no need to call PyErr_Clear
          // PyErr_Fetch: "Retrieve the error indicator into three variables whose addresses are passed. If the error indicator is not set, set all three
          // variables to NULL. If it is set, it will be cleared and you own a reference to each object retrieved. The value and traceback
          // object may be NULL even when the type object is not."
          PyObject *ptype, *pvalue, *ptraceback;
          PyErr_Fetch(&ptype, &pvalue, &ptraceback);

          PyObject* pvaluePythonObject = PyObject_Str(pvalue); //New reference
          std::string errorMessage = PyString_AsString(pvaluePythonObject);
          Py_XDECREF(pvaluePythonObject);

          Py_XDECREF(ptype);
          Py_XDECREF(pvalue);
          Py_XDECREF(ptraceback);

          Py_XDECREF(args);
          Py_XDECREF(pythonResult);
          throw blackbox::BlackBoxException(errorMessage);
        }
      }
    }
    else
    {
      Py_XDECREF(args);
      Py_XDECREF(pythonResult);
      throw blackbox::BlackBoxException("The objective_function's return value must be a tuple or list that has the size as num_states [from else]");
    }

    Py_XDECREF(args);
    Py_XDECREF(pythonResult);
    return ret;
  }

  PyObject* pythonObjectiveFunction_;
};

class LPSolverPython : public blackbox::LPSolver
{
public:
  LPSolverPython(PyObject* pythonLPSolver, blackbox::LocalInteractionPtr localInteractionPtr) : LPSolver(localInteractionPtr), pythonLPSolver_(pythonLPSolver)
  {
    if (pythonLPSolver_ == NULL || pythonLPSolver_ == Py_None || !PyCallable_Check(pythonLPSolver_))
      throw blackbox::BlackBoxException("lp solver is invalid");
  }

  virtual ~LPSolverPython() {}

private:
  virtual std::vector<double> solveImpl(const std::vector<double>& f, const std::vector<std::vector<double> >& Aineq, const std::vector<double>& bineq,
                                      const std::vector<std::vector<double> >& Aeq, const std::vector<double>& beq, const std::vector<double>& lb,
                      const std::vector<double>& ub) const
  {
    // f is 1 by numVars
    // Aineq is AineqSize by numVars
    // bineq is 1 by AineqSize
    // Aeq is AeqSize by numVars
    // beq is 1 by AeqSize
    // lb is 1 by numVars
    // ub is 1 by numVars
    // return value should be 1 by numVars
    int AineqSize = static_cast<int>(Aineq.size());
    int AeqSize = static_cast<int>(Aeq.size());
    int numVars = static_cast<int>(lb.size());

    PyObject* fPython = PyList_New(numVars);
    for (int i = 0; i < numVars; ++i)
      PyList_SetItem(fPython, i, PyFloat_FromDouble(f[i]));

    PyObject* AineqPython = PyList_New(AineqSize);
    for (int i = 0; i < AineqSize; ++i)
    {
      PyObject* AineqPythonPart = PyList_New(numVars);
      for (int j = 0; j < numVars; ++j)
        PyList_SetItem(AineqPythonPart, j, PyFloat_FromDouble(Aineq[i][j]));
      PyList_SetItem(AineqPython, i, AineqPythonPart);
    }

    PyObject* bineqPython = PyList_New(AineqSize);
    for (int i = 0; i < AineqSize; ++i)
      PyList_SetItem(bineqPython, i, PyFloat_FromDouble(bineq[i]));

    PyObject* AeqPython = PyList_New(AeqSize);
    for (int i = 0; i < AeqSize; ++i)
    {
      PyObject* AeqPythonPart = PyList_New(numVars);
      for (int j = 0; j < numVars; ++j)
        PyList_SetItem(AeqPythonPart, j, PyFloat_FromDouble(Aeq[i][j]));
      PyList_SetItem(AeqPython, i, AeqPythonPart);
    }

    PyObject* beqPython = PyList_New(AeqSize);
    for (int i = 0; i < AeqSize; ++i)
      PyList_SetItem(beqPython, i, PyFloat_FromDouble(beq[i]));

    PyObject* lbPython = PyList_New(numVars);
    for (int i = 0; i < numVars; ++i)
      PyList_SetItem(lbPython, i, PyFloat_FromDouble(lb[i]));

    PyObject* ubPython = PyList_New(numVars);
    for (int i = 0; i < numVars; ++i)
      PyList_SetItem(ubPython, i, PyFloat_FromDouble(ub[i]));

    PyObject* args = PyTuple_New(7);
    PyTuple_SetItem(args, 0, fPython);
    PyTuple_SetItem(args, 1, AineqPython);
    PyTuple_SetItem(args, 2, bineqPython);
    PyTuple_SetItem(args, 3, AeqPython);
    PyTuple_SetItem(args, 4, beqPython);
    PyTuple_SetItem(args, 5, lbPython);
    PyTuple_SetItem(args, 6, ubPython);

    PyObject* pythonResult = PyObject_CallObject(pythonLPSolver_, args);

    if (PyErr_Occurred())
    {
      // pvalue contains error message, ptraceback contains stack snapshot and many other information (see python traceback structure)
      // PyErr_Fetch will clear the error indicator if it is set, so here no need to call PyErr_Clear
      // PyErr_Fetch: "Retrieve the error indicator into three variables whose addresses are passed. If the error indicator is not set, set all three
      // variables to NULL. If it is set, it will be cleared and you own a reference to each object retrieved. The value and traceback
      // object may be NULL even when the type object is not."
      PyObject *ptype, *pvalue, *ptraceback;
      PyErr_Fetch(&ptype, &pvalue, &ptraceback);

      PyObject* pvaluePythonObject = PyObject_Str(pvalue); //New reference
      std::string errorMessage = PyString_AsString(pvaluePythonObject);
      Py_XDECREF(pvaluePythonObject);

      Py_XDECREF(ptype);
      Py_XDECREF(pvalue);
      Py_XDECREF(ptraceback);

      Py_XDECREF(args);
      throw blackbox::BlackBoxException(errorMessage);
    }

    if (pythonResult == NULL || pythonResult == Py_None)
    {
      Py_XDECREF(args);
      throw blackbox::BlackBoxException("lp_solver's return value is NULL or Py_None");
    }

    std::vector<double> ret;

    if (PyTuple_Check(pythonResult))
    {
      int m = static_cast<int>(PyTuple_Size(pythonResult));
      ret.resize(m);
      for (int i = 0; i < m; ++i)
      {
        PyObject* pythonResultPart = PyTuple_GetItem(pythonResult, i);
        ret[i] = PyFloat_AsDouble(pythonResultPart);
        if (PyErr_Occurred())
        {
          // pvalue contains error message, ptraceback contains stack snapshot and many other information (see python traceback structure)
          // PyErr_Fetch will clear the error indicator if it is set, so here no need to call PyErr_Clear
          // PyErr_Fetch: "Retrieve the error indicator into three variables whose addresses are passed. If the error indicator is not set, set all three
          // variables to NULL. If it is set, it will be cleared and you own a reference to each object retrieved. The value and traceback
          // object may be NULL even when the type object is not."
          PyObject *ptype, *pvalue, *ptraceback;
          PyErr_Fetch(&ptype, &pvalue, &ptraceback);

          PyObject* pvaluePythonObject = PyObject_Str(pvalue); //New reference
          std::string errorMessage = PyString_AsString(pvaluePythonObject);
          Py_XDECREF(pvaluePythonObject);

          Py_XDECREF(ptype);
          Py_XDECREF(pvalue);
          Py_XDECREF(ptraceback);

          Py_XDECREF(args);
          Py_XDECREF(pythonResult);
          throw blackbox::BlackBoxException(errorMessage);
        }
      }
    }
    else if (PyList_Check(pythonResult))
    {
      int m = static_cast<int>(PyList_Size(pythonResult));
      ret.resize(m);
      for (int i = 0; i < m; ++i)
      {
        PyObject* pythonResultPart = PyList_GetItem(pythonResult, i);
        ret[i] = PyFloat_AsDouble(pythonResultPart);
        if (PyErr_Occurred())
        {
          // pvalue contains error message, ptraceback contains stack snapshot and many other information (see python traceback structure)
          // PyErr_Fetch will clear the error indicator if it is set, so here no need to call PyErr_Clear
          // PyErr_Fetch: "Retrieve the error indicator into three variables whose addresses are passed. If the error indicator is not set, set all three
          // variables to NULL. If it is set, it will be cleared and you own a reference to each object retrieved. The value and traceback
          // object may be NULL even when the type object is not."
          PyObject *ptype, *pvalue, *ptraceback;
          PyErr_Fetch(&ptype, &pvalue, &ptraceback);

          PyObject* pvaluePythonObject = PyObject_Str(pvalue); //New reference
          std::string errorMessage = PyString_AsString(pvaluePythonObject);
          Py_XDECREF(pvaluePythonObject);

          Py_XDECREF(ptype);
          Py_XDECREF(pvalue);
          Py_XDECREF(ptraceback);

          Py_XDECREF(args);
          Py_XDECREF(pythonResult);
          throw blackbox::BlackBoxException(errorMessage);
        }
      }
    }
    else
    {
      Py_XDECREF(args);
      Py_XDECREF(pythonResult);
      throw blackbox::BlackBoxException("The lp_solver's return value is wrong [from else]");
    }

    Py_XDECREF(args);
    Py_XDECREF(pythonResult);
    return ret;
  }

  PyObject* pythonLPSolver_;
};

PyObject* constructBlackBoxResult(const blackbox::BlackBoxResult& blackBoxResult)
{
  PyObject* ret = PyDict_New();

  // best_solution
  PyObject* bestSolution = PyList_New(blackBoxResult.bestSolution.size());
  for (int i = 0; i < blackBoxResult.bestSolution.size(); ++i)
    PyList_SetItem(bestSolution, i, PyInt_FromLong(blackBoxResult.bestSolution[i]));

  // best_energy
  PyObject* bestEnergy = PyFloat_FromDouble(blackBoxResult.bestEnergy);

  // num_state_evaluations
  PyObject* numStateEvaluations = PyLong_FromLongLong(blackBoxResult.info.numStateEvaluations);

  // num_obj_func_calls
  PyObject* numObjFuncCalls = PyLong_FromLongLong(blackBoxResult.info.numObjFuncCalls);

  // num_solver_calls
  PyObject* numSolverCalls = PyLong_FromLongLong(blackBoxResult.info.numSolverCalls);

  // num_lp_solver_calls
  PyObject* numLPSolverCalls = PyLong_FromLongLong(blackBoxResult.info.numLPSolverCalls);

  // state_evaluations_time
  PyObject* stateEvaluationsTime = PyFloat_FromDouble(blackBoxResult.info.stateEvaluationsTime);

  // solver_calls_time
  PyObject* solverCallsTime = PyFloat_FromDouble(blackBoxResult.info.solverCallsTime);

  // lp_solver_calls_time
  PyObject* lpSolverCallsTime = PyFloat_FromDouble(blackBoxResult.info.lpSolverCallsTime);

  // total_time
  PyObject* totalTime = PyFloat_FromDouble(blackBoxResult.info.totalTime);

  // history
  PyObject* history = PyList_New(blackBoxResult.info.progressTable.size());
  for (int i = 0; i < blackBoxResult.info.progressTable.size(); ++i)
  {
    PyObject* historyPart = PyList_New(6);
    PyList_SetItem(historyPart, 0, PyLong_FromLongLong(blackBoxResult.info.progressTable[i].first[0]));
    PyList_SetItem(historyPart, 1, PyLong_FromLongLong(blackBoxResult.info.progressTable[i].first[1]));
    PyList_SetItem(historyPart, 2, PyLong_FromLongLong(blackBoxResult.info.progressTable[i].first[2]));
    PyList_SetItem(historyPart, 3, PyLong_FromLongLong(blackBoxResult.info.progressTable[i].first[3]));
    PyList_SetItem(historyPart, 4, PyFloat_FromDouble(blackBoxResult.info.progressTable[i].second.first));
    PyList_SetItem(historyPart, 5, PyFloat_FromDouble(blackBoxResult.info.progressTable[i].second.second));

    PyList_SetItem(history, i, historyPart);
  }

  // Putting something in a dictionary is "storing" it. Therefore PyDict_SetItem() INCREFs both the key and the val. So here it needs to
  // Py_DECREF both the key and the val to make its reference count back to 1
  PyDict_SetItemString(ret, "best_solution", bestSolution);
  Py_XDECREF(bestSolution);

  PyDict_SetItemString(ret, "best_energy", bestEnergy);
  Py_XDECREF(bestEnergy);

  PyDict_SetItemString(ret, "num_state_evaluations", numStateEvaluations);
  Py_XDECREF(numStateEvaluations);

  PyDict_SetItemString(ret, "num_obj_func_calls", numObjFuncCalls);
  Py_XDECREF(numObjFuncCalls);

  PyDict_SetItemString(ret, "num_solver_calls", numSolverCalls);
  Py_XDECREF(numSolverCalls);

  PyDict_SetItemString(ret, "num_lp_solver_calls", numLPSolverCalls);
  Py_XDECREF(numLPSolverCalls);

  PyDict_SetItemString(ret, "state_evaluations_time", stateEvaluationsTime);
  Py_XDECREF(stateEvaluationsTime);

  PyDict_SetItemString(ret, "solver_calls_time", solverCallsTime);
  Py_XDECREF(solverCallsTime);

  PyDict_SetItemString(ret, "lp_solver_calls_time", lpSolverCallsTime);
  Py_XDECREF(lpSolverCallsTime);

  PyDict_SetItemString(ret, "total_time", totalTime);
  Py_XDECREF(totalTime);

  PyDict_SetItemString(ret, "history", history);
  Py_XDECREF(history);

  return ret;
}

void extractLongLongIntFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, long long int& i)
{
  PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

  if (tmp == NULL || tmp == Py_None)
    return;

  auto v = PyInt_AsLong(tmp);
  if (v == -1 && PyErr_Occurred()) throw blackbox::BlackBoxException(errorMsg);
  i = v;
}

void extractDoubleFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, double& b)
{
  PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

  if (tmp == NULL || tmp == Py_None)
    return;

  if (PyFloat_Check(tmp))
    b = PyFloat_AsDouble(tmp);
  else if (PyInt_Check(tmp))
    b = static_cast<double>(PyInt_AsLong(tmp));
  else if (PyLong_Check(tmp))
    b = static_cast<double>(PyLong_AsLong(tmp));
  else
    throw blackbox::BlackBoxException(errorMsg);
}

void extractIntFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, unsigned int& i)
{
  PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

  if (tmp == NULL || tmp == Py_None)
    return;

  if (!PyInt_Check(tmp))
    throw blackbox::BlackBoxException(errorMsg);

  i = static_cast<unsigned int>(PyInt_AsLong(tmp));
}

void extractIntFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, int& i)
{
  PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

  if (tmp == NULL || tmp == Py_None)
    return;

  if (!PyInt_Check(tmp))
    throw blackbox::BlackBoxException(errorMsg);

  i = PyInt_AsLong(tmp);
}

void extractListFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, std::vector<int>& vi)
{
  PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

  if (tmp == NULL || tmp == Py_None)
    return;

  if (!PyList_Check(tmp))
    throw blackbox::BlackBoxException(errorMsg);

  int len = static_cast<int>(PyList_Size(tmp));
  vi.resize(len);
  for (int i = 0; i < len; ++i)
  {
    PyObject* t = PyList_GetItem(tmp, i);
    if (!PyInt_Check(t))
      throw blackbox::BlackBoxException(errorMsg);
    vi[i] = PyInt_AsLong(t);
  }
}

void extractStringFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, blackbox::IsingQubo& isingQubo)
{
  PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

  if (tmp == NULL || tmp == Py_None)
    return;

  if (!PyString_Check(tmp))
    throw blackbox::BlackBoxException(errorMsg);

  std::string str = PyString_AsString(tmp);
  if (str == "ising")
    isingQubo = blackbox::ISING;
  else if (str == "qubo")
    isingQubo = blackbox::QUBO;
  else
    throw blackbox::BlackBoxException(errorMsg);
}

void extractBooleanFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, bool& b)
{
  PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

  if (tmp == NULL || tmp == Py_None)
    return;

  if (!PyBool_Check(tmp))
    throw blackbox::BlackBoxException(errorMsg);

  b = (tmp == Py_True) ? true : false;
}

void checkBlackBoxParams(PyObject* params, blackbox::BlackBoxExternalParams& blackBoxExternalParams)
{
  std::set<std::string> paramsNameSet;
  paramsNameSet.insert("draw_sample");
  paramsNameSet.insert("exit_threshold_value");
  paramsNameSet.insert("init_solution");
  paramsNameSet.insert("ising_qubo");
  paramsNameSet.insert("lp_solver");
  paramsNameSet.insert("max_num_state_evaluations");
  paramsNameSet.insert("random_seed");
  paramsNameSet.insert("timeout");
  paramsNameSet.insert("verbose");

  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(params, &pos, &key, &value))
  {
    if (key == NULL || key == Py_None || !PyString_Check(key))
      throw blackbox::BlackBoxException("params' key must be a string");

    std::string str = PyString_AsString(key);
    if (paramsNameSet.find(str) == paramsNameSet.end())
      throw blackbox::BlackBoxException(std::string(1, '"') + str + "\" is not a valid parameter for solve_qsage");
  }

  extractBooleanFromPythonDict(params, "draw_sample", "draw_sample must be True or False", blackBoxExternalParams.drawSample);

  extractDoubleFromPythonDict(params, "exit_threshold_value", "exit_threshold_value parameter must be a number", blackBoxExternalParams.exitThresholdValue);

  extractListFromPythonDict(params, "init_solution", "init_solution must be a list of size num_vars containing only -1/1 or 0/1", blackBoxExternalParams.initialSolution);

  extractStringFromPythonDict(params, "ising_qubo", "ising_qubo must be \"ising\" or \"qubo\"", blackBoxExternalParams.isingQubo);

  PyObject* lpSolverPython = PyDict_GetItemString(params, "lp_solver"); // Borrowed reference
  if (lpSolverPython != NULL && lpSolverPython != Py_None)
    blackBoxExternalParams.lpSolverPtr.reset(new LPSolverPython(lpSolverPython, blackBoxExternalParams.localInteractionPtr));

  extractLongLongIntFromPythonDict(params, "max_num_state_evaluations", "max_num_state_evaluations parameter must be an integer >= 0", blackBoxExternalParams.maxNumStateEvaluations);

  extractIntFromPythonDict(params, "random_seed", "random_seed must be an integer", blackBoxExternalParams.randomSeed);

  extractDoubleFromPythonDict(params, "timeout", "timeout parameter must be a number >= 0", blackBoxExternalParams.timeout);

  extractIntFromPythonDict(params, "verbose", "verbose parameter must be an integer [0, 2]", blackBoxExternalParams.verbose);
}

class LocalInteractionPython : public blackbox::LocalInteraction
{
private:
  virtual void displayOutputImpl(const std::string& msg) const
  {
    PySys_WriteStdout("%s", msg.c_str());
  }

  virtual bool cancelledImpl() const
  {
    if (PyErr_CheckSignals())
    {
      PyErr_Clear();
      return true;
    }

    return false;
  }
};

} // anonymous namespace

PyObject* solve_qsage_impl(PyObject* objective_function, int num_vars, PyObject* ising_solver, PyObject* params)
{
  blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionPython());
  blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new BlackBoxObjectiveFunctionPython(objective_function, localInteractionPtr));
  blackbox::IsingSolverPtr isingSolverPtr(new IsingSolverPython(ising_solver));
  blackbox::BlackBoxExternalParams blackBoxExternalParams;
  blackBoxExternalParams.localInteractionPtr = localInteractionPtr;
  checkBlackBoxParams(params, blackBoxExternalParams);
  return constructBlackBoxResult(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, num_vars, isingSolverPtr, blackBoxExternalParams));
}
