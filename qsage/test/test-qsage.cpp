//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "gtest/gtest.h"

#include <numeric>
#include <algorithm>

#include "blackbox.hpp"

namespace
{

std::vector<std::pair<int, int> > getChimeraAdjacency(int M, int N, int L)
{
	std::vector<std::pair<int, int> > ret;

	int m = M;
	int n = N;
	int k = L;

	// vertical edges:
	for (int j = 0; j < n; ++j)
	{
		int start = k * 2 * j;
		for (int i = 0; i < m - 1; ++i)
		{
			for (int t = 0; t < k; ++t)
			{
				int fm = start + t;
				int to = start + t + n * k * 2;
				ret.push_back(std::make_pair(fm, to));
			}
			start = start + n * k * 2;
		}
	}

	// horizontal edges:
	for (int i = 0; i < m; ++i)
	{
		int start = k * (2 * n * i + 1);
		for (int j = 0; j < n - 1; ++j)
		{
			for (int t = 0; t < k; ++t)
			{
				int fm = start + t;
				int to = start + t + k * 2;
				ret.push_back(std::make_pair(fm, to));
			}
			start = start + k * 2;
		}
	}

	// inside-cell edges:
	for (int i = 0; i < m; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			int add = (i * n + j) * k * 2;
			for (int t = 0; t < k; ++t)
				for (int u = k; u < 2 * k; ++u)
					ret.push_back(std::make_pair(t + add, u + add));
		}
	}

	return ret;
}

double calculateIsingEnergy(const std::vector<double>& h, const std::map<std::pair<int, int>, double>& J, const std::vector<int>& s)
{
	double energy = 0.0;

	for (std::map<std::pair<int, int>, double>::const_iterator it = J.begin(), end = J.end(); it != end; ++it)
		energy += s[it->first.first] * it->second * s[it->first.second];

	int sz = std::min(static_cast<int>(h.size()), static_cast<int>(s.size()));
	for (int i = 0; i < sz; ++i)
		energy += h[i] * s[i];

	return energy;
}

class DummyObjectiveFunction : public blackbox::BlackBoxObjectiveFunction
{
public:
	DummyObjectiveFunction(blackbox::LocalInteractionPtr localInteractionPtr) : BlackBoxObjectiveFunction(localInteractionPtr) {}
	
	virtual ~DummyObjectiveFunction() {}

private:
	virtual std::vector<double> computeObjectValueImpl(const std::vector<std::vector<int> >& states) const
	{
		int numStates = static_cast<int>(states.size());
		int stateLen = static_cast<int>(states[0].size());
		std::vector<double> ret;
		for (int i = 0; i < numStates; ++i)
		{
			double d1 = std::accumulate(states[i].begin(), states[i].begin() + stateLen / 2, 0);
			double d2 = std::accumulate(states[i].begin() + stateLen / 2, states[i].end(), 0);
			ret.push_back(d1 - d2);
		}

		return ret;
	}
};

class DummyObjectiveFunctionReturnValueWrongSize : public blackbox::BlackBoxObjectiveFunction
{
public:
	DummyObjectiveFunctionReturnValueWrongSize(blackbox::LocalInteractionPtr localInteractionPtr) : BlackBoxObjectiveFunction(localInteractionPtr) {}
	
	virtual ~DummyObjectiveFunctionReturnValueWrongSize() {}

private:
	virtual std::vector<double> computeObjectValueImpl(const std::vector<std::vector<int> >& states) const
	{
		int numStates = static_cast<int>(states.size());
		int stateLen = static_cast<int>(states[0].size());
		std::vector<double> ret;
		for (int i = 0; i < numStates; ++i)
		{
			double d1 = std::accumulate(states[i].begin(), states[i].begin() + stateLen / 2, 0);
			double d2 = std::accumulate(states[i].begin() + stateLen / 2, states[i].end(), 0);
			ret.push_back(d1 - d2);
		}

		// make ret's size incorrect
		ret.push_back(1.0);

		return ret;
	}
};

class LPSolverReturnValueWrongSize : public blackbox::LPSolver
{
public:
	LPSolverReturnValueWrongSize(blackbox::LocalInteractionPtr localInteractionPtr) : LPSolver(localInteractionPtr) {}
	
	virtual ~LPSolverReturnValueWrongSize() {}

private:

	virtual std::vector<double> solveImpl(const std::vector<double>& f, const std::vector<std::vector<double> >& Aineq, const std::vector<double>& bineq, 
		                                  const std::vector<std::vector<double> >& Aeq, const std::vector<double>& beq, const std::vector<double>& lb, const std::vector<double>& ub) const
	{
		std::vector<double> ret(f.size(), 1.0);
		
		// make ret's size incorrect
		ret.push_back(1.0);

		return ret;
	}
};

class LocalInteractionTest : public blackbox::LocalInteraction
{
private:
	virtual void displayOutputImpl(const std::string& str) const
	{
		std::cout << str;
	}
	
	virtual bool cancelledImpl() const
	{
		return false;
	}
};

class IsingSolverTest : public blackbox::IsingSolver
{
public:
	IsingSolverTest()
	{
		for (int i = 0; i < 128; ++i)
			qubits_.push_back(i);

		couplers_ = getChimeraAdjacency(4, 4, 4);
	}

private:
	virtual void solveIsingImpl(const std::vector<double>& h, const std::map<std::pair<int, int>, double>& J, std::vector<std::vector<int> >& solutions, std::vector<double>& energies, std::vector<int>& numOccurrences) const
	{
		for (int i = 0; i < 100; ++i)
		{
			std::vector<int> tmp;
			for (int j = 0; j < 128; ++j)
				tmp.push_back(2 * std::rand() % 2 - 1);

			solutions.push_back(tmp);
			energies.push_back(calculateIsingEnergy(h, J, tmp));
		}
	}
};

}  // anonymous namespace

TEST(QSageTest, InvalidSolverPtr)
{
	blackbox::IsingSolverPtr isingSolverPtrNULL;
	blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionTest());
	blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new DummyObjectiveFunction(localInteractionPtr));
	int numVars = 20;
	blackbox::BlackBoxExternalParams blackBoxExternalParams;
	blackBoxExternalParams.localInteractionPtr = localInteractionPtr;
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtrNULL, blackBoxExternalParams), blackbox::BlackBoxException);
}

TEST(QSageTest, NumVarsIsZero)
{
	blackbox::IsingSolverPtr isingSolverPtr(new IsingSolverTest());
	blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionTest());
	blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new DummyObjectiveFunction(localInteractionPtr));
	int numVars = 0;
	blackbox::BlackBoxExternalParams blackBoxExternalParams;
	blackBoxExternalParams.localInteractionPtr = localInteractionPtr;
	blackbox::BlackBoxResult result = blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams);
	EXPECT_EQ(0, result.bestSolution.size());	
}

TEST(QSageTest, InvalidParameters)
{
	blackbox::IsingSolverPtr isingSolverPtr(new IsingSolverTest());
	blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionTest());
	blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new DummyObjectiveFunction(localInteractionPtr));
	int numVars = 20;
	blackbox::BlackBoxExternalParams defaultBlackBoxExternalParams;
	defaultBlackBoxExternalParams.localInteractionPtr = localInteractionPtr;

	blackbox::BlackBoxExternalParams blackBoxExternalParams = defaultBlackBoxExternalParams;

	// numVars must be >= 0
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, -1, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);

	// init_solution contains integer that is not -1/1 when the isingQubo is "ising"
	blackBoxExternalParams.initialSolution.resize(numVars, 1);
	blackBoxExternalParams.initialSolution[0] = 0;
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
	blackBoxExternalParams = defaultBlackBoxExternalParams;

	// init_solution's length is not the same as number of variables
	blackBoxExternalParams.initialSolution.resize(numVars + 1, 1);
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
	blackBoxExternalParams = defaultBlackBoxExternalParams;

	// init_solution contains integer that is not 0/1 when the isingQubo is "qubo"
	blackBoxExternalParams.isingQubo = blackbox::QUBO;
	blackBoxExternalParams.initialSolution.resize(numVars, 1);
	blackBoxExternalParams.initialSolution[0] = -1;
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
	blackBoxExternalParams = defaultBlackBoxExternalParams;

	// init_solution's length is not the same as number of variables
	blackBoxExternalParams.isingQubo = blackbox::QUBO;
	blackBoxExternalParams.initialSolution.resize(numVars + 1, 1);
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
	blackBoxExternalParams = defaultBlackBoxExternalParams;	

	// max_num_state_evaluations must be >= 0
	blackBoxExternalParams.maxNumStateEvaluations = -1;
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
	blackBoxExternalParams = defaultBlackBoxExternalParams;

	// timeout must be >= 0
	blackBoxExternalParams.timeout = -1.0;
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
	blackBoxExternalParams = defaultBlackBoxExternalParams;

	// verbose must be [0, 2]
	blackBoxExternalParams.verbose = -1;
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
	blackBoxExternalParams = defaultBlackBoxExternalParams;
	blackBoxExternalParams.verbose = 3;
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
	blackBoxExternalParams = defaultBlackBoxExternalParams;
}

TEST(QSageTest, ObjectiveFunctionReturnValueWrongSize)
{
	blackbox::IsingSolverPtr isingSolverPtr(new IsingSolverTest());
	blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionTest());
	blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new DummyObjectiveFunctionReturnValueWrongSize(localInteractionPtr));
	int numVars = 20;
	blackbox::BlackBoxExternalParams blackBoxExternalParams;
	blackBoxExternalParams.localInteractionPtr = localInteractionPtr;
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
}

TEST(QSageTest, LPSolverReturnValueWrongSize)
{
	blackbox::IsingSolverPtr isingSolverPtr(new IsingSolverTest());
	blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionTest());
	blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new DummyObjectiveFunction(localInteractionPtr));
	int numVars = 20;
	blackbox::BlackBoxExternalParams blackBoxExternalParams;
	blackBoxExternalParams.localInteractionPtr = localInteractionPtr;
	blackBoxExternalParams.lpSolverPtr.reset(new LPSolverReturnValueWrongSize(localInteractionPtr));
	EXPECT_THROW(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams), blackbox::BlackBoxException);
}

/*
TEST(QSageTest, CheckResult)
{
	blackbox::IsingSolverPtr isingSolverPtr(new IsingSolverTest());
	blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionTest());
	blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new DummyObjectiveFunction(localInteractionPtr));
	int numVars = 20;
	blackbox::BlackBoxExternalParams blackBoxExternalParams;
	blackBoxExternalParams.localInteractionPtr = localInteractionPtr;
	blackBoxExternalParams.timeout = 1.0;
	blackBoxExternalParams.verbose = 2;
	//blackBoxExternalParams.lpSolverPtr.reset(new LPSolverReturnValueWrongSize(localInteractionPtr));
	//blackBoxExternalParams.isingQubo = blackbox::IsingQubo::ISING;
	blackbox::BlackBoxResult result = blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, numVars, isingSolverPtr, blackBoxExternalParams);
	std::cout << result.bestEnergy << std::endl;
	for (int i = 0; i < result.bestSolution.size(); ++i)
		std::cout << result.bestSolution[i] << " ";
	std::cout << std::endl;
}
*/
