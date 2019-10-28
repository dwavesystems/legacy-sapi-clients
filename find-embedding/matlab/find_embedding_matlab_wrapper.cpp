//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <mex.h>

#include "find_embedding.hpp"

#include <set>
#include <string>

extern "C" bool utIsInterruptPending();
extern "C" void utSetInterruptPending(bool);

namespace
{

template <class T>
void parseScalar(const mxArray* a, const char* errorMsg, T& t)
{
	if (   !a
		|| mxIsEmpty(a)
		|| mxIsSparse(a)
		|| !mxIsDouble(a)
		|| mxIsComplex(a)	    
		|| mxGetNumberOfDimensions(a) != 2
		|| mxGetNumberOfElements(a) != 1 )
		throw find_embedding_::FindEmbeddingException(errorMsg);

	if (mxIsNaN(static_cast<double>(*mxGetPr(a))))
		throw find_embedding_::FindEmbeddingException(errorMsg);

	t = static_cast<T>(*mxGetPr(a));
}

void parseMatrix(const mxArray* a, const char* errorMsg, std::map<std::pair<int, int>, int>& aMap, int& aSize)
{
	if (!a)
		throw find_embedding_::FindEmbeddingException(errorMsg);

	int numRows = static_cast<int>(mxGetM(a));
	int numCols = static_cast<int>(mxGetN(a));	

	if (   mxGetNumberOfDimensions(a) != 2
		|| !mxIsDouble(a) 
		|| mxIsComplex(a)
		|| numRows != numCols)
		throw find_embedding_::FindEmbeddingException(errorMsg);

	aSize = numRows;

	const double* data = mxGetPr(a);

	if (mxIsSparse(a))
	{
		const mwIndex* jc = mxGetJc(a);
		const mwIndex* ir = mxGetIr(a);
		for (int col = 0; col < numCols; ++col)
			for (mwIndex index = jc[col]; index < jc[col + 1]; ++index, ++data)
			{
				int row = static_cast<int>(ir[index]);
				// ignore the 0 entries
				if (*data != 0.0)
					aMap.insert(std::make_pair(std::make_pair(row, col), 1));
			}
	}
	else
	{			
		for (int col = 0; col < numCols; ++col)
			for (int row = 0; row < numRows; ++row, ++data)
			{
				// ignore the 0 entries
				if (*data != 0.0)
					aMap.insert(std::make_pair(std::make_pair(row, col), 1));
			}
	}
}

void parseBoolean(const mxArray* a, const char* errorMsg, bool& b)
{
	if (   !a
		|| !mxIsLogicalScalar(a)
		|| mxIsEmpty(a) )
		throw find_embedding_::FindEmbeddingException(errorMsg);
	
	b = mxIsLogicalScalarTrue(a);
}

class LocalInteractionMATLAB : public find_embedding_::LocalInteraction
{
private:
	virtual void displayOutputImpl(const std::string& msg) const
	{
		mexPrintf("%s", msg.c_str());
		mexEvalString("drawnow;");
	}

	virtual bool cancelledImpl() const
	{
		if (utIsInterruptPending())
		{
			utSetInterruptPending(false);
			return true;
		}

		return false;
	}
};

void checkFindEmbeddingParameters(const mxArray* paramsArray, find_embedding_::FindEmbeddingExternalParams& findEmbeddingExternalParams)
{
	// mxIsEmpty(paramsArray) handles the case when paramsArray is: struct('a', {}), since isempty(struct('a', {})) is true !!!
	// mxGetM(paramsArray) != 1 || mxGetN(paramsArray) != 1 handles the case when paramsArray is: struct('a', {1 1}) or struct('a', {1; 1}),
	// since size(struct('a', {1 1})) is 1 by 2 and size(struct('a', {1; 1})) is 2 by 1 !!!
	if (!mxIsStruct(paramsArray) || mxIsEmpty(paramsArray) || mxGetNumberOfDimensions(paramsArray) != 2 || mxGetM(paramsArray) != 1 || mxGetN(paramsArray) != 1)
		throw find_embedding_::FindEmbeddingException("find_embedding_mex parameters must be a non-empty 1 by 1 structure");

	std::set<std::string> paramsNameSet;
	paramsNameSet.insert("fast_embedding");
	paramsNameSet.insert("max_no_improvement");
	paramsNameSet.insert("random_seed");
	paramsNameSet.insert("timeout");
	paramsNameSet.insert("tries");
	paramsNameSet.insert("verbose");

	int numFields = mxGetNumberOfFields(paramsArray);
	for (int i = 0; i < numFields; ++i)
	{
		std::string str = mxGetFieldNameByNumber(paramsArray, i);
		if (paramsNameSet.find(str) == paramsNameSet.end())
			throw find_embedding_::FindEmbeddingException(std::string(1, '\'') + str + "' is not a valid parameter for findEmbedding");
	}

	const mxArray* fieldValueArray = mxGetField(paramsArray, 0, "fast_embedding");
	if (fieldValueArray)
		parseBoolean(fieldValueArray, "fast_embedding must be a boolean value", findEmbeddingExternalParams.fastEmbedding);

	fieldValueArray = mxGetField(paramsArray, 0, "max_no_improvement");
	if (fieldValueArray)
		parseScalar<int>(fieldValueArray, "max_no_improvement must be an integer >= 0", findEmbeddingExternalParams.maxNoImprovement);

	fieldValueArray = mxGetField(paramsArray, 0, "random_seed");
	if (fieldValueArray)
		parseScalar<unsigned int>(fieldValueArray, "random_seed must be an integer", findEmbeddingExternalParams.randomSeed);

	fieldValueArray = mxGetField(paramsArray, 0, "timeout");
	if (fieldValueArray)
		parseScalar<double>(fieldValueArray, "timeout parameter must be a number >= 0.0", findEmbeddingExternalParams.timeout);

	fieldValueArray = mxGetField(paramsArray, 0, "tries");
	if (fieldValueArray)
		parseScalar<int>(fieldValueArray, "tries parameter must be an integer >= 0", findEmbeddingExternalParams.tries);

	fieldValueArray = mxGetField(paramsArray, 0, "verbose");
	if (fieldValueArray)
		parseScalar<int>(fieldValueArray, "verbose parameter must be an integer [0, 1]", findEmbeddingExternalParams.verbose);
}

}

// problem, A, params
void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	if (nrhs != 3)
		mexErrMsgTxt("Wrong number of arguments");
 
	if (nlhs > 1)
		mexErrMsgTxt("Too many outputs");

	std::vector<std::vector<int> > result;	
	
	try
	{
		std::map<std::pair<int, int>, int> SMap;
		int SSize;
		parseMatrix(prhs[0], "parameter must be a full or sparse square matrix", SMap, SSize);
		compressed_matrix::CompressedMatrix<int> S(SSize, SSize, SMap);

		std::map<std::pair<int, int>, int> AMap;
		int ASize;
		parseMatrix(prhs[1], "parameter must be a full or sparse square matrix", AMap, ASize);
		compressed_matrix::CompressedMatrix<int> A(ASize, ASize, AMap);

		find_embedding_::FindEmbeddingExternalParams findEmbeddingExternalParams;
		findEmbeddingExternalParams.localInteractionPtr.reset(new LocalInteractionMATLAB());
		checkFindEmbeddingParameters(prhs[2], findEmbeddingExternalParams);

		result = find_embedding_::findEmbedding(S, A, findEmbeddingExternalParams);
	}
	catch (const find_embedding_::FindEmbeddingException& e)
	{
		mexErrMsgTxt(e.what().c_str());
	}
	catch (...)
	{
		mexErrMsgTxt("unknown error");
	}

	plhs[0] = mxCreateCellMatrix((result.size() == 0) ? 0 : 1, result.size());
	for (unsigned int i = 0; i < result.size(); ++i)
	{  
		mxArray* cellPart = mxCreateDoubleMatrix(1, result[i].size(), mxREAL);
		std::copy(result[i].begin(), result[i].end(), mxGetPr(cellPart));
		mxSetCell(plhs[0], i, cellPart);
	}
}
