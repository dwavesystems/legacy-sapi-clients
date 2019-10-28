//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "find_embedding_python_wrapper.hpp"
#include "find_embedding.hpp"

#include <map>
#include <set>
#include <algorithm>

namespace
{

void getAdj(PyObject* obj, std::map<std::pair<int, int>, int>& m, int& dim)
{
	PyObject* item;
	dim = 0;

	PyObject *iterator = PyObject_GetIter(obj);
	if (iterator == NULL)
		throw find_embedding_::FindEmbeddingException("Q and A must be an object that supports iterator");
	
	while (item = PyIter_Next(iterator))
	{
		if (PySequence_Size(item) != 2)
			throw find_embedding_::FindEmbeddingException("Q and A's item must have size 2");
		int tmp_1 = PyInt_AsLong(PySequence_Fast_GET_ITEM(item, 0)); // Borrowed reference
		int tmp_2 = PyInt_AsLong(PySequence_Fast_GET_ITEM(item, 1)); // Borrowed reference
		if (tmp_1 < 0 || tmp_2 < 0)
			throw find_embedding_::FindEmbeddingException("Q or A's item is incorrect");
		dim = std::max(dim, std::max(tmp_1 + 1, tmp_2 + 1)); // because index is 0-based
		m.insert(std::make_pair(std::make_pair(tmp_1, tmp_2), 1));

		// release reference when done
		Py_DECREF(item);
	}
	
	Py_DECREF(iterator);
	
	if (PyErr_Occurred())
		throw find_embedding_::FindEmbeddingException("Q and A must be an object that supports iterator");
}

void extractIntFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, int& i)
{
	PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

	if (tmp == NULL || tmp == Py_None)
		return;

	if (!PyInt_Check(tmp))
		throw find_embedding_::FindEmbeddingException(errorMsg);

	i = PyInt_AsLong(tmp);
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
		throw find_embedding_::FindEmbeddingException(errorMsg);
}

void extractBooleanFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, bool& b)
{
	PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

	if (tmp == NULL || tmp == Py_None)
		return;

	if (!PyBool_Check(tmp))
		throw find_embedding_::FindEmbeddingException(errorMsg);

	b = (tmp == Py_True) ? true : false;
}

void extractIntFromPythonDict(PyObject* d, const char* key, const std::string& errorMsg, unsigned int& i)
{
	PyObject* tmp = PyDict_GetItemString(d, key); // Borrowed reference

	if (tmp == NULL || tmp == Py_None)
		return;

	if (!PyInt_Check(tmp))
		throw find_embedding_::FindEmbeddingException(errorMsg);

	i = static_cast<unsigned int>(PyInt_AsLong(tmp));
}

void checkFindEmbeddingParams(PyObject* params, find_embedding_::FindEmbeddingExternalParams& feExternalParams)
{
	std::set<std::string> paramsNameSet;
	paramsNameSet.insert("fast_embedding");
	paramsNameSet.insert("max_no_improvement");
	paramsNameSet.insert("random_seed");
	paramsNameSet.insert("timeout");
	paramsNameSet.insert("tries");
	paramsNameSet.insert("verbose");

	PyObject *key, *value;
	Py_ssize_t pos = 0;
	while (PyDict_Next(params, &pos, &key, &value))
	{
		if (key == NULL || key == Py_None || !PyString_Check(key))
			throw find_embedding_::FindEmbeddingException("params' key must be a string");

		std::string str = PyString_AsString(key);
		if (paramsNameSet.find(str) == paramsNameSet.end())
			throw find_embedding_::FindEmbeddingException(std::string(1, '"') + str + "\" is not a valid parameter for find_embedding");
	}

	extractBooleanFromPythonDict(params, "fast_embedding", "fast_embedding must be a boolean", feExternalParams.fastEmbedding);
	
	extractIntFromPythonDict(params, "max_no_improvement", "max_no_improvement must be an integer >= 0", feExternalParams.maxNoImprovement);

	extractIntFromPythonDict(params, "random_seed", "random_seed must be an integer", feExternalParams.randomSeed);

	extractDoubleFromPythonDict(params, "timeout", "timeout parameter must be a number >= 0", feExternalParams.timeout);
	
	extractIntFromPythonDict(params, "tries", "tries parameter must be an integer >= 0", feExternalParams.tries);

	extractIntFromPythonDict(params, "verbose", "verbose parameter must be an integer [0, 1]", feExternalParams.verbose);
}

PyObject* constructEmbeddingResult(const std::vector<std::vector<int> >& result)
{
	PyObject* embeddings = PyList_New(result.size());

	for (int i = 0; i < result.size(); ++i)
	{
		PyObject* embeddingsPart = PyList_New(result[i].size());
		for (int j = 0; j < result[i].size(); ++j)
			PyList_SetItem(embeddingsPart, j, PyInt_FromLong(result[i][j]));
		PyList_SetItem(embeddings, i, embeddingsPart);
	}

	return embeddings;
}

class LocalInteractionPython : public find_embedding_::LocalInteraction
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

}


PyObject* find_embedding_impl(PyObject* Q, PyObject* A, PyObject* params)
{
	std::map<std::pair<int, int>, int> QMap;
	int QSize;
	getAdj(Q, QMap, QSize);
	compressed_matrix::CompressedMatrix<int> QInput(QSize, QSize, QMap);
	std::map<std::pair<int, int>, int> AMap;
	int ASize;
	getAdj(A, AMap, ASize);
	compressed_matrix::CompressedMatrix<int> AInput(ASize, ASize, AMap);
	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new LocalInteractionPython());
	checkFindEmbeddingParams(params, feExternalParams);
	return constructEmbeddingResult(find_embedding_::findEmbedding(QInput, AInput, feExternalParams));
}
