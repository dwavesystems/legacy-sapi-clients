//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef BLACKBOX_HPP_INCLUDED
#define BLACKBOX_HPP_INCLUDED

#include <vector>
#include <utility>
#include <string>
#include <memory>
#include <map>

namespace blackbox
{

class LocalInteraction
{
public:
	virtual ~LocalInteraction() {}
	void displayOutput(const std::string& msg) const { displayOutputImpl(msg); }
	bool cancelled() const { return cancelledImpl(); }

private:
	virtual void displayOutputImpl(const std::string&) const = 0;
	virtual bool cancelledImpl() const = 0;
};

typedef std::shared_ptr<LocalInteraction> LocalInteractionPtr;

class BlackBoxException
{
public:
	BlackBoxException(const std::string& m = "blackbox exception") : message(m) {}
	const std::string& what() const {return message;}
private:
	std::string message;
};

class ProblemCancelledException : public BlackBoxException
{
public:
	ProblemCancelledException(const std::string& m = "problem cancelled exception") : BlackBoxException(m) {}
	const std::string& what() const {return message;}
private:
	std::string message;
};

class LPSolver
{
public:
	LPSolver(LocalInteractionPtr localInteractionPtr) : localInteractionPtr_(localInteractionPtr)
	{
		if (!localInteractionPtr_ || !localInteractionPtr_.get())
			throw BlackBoxException("lp solver is invalid");
	}

	std::vector<double> solve(const std::vector<double>& f, const std::vector<std::vector<double> >& Aineq, const std::vector<double>& bineq, 
		                      const std::vector<std::vector<double> >& Aeq, const std::vector<double>& beq, const std::vector<double>& lb, const std::vector<double>& ub) const
	{
		if (localInteractionPtr_->cancelled())
			throw ProblemCancelledException();

		std::vector<double> ret = solveImpl(f, Aineq, bineq, Aeq, beq, lb, ub);
		if (ret.size() != f.size())
			throw BlackBoxException("lp solver's answer size is not correct.");

		return ret;
	}

	virtual ~LPSolver() {}

private:

	virtual std::vector<double> solveImpl(const std::vector<double>& f, const std::vector<std::vector<double> >& Aineq, const std::vector<double>& bineq, 
		                                  const std::vector<std::vector<double> >& Aeq, const std::vector<double>& beq, const std::vector<double>& lb, const std::vector<double>& ub) const = 0;

	LocalInteractionPtr localInteractionPtr_;
};

typedef std::shared_ptr<LPSolver> LPSolverPtr;

typedef enum IsingQubo
{
	ISING = 0,
	QUBO
} IsingQubo;

struct BlackBoxExternalParams
{
	BlackBoxExternalParams();

	bool drawSample;
	double exitThresholdValue;  // any number
	std::vector<int> initialSolution;
	IsingQubo isingQubo;
	// actually not controled by user, not initialized here, but initialized in Python, MATLAB, C wrappers level
	LocalInteractionPtr localInteractionPtr;
	LPSolverPtr lpSolverPtr;	
	long long int maxNumStateEvaluations;	
	unsigned int randomSeed;
	double timeout;  // >= 0
	int verbose;  // 0, 1, 2
};

struct BlackBoxInfo
{
	BlackBoxInfo() : numStateEvaluations(0), numObjFuncCalls(0), numSolverCalls(0), numLPSolverCalls(0), stateEvaluationsTime(0.0), solverCallsTime(0.0), lpSolverCallsTime(0.0), totalTime(0.0) {}

	long long int numStateEvaluations;
	long long int numObjFuncCalls;
	long long int numSolverCalls;
	long long int numLPSolverCalls;
	double stateEvaluationsTime;
	double solverCallsTime;
	double lpSolverCallsTime;
	double totalTime;
	std::vector<std::pair<std::vector<long long int>, std::pair<double, double> > > progressTable;
};

struct BlackBoxResult
{
	BlackBoxResult() : bestEnergy(0.0) {}

	std::vector<int> bestSolution;
	double bestEnergy;

	BlackBoxInfo info;	
};

class BlackBoxObjectiveFunction
{
public:
	BlackBoxObjectiveFunction(LocalInteractionPtr localInteractionPtr) : localInteractionPtr_(localInteractionPtr)
	{
		if (!localInteractionPtr_ || !localInteractionPtr_.get())
			throw BlackBoxException("localInteractionPtr is NULL");
	}

	std::vector<double> computeObjectValue(const std::vector<std::vector<int> >& states) const
	{
		if (localInteractionPtr_->cancelled())
			throw ProblemCancelledException();

		std::vector<double> ret = computeObjectValueImpl(states);
		if (ret.size() != states.size())
			throw BlackBoxException("objective function's answer size is not correct.");

		return ret;
	}

	virtual ~BlackBoxObjectiveFunction() {}

private:
	virtual std::vector<double> computeObjectValueImpl(const std::vector<std::vector<int> >& states) const = 0;

	LocalInteractionPtr localInteractionPtr_;
};

typedef std::shared_ptr<BlackBoxObjectiveFunction> BlackBoxObjectiveFunctionPtr;

class IsingSolver
{
public:
	IsingSolver() : hMin_(-2.0), hMax_(2.0), jMin_(-1.0), jMax_(1.0)
	{

	}

	void solveIsing(const std::vector<double>& h, const std::map<std::pair<int, int>, double>& J, std::vector<std::vector<int> >& solutions, std::vector<double>& energies, std::vector<int>& numOccurrences) const
	{
		solveIsingImpl(h, J, solutions, energies, numOccurrences);
	}

	virtual ~IsingSolver() {}

	std::vector<int> qubits_;
	std::vector<std::pair<int, int> > couplers_;
	double hMin_;
	double hMax_;
	double jMin_;
	double jMax_;

private:
	virtual void solveIsingImpl(const std::vector<double>& h, const std::map<std::pair<int, int>, double>& J, std::vector<std::vector<int> >& solutions, std::vector<double>& energies, std::vector<int>& numOccurrences) const = 0;
};

typedef std::shared_ptr<IsingSolver> IsingSolverPtr;

BlackBoxResult solveBlackBox(BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr, int numVars, IsingSolverPtr isingSolverPtr, BlackBoxExternalParams& blackBoxExternalParams);

} // namespace blackbox

#endif  // BLACKBOX_HPP_INCLUDED
