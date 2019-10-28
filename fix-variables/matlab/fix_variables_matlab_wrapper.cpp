//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <mex.h>
#include <memory>
#include "fix_variables.hpp"

//input: Q, can be sparse matrix or full matrix; method, 1 (default) or 2
//output: [fixedVars newQ offset]
void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	//parameter checking
	if (nrhs < 1 || nrhs > 2)
		mexErrMsgTxt("One or two inputs required. See help fix_variables_mex for more details.");

	if (nlhs > 3)
		mexErrMsgTxt("At most three outputs. See help fix_variables_mex for more details.");

	const mxArray* param;
	const double* data;
	size_t m, n;
	double* ptr;

	//Q
	std::vector<int> rowOffsets;
	std::vector<int> colIndices;
	std::vector<double> values;
	param = prhs[0];
	if (!mxIsEmpty(param))
	{
		m = mxGetM(param);
		n = mxGetN(param);

		if (   mxGetNumberOfDimensions(param) != 2
			|| m != n
			|| !mxIsDouble(param) 
			|| mxIsComplex(param) )
			mexErrMsgTxt("Parameter 1 must be a two-dimensional full or sparse N by N matrix which contains doubles for Q defining a QUBO problem x'*Q*x.");

		if (!mxIsSparse(param))
		{       
			rowOffsets.resize(n + 1);
			data = mxGetPr(param);
			int offset = 0;
			for (size_t j = 0; j < n; ++j) //col
			{
				rowOffsets[j] = offset;
				for (size_t i = 0; i < m; ++i) //row
				{
					if ((*data) != 0.0) //need to check if it is zero, important !!!
					{
						colIndices.push_back(static_cast<int>(i));
						values.push_back(*data);
						++offset;
					}
					++data;
				}
			}
			rowOffsets.back() = offset;
		}
		else
		{
			mwIndex* ir = mxGetIr(param); //the ir array, row indices
			mwIndex* jc = mxGetJc(param); //the jc array, column offsets
			data = mxGetPr(param); //the pr array, actual data
			mwIndex nnz = jc[n];
			rowOffsets.assign(jc, jc + n + 1);
			colIndices.assign(ir, ir + nnz);
			values.assign(data, data + nnz);
		}
	}
	else
	{
		m = 0;
		n = 0;
		rowOffsets.resize(n + 1, 0);
	}

	//call the C++ codes
	fix_variables_::FixVariablesResult result;

	try
	{
		compressed_matrix::CompressedMatrix<double> Q(m, n, rowOffsets, colIndices, values);
		
		//method
		int method = 1; //default using method 1
		if (nrhs == 2)
		{
			if (   !prhs[1]
		        || !mxIsChar(prhs[1]))
				throw fix_variables_::FixVariablesException("method must be \"optimized\" or \"standard\"");
				
			std::shared_ptr<char> fieldValue(mxArrayToString(prhs[1]), mxFree);
			std::string str = fieldValue.get();
			if (str == "optimized")
				method = 1;
			else if (str == "standard")
				method = 2;
			else
				throw fix_variables_::FixVariablesException("method must be \"optimized\" or \"standard\"");
		}

		result = fix_variables_::fixQuboVariables(Q, method);
	}
	catch (const fix_variables_::FixVariablesException& e)
	{
		mexErrMsgTxt(e.what().c_str());
	}
	catch (...)
	{
		mexErrMsgTxt("unknown error");
	}

	//convert the C++ result to MATLAB result	
	mxArray* fixedVars = mxCreateDoubleMatrix(result.fixedVars.size(), 2, mxREAL);
	ptr = mxGetPr(fixedVars);
	for (int i = 0; i < result.fixedVars.size(); ++i)
		ptr[i] = result.fixedVars[i].first; //already 1-based, no need to +1

	for (int i = 0; i < result.fixedVars.size(); ++i)
		ptr[i + result.fixedVars.size()] = result.fixedVars[i].second;

	plhs[0] = fixedVars;	

	if (nlhs >= 2)
	{
		int QNumRows = result.newQ.numRows();
		int QNumCols = result.newQ.numCols();
		mxArray* newQ = mxCreateSparse(QNumRows, QNumCols, result.newQ.nnz(), mxREAL);

		double* pr  = mxGetPr(newQ);
		//int* ir = (int*)mxGetIr(newQ); //doesn't work in linux machine
		mwIndex* ir = mxGetIr(newQ);
		//int* jc = (int*)mxGetJc(newQ); //doesn't work in linux machine
		mwIndex* jc = mxGetJc(newQ);

		for (int j = 0; j < QNumCols + 1; ++j)
			jc[j] = 0;

		const std::vector<int>& ro = result.newQ.rowOffsets();
		const std::vector<int>& ci = result.newQ.colIndices();
		const std::vector<double>& v = result.newQ.values();

		//compute the starting locations for each col
		for (int i = 0; i < ci.size(); ++i)
			++jc[ci[i] + 1];

		for (int j = 1; j < QNumCols + 1; ++j)
			jc[j] += jc[j - 1];

		//add all the data 
		for (int i = 0; i < QNumRows; ++i)
		{
			int start = ro[i];
			int end = ro[i + 1];
			for (int j = start; j < end; ++j)
			{
				int r = i;
				int c = ci[j];
				ir[jc[c]] = r;
				pr[jc[c]] = v[j];
				++jc[c];
			}		
		}

		//restore the correct value on the jc array by shifting it by one
		for (int j = QNumCols; j > 0; --j)
			jc[j] = jc[j - 1];

		jc[0] = 0;

		plhs[1] = newQ;
	}

	if (nlhs == 3)
	{
		mxArray* offset = mxCreateDoubleScalar(result.offset);
		plhs[2] = offset;
	}   
}
