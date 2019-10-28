//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <mex.h>

#include "blackbox.hpp"

#include <map>
#include <utility>
#include <set>
#include <string>
#include <algorithm>

extern "C" bool utIsInterruptPending();
extern "C" void utSetInterruptPending(bool);

namespace
{

const char* fieldNames[] = {"num_state_evaluations", "num_obj_func_calls", "num_solver_calls", "num_lp_solver_calls",
	                        "state_evaluations_time", "solver_calls_time", "lp_solver_calls_time", "total_time", "history"};

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
		throw blackbox::BlackBoxException(errorMsg);

	if (mxIsNaN(static_cast<double>(*mxGetPr(a))))
		throw blackbox::BlackBoxException(errorMsg);

	t = static_cast<T>(*mxGetPr(a));
}

template <class T>
void parse1DMatrix(const mxArray* a, const char* errorMsg, std::vector<T>& v)
{
	if (   !a
		|| mxIsSparse(a)
		|| mxIsEmpty(a)
		|| !mxIsDouble(a) 
		|| mxIsComplex(a)
		|| mxGetNumberOfDimensions(a) != 2 )
		throw blackbox::BlackBoxException(errorMsg);

	double* data = mxGetPr(a);
	size_t dataSize = mxGetNumberOfElements(a);
	for (size_t j = 0; j < dataSize; ++j)
	{
		if (mxIsNaN(data[j]))
			throw blackbox::BlackBoxException(errorMsg);
		v.push_back(static_cast<T>(data[j]));
	}
}

template <class T>
void parse2DMatrix(const mxArray* a, const char* errorMsg, std::vector<std::vector<T> >& v)
{
	if (   !a
		|| mxIsSparse(a)
		|| mxIsEmpty(a)
		|| !mxIsDouble(a) 
		|| mxIsComplex(a)
		|| mxGetNumberOfDimensions(a) != 2 )
		throw blackbox::BlackBoxException(errorMsg);

	double* data = mxGetPr(a);
	int numRows = static_cast<int>(mxGetM(a)); // the number of qubits
	int numCols = static_cast<int>(mxGetN(a)); // the number of solutions
	v.assign(numCols, std::vector<T>(numRows));
	for (int j = 0; j < numCols; ++j)
		for (int i = 0; i < numRows; ++i)
		{
			if (mxIsNaN(data[j]))
				throw blackbox::BlackBoxException(errorMsg);
			v[j][i] = static_cast<T>(data[j * numRows + i]);
		}
}

void parseString(const mxArray* a, const char* errorMsg, std::string& s)
{
	if (   !a
		|| !mxIsChar(a))
		throw blackbox::BlackBoxException(errorMsg);

	std::shared_ptr<char> fieldValue(mxArrayToString(a), mxFree);
	s = fieldValue.get();
}

void parseBoolean(const mxArray* a, const char* errorMsg, bool& b)
{
	if (   !a
		|| !mxIsLogicalScalar(a)
		|| mxIsEmpty(a) )
		throw blackbox::BlackBoxException(errorMsg);
	
	b = mxIsLogicalScalarTrue(a);
}

class LocalInteractionMatlab : public blackbox::LocalInteraction
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

class IsingSolverMatlab : public blackbox::IsingSolver
{
public:
	IsingSolverMatlab(const mxArray* isingSolver) : isingSolver_(isingSolver)
	{
		if (   !isingSolver_
			|| !mxIsStruct(isingSolver_)
			|| mxIsEmpty(isingSolver_)
			|| !mxGetField(isingSolver_, 0, "solve_ising")
			|| !mxGetField(isingSolver_, 0, "qubits")
			|| !mxGetField(isingSolver_, 0, "couplers") )
			throw blackbox::BlackBoxException("MATLAB ising solver must be a structure that has field: \"solve_ising\", \"qubits\", \"couplers\"");

		parse1DMatrix<int>(mxGetField(isingSolver_, 0, "qubits"), "MATLAB ising solver's \"qubits\" field must be a matrix", qubits_);
		
		const mxArray* a = mxGetField(isingSolver_, 0, "couplers");
		if (   !a
		    || mxIsSparse(a)
		    || mxIsEmpty(a)
		    || !mxIsDouble(a) 
		    || mxIsComplex(a)
		    || mxGetNumberOfDimensions(a) != 2 )
			throw blackbox::BlackBoxException("MATLAB ising solver's \"couplers\" field must be a 2 by n matrix");
		
		double* data = mxGetPr(a);
		int numRows = static_cast<int>(mxGetM(a)); // assume numRows is 2
		int numCols = static_cast<int>(mxGetN(a)); // the number of couplers
		// assume the MATLAB couplers is a 2 by n matrix
		if (numRows != 2)
			throw blackbox::BlackBoxException("MATLAB ising solver's \"couplers\" field must be a 2 by n matrix");

		couplers_.resize(numCols);
		for (int j = 0; j < numCols; ++j)
		{
			couplers_[j].first = static_cast<int>(data[j * numRows]);
		    couplers_[j].second = static_cast<int>(data[j * numRows + 1]);
		}
		a = mxGetField(isingSolver_, 0, "h_range");
		if (a)
		{
			std::vector<double> hRange;
			parse1DMatrix<double>(a, "MATLAB ising solver's \"h_range\" field must be a 1 by 2 matrix", hRange);
			if (hRange.size() != 2)
				throw blackbox::BlackBoxException("MATLAB ising solver's \"h_range\" field must be a 1 by 2 matrix");
			hMin_ = hRange[0];
			hMax_ = hRange[1];
		}

		a = mxGetField(isingSolver_, 0, "j_range");
		if (a)
		{
			std::vector<double> jRange;
			parse1DMatrix<double>(a, "MATLAB ising solver's \"j_range\" field must be a 1 by 2 matrix", jRange);
			if (jRange.size() != 2)
				throw blackbox::BlackBoxException("MATLAB ising solver's \"j_range\" field must be a 1 by 2 matrix");
			jMin_ = jRange[0];
			jMax_ = jRange[1];
		}
	}

	virtual ~IsingSolverMatlab() {}

private:
	virtual void solveIsingImpl(const std::vector<double>& h, const std::map<std::pair<int, int>, double>& J, std::vector<std::vector<int> >& solutions, std::vector<double>& energies, std::vector<int>& numOccurrences) const
	{
		int JNumRows = 0;
		int JNumCols = 0;
		for (std::map<std::pair<int, int>, double>::const_iterator it = J.begin(), end = J.end(); it != end; ++it)
		{
			JNumRows = std::max(JNumRows, it->first.first + 1);
			JNumCols = std::max(JNumCols, it->first.second + 1);
		}

		int JDim = std::max(JNumRows, JNumCols);

		double* ptr;
		mxArray* input[3]; //now the first parameter must be the MATLAB function handle
		int nrhs = 3;
		input[0] = const_cast<mxArray*>(mxGetField(isingSolver_, 0, "solve_ising"));
		input[1] = mxCreateDoubleMatrix(h.size(), 1, mxREAL);
		input[2] = mxCreateDoubleMatrix(JDim, JDim, mxREAL);
		ptr = mxGetPr(input[1]);
		for (int i = 0; i < h.size(); ++i)
			ptr[i] = h[i];
		ptr = mxGetPr(input[2]);
		for (std::map<std::pair<int, int>, double>::const_iterator it = J.begin(), end = J.end(); it != end; ++it)
			ptr[JDim * it->first.second + it->first.first] = it->second;

		mxArray* output[1];
		int nlhs = 1;

		if (mexCallMATLABWithTrap(nlhs, output, nrhs, input, "feval"))
			throw blackbox::BlackBoxException("The MATLAB solve ising function call failed");

		if (   !output[0]
			|| !mxIsStruct(output[0])
			|| mxIsEmpty(output[0]) )
			throw blackbox::BlackBoxException("The MATLAB ising solver's solve_ising function return value is invalid");

		parse2DMatrix(mxGetField(output[0], 0, "solutions"), "The MATLAB ising solver's solve_ising function return value solutions is invalid", solutions);
		parse1DMatrix<double>(mxGetField(output[0], 0, "energies"), "The MATLAB ising solver's solve_ising function return value energies is invalid", energies);
		const mxArray* ma = mxGetField(output[0], 0, "num_occurrences");
		if (ma)
			parse1DMatrix<int>(ma, "The MATLAB ising solver's solve_ising function return value num_occurrences is invalid", numOccurrences);

		mxDestroyArray(input[1]);
		mxDestroyArray(output[0]);
	}

	const mxArray* isingSolver_;
};

class BlackBoxObjectiveFunctionMatlab : public blackbox::BlackBoxObjectiveFunction
{
public:
	BlackBoxObjectiveFunctionMatlab(const mxArray* matlabObjectiveFunction, blackbox::LocalInteractionPtr localInteractionPtr) : BlackBoxObjectiveFunction(localInteractionPtr), matlabObjectiveFunction_(matlabObjectiveFunction)
	{
		if (!matlabObjectiveFunction)
			throw blackbox::BlackBoxException("MATLAB objective function is invalid");
	}

	virtual ~BlackBoxObjectiveFunctionMatlab() {}

private:
	virtual std::vector<double> computeObjectValueImpl(const std::vector<std::vector<int> >& states) const
	{
		int numStates = static_cast<int>(states.size());
		int stateLen = static_cast<int>(states[0].size());
		double* ptr;
		mxArray* input[2]; //now the first parameter must be the MATLAB function handle
		int nrhs = 2;
		input[0] = const_cast<mxArray*>(matlabObjectiveFunction_);
		input[1] = mxCreateDoubleMatrix(stateLen, numStates, mxREAL);
		ptr = mxGetPr(input[1]);
		int index = 0;
		for (int i = 0; i < states.size(); ++i)
			for (int j = 0; j < states[i].size(); ++j)
				ptr[index++] = static_cast<double>(states[i][j]);

		mxArray* output[1];
		int nlhs = 1;

		if (mexCallMATLABWithTrap(nlhs, output, nrhs, input, "feval"))
			throw blackbox::BlackBoxException("The MATLAB objective function call failed");

		if (   !output[0]
		    || mxIsSparse(output[0])
			|| !mxIsDouble(output[0]) 
			|| mxIsComplex(output[0])
			|| mxIsEmpty(output[0])
			|| mxGetNumberOfDimensions(output[0]) != 2)
			throw blackbox::BlackBoxException("The objective_function's return value is invalid");

		std::vector<double> ret;
		ptr = mxGetPr(output[0]);
		size_t sz = mxGetNumberOfElements(output[0]);
		for (size_t i = 0; i < sz; ++i)
			ret.push_back(ptr[i]);

		mxDestroyArray(input[1]);
		mxDestroyArray(output[0]);

		return ret;
	}

	const mxArray* matlabObjectiveFunction_;
};

class LPSolverMatlab : public blackbox::LPSolver
{
public:
	LPSolverMatlab(const mxArray* matlabLPSolver, blackbox::LocalInteractionPtr localInteractionPtr) : LPSolver(localInteractionPtr), matlabLPSolver_(matlabLPSolver)
	{
		if (   !matlabLPSolver
			|| mxIsEmpty(matlabLPSolver) )
			throw blackbox::BlackBoxException("MATLAB lp_solver is invalid");
	}

	virtual ~LPSolverMatlab() {}

private:
	virtual std::vector<double> solveImpl(const std::vector<double>& f, const std::vector<std::vector<double> >& Aineq, const std::vector<double>& bineq, 
		                                  const std::vector<std::vector<double> >& Aeq, const std::vector<double>& beq, const std::vector<double>& lb,
										  const std::vector<double>& ub) const
	{
		double* ptr;
		mxArray* input[8]; //now the first parameter must be the MATLAB function handle
		int nrhs = 8;

		input[0] = const_cast<mxArray*>(matlabLPSolver_);
		// f is 1 by numVars
		// Aineq is AineqSize by numVars
		// bineq is 1 by AineqSize
		// Aeq is AeqSize by numVars
		// beq is 1 by AeqSize
		// lb is 1 by numVars
		// ub is 1 by numVars
		// return value should be 1 by numVars
		unsigned int AineqSize = static_cast<unsigned int>(Aineq.size());
		unsigned int AeqSize = static_cast<unsigned int>(Aeq.size());
		unsigned int numVars = static_cast<unsigned int>(lb.size());

		// MATLAB f is numVars by 1
		input[1] = mxCreateDoubleMatrix(numVars, 1, mxREAL);
		ptr = mxGetPr(input[1]);
		for (unsigned int i = 0; i < numVars; ++i)
			ptr[i] = f[i];

		// MATLAB Aineq is AineqSize by numVars
		input[2] = mxCreateDoubleMatrix(AineqSize, numVars, mxREAL);
		ptr = mxGetPr(input[2]);
		for (unsigned int j = 0; j < numVars; ++j)
			for (unsigned int i = 0; i < AineqSize; ++i)
				ptr[j * AineqSize + i] = Aineq[i][j];

		// MATLAB bineq is AineqSize by 1
		input[3] = mxCreateDoubleMatrix(AineqSize, 1, mxREAL);
		ptr = mxGetPr(input[3]);
		for (unsigned int i = 0; i < AineqSize; ++i)
			ptr[i] = bineq[i];

		// MATLAB Aeq is AeqSize by numVars
		input[4] = mxCreateDoubleMatrix(AeqSize, numVars, mxREAL);
		ptr = mxGetPr(input[4]);
		for (unsigned int j = 0; j < numVars; ++j)
			for (unsigned int i = 0; i < AeqSize; ++i)
				ptr[j * AeqSize + i] = Aeq[i][j];

		// MATLAB beq is AeqSize by 1
		input[5] = mxCreateDoubleMatrix(AeqSize, 1, mxREAL);
		ptr = mxGetPr(input[5]);
		for (unsigned int i = 0; i < AeqSize; ++i)
			ptr[i] = beq[i];

		// MATLAB lb is numVars by 1
		input[6] = mxCreateDoubleMatrix(numVars, 1, mxREAL);
		ptr = mxGetPr(input[6]);
		for (unsigned int i = 0; i < numVars; ++i)
			ptr[i] = lb[i];

		// MATLAB ub is numVars by 1
		input[7] = mxCreateDoubleMatrix(numVars, 1, mxREAL);
		ptr = mxGetPr(input[7]);
		for (unsigned int i = 0; i < numVars; ++i)
			ptr[i] = ub[i];

		mxArray* output[1];
		int nlhs = 1;

		if (mexCallMATLABWithTrap(nlhs, output, nrhs, input, "feval"))
			throw blackbox::BlackBoxException("The MATLAB lp solver call failed");

		if (   !output[0]
		    || mxIsSparse(output[0])
			|| !mxIsDouble(output[0]) 
			|| mxIsComplex(output[0])
			|| mxIsEmpty(output[0])
			|| mxGetNumberOfDimensions(output[0]) != 2)
			throw blackbox::BlackBoxException("The lp_solver's return value is invalid");

		std::vector<double> ret;
		ptr = mxGetPr(output[0]);
		size_t sz = mxGetNumberOfElements(output[0]);
		for (size_t i = 0; i < sz; ++i)
			ret.push_back(ptr[i]);

		for (int i = 1; i < nrhs; ++i)
			mxDestroyArray(input[i]);

		mxDestroyArray(output[0]);

		return ret;
	}

	const mxArray* matlabLPSolver_;
};

void checkBlackBoxParameters(const mxArray* paramsArray, blackbox::BlackBoxExternalParams& blackBoxExternalParams)
{
	// mxIsEmpty(paramsArray) handles the case when paramsArray is: struct('a', {}), since isempty(struct('a', {})) is true !!!
	// mxGetM(paramsArray) != 1 || mxGetN(paramsArray) != 1 handles the case when paramsArray is: struct('a', {1 1}) or struct('a', {1; 1}),
	// since size(struct('a', {1 1})) is 1 by 2 and size(struct('a', {1; 1})) is 2 by 1 !!!
	if (!mxIsStruct(paramsArray) || mxIsEmpty(paramsArray) || mxGetNumberOfDimensions(paramsArray) != 2 || mxGetM(paramsArray) != 1 || mxGetN(paramsArray) != 1)
		throw blackbox::BlackBoxException("solve_qsage_mex parameters must be a non-empty 1 by 1 structure");

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

	int numFields = mxGetNumberOfFields(paramsArray);
	for (int i = 0; i < numFields; ++i)
	{
		std::string str = mxGetFieldNameByNumber(paramsArray, i);
		if (paramsNameSet.find(str) == paramsNameSet.end())
			throw blackbox::BlackBoxException(std::string(1, '\'') + str + "' is not a valid parameter for solveQSage");
	}

	const mxArray* fieldValueArray = mxGetField(paramsArray, 0, "draw_sample");
	if (fieldValueArray)
		parseBoolean(fieldValueArray, "draw_sample must be a boolean value", blackBoxExternalParams.drawSample);

	fieldValueArray = mxGetField(paramsArray, 0, "exit_threshold_value");
	if (fieldValueArray)
		parseScalar<double>(fieldValueArray, "exit_threshold_value parameter must be a number", blackBoxExternalParams.exitThresholdValue);

	fieldValueArray = mxGetField(paramsArray, 0, "init_solution");
	if (fieldValueArray)
		parse1DMatrix<int>(fieldValueArray, "init_solution must be a matrix", blackBoxExternalParams.initialSolution);

	fieldValueArray = mxGetField(paramsArray, 0, "ising_qubo");
	if (fieldValueArray)
	{
		std::string str;
		parseString(fieldValueArray, "ising_qubo must be \"ising\" or \"qubo\"", str);
		if (str != "ising" && str != "qubo")
			throw blackbox::BlackBoxException("ising_qubo must be \"ising\" or \"qubo\"");
		if (str == "ising")
			blackBoxExternalParams.isingQubo = blackbox::ISING;
		else
			blackBoxExternalParams.isingQubo = blackbox::QUBO;
	}
	
	fieldValueArray = mxGetField(paramsArray, 0, "lp_solver");
	if (fieldValueArray)
		blackBoxExternalParams.lpSolverPtr.reset(new LPSolverMatlab(fieldValueArray, blackBoxExternalParams.localInteractionPtr));

	fieldValueArray = mxGetField(paramsArray, 0, "max_num_state_evaluations");
	if (fieldValueArray)
		parseScalar<long long int>(fieldValueArray, "max_num_state_evaluations parameter must be >= 0", blackBoxExternalParams.maxNumStateEvaluations);
	
	fieldValueArray = mxGetField(paramsArray, 0, "random_seed");
	if (fieldValueArray)
		parseScalar<unsigned int>(fieldValueArray, "random_seed must be an integer", blackBoxExternalParams.randomSeed);

	fieldValueArray = mxGetField(paramsArray, 0, "timeout");
	if (fieldValueArray)
		parseScalar<double>(fieldValueArray, "timeout parameter must be a number >= 0", blackBoxExternalParams.timeout);

	fieldValueArray = mxGetField(paramsArray, 0, "verbose");
	if (fieldValueArray)
		parseScalar<int>(fieldValueArray, "verbose parameter must be an integer [0, 2]", blackBoxExternalParams.verbose);
}

}

// solver, objective function, num_vars, params
void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	if (nrhs != 4)
		mexErrMsgTxt("Wrong number of arguments");

	if (nlhs > 3)
		mexErrMsgTxt("Too many outputs");

	blackbox::BlackBoxResult result;

	try
	{
		blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionMatlab());
		blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new BlackBoxObjectiveFunctionMatlab(prhs[0], localInteractionPtr));

		int numVars;
		parseScalar<int>(prhs[1], "num_vars must be a non-negative integer", numVars);

		blackbox::IsingSolverPtr isingSolverPtr(new IsingSolverMatlab(prhs[2]));

		blackbox::BlackBoxExternalParams blackBoxExternalParams;
		blackBoxExternalParams.localInteractionPtr = localInteractionPtr;
		checkBlackBoxParameters(prhs[3], blackBoxExternalParams);

		result = blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams);
	}
	catch (const blackbox::BlackBoxException& e)
	{
		mexErrMsgTxt(e.what().c_str());
	}
	catch (...)
	{
		mexErrMsgTxt("runtime error");
	}

	plhs[0] = mxCreateDoubleMatrix(result.bestSolution.size(), 1, mxREAL);
	double* ptr = mxGetPr(plhs[0]);
	for (unsigned int i = 0; i < result.bestSolution.size(); ++i)
		ptr[i] = static_cast<double>(result.bestSolution[i]);

	plhs[1] = mxCreateDoubleScalar(result.bestEnergy);
	plhs[2] = mxCreateStructMatrix(1, 1, 9, fieldNames);
	mxSetFieldByNumber(plhs[2], 0, 0, mxCreateDoubleScalar(static_cast<double>(result.info.numStateEvaluations)));
	mxSetFieldByNumber(plhs[2], 0, 1, mxCreateDoubleScalar(static_cast<double>(result.info.numObjFuncCalls)));
	mxSetFieldByNumber(plhs[2], 0, 2, mxCreateDoubleScalar(static_cast<double>(result.info.numSolverCalls)));
	mxSetFieldByNumber(plhs[2], 0, 3, mxCreateDoubleScalar(static_cast<double>(result.info.numLPSolverCalls)));
	mxSetFieldByNumber(plhs[2], 0, 4, mxCreateDoubleScalar(result.info.stateEvaluationsTime));
	mxSetFieldByNumber(plhs[2], 0, 5, mxCreateDoubleScalar(result.info.solverCallsTime));
	mxSetFieldByNumber(plhs[2], 0, 6, mxCreateDoubleScalar(result.info.lpSolverCallsTime));
	mxSetFieldByNumber(plhs[2], 0, 7, mxCreateDoubleScalar(result.info.totalTime));
	mxArray* history = mxCreateDoubleMatrix(result.info.progressTable.size(), 6, mxREAL);
	ptr = mxGetPr(history);
	for (unsigned int i = 0; i < result.info.progressTable.size(); ++i)
	{
		ptr[i] = static_cast<double>(result.info.progressTable[i].first[0]);
		ptr[i + result.info.progressTable.size()] = static_cast<double>(result.info.progressTable[i].first[1]);
		ptr[i + 2 * result.info.progressTable.size()] = static_cast<double>(result.info.progressTable[i].first[2]);
		ptr[i + 3 * result.info.progressTable.size()] = static_cast<double>(result.info.progressTable[i].first[3]);
		ptr[i + 4 * result.info.progressTable.size()] = result.info.progressTable[i].second.first;
		ptr[i + 5 * result.info.progressTable.size()] = result.info.progressTable[i].second.second;
	}
	mxSetFieldByNumber(plhs[2], 0, 8, history);
}
