//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "fix_variables_python_wrapper.hpp"
#include "fix_variables.hpp"

#include <map>
#include <set>
#include <algorithm>

namespace
{

PyObject* constructFixVariablesResult(const fix_variables_::FixVariablesResult& result)
{
	PyObject* fixedVars = PyDict_New();
	// result.fixedVars[i].first - 1 makes the variable index 0-based since results from C++ codes are 1-based
	for (int i = 0; i < result.fixedVars.size(); ++i)
	{
		PyObject* key = PyInt_FromLong(result.fixedVars[i].first - 1);
		PyObject* val = PyInt_FromLong(result.fixedVars[i].second);
		PyDict_SetItem(fixedVars, key, val);
		Py_DECREF(key);
		Py_DECREF(val);
	}

	PyObject* newQ = PyDict_New();
	for (int i = 0; i < result.newQ.numRows(); ++i)
	{
		int start = result.newQ.rowOffsets()[i];
		int end = result.newQ.rowOffsets()[i + 1];
		for (int j = start; j < end; ++j)
		{
			int r = i;
			int c = result.newQ.colIndices()[j];
			PyObject* key = PyTuple_New(2);
			PyTuple_SetItem(key, 0, PyInt_FromLong(r));
			PyTuple_SetItem(key, 1, PyInt_FromLong(c));
			PyObject* val = PyFloat_FromDouble(result.newQ.values()[j]);
			PyDict_SetItem(newQ, key, val);
			Py_DECREF(key);
			Py_DECREF(val);
		}
	}

	PyObject* offset = PyFloat_FromDouble(result.offset);

	PyObject* ret = PyDict_New();

	// Putting something in a dictionary is "storing" it. Therefore PyDict_SetItem() INCREFs both the key and the val. So here it needs to
	// Py_DECREF both the key and the val to make its reference count back to 1
	PyDict_SetItemString(ret, "fixed_variables", fixedVars);
	Py_XDECREF(fixedVars);

	PyDict_SetItemString(ret, "new_Q", newQ);
	Py_XDECREF(newQ);

	PyDict_SetItemString(ret, "offset", offset);
	Py_XDECREF(offset);

	return ret;
}

void parseS(PyObject* S, std::map<std::pair<int, int>, double>& SMap, int& dim)
{
	PyObject* item;
	
	dim = 0;

	if (PyDict_Check(S))
	{
		if (static_cast<int>(PyDict_Size(S)) < 0)
			throw fix_variables_::FixVariablesException("Q's size is incorrect");

		PyObject *key, *value;
		Py_ssize_t pos = 0;
		while (PyDict_Next(S, &pos, &key, &value))
		{
			if (key == NULL || key == Py_None || !PyTuple_Check(key) || PyTuple_Size(key) != 2)
				throw fix_variables_::FixVariablesException("Q's key is incorrect");

			item = PyTuple_GetItem(key, 0);
			if (item == NULL || item == Py_None || !PyInt_Check(item))
				throw fix_variables_::FixVariablesException("Q's key is incorrect");
			int tmp_1 = PyInt_AsLong(item);

			item = PyTuple_GetItem(key, 1);
			if (item == NULL || item == Py_None || !PyInt_Check(item))
				throw fix_variables_::FixVariablesException("Q's key is incorrect");
			int tmp_2 = PyInt_AsLong(item);

			if (tmp_1 < 0 || tmp_2 < 0)
				throw fix_variables_::FixVariablesException("Q's key is incorrect");

			dim = std::max(dim, std::max(tmp_1 + 1, tmp_2 + 1)); // because index is 0-based

			if (value == NULL || value == Py_None)
				throw fix_variables_::FixVariablesException("Q's value is incorrect");

			double tmp = PyFloat_AsDouble(value);
			if (PyErr_Occurred())
				throw fix_variables_::FixVariablesException("Q's item is not correct");

			SMap.insert(std::make_pair(std::make_pair(tmp_1, tmp_2), tmp));
		}
	}
	else
		throw fix_variables_::FixVariablesException("Q must be a dictionary");
}

} // anonymous namespace


PyObject* fix_variables_impl(PyObject* Q, PyObject* method)
{
	std::map<std::pair<int, int>, double> QMap;
	int QSize;
	parseS(Q, QMap, QSize);
	compressed_matrix::CompressedMatrix<double> QInput(QSize, QSize, QMap);
	int mtd = 1;
	if (method != Py_None)
	{
		if (!PyString_Check(method))
			throw fix_variables_::FixVariablesException("method must be \"optimized\" or \"standard\"");
		
		std::string str = PyString_AsString(method);
		if (str == "optimized")
			mtd = 1;
		else if (str == "standard")
			mtd = 2;
		else
			throw fix_variables_::FixVariablesException("method must be \"optimized\" or \"standard\"");
	}
	return constructFixVariablesResult(fix_variables_::fixQuboVariables(QInput, mtd));
}
