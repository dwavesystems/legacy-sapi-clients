//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "blackbox.hpp"

#include <cctype>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <set>
#include <deque>
#include <chrono>
#include <sstream>
//#include <random>
#include <boost/random.hpp>

#include <boost/math/special_functions/round.hpp>

#include <coin/ClpSimplex.hpp>

//#include <fstream>

//const bool checkResults = true;
//int compareCnt = 0;

namespace
{

bool isNaN(double a)
{
	return (a != a);
}

struct EnergyAndIndex
{
	double energy;
	int index;
};

bool compareEnergyAndIndex(const EnergyAndIndex& a, const EnergyAndIndex& b)
{
	if (   a.energy < b.energy
        || (a.energy == b.energy && a.index < b.index) )
		return true;
	else
		return false;	
}

bool compareAbs(double a, double b)
{
	return std::abs(a) < std::abs(b);
}

struct RandomNumberGenerator : std::unary_function<int, int>
{
	boost::mt19937& rng_;
	int operator()(int i)
	{
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> > ui(rng_, boost::uniform_int<>(0, i - 1));
		return ui();
	}

	RandomNumberGenerator(boost::mt19937& rng) : rng_(rng) {}
};

std::vector<std::vector<int> > convertVariablesIsingToQubo(const std::vector<std::vector<int> >& isingVariables)
{
	std::vector<std::vector<int> > quboVariables = isingVariables;
	for (int i = 0; i < quboVariables.size(); ++i)
		for (int j = 0; j < quboVariables[i].size(); ++j)
			quboVariables[i][j] = (quboVariables[i][j] + 1) / 2;

	return quboVariables;
}

std::vector<int> convertVariablesIsingToQubo(const std::vector<int>& isingVariables)
{
	std::vector<int> quboVariables = isingVariables;
	for (int i = 0; i < quboVariables.size(); ++i)
		quboVariables[i] = (quboVariables[i] + 1) / 2;

	return quboVariables;
}

std::vector<int> convertVariablesQuboToIsing(const std::vector<int>& quboVariables)
{
	std::vector<int> isingVariables = quboVariables;
	for (int i = 0; i < isingVariables.size(); ++i)
		isingVariables[i] = 2 * isingVariables[i] - 1;

	return isingVariables;
}

// BlackBox external params
const long long int DEFAULT_MAX_NUM_STATE_EVALUATIONS = 50000000;
const double DEFAULT_EXIT_THRESHOLD_VALUE = -std::numeric_limits<double>::infinity();
const int DEFAULT_VERBOSE = 0;
const double DEFAULT_TIMEOUT = 10.0;
const blackbox::IsingQubo DEFAULT_ISINGQUBO = blackbox::ISING;
const bool DEFAULT_DRAW_SAMPLE = true;

// BlackBox internal params
const int DEFAULT_MIN_DIST = 5;
const int DEFAULT_MIN_DIST_COUNTER_THRESHOLD = 10;
const double DEFAULT_ADD_FRACTION = 0.0;
const int DEFAULT_FLAT_COUNTER = 10;
const int DEFAULT_LOW_TEMPERATURE_COUNTER = 0;
const int DEFAULT_HIGH_TEMPERATURE_COUNTER = 0;
const double DEFAULT_BETA_H = 1.0;
const double DEFAULT_BETA_J = 1.0;
const int DEFAULT_TEMPERATURE_COUNTER_THRESHOLD = 5;
const double DEFAULT_TEMPERATURE_SCALE = 1.1;
const int DEFAULT_MAX_RETRY = 10;

struct BlackBoxInternalParams
{
	BlackBoxInternalParams()
	{		
		minDist = DEFAULT_MIN_DIST;
		minDistCounterThreshold = DEFAULT_MIN_DIST_COUNTER_THRESHOLD;
		addFraction = DEFAULT_ADD_FRACTION;
		flatCounter = DEFAULT_FLAT_COUNTER;
		lowTemperatureCounter = DEFAULT_LOW_TEMPERATURE_COUNTER;
		highTemperatureCounter = DEFAULT_HIGH_TEMPERATURE_COUNTER;
		betaH = DEFAULT_BETA_H;
		betaJ = DEFAULT_BETA_J;
		temperatureCounterThreshold = DEFAULT_TEMPERATURE_COUNTER_THRESHOLD;
		temperatureScale = DEFAULT_TEMPERATURE_SCALE;
		maxRetry = DEFAULT_MAX_RETRY;
	}

	int iter;
	bool newPhase;
	int minDist;
	int minDistCounter;
	int minDistCounterThreshold;
	double addFraction;
	int flatCounter;
	int tabuTenure;
	std::vector<std::vector<int> > emb;
	std::vector<int> fmInd;
	std::vector<int> groupRep;
	std::vector<std::vector<double> > Aeq;
	std::vector<double> beq;
	std::vector<int> order;
	std::vector<int> antiOrder;
	std::vector<int> subset;
	int lowTemperatureCounter;
	int highTemperatureCounter;
	double betaH;
	double betaJ;
	int temperatureCounterThreshold;
	double temperatureScale;
	int maxRetry;
	int numSamples;
	std::deque<int> lastSampleNums;
};

// default LPSolver for BlackBoxSolver
class CLPSolver : public blackbox::LPSolver
{
public:
	CLPSolver(blackbox::LocalInteractionPtr localInteractionPtr) : LPSolver(localInteractionPtr) {}

	virtual ~CLPSolver() {}

private:
	
	virtual std::vector<double> solveImpl(const std::vector<double>& f, const std::vector<std::vector<double> >& Aineq, const std::vector<double>& bineq, 
		                                  const std::vector<std::vector<double> >& Aeq, const std::vector<double>& beq, const std::vector<double>& lb, const std::vector<double>& ub) const
	{
		/*
		std::ofstream fout("f.txt");
		fout<<"CLPSolver::solve............................1\n";
		fout.flush();
		fout<<"f.size(): "<<f.size()<<std::endl;fout.flush();
		if (!Aineq.empty())
		{
			fout<<"Aineq's size: "<<Aineq.size()<<" "<<Aineq[0].size()<<std::endl;
			fout.flush();
		}
		fout<<"bineq's size: "<<bineq.size()<<std::endl;fout.flush();
		if (!Aeq.empty())
		{
			fout<<"Aeq's size: "<<Aeq.size()<<" "<<Aeq[0].size()<<std::endl;
			fout.flush();
		}
		fout<<"beq's size: "<<beq.size()<<std::endl;fout.flush();
		fout<<"lb's size: "<<lb.size()<<std::endl;fout.flush();
		fout<<"ub's size: "<<ub.size()<<std::endl;fout.flush();

		std::cout<<"f.size(): "<<f.size()<<std::endl;
		std::cout<<"Aineq's size: "<<Aineq.size()<<" "<<Aineq[0].size()<<std::endl;
		std::cout<<"bineq's size: "<<bineq.size()<<std::endl;
		std::cout<<"Aeq's size: "<<Aeq.size()<<" "<<Aeq[0].size()<<std::endl;
		std::cout<<"beq's size: "<<beq.size()<<std::endl;
		std::cout<<"lb's size: "<<lb.size()<<std::endl;
		std::cout<<"ub's size: "<<ub.size()<<std::endl;
		*/

		std::vector<int> rowIndices;
		std::vector<int> colIndices;
		std::vector<double> elements;
		CoinBigIndex numels = 0;
		for (int i = 0; i < Aineq.size(); ++i)
			for (int j = 0; j < Aineq[i].size(); ++j)
			{
				if (Aineq[i][j] != 0.0)
				{
					rowIndices.push_back(i);
					colIndices.push_back(j);
					elements.push_back(Aineq[i][j]);
					++numels;
				}
			}

		int AineqRowSize = static_cast<int>(Aineq.size());

		for (int i = 0; i < Aeq.size(); ++i)
			for (int j = 0; j < Aeq[i].size(); ++j)
			{
				if (Aeq[i][j] != 0.0)
				{
					rowIndices.push_back(i + AineqRowSize);
					colIndices.push_back(j);
					elements.push_back(Aeq[i][j]);
					++numels;
				}
			}

		const int* rowIndicesPtr = NULL;
		if (!rowIndices.empty())
			rowIndicesPtr = &rowIndices[0];
		const int* colIndicesPtr = NULL;
		if (!colIndices.empty())
			colIndicesPtr = &colIndices[0];
		const double* elementsPtr = NULL;
		if (!elements.empty())
			elementsPtr = &elements[0];
	
		CoinPackedMatrix matrix(false, rowIndicesPtr, colIndicesPtr, elementsPtr, numels);

		std::vector<double> rowlb(AineqRowSize, -std::numeric_limits<double>::infinity());
		rowlb.insert(rowlb.end(), beq.begin(), beq.end());

		std::vector<double> rowub = bineq;
		rowub.insert(rowub.end(), beq.begin(), beq.end());

		const std::vector<double>& collb = lb;
		const std::vector<double>& colub = ub;

		const std::vector<double>& obj = f;

		std::shared_ptr<ClpSimplex> model(new ClpSimplex());
	
		const double* collbPtr = NULL;
		if (!collb.empty())
			collbPtr = &collb[0];
		const double* colubPtr = NULL;
		if (!colub.empty())
			colubPtr = &colub[0];
		const double* objPtr = NULL;
		if (!obj.empty())
			objPtr = &obj[0];
		const double* rowlbPtr = NULL;
		if (!rowlb.empty())
			rowlbPtr = &rowlb[0];
		const double* rowubPtr = NULL;
		if (!rowub.empty())
			rowubPtr = &rowub[0];

		model->loadProblem(matrix, collbPtr, colubPtr, objPtr, rowlbPtr, rowubPtr);

		model->setLogLevel(0);

		model->dual();

		//fout << model.sumDualInfeasibilities() << " " << model.numberDualInfeasibilities() << " " << model.currentDualTolerance() << " " << model.largestDualError() << " " << model.maximumIterations() << " " << model.maximumSeconds() << std::endl;
		//fout.flush();

		const double* solution = model->getColSolution();
		//outputIntermediateResults("model's objective value = %f\n", model.getObjValue());
		std::vector<double> ret(f.size());
		for (int i = 0; i < f.size(); ++i)
			ret[i] = solution[i];

		//fout.close();

		return ret;
	}
};

class BlackBoxSolver
{
public:
	BlackBoxSolver(blackbox::IsingSolverPtr isingSolverPtr) : isingSolverPtr_(isingSolverPtr)
	{
		if (!isingSolverPtr_ || !isingSolverPtr_.get())
			throw blackbox::BlackBoxException();

		qubits_ = isingSolverPtr_->qubits_;
		couplers_ = isingSolverPtr_->couplers_;

		hMin_ = isingSolverPtr_->hMin_;
		hMax_ = isingSolverPtr_->hMax_;
		jMin_ = isingSolverPtr_->jMin_;
		jMax_ = isingSolverPtr_->jMax_;

		// valid the above solver properties
		if (qubits_.empty() || couplers_.empty())
			throw blackbox::BlackBoxException("solver's qubits and couplers must be non-empty");

		std::sort(qubits_.begin(), qubits_.end());
		std::set<int> uniqueQubits(qubits_.begin(), qubits_.end());
		if (uniqueQubits.size() != qubits_.size())
			throw blackbox::BlackBoxException("solver's qubits contain duplicates");

		numQubits_ = qubits_.back() + 1;

		for (int i = 0; i < couplers_.size(); ++i)
		{
			if (couplers_[i].first > couplers_[i].second)
				std::swap(couplers_[i].first, couplers_[i].second);
			else if (couplers_[i].first == couplers_[i].second)
				throw blackbox::BlackBoxException("solver's couplers contain invalid coupler");

			if (   uniqueQubits.find(couplers_[i].first) == uniqueQubits.end()
				|| uniqueQubits.find(couplers_[i].second) == uniqueQubits.end() )
				throw blackbox::BlackBoxException("solver's couplers contain invalid coupler");
		}

		std::set<std::pair<int, int> > uniqueCouplers(couplers_.begin(), couplers_.end());
		if (uniqueCouplers.size() != couplers_.size())
			throw blackbox::BlackBoxException("solver's couplers contain duplicates");

		if (hMin_ > hMax_)
			throw blackbox::BlackBoxException("h_min must be <= h_max");

		if (jMin_ > jMax_)
			throw blackbox::BlackBoxException("j_min must be <= j_max");

		numActiveQubits_ = static_cast<int>(qubits_.size());

		std::map<int, int> qubitsIndexMap;
	
		for (int i = 0; i < qubits_.size(); ++i)
			qubitsIndexMap.insert(std::make_pair(qubits_[i], i));
	
		for (int i = 0; i < numQubits_; ++i)
		{
			if (qubitsIndexMap.find(i) == qubitsIndexMap.end())
				undefinedQubits_.push_back(i);
		}

		for (int i = 0; i < couplers_.size(); ++i)
		{
			iIndex_.push_back(couplers_[i].first);
			jIndex_.push_back(couplers_[i].second);
		}

		for (int i = 0; i < couplers_.size(); ++i)
		{
			subiIndex_.push_back(qubitsIndexMap[couplers_[i].first]);
			subjIndex_.push_back(qubitsIndexMap[couplers_[i].second]);
		}
	}

	virtual blackbox::BlackBoxResult solve(blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr, int numVars, blackbox::BlackBoxExternalParams& blackBoxExternalParams) const
	{
		std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

		// check parameters
		if (!blackBoxObjectiveFunctionPtr)
			throw blackbox::BlackBoxException("objective function pointer is NULL");

		if (!(numVars >= 0))
			throw blackbox::BlackBoxException("num_vars must be a an integer >= 0");

		if (isNaN(blackBoxExternalParams.exitThresholdValue))
			throw blackbox::BlackBoxException("exit_threshold_value is NaN");

		if (!blackBoxExternalParams.initialSolution.empty() && blackBoxExternalParams.initialSolution.size() != numVars)
			throw blackbox::BlackBoxException("init_solution parameter must have the same length as number of variables");

		for (int i = 0; i < blackBoxExternalParams.initialSolution.size(); ++i)
		{
			if (blackBoxExternalParams.isingQubo == blackbox::ISING && blackBoxExternalParams.initialSolution[i] != 1 && blackBoxExternalParams.initialSolution[i] != -1)
				throw blackbox::BlackBoxException("init_solution parameter must only contain -1/1");

			if (blackBoxExternalParams.isingQubo == blackbox::QUBO && blackBoxExternalParams.initialSolution[i] != 1 && blackBoxExternalParams.initialSolution[i] != 0)
				throw blackbox::BlackBoxException("init_solution parameter must only contain 0/1");
		}

		if (blackBoxExternalParams.isingQubo == blackbox::QUBO)
			blackBoxExternalParams.initialSolution = convertVariablesQuboToIsing(blackBoxExternalParams.initialSolution);

		if (!blackBoxExternalParams.localInteractionPtr)
			throw blackbox::BlackBoxException("localInteractionPtr parameter is NULL");
		
		if (!blackBoxExternalParams.lpSolverPtr)
			blackBoxExternalParams.lpSolverPtr.reset(new CLPSolver(blackBoxExternalParams.localInteractionPtr));

		if (!(blackBoxExternalParams.maxNumStateEvaluations >= 0))
			throw blackbox::BlackBoxException("max_num_state_evaluations parameter must be an integer >= 0");

		if (isNaN(blackBoxExternalParams.timeout))
			throw blackbox::BlackBoxException("timeout parameter is NaN");

		if (!(blackBoxExternalParams.timeout >= 0.0))
			throw blackbox::BlackBoxException("timeout parameter must be a number >= 0.0");

		if (!(blackBoxExternalParams.verbose >= 0 && blackBoxExternalParams.verbose <= 2))
			throw blackbox::BlackBoxException("verbose parameter must be an integer [0, 2]");		

		std::chrono::high_resolution_clock::time_point tmpStartTime;

		blackbox::BlackBoxResult blackBoxResult;

		if (numVars == 0)
			return blackBoxResult;

		// for numVars <= 10, do brute-force search
		if (numVars <= 10)
		{
			int totalNumSolutions = (1 << numVars);
			std::vector<std::vector<int> > solutions(totalNumSolutions, std::vector<int>(numVars));
			for (int i = 0; i < totalNumSolutions; ++i)
			{
				for (int j = 0; j < numVars; ++j)
				{
					if (i & (1 << j))
						solutions[i][j] = 1;
					else
						solutions[i][j] = -1;
				}
			}
		
			tmpStartTime = std::chrono::high_resolution_clock::now();
			std::vector<double> energies;
			if (blackBoxExternalParams.isingQubo == blackbox::ISING)
				energies = blackBoxObjectiveFunctionPtr->computeObjectValue(solutions);
			else
				energies = blackBoxObjectiveFunctionPtr->computeObjectValue(convertVariablesIsingToQubo(solutions));

			blackBoxResult.info.stateEvaluationsTime += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tmpStartTime).count()) / 1000.0;

			std::vector<double>::const_iterator minEnergyIterator = std::min_element(energies.begin(), energies.end());
			int minEnergyIndex = static_cast<int>(minEnergyIterator - energies.begin());
			if (blackBoxExternalParams.isingQubo == blackbox::ISING)
				blackBoxResult.bestSolution = solutions[minEnergyIndex];
			else
				blackBoxResult.bestSolution = convertVariablesIsingToQubo(solutions[minEnergyIndex]);
			blackBoxResult.bestEnergy = *minEnergyIterator;
			blackBoxResult.info.numStateEvaluations = (1LL << numVars);
			blackBoxResult.info.numObjFuncCalls = 1;
			std::vector<long long int> ptPart(4);
			ptPart[0] = blackBoxResult.info.numStateEvaluations;
			ptPart[1] = blackBoxResult.info.numObjFuncCalls;
			ptPart[2] = blackBoxResult.info.numSolverCalls;
			ptPart[3] = blackBoxResult.info.numLPSolverCalls;
			blackBoxResult.info.progressTable.push_back(std::make_pair(ptPart, std::make_pair(static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0, blackBoxResult.bestEnergy)));

			blackBoxResult.info.totalTime = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0;
			return blackBoxResult;
		}

		BlackBoxInternalParams blackBoxInternalParams;
		blackBoxInternalParams.iter = 0;
		blackBoxInternalParams.minDistCounter = 0;

		boost::mt19937 rng(blackBoxExternalParams.randomSeed);
	
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> > uiZeroOne(rng, boost::uniform_int<>(0, 1));
	
		int tabuTenureLowerBound = static_cast<int>(boost::math::round<double>(static_cast<double>(numVars) / 4));
		int tabuTenureUpperBound = static_cast<int>(boost::math::round<double>(static_cast<double>(numVars) / 2));
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> > uiForTabuTenure(rng, boost::uniform_int<>(tabuTenureLowerBound, tabuTenureUpperBound));


		int tabuTenure = uiForTabuTenure(); // should use
		//int tabuTenure = numVars / 2; // remove randomness
		blackBoxInternalParams.tabuTenure = tabuTenure;

		if (blackBoxExternalParams.drawSample)
			generateEmbedding(numVars, blackBoxInternalParams, rng);

		/*
		// ######################################### start of debugging #########################################
		// check blackBoxInternalParams.emb
		if (checkResults)
		{
			std::vector<std::vector<int> > expectedEmb;

			char s[100000];
			std::ifstream fin("emb.txt");
			while (fin.getline(s, 100000))
			{
				std::vector<int> tmpEmb;
				std::stringstream ss(s);
				int i;
				while (ss>>i)
					tmpEmb.push_back(i);
				expectedEmb.push_back(tmpEmb);
			}

			bool flag = true;

			if (expectedEmb.size() != blackBoxInternalParams.emb.size())
			{
				flag = false;
				outputIntermediateResults("emb size is wrong !!!!!!!!!!!!!\n");
			}

			for (int i = 0; i < blackBoxInternalParams.emb.size(); ++i)
			{
				if (expectedEmb[i].size() != blackBoxInternalParams.emb[i].size())
				{
					flag = false;
					outputIntermediateResults("emb size is wrong !!!!!!!!!!!!!\n");
					break;
				}

				for (int j = 0; j < blackBoxInternalParams.emb[i].size(); ++j)
				{
					if (expectedEmb[i][j] != blackBoxInternalParams.emb[i][j] + 1)
					{
						flag = false;
						outputIntermediateResults("emb is wrong !!!!!!!!!!!!!\n");
						break;
					}
				}

				if (!flag)
					break;
			}

			fin.close();

			if (flag)
				outputIntermediateResults("emb is correct !\n");
		}

		// check blackBoxInternalParams.fmInd
		if (checkResults)
		{
			std::vector<int> expectedFmInd;
			std::ifstream fin("fmInd.txt");
			int i;
			while (fin>>i)
				expectedFmInd.push_back(i);

			bool flag = true;

			if (expectedFmInd.size() != blackBoxInternalParams.fmInd.size())
			{
				flag = false;
				outputIntermediateResults("fmInd's size is wrong !!!\n");
			}

			for (int i = 0; i < blackBoxInternalParams.fmInd.size(); ++i)
			{
				if (expectedFmInd[i] != blackBoxInternalParams.fmInd[i] + 1)
				{
					flag = false;
					outputIntermediateResults("fmInd is wrong !!!!!!!!!!!!!\n");
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("fmInd is correct !\n");
		}

		// check blackBoxInternalParams.groupRep
		if (checkResults)
		{
			std::vector<int> expectedGroupRep;
			std::ifstream fin("groupRep.txt");
			int i;
			while (fin>>i)
				expectedGroupRep.push_back(i);

			bool flag = true;

			if (expectedGroupRep.size() != blackBoxInternalParams.groupRep.size())
			{
				flag = false;
				outputIntermediateResults("groupRep's size is wrong !!!\n");
			}

			for (int i = 0; i < blackBoxInternalParams.groupRep.size(); ++i)
			{
				if (expectedGroupRep[i] != blackBoxInternalParams.groupRep[i] + 1)
				{
					flag = false;
					outputIntermediateResults("groupRep is wrong !!!!!!!!!!!!!\n");
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("groupRep is correct !\n");
		}
		// ######################################### end of debugging #########################################
		*/


		blackBoxResult.bestSolution = blackBoxExternalParams.initialSolution;

		if (blackBoxResult.bestSolution.empty())
		{		
			blackBoxResult.bestSolution.resize(numVars);
			for (int i = 0; i < numVars; ++i)
				blackBoxResult.bestSolution[i] = 2 * uiZeroOne() - 1;
		}

		tmpStartTime = std::chrono::high_resolution_clock::now();
		if (blackBoxExternalParams.isingQubo == blackbox::ISING)
			blackBoxResult.bestEnergy = (blackBoxObjectiveFunctionPtr->computeObjectValue(std::vector<std::vector<int> >(1, blackBoxResult.bestSolution)))[0];
		else
			blackBoxResult.bestEnergy = (blackBoxObjectiveFunctionPtr->computeObjectValue(std::vector<std::vector<int> >(1, convertVariablesIsingToQubo(blackBoxResult.bestSolution))))[0];
		blackBoxResult.info.stateEvaluationsTime += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tmpStartTime).count()) / 1000.0;

		std::vector<int> tmpBestSolution = blackBoxResult.bestSolution;
		double tmpBestEnergy = blackBoxResult.bestEnergy;

		std::list<int> tabuList;

		++blackBoxResult.info.numStateEvaluations;
		++blackBoxResult.info.numObjFuncCalls;

		std::vector<long long int> ptPart(4);
		ptPart[0] = blackBoxResult.info.numStateEvaluations;
		ptPart[1] = blackBoxResult.info.numObjFuncCalls;
		ptPart[2] = blackBoxResult.info.numSolverCalls;
		ptPart[3] = blackBoxResult.info.numLPSolverCalls;
		blackBoxResult.info.progressTable.push_back(std::make_pair(ptPart, std::make_pair(static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0, blackBoxResult.bestEnergy)));

		std::vector<int> phaseStartSolution = blackBoxResult.bestSolution;
		std::vector<int> phaseCurrentSolution;
		std::vector<int> phaseBestSolution;

		while (blackBoxResult.info.numStateEvaluations < blackBoxExternalParams.maxNumStateEvaluations)
		{
			blackBoxInternalParams.newPhase = true;

			double phaseBestEnergy = std::numeric_limits<double>::infinity();
			int isInterrupted = 0;
			int runTabuReturnValue;

			try
			{
				runTabuReturnValue = runTabu(phaseStartSolution, blackBoxObjectiveFunctionPtr, blackBoxResult.bestEnergy, blackBoxExternalParams, blackBoxInternalParams,
											 tabuList, tabuTenure, phaseCurrentSolution, phaseBestSolution, blackBoxResult.info, rng, startTime);
			}
			catch (const blackbox::ProblemCancelledException&)
			{
				isInterrupted = 1;
			}

			while (true)
			{
				try
				{
					if (blackBoxExternalParams.isingQubo == blackbox::ISING)
						phaseBestEnergy = (blackBoxObjectiveFunctionPtr->computeObjectValue(std::vector<std::vector<int> >(1, phaseBestSolution)))[0];
					else
						phaseBestEnergy = (blackBoxObjectiveFunctionPtr->computeObjectValue(std::vector<std::vector<int> >(1, convertVariablesIsingToQubo(phaseBestSolution))))[0];
					break;
				}
				catch (const blackbox::ProblemCancelledException&)
				{
					isInterrupted = 1;
				}
			}

			if (phaseBestEnergy < blackBoxResult.bestEnergy)
			{
				blackBoxResult.bestEnergy = phaseBestEnergy;
				blackBoxResult.bestSolution = phaseBestSolution;
				std::vector<long long int> ptPartTmp(4);
				ptPartTmp[0] = blackBoxResult.info.numStateEvaluations;
				ptPartTmp[1] = blackBoxResult.info.numObjFuncCalls;
				ptPartTmp[2] = blackBoxResult.info.numSolverCalls;
				ptPartTmp[3] = blackBoxResult.info.numLPSolverCalls;
				blackBoxResult.info.progressTable.push_back(std::make_pair(ptPartTmp, std::make_pair(static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0, blackBoxResult.bestEnergy)));
				blackBoxInternalParams.minDistCounter = 0;
			}

			if (isInterrupted) // Ctrl-C
			{
				if (blackBoxExternalParams.verbose >= 1)
					blackBoxExternalParams.localInteractionPtr->displayOutput("\nQSage interrupted by Ctrl-C.\n");

				break;
			}

			if (runTabuReturnValue == 1) // reach timeout
			{
				if (blackBoxExternalParams.verbose >= 1)
				{
					std::stringstream ss;
					ss << "\n" << blackBoxExternalParams.timeout << " seconds timeout has been reached.\n";
					blackBoxExternalParams.localInteractionPtr->displayOutput(ss.str());
				}

				break;
			}

			if (runTabuReturnValue == 2) // reach exit_threshold_value
			{
				if (blackBoxExternalParams.verbose >= 1)
				{
					std::stringstream ss;
					ss << "\nexit_threshold_value " << blackBoxExternalParams.exitThresholdValue << " has been reached.\n";
					blackBoxExternalParams.localInteractionPtr->displayOutput(ss.str());
				}
				break;
			}

			if (runTabuReturnValue == 3) // reach max_num_state_evaluations
			{
				if (blackBoxExternalParams.verbose >= 1)
				{
					std::stringstream ss;
					ss << "\n" << blackBoxExternalParams.maxNumStateEvaluations << " max_num_state_evaluations has been reached.\n";
					blackBoxExternalParams.localInteractionPtr->displayOutput(ss.str());
				}
				break;
			}

			if (phaseBestEnergy < tmpBestEnergy)
			{
				tmpBestEnergy = phaseBestEnergy;
				tmpBestSolution = phaseBestSolution;
			}

			int onesInBestSolution = static_cast<int>(std::count(blackBoxResult.bestSolution.begin(), blackBoxResult.bestSolution.end(), 1));
		
			int diffCounter = 0;
			for (int i = 0; i < numVars; ++i)
			{
				if (phaseCurrentSolution[i] != tmpBestSolution[i])
					++diffCounter;
			}

			if (blackBoxExternalParams.verbose >= 1)
			{
				std::stringstream ss;
				ss << "[num_state_evaluations = " << blackBoxResult.info.numStateEvaluations << ", num_obj_func_calls = " << blackBoxResult.info.numObjFuncCalls << ", num_solver_calls = " << blackBoxResult.info.numSolverCalls << ", num_lp_solver_calls = " << blackBoxResult.info.numLPSolverCalls << "], best_energy = " << blackBoxResult.bestEnergy << ", distance to best_energy = " << diffCounter << "\n";
				blackBoxExternalParams.localInteractionPtr->displayOutput(ss.str());
			}

			if (diffCounter <= blackBoxInternalParams.minDist)
			{
				++blackBoxInternalParams.minDistCounter;
				if (blackBoxInternalParams.minDistCounter > blackBoxInternalParams.minDistCounterThreshold)
				{
					for (int i = 0; i < numVars; ++i) // should use
						phaseCurrentSolution[i] = 2 * uiZeroOne() - 1;
					//phaseCurrentSolution = initialSolution; // remove randomness

					tabuList.clear();
					tabuTenure = uiForTabuTenure(); // should use
					//tabuTenure = numVars / 2; // remove randomness
					blackBoxInternalParams.minDistCounter = 0;
					tmpBestEnergy = std::numeric_limits<double>::infinity();
					if (blackBoxExternalParams.verbose >= 1)
						blackBoxExternalParams.localInteractionPtr->displayOutput("..................Restarting..................\n");
				}
			}

			phaseStartSolution = phaseCurrentSolution;
		}

		blackBoxResult.info.totalTime = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0;
		if (blackBoxExternalParams.isingQubo == blackbox::QUBO)
			blackBoxResult.bestSolution = convertVariablesIsingToQubo(blackBoxResult.bestSolution);
		
		return blackBoxResult;
	}
	
	virtual ~BlackBoxSolver() {}

private:
	// return 0 when normal; 1 when timeout; 2 when reach exit_threshold_value; 3 when reach max_num_state_evaluations
	int runTabu(const std::vector<int>& phaseStartSolution, blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr, double bestEnergy,
		        const blackbox::BlackBoxExternalParams& blackBoxExternalParams, BlackBoxInternalParams& blackBoxInternalParams, std::list<int>& tabuList,
				int tabuTenure, std::vector<int>& phaseCurrentSolution, std::vector<int>& phaseBestSolution, blackbox::BlackBoxInfo& blackBoxInfo,
				boost::mt19937& rng, const std::chrono::high_resolution_clock::time_point& startTime) const
	{
		std::chrono::high_resolution_clock::time_point tmpStartTime;
		phaseCurrentSolution = phaseStartSolution;
		phaseBestSolution = phaseStartSolution;
		int numVars = static_cast<int>(phaseCurrentSolution.size());
		bool downFlag = false;
		int flatCounter = 0;

		RandomNumberGenerator randomNumberGenerator(rng);

		while (true)
		{
			++blackBoxInternalParams.iter;

			std::vector<std::vector<int> > solutions(numVars, phaseCurrentSolution);
			for (int i = 0; i < numVars; ++i)
				solutions[i][i] = -solutions[i][i];

			tmpStartTime = std::chrono::high_resolution_clock::now();
			double currentEnergy;
			if (blackBoxExternalParams.isingQubo == blackbox::ISING)
				currentEnergy = (blackBoxObjectiveFunctionPtr->computeObjectValue(std::vector<std::vector<int> >(1, phaseCurrentSolution)))[0];
			else
				currentEnergy = (blackBoxObjectiveFunctionPtr->computeObjectValue(std::vector<std::vector<int> >(1, convertVariablesIsingToQubo(phaseCurrentSolution))))[0];
			blackBoxInfo.stateEvaluationsTime += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tmpStartTime).count()) / 1000.0;
			++blackBoxInfo.numStateEvaluations;
			++blackBoxInfo.numObjFuncCalls;

			tmpStartTime = std::chrono::high_resolution_clock::now();
			std::vector<double> energies;
			if (blackBoxExternalParams.isingQubo == blackbox::ISING)
				energies = blackBoxObjectiveFunctionPtr->computeObjectValue(solutions);
			else
				energies = blackBoxObjectiveFunctionPtr->computeObjectValue(convertVariablesIsingToQubo(solutions));
			blackBoxInfo.stateEvaluationsTime += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tmpStartTime).count()) / 1000.0;
			blackBoxInfo.numStateEvaluations += numVars;
			++blackBoxInfo.numObjFuncCalls;

			if (blackBoxExternalParams.drawSample)
			{
				std::vector<std::vector<int> > population;
				generatePopulation(blackBoxObjectiveFunctionPtr, phaseCurrentSolution, currentEnergy, solutions, energies,
								   blackBoxExternalParams, blackBoxInternalParams, blackBoxInfo, population, rng);

				tmpStartTime = std::chrono::high_resolution_clock::now();
				std::vector<double> populationEnergies;
				if (blackBoxExternalParams.isingQubo == blackbox::ISING)
					populationEnergies = blackBoxObjectiveFunctionPtr->computeObjectValue(population);
				else
					populationEnergies = blackBoxObjectiveFunctionPtr->computeObjectValue(convertVariablesIsingToQubo(population));
				blackBoxInfo.stateEvaluationsTime += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tmpStartTime).count()) / 1000.0;
				blackBoxInfo.numStateEvaluations += population.size();
				++blackBoxInfo.numObjFuncCalls;

				solutions.insert(solutions.end(), population.begin(), population.end());
				energies.insert(energies.end(), populationEnergies.begin(), populationEnergies.end());
			}

			blackBoxInternalParams.newPhase = false;
		
			std::vector<int> randomShuffledIndices;
			for (int i = 0; i < energies.size(); ++i)
				randomShuffledIndices.push_back(i);
			std::random_shuffle(randomShuffledIndices.begin(), randomShuffledIndices.end(), randomNumberGenerator); // should use

			std::vector<double> randomShuffledEnergies;
			for (int i = 0; i < energies.size(); ++i)
				randomShuffledEnergies.push_back(energies[randomShuffledIndices[i]]);

			std::vector<double>::const_iterator minEnergyIterator = std::min_element(randomShuffledEnergies.begin(), randomShuffledEnergies.end());
			int minEnergyShuffledIndex = static_cast<int>(minEnergyIterator - randomShuffledEnergies.begin());
			int minEnergyIndex = randomShuffledIndices[minEnergyShuffledIndex];
			double minEnergy = *minEnergyIterator;
		
			if (blackBoxExternalParams.verbose >= 2)
			{
				std::stringstream ss;
				ss << "min_energy = " << minEnergy << "\n";
				blackBoxExternalParams.localInteractionPtr->displayOutput(ss.str());
			}


			/*
			// ######################################### start of debugging #########################################
			if (checkResults)
				++compareCnt;
			// ######################################### end of debugging #########################################
			*/


			if (minEnergy < bestEnergy)
			{
				bestEnergy = minEnergy;
				phaseBestSolution = solutions[minEnergyIndex];
				phaseCurrentSolution = phaseBestSolution;
				downFlag = true;
				blackBoxInternalParams.iter = 0;
			}
			else
			{
				std::vector<int> tabuIndices(phaseCurrentSolution.size(), 0);
				for (std::list<int>::const_iterator it = tabuList.begin(), end = tabuList.end(); it != end; ++it)
					tabuIndices[*it] = 1;
				std::vector<int> validIndices;
				for (int i = 0; i < solutions.size(); ++i)
					for (int j = 0; j < numVars; ++j)
					{
						if (   solutions[i][j] != phaseCurrentSolution[j]
							&& tabuIndices[j] == 0)
							{
								validIndices.push_back(i);
								break;
							}
					}

				std::vector<int> randomShuffledValidIndices;
				for (int i = 0; i < validIndices.size(); ++i)
					randomShuffledValidIndices.push_back(i);
				std::random_shuffle(randomShuffledValidIndices.begin(), randomShuffledValidIndices.end(), randomNumberGenerator); // should use

				std::vector<double> randomShuffledValidEnergies;
				for (int i = 0; i < validIndices.size(); ++i)
					randomShuffledValidEnergies.push_back(energies[validIndices[randomShuffledValidIndices[i]]]);

				std::vector<double>::const_iterator tmpMinEnergyIterator = std::min_element(randomShuffledValidEnergies.begin(), randomShuffledValidEnergies.end());
				int tmpMinEnergyShuffledIndex = static_cast<int>(tmpMinEnergyIterator - randomShuffledValidEnergies.begin());
				int tmpMinEnergyIndex = randomShuffledValidIndices[tmpMinEnergyShuffledIndex]; // this is the index for validIndices
				double tmpMinEnergy = *tmpMinEnergyIterator;
			
				if (tmpMinEnergy < currentEnergy)
				{
					downFlag = true;
					flatCounter = 0;
				}
				else if (tmpMinEnergy == currentEnergy)
					++flatCounter;
				else
					flatCounter = 0;

				std::vector<int> addedIndices;
				int tmpMinEnergyValidIndex = validIndices[tmpMinEnergyIndex];
				for (int j = 0; j < numVars; ++j)
				{
					if (   solutions[tmpMinEnergyValidIndex][j] != phaseCurrentSolution[j]
						&& tabuIndices[j] == 0)
							addedIndices.push_back(j);
				}

				int tmpDiffCnt = 0;
				for (int j = 0; j < numVars; ++j)
				{
					if (solutions[tmpMinEnergyValidIndex][j] != phaseCurrentSolution[j])
						++tmpDiffCnt;
				}

				if (blackBoxExternalParams.verbose >= 2)
				{
					std::stringstream ss;
					ss << "move_length = " << tmpDiffCnt << "\n";
					blackBoxExternalParams.localInteractionPtr->displayOutput(ss.str());
				}

				phaseCurrentSolution = solutions[tmpMinEnergyValidIndex];

				std::vector<int> randomShuffledAddedIndices;
				for (int i = 0; i < addedIndices.size(); ++i)
					randomShuffledAddedIndices.push_back(i);
				std::random_shuffle(randomShuffledAddedIndices.begin(), randomShuffledAddedIndices.end(), randomNumberGenerator); // should use

				int numIndicesToAdd = std::max(static_cast<int>(1), static_cast<int>(blackBoxInternalParams.addFraction * randomShuffledAddedIndices.size()));
				for (int i = 0; i < numIndicesToAdd; ++i)
					tabuList.push_back(addedIndices[randomShuffledAddedIndices[i]]);

				if (tabuList.size() > tabuTenure)
				{
					int numIndicesToRemove = static_cast<int>(tabuList.size()) - tabuTenure;
					for (int i = 0; i < numIndicesToRemove; ++i)
						tabuList.pop_front();
				}

				if (   (tmpMinEnergy > currentEnergy && downFlag)
					|| flatCounter > blackBoxInternalParams.flatCounter)
					break;
			}

			if ((static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0) >= blackBoxExternalParams.timeout)
				return 1;

			if (bestEnergy <= blackBoxExternalParams.exitThresholdValue)
				return 2;

			if (blackBoxInfo.numStateEvaluations >= blackBoxExternalParams.maxNumStateEvaluations)
				return 3;
		}

		return 0;
	}
	
	void generatePopulation(blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr, const std::vector<int>& currentSolution, double currentEnergy,
							const std::vector<std::vector<int> >& currentNeighbours, const std::vector<double>& currentNeighbourEnergies,
							const blackbox::BlackBoxExternalParams& blackBoxExternalParams, BlackBoxInternalParams& blackBoxInternalParams, blackbox::BlackBoxInfo& blackBoxInfo,
							std::vector<std::vector<int> >& population, boost::mt19937& rng) const
	{
		if (blackBoxInternalParams.newPhase)
		{
			RandomNumberGenerator randomNumberGenerator(rng);

			int orderSize = std::min(numActiveQubits_, static_cast<int>(currentSolution.size()));
			blackBoxInternalParams.order.clear();
			for (int i = 0; i < orderSize; ++i)
				blackBoxInternalParams.order.push_back(i);
			std::random_shuffle(blackBoxInternalParams.order.begin(), blackBoxInternalParams.order.end(), randomNumberGenerator); // should use

			blackBoxInternalParams.antiOrder.resize(orderSize);
			for (int i = 0; i < orderSize; ++i)
				blackBoxInternalParams.antiOrder[blackBoxInternalParams.order[i]] = i;

			if (numActiveQubits_ < currentSolution.size())
			{
				std::vector<int> tmpPerm;
				for (int i = 0; i < currentSolution.size(); ++i)
					tmpPerm.push_back(i);
				std::random_shuffle(tmpPerm.begin(), tmpPerm.end(), randomNumberGenerator); // should use
				blackBoxInternalParams.subset.assign(tmpPerm.begin(), tmpPerm.begin() + numActiveQubits_);
			}
		}

		int numVars = static_cast<int>(currentSolution.size());

		std::vector<double> h;
		std::map<std::pair<int, int>, double> J;

		if (numVars <= numActiveQubits_)
			generateModel(currentSolution, currentNeighbours, currentNeighbourEnergies, blackBoxExternalParams, blackBoxInternalParams, blackBoxInfo, h, J);
		else
		{
			int subsetSize = static_cast<int>(blackBoxInternalParams.subset.size());
			std::vector<int> currentSolutionSubset(subsetSize);
			for (int i = 0; i < subsetSize; ++i)
				currentSolutionSubset[i] = currentSolution[blackBoxInternalParams.subset[i]];

			std::vector<std::vector<int> > currentNeighboursSubset(subsetSize, std::vector<int>(subsetSize));
			for (int i = 0; i < subsetSize; ++i)
				for (int j = 0; j < subsetSize; ++j)
					currentNeighboursSubset[i][j] = currentNeighbours[blackBoxInternalParams.subset[i]][blackBoxInternalParams.subset[j]];

			std::vector<double> currentNeighbourEnergiesSubset(subsetSize);
			for (int i = 0; i < subsetSize; ++i)
				currentNeighbourEnergiesSubset[i] = currentNeighbourEnergies[blackBoxInternalParams.subset[i]];

			generateModel(currentSolutionSubset, currentNeighboursSubset, currentNeighbourEnergiesSubset, blackBoxExternalParams, blackBoxInternalParams, blackBoxInfo, h, J);
		}

		int retryCounter = 0;

		std::vector<std::vector<int> > solutions;
		std::vector<double> energies;
		std::vector<int> numOccurrences;
	
		std::chrono::high_resolution_clock::time_point tmpStartTime = std::chrono::high_resolution_clock::now();
		while (true)
		{
			try
			{
				isingSolverPtr_->solveIsing(h, J, solutions, energies, numOccurrences);
				break;
			}
			catch (const blackbox::ProblemCancelledException&)
			{
				throw;
			}
			catch (const blackbox::BlackBoxException&)
			{
				++retryCounter;
				if (retryCounter > blackBoxInternalParams.maxRetry)
					throw;
			}
		}
		blackBoxInfo.solverCallsTime += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tmpStartTime).count()) / 1000.0;

		/*
		// ######################################### start of debugging #########################################
		//check h
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "h_%d.txt", compareCnt);

			std::vector<double> expected_h;
			std::ifstream fin(fileName);
			double d;
			while (fin>>d)
				expected_h.push_back(d);

			std::vector<double> h(numQubits_, 0.0);
			for (int i = 0; i < problem.size(); ++i)
			{
				if (problem[i].i == problem[i].j)
					h[problem[i].i] += problem[i].value;
			}

			bool flag = true;
			if (expected_h.size() != h.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < h.size(); ++i)
			{
				if (std::abs(expected_h[i] - h[i]) > 1e-6)
				//if (expected_h[i] != h[i])
				{
					outputIntermediateResults("%e %e diff = %e\n", expected_h[i], h[i], expected_h[i] - h[i]);
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					flag = false;
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);

			char fileName2[1000];
			sprintf(fileName2, "h_from_cpp_%d.txt", compareCnt);
			std::ofstream fout(fileName2);
			for (int i = 0; i < h.size(); ++i)
				fout<<h[i]<<" ";
			fout.close();

			char fileName3[1000];
			sprintf(fileName3, "problem_from_cpp_%d.txt", compareCnt);
			std::ofstream fout2(fileName3);
			for (int i = 0; i < problem.size(); ++i)
				fout2<<problem[i].i<<" "<<problem[i].j<<" "<<problem[i].value<<"\n";
			fout2.close();
		}

		//check J
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "J_%d.txt", compareCnt);

			std::vector<std::vector<double> > expected_J;

			char s[100000];
			std::ifstream fin(fileName);
			while (fin.getline(s, 100000))
			{
				std::vector<double> tmp_J;
				std::stringstream ss(s);
				double d;
				while (ss>>d)
					tmp_J.push_back(d);
				expected_J.push_back(tmp_J);
			}

			std::vector<std::vector<double> > J(numQubits_, std::vector<double>(numQubits_, 0.0));
			for (int i = 0; i < problem.size(); ++i)
			{
				if (problem[i].i != problem[i].j)
					J[std::min(problem[i].i, problem[i].j)][std::max(problem[i].i, problem[i].j)] += problem[i].value;
			}

			bool flag = true;
			if (expected_J.size() != J.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong_1 !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < J.size(); ++i)
			{
				if (expected_J[i].size() != J[i].size())
				{
					flag = false;
					outputIntermediateResults("%s's size is wrong_2 !!!!!!!!!!!!!\n", fileName);
					break;
				}

				for (int j = 0; j < J[i].size(); ++j)
				{
					if (std::abs(expected_J[i][j] - J[i][j]) > 1e-6)
					//if (expected_J[i][j] != J[i][j])
					{
						outputIntermediateResults("%e %e diff = %e\n", expected_J[i][j], J[i][j], expected_J[i][j] - J[i][j]);
						flag = false;
						outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
						break;
					}
				}

				if (!flag)
					break;
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);

			char fileName2[1000];
			sprintf(fileName2, "J_from_cpp_%d.txt", compareCnt);
			std::ofstream fout(fileName2);
			for (int i = 0; i < J.size(); ++i)
			{
				for (int j = 0; j < J[i].size(); ++j)
					fout<<J[i][j]<<" ";
				fout<<"\n";
			}
			fout.close();
		}

		// check answer's solutions
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "answer_from_sapi_%d.txt", compareCnt);

			std::vector<std::vector<int> > expectedSolutions;

			char s[100000];
			std::ifstream fin(fileName);
			while (fin.getline(s, 100000))
			{
				std::vector<int> tmpSolution;
				std::stringstream ss(s);
				double d;
				while (ss>>d)
					tmpSolution.push_back(static_cast<int>(d));
				expectedSolutions.push_back(tmpSolution);
			}

			std::vector<std::vector<int> > solutions(isingResult.energies.size());
			int answerLen = static_cast<int>(isingResult.solutions.size()) / static_cast<int>(isingResult.energies.size());
			for (int i = 0; i < isingResult.energies.size(); ++i)
			{
				for (int j = i * answerLen; j < (i + 1) * answerLen; ++j)
					solutions[i].push_back(static_cast<int>(isingResult.solutions[j]));
			}

			bool flag = true;
			if (expectedSolutions.size() != solutions.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < solutions.size(); ++i)
			{
				if (expectedSolutions[i].size() != solutions[i].size())
				{
					flag = false;
					outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}

				for (int j = 0; j < solutions[i].size(); ++j)
				{
					if (expectedSolutions[i][j] != solutions[i][j])
					{
						flag = false;
						outputIntermediateResults("%s is wrong !!!!!!!!!!!!! %d %d %d %d\n", fileName, expectedSolutions[i][j], solutions[i][j], i, j);
						break;
					}
				}

				if (!flag)
					break;
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);

			char fileName2[1000];
			sprintf(fileName2, "answer_from_sapi_cpp_%d.txt", compareCnt);
			std::ofstream fout(fileName2);
			for (int i = 0; i < solutions.size(); ++i)
			{
				for (int j = 0; j < solutions[i].size(); ++j)
					fout<<solutions[i][j]<<" ";
				fout<<"\n";
			}
			fout.close();
		}
		// ######################################### end of debugging #########################################
		*/


		++blackBoxInfo.numSolverCalls;

		if (!numOccurrences.empty())
			blackBoxInternalParams.numSamples = std::accumulate(numOccurrences.begin(), numOccurrences.end(), 0);
		else
			blackBoxInternalParams.numSamples = static_cast<int>(solutions.size());

		int numAnswers = static_cast<int>(energies.size());
		int answersLen = static_cast<int>(solutions[0].size());
		std::vector<std::vector<int> > pop;

		if (blackBoxInternalParams.emb.empty())
		{
			pop.resize(numAnswers);

			for (int i = 0; i < numAnswers; ++i)
				for (int j = 0; j < qubits_.size(); ++j)
					pop[i].push_back(solutions[i][qubits_[j]]);
		}
		else
		{
			std::set<std::vector<int> > uniqueSolutions;
			for (int i = 0; i < numAnswers; ++i)
			{
				std::vector<int> tmpSolution;
				for (int j = 0; j < blackBoxInternalParams.groupRep.size(); ++j)
					tmpSolution.push_back(solutions[i][blackBoxInternalParams.groupRep[j]]);
			
				uniqueSolutions.insert(tmpSolution);
			}

			pop.assign(uniqueSolutions.begin(), uniqueSolutions.end());
		}
	
		population = reorder(pop, blackBoxInternalParams.antiOrder);

		if (numActiveQubits_ < currentSolution.size())
		{
			pop.clear();
			pop.resize(population.size());
			for (int i = 0; i < population.size(); ++i)
				pop[i] = currentSolution;

			for (int i = 0; i < pop.size(); ++i)
				for (int j = 0; j < blackBoxInternalParams.subset.size(); ++j)
					pop[i][blackBoxInternalParams.subset[j]] = population[i][j];

			population = pop;
		}

		if (blackBoxExternalParams.verbose >= 2)
		{
			std::stringstream ss;
			ss << "sample_num = " << population.size() << "\n";
			blackBoxExternalParams.localInteractionPtr->displayOutput(ss.str());
		}
	
		blackBoxInternalParams.lastSampleNums.push_front(static_cast<int>(population.size()));
		if (static_cast<int>(blackBoxInternalParams.lastSampleNums.size()) > blackBoxInternalParams.temperatureCounterThreshold)
			blackBoxInternalParams.lastSampleNums.pop_back();

		adjustTemperature(blackBoxInternalParams, static_cast<int>(population.size()));


		/*
		// ######################################### start of debugging #########################################
		// check population
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "population_%d.txt", compareCnt);

			std::vector<std::vector<int> > expectedPopulation;

			char s[100000];
			std::ifstream fin(fileName);
			while (fin.getline(s, 100000))
			{
				std::vector<int> tmpPopulation;
				std::stringstream ss(s);
				double d;
				while (ss>>d)
					tmpPopulation.push_back(static_cast<int>(d));
				expectedPopulation.push_back(tmpPopulation);
			}

			bool flag = true;
			if (expectedPopulation.size() != population.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong_1 !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < population.size(); ++i)
			{
				if (expectedPopulation[i].size() != population[i].size())
				{
					flag = false;
					outputIntermediateResults("%s's size is wrong_2 !!!!!!!!!!!!!\n", fileName);
					break;
				}

				for (int j = 0; j < population[i].size(); ++j)
				{
					if (expectedPopulation[i][j] != population[i][j])
					{
						flag = false;
						outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
						break;
					}
				}

				if (!flag)
					break;
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
		// ######################################### end of debugging #########################################
		*/
	}
	
	// ret is the concatenation of h and J
	void phi(const std::vector<int>& sample, BlackBoxInternalParams& blackBoxInternalParams, std::vector<double>& ret) const
	{
		std::vector<double> extS;
		if (!blackBoxInternalParams.emb.empty())
		{
			extS.assign(numActiveQubits_, 0.0);
			for (int i = 0; i < blackBoxInternalParams.emb.size(); ++i)
				for (int j = 0; j < blackBoxInternalParams.emb[i].size(); ++j)
					extS[blackBoxInternalParams.emb[i][j]] = static_cast<double>(sample[i]);
		}
		else
			extS.assign(sample.begin(), sample.end());

		ret = extS;
		for (int i = 0; i < subiIndex_.size(); ++i)
			ret.push_back(extS[subiIndex_[i]] * extS[subjIndex_[i]]);
	}

	void phi(const std::vector<std::vector<int> >& sample, BlackBoxInternalParams& blackBoxInternalParams, std::vector<std::vector<double> >& ret) const
	{
		ret.resize(sample.size());
		for (int i = 0 ; i < sample.size(); ++i)
			phi(sample[i], blackBoxInternalParams, ret[i]);
	}

	std::vector<int> reorder(const std::vector<int>& sample, const std::vector<int>& order) const
	{
		std::vector<int> tmpSample(sample);
		for (int i = 0; i < sample.size(); ++i)
			tmpSample[i] = sample[order[i]];

		return tmpSample;
	}

	std::vector<std::vector<int> > reorder(const std::vector<std::vector<int> >& sample, const std::vector<int>& order) const
	{
		std::vector<std::vector<int> > tmpSample;
		for (int i = 0; i <sample.size(); ++i)
			tmpSample.push_back(reorder(sample[i], order));

		return tmpSample;
	}

	void adjustTemperature(BlackBoxInternalParams& blackBoxInternalParams, int numSamples) const
	{
		if (numSamples < blackBoxInternalParams.numSamples / 2)
		{
			++blackBoxInternalParams.lowTemperatureCounter;
			--blackBoxInternalParams.highTemperatureCounter;
		}
		else if (numSamples > 0.95 * blackBoxInternalParams.numSamples)
		{
			++blackBoxInternalParams.highTemperatureCounter;
			--blackBoxInternalParams.lowTemperatureCounter;	
		}

		if (blackBoxInternalParams.lowTemperatureCounter > blackBoxInternalParams.temperatureCounterThreshold)
		{
			double lastSampleNumsMean = static_cast<double>(std::accumulate(blackBoxInternalParams.lastSampleNums.begin(), blackBoxInternalParams.lastSampleNums.end(), 0)) / static_cast<double>(blackBoxInternalParams.lastSampleNums.size());
			if (lastSampleNumsMean < 0.01 * static_cast<double>(blackBoxInternalParams.numSamples))
			{
				blackBoxInternalParams.betaH /= 2;
				blackBoxInternalParams.betaJ /= 2;
			}
			else
			{
				if (blackBoxInternalParams.betaH > blackBoxInternalParams.betaJ)
					blackBoxInternalParams.betaH = std::max(blackBoxInternalParams.betaH / blackBoxInternalParams.temperatureScale, blackBoxInternalParams.betaJ);
				else if (blackBoxInternalParams.betaH < blackBoxInternalParams.betaJ)
					blackBoxInternalParams.betaJ = std::max(blackBoxInternalParams.betaJ / blackBoxInternalParams.temperatureScale, blackBoxInternalParams.betaH);
				else
				{
					blackBoxInternalParams.betaJ /= blackBoxInternalParams.temperatureScale;
					blackBoxInternalParams.betaH = blackBoxInternalParams.betaJ;
				}
			}

			blackBoxInternalParams.lowTemperatureCounter = 0;
			blackBoxInternalParams.highTemperatureCounter = 0;
		}
		else if (blackBoxInternalParams.highTemperatureCounter > blackBoxInternalParams.temperatureCounterThreshold)
		{
			if (blackBoxInternalParams.betaH < hMax_)
			{
				blackBoxInternalParams.betaH = std::min(blackBoxInternalParams.betaH * blackBoxInternalParams.temperatureScale, hMax_);
				blackBoxInternalParams.betaJ = std::min(blackBoxInternalParams.betaJ * blackBoxInternalParams.temperatureScale, jMax_);
			}
			else
				blackBoxInternalParams.betaJ /= blackBoxInternalParams.temperatureScale;
			blackBoxInternalParams.lowTemperatureCounter = 0;
			blackBoxInternalParams.highTemperatureCounter = 0;
		}
	}

	void generateModel(const std::vector<int>& currentSolution, const std::vector<std::vector<int> >& currentNeighbours, const std::vector<double>& currentNeighbourEnergies,
		               const blackbox::BlackBoxExternalParams& blackBoxExternalParams, BlackBoxInternalParams& blackBoxInternalParams, blackbox::BlackBoxInfo& blackBoxInfo,
					   std::vector<double>& h, std::map<std::pair<int, int>, double>& J) const
	{
		/*
		// ######################################### start of debugging #########################################
		// check currentSolution
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "currentSolution_%d.txt", compareCnt);
			std::vector<int> expectedCurrentSolution;
			std::ifstream fin(fileName);
			int i;
			while (fin>>i)
				expectedCurrentSolution.push_back(i);

			bool flag = true;
			if (expectedCurrentSolution.size() != currentSolution.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < currentSolution.size(); ++i)
			{
				if (expectedCurrentSolution[i] != currentSolution[i])
				{
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					flag = false;
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check currentNeighbours
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "currentNeighbours_%d.txt", compareCnt);

			std::vector<std::vector<int> > expectedCurrentNeighbours;

			char s[100000];
			std::ifstream fin(fileName);
			while (fin.getline(s, 100000))
			{
				std::vector<int> tmpCurrentNeighbour;
				std::stringstream ss(s);
				int i;
				while (ss>>i)
					tmpCurrentNeighbour.push_back(i);
				expectedCurrentNeighbours.push_back(tmpCurrentNeighbour);
			}

			bool flag = true;
			if (expectedCurrentNeighbours.size() != currentNeighbours.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong_1 !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < currentNeighbours.size(); ++i)
			{
				if (expectedCurrentNeighbours[i].size() != currentNeighbours[i].size())
				{
					flag = false;
					outputIntermediateResults("%s's size is wrong_2 !!!!!!!!!!!!!\n", fileName);
					break;
				}

				for (int j = 0; j < currentNeighbours[i].size(); ++j)
				{
					if (expectedCurrentNeighbours[i][j] != currentNeighbours[i][j])
					{
						flag = false;
						outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
						break;
					}
				}

				if (!flag)
					break;
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
	

		// check currentNeighbourEnergies
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "currentNeighbourEnergies_%d.txt", compareCnt);

			std::vector<double> expectedCurrentNeighbourEnergies;
			std::ifstream fin(fileName);
			double d;
			while (fin>>d)
				expectedCurrentNeighbourEnergies.push_back(d);

			bool flag = true;
			if (expectedCurrentNeighbourEnergies.size() != currentNeighbourEnergies.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < currentNeighbourEnergies.size(); ++i)
			{
				if (expectedCurrentNeighbourEnergies[i] != currentNeighbourEnergies[i])
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
		// ######################################### end of debugging #########################################
		*/

		int numVars = numActiveQubits_;
		std::vector<double> phiSStar;
		phi(reorder(currentSolution, blackBoxInternalParams.order), blackBoxInternalParams, phiSStar);

		/*
		// ######################################### start of debugging #########################################
		// check phiSStar
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "phiSStar_%d.txt", compareCnt);

			std::vector<double> expectedPhiSStar;
			std::ifstream fin(fileName);
			double d;
			while (fin>>d)
				expectedPhiSStar.push_back(d);

			bool flag = true;
			if (expectedPhiSStar.size() != phiSStar.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < phiSStar.size(); ++i)
			{
				if (expectedPhiSStar[i] != phiSStar[i])
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
		// ######################################### end of debugging #########################################
		*/

		std::vector<int> theta;
		int lower = 0;
		int upper = static_cast<int>(iIndex_.size()) + numActiveQubits_;
		for (int i = lower; i < upper; ++i)
			theta.push_back(i);
	
		std::vector<int> delta;
		lower = upper;
		upper = upper + static_cast<int>(currentNeighbours.size()) - 1;
		for (int i = lower; i < upper; ++i)
			delta.push_back(i);

		std::vector<int> t;
		lower = upper;
		upper = upper + numVars;
		for (int i = lower; i < upper; ++i)
			t.push_back(i);


		/*
		// ######################################### start of debugging #########################################
		// check theta
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "theta_%d.txt", compareCnt);

			std::vector<int> expectedTheta;
			std::ifstream fin(fileName);
			int i;
			while (fin>>i)
				expectedTheta.push_back(i);

			bool flag = true;
			if (expectedTheta.size() != theta.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < theta.size(); ++i)
			{
				if (expectedTheta[i] != theta[i] + 1)
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check delta
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "delta_%d.txt", compareCnt);

			std::vector<int> expectedDelta;
			std::ifstream fin(fileName);
			int i;
			while (fin>>i)
				expectedDelta.push_back(i);

			bool flag = true;
			if (expectedDelta.size() != delta.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < delta.size(); ++i)
			{
				if (expectedDelta[i] != delta[i] + 1)
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check t
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "t_%d.txt", compareCnt);

			std::vector<int> expectedT;
			std::ifstream fin(fileName);
			int i;
			while (fin>>i)
				expectedT.push_back(i);

			bool flag = true;
			if (expectedT.size() != t.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < t.size(); ++i)
			{
				if (expectedT[i] != t[i] + 1)
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
		// ######################################### end of debugging #########################################
		*/

		int vNum = t.back() + 1;

		std::vector<EnergyAndIndex> eai;
		for (int i = 0; i < currentNeighbourEnergies.size(); ++i)
		{
			EnergyAndIndex tmp;
			tmp.energy = currentNeighbourEnergies[i];
			tmp.index = i;
			eai.push_back(tmp);
		}

		std::sort(eai.begin(), eai.end(), compareEnergyAndIndex);

		std::vector<double> ss(eai.size());
		std::vector<int> si(eai.size());
		for (int i = 0; i < eai.size(); ++i)
		{
			ss[i] = eai[i].energy;
			si[i] = eai[i].index;
		}

		/*
		// ######################################### start of debugging #########################################
		// check ss
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "ss_%d.txt", compareCnt);

			std::vector<double> expectedSs;
			std::ifstream fin(fileName);
			double d;
			while (fin>>d)
				expectedSs.push_back(d);

			bool flag = true;
			if (expectedSs.size() != ss.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < ss.size(); ++i)
			{
				if (expectedSs[i] != ss[i])
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check si
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "si_%d.txt", compareCnt);

			std::vector<int> expectedSi;
			std::ifstream fin(fileName);
			int i;
			while (fin>>i)
				expectedSi.push_back(i);

			bool flag = true;
			if (expectedSi.size() != si.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < si.size(); ++i)
			{
				if (expectedSi[i] != si[i] + 1)
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
		// ######################################### end of debugging #########################################
		*/


		std::vector<std::vector<double> > phiS;
		phi(reorder(currentNeighbours, blackBoxInternalParams.order), blackBoxInternalParams, phiS);

	
		/*
		// ######################################### start of debugging #########################################
		// check phiS
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "phiS_%d.txt", compareCnt);

			std::vector<std::vector<double> > expectedPhiS;

			char s[100000];
			std::ifstream fin(fileName);
			while (fin.getline(s, 100000))
			{
				std::vector<double> tmp;
				std::stringstream ss(s);
				double d;
				while (ss>>d)
					tmp.push_back(d);
				expectedPhiS.push_back(tmp);
			}

			bool flag = true;
			if (expectedPhiS.size() != phiS.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong_1 !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < phiS.size(); ++i)
			{
				if (expectedPhiS[i].size() != phiS[i].size())
				{
					flag = false;
					outputIntermediateResults("%s's size is wrong_2 !!!!!!!!!!!!!\n", fileName);
					break;
				}

				for (int j = 0; j < phiS[i].size(); ++j)
				{
					if (expectedPhiS[i][j] != phiS[i][j])
					{
						flag = false;
						outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
						break;
					}
				}

				if (!flag)
					break;
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
		// ######################################### end of debugging #########################################
		*/

		std::vector<double> f(vNum, 0.0);

		for (int i = 0; i < delta.size(); ++i)
			f[delta[i]] = 1.0;

		for (int i = 0; i <t.size(); ++i)
			f[t[i]] = 1.0;

		std::vector<std::vector<double> > Aineq(si.size() - 1 + 2 * phiSStar.size(), std::vector<double>(vNum, 0.0));
		std::vector<double> bineq(Aineq.size(), 0.0);
		for (int i = 0; i < si.size() - 1; ++i)
		{
			for (int j = 0; j < theta.size(); ++j)
				Aineq[i][theta[j]] = phiS[si[i]][j] - phiS[si[i + 1]][j];

			Aineq[i][delta[i]] = -1.0;
		}

		std::vector<double> df;
		for (int i = 0; i < si.size() - 1; ++i)
			df.push_back(currentNeighbourEnergies[si[i]] - currentNeighbourEnergies[si[i + 1]]);

		double dfMaxAbsValue = abs(*std::max_element(df.begin(), df.end(), compareAbs));
		if (dfMaxAbsValue != 0)
		{
			for (int i = 0; i < si.size() - 1; ++i)
				bineq[i] = df[i] / dfMaxAbsValue;
		}

		for (int i = 0; i < numVars; ++i)
		{
			Aineq[2 * i + si.size() - 1][theta[qubits_[i]]] = 1.0;
			Aineq[2 * i + si.size() - 1][t[i]] = -1.0;
			bineq[2 * i + si.size() - 1] = -phiSStar[qubits_[i]];
			Aineq[2 * i + si.size()][theta[qubits_[i]]] = -1.0;
			Aineq[2 * i + si.size()][t[i]] = -1.0;
			bineq[2 * i + si.size()] = phiSStar[qubits_[i]];
		}

		std::vector<double> lb(vNum, -std::numeric_limits<double>::infinity());
		std::vector<double> ub(vNum, std::numeric_limits<double>::infinity());

		for (int i = numActiveQubits_; i < theta.size(); ++i)
		{
			ub[theta[i]] = 1.0 / 4.0;
			lb[theta[i]] = -1.0 / 4.0;
		}

		for (int i = 0; i < numActiveQubits_; ++i)
		{
			ub[theta[i]] = hMax_;
			lb[theta[i]] = hMin_;
		}

		for (int i = 0; i < delta.size(); ++i)
			lb[delta[i]] = 0.0;

		for (int i = 0; i < t.size(); ++i)
			lb[t[i]] = 0.0;

		for (int i = 0; i < blackBoxInternalParams.fmInd.size(); ++i)
		{
			ub[theta[numActiveQubits_ + blackBoxInternalParams.fmInd[i]]] = 1.0;
			lb[theta[numActiveQubits_ + blackBoxInternalParams.fmInd[i]]] = -1.0;
		}

		if (blackBoxInternalParams.Aeq.empty())
		{
			if (!blackBoxInternalParams.emb.empty())
			{
				std::vector<std::vector<double> > Aeqj(blackBoxInternalParams.fmInd.size(), std::vector<double>(vNum, 0.0));
				for (int i = 0; i < blackBoxInternalParams.fmInd.size(); ++i)
					Aeqj[i][blackBoxInternalParams.fmInd[i] + numActiveQubits_] = 1.0;

				std::vector<double> beqj(blackBoxInternalParams.fmInd.size(), jMin_);
				std::vector<std::vector<double> > Aeqh(numActiveQubits_ - blackBoxInternalParams.emb.size(), std::vector<double>(vNum, 0.0));
				int k = 0;
				for (int i = 0; i < blackBoxInternalParams.emb.size(); ++i)
					for (int j = 0; j <blackBoxInternalParams.emb[i].size() - 1; ++j)
					{
						Aeqh[k][theta[blackBoxInternalParams.emb[i][j]]] = 1.0;
						Aeqh[k][theta[blackBoxInternalParams.emb[i][j + 1]]] = -1.0;
						++k;
					}

				std::vector<double> beqh(numActiveQubits_ - blackBoxInternalParams.emb.size(), 0.0);

				blackBoxInternalParams.Aeq = Aeqj;
				blackBoxInternalParams.Aeq.insert(blackBoxInternalParams.Aeq.end(), Aeqh.begin(), Aeqh.end());

				blackBoxInternalParams.beq = beqj;
				blackBoxInternalParams.beq.insert(blackBoxInternalParams.beq.end(), beqh.begin(), beqh.end());
			}
		}

		/*
		// ######################################### start of debugging #########################################
		// check f
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "f_%d.txt", compareCnt);

			std::vector<double> expectedF;
			std::ifstream fin(fileName);
			double d;
			while (fin>>d)
				expectedF.push_back(d);

			bool flag = true;
			if (expectedF.size() != f.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < f.size(); ++i)
			{
				if (expectedF[i] != f[i])
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

	
		// check Aineq
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "Aineq_%d.txt", compareCnt);

			std::vector<std::vector<double> > expectedAineq;

			char s[100000];
			std::ifstream fin(fileName);
			while (fin.getline(s, 100000))
			{
				std::vector<double> tmp;
				std::stringstream ss(s);
				double d;
				while (ss>>d)
					tmp.push_back(d);
				expectedAineq.push_back(tmp);
			}

			bool flag = true;
			if (expectedAineq.size() != Aineq.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong_1 !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < Aineq.size(); ++i)
			{
				if (expectedAineq[i].size() != Aineq[i].size())
				{
					flag = false;
					outputIntermediateResults("%s's size is wrong_2 !!!!!!!!!!!!!\n", fileName);
					break;
				}

				for (int j = 0; j < Aineq[i].size(); ++j)
				{
					if (expectedAineq[i][j] != Aineq[i][j])
					{
						flag = false;
						outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
						break;
					}
				}

				if (!flag)
					break;
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check bineq
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "bineq_%d.txt", compareCnt);

			std::vector<double> expectedBineq;
			std::ifstream fin(fileName);
			double d;
			while (fin>>d)
				expectedBineq.push_back(d);

			bool flag = true;
			if (expectedBineq.size() != bineq.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < bineq.size(); ++i)
			{
				if (std::abs(expectedBineq[i] - bineq[i]) > 1e-6)
				//if (expectedBineq[i] != bineq[i])
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check Aeq
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "Aeq_%d.txt", compareCnt);

			std::vector<std::vector<double> > expectedAeq;

			char s[100000];
			std::ifstream fin(fileName);
			while (fin.getline(s, 100000))
			{
				std::vector<double> tmp;
				std::stringstream ss(s);
				double d;
				while (ss>>d)
					tmp.push_back(d);
				expectedAeq.push_back(tmp);
			}

			bool flag = true;
			if (expectedAeq.size() != blackBoxInternalParams.Aeq.size())
				outputIntermediateResults("%s's size is wrong_1 !!!!!!!!!!!!!\n", fileName);
		
			for (int i = 0; i < blackBoxInternalParams.Aeq.size(); ++i)
			{
				if (expectedAeq[i].size() != blackBoxInternalParams.Aeq[i].size())
				{
					flag = false;
					outputIntermediateResults("%s's size is wrong_2 !!!!!!!!!!!!!\n", fileName);
					break;
				}

				for (int j = 0; j < blackBoxInternalParams.Aeq[i].size(); ++j)
				{
					if (expectedAeq[i][j] != blackBoxInternalParams.Aeq[i][j])
					{
						flag = false;
						outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
						break;
					}
				}

				if (!flag)
					break;
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check beq
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "beq_%d.txt", compareCnt);

			std::vector<double> expectedBeq;
			std::ifstream fin(fileName);
			double d;
			while (fin>>d)
				expectedBeq.push_back(d);

			bool flag = true;

			if (expectedBeq.size() != blackBoxInternalParams.beq.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < blackBoxInternalParams.beq.size(); ++i)
			{
				if (expectedBeq[i] != blackBoxInternalParams.beq[i])
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check lb
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "lb_%d.txt", compareCnt);

			std::vector<double> expectedLb;
			std::ifstream fin(fileName);
			double d;
			char s[100];
			while (fin.getline(s, 100))
			{
				if (strcmp(s, "            -Inf") == 0)
					expectedLb.push_back(-std::numeric_limits<double>::infinity());
				else
				{
					std::stringstream ss(s);
					ss>>d;
					expectedLb.push_back(d);
				}
			}

			bool flag = true;

			if (expectedLb.size() != lb.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < lb.size(); ++i)
			{
				if (expectedLb[i] != lb[i])
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}

		// check ub
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "ub_%d.txt", compareCnt);

			std::vector<double> expectedUb;
			std::ifstream fin(fileName);
			double d;
			char s[100];
			while (fin.getline(s, 100))
			{
				if (strcmp(s, "             Inf") == 0)
					expectedUb.push_back(std::numeric_limits<double>::infinity());
				else
				{
					std::stringstream ss(s);
					ss>>d;
					expectedUb.push_back(d);
				}
			}

			bool flag = true;

			if (expectedUb.size() != ub.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < ub.size(); ++i)
			{
				if (expectedUb[i] != ub[i])
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
		// ######################################### end of debugging #########################################
		*/

		std::chrono::high_resolution_clock::time_point tmpStartTime = std::chrono::high_resolution_clock::now();
		std::vector<double> x = blackBoxExternalParams.lpSolverPtr->solve(f, Aineq, bineq, blackBoxInternalParams.Aeq, blackBoxInternalParams.beq, lb, ub);
		blackBoxInfo.lpSolverCallsTime += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tmpStartTime).count()) / 1000.0;

		++blackBoxInfo.numLPSolverCalls;


		/*
		// ######################################### start of debugging #########################################
		// check x
		if (checkResults)
		{
			char fileName[1000];
			sprintf(fileName, "x_%d.txt", compareCnt);

			std::vector<double> expectedX;
			std::ifstream fin(fileName);
			double d;
			while (fin>>d)
				expectedX.push_back(d);

			bool flag = true;

			if (expectedX.size() != x.size())
			{
				flag = false;
				outputIntermediateResults("%s's size is wrong !!!!!!!!!!!!!\n", fileName);
			}

			for (int i = 0; i < x.size(); ++i)
			{
				if (std::abs(expectedX[i] - x[i]) > 1e-5)
				{
					flag = false;
					outputIntermediateResults("%s is wrong !!!!!!!!!!!!!\n", fileName);
					break;
				}
			}

			fin.close();

			if (flag)
				outputIntermediateResults("%s is correct !\n", fileName);
		}
		// ######################################### end of debugging #########################################
		*/
		std::map<int, double> hMap;
		for (int i = 0; i < numActiveQubits_; ++i)
		{
			double tmp = x[i] * blackBoxInternalParams.betaH;
			if (tmp < hMin_)
				tmp = hMin_;
			else if (tmp > hMax_)
				tmp = hMax_;
			if (tmp != 0.0)
				hMap[qubits_[i]] += tmp;
		}

		if (!hMap.empty())
		{
			h.assign(std::max_element(hMap.begin(), hMap.end())->first + 1, 0.0);
			for (std::map<int, double>::const_iterator it = hMap.begin(), end = hMap.end(); it != end; ++it)
				h[it->first] = it->second;
		}

		std::set<int> fmIndSet(blackBoxInternalParams.fmInd.begin(), blackBoxInternalParams.fmInd.end());
		for (int i = numActiveQubits_; i < theta.size(); ++i)
		{
			double tmp = x[i] * blackBoxInternalParams.betaJ;
			if (fmIndSet.find(i - numActiveQubits_) != fmIndSet.end())
				tmp = jMin_;
			if (tmp < jMin_)
				tmp = jMin_;
			else if (tmp > jMax_)
				tmp = jMax_;
			if (tmp != 0.0)
				J[std::make_pair(iIndex_[i - numActiveQubits_], jIndex_[i - numActiveQubits_])] += tmp;
		}
	}

	void generateEmbedding(int numVars, BlackBoxInternalParams& blackBoxInternalParams, boost::mt19937& rng) const
	{
		if (numVars < numActiveQubits_)
		{
			RandomNumberGenerator randomNumberGenerator(rng);

			std::vector<std::vector<int> > tmpAdj(numQubits_, std::vector<int>(numQubits_, 0));
			for (int i = 0; i < iIndex_.size(); ++i)
			{
				tmpAdj[iIndex_[i]][jIndex_[i]] = 1;
				tmpAdj[jIndex_[i]][iIndex_[i]] = 1;
			}

			std::vector<std::vector<int> > tmpEmb(numVars);

			std::vector<int> r(numActiveQubits_);
			for (int i = 0; i < numActiveQubits_; ++i)
				r[i] = i;

			std::random_shuffle(r.begin(), r.end(), randomNumberGenerator); // should use

			std::vector<int> root(numVars);
			for (int i = 0; i < numVars; ++i)
				root[i] = qubits_[r[i]];

			blackBoxInternalParams.groupRep = root;

			for (int i = 0; i < root.size(); ++i)
				tmpEmb[i].push_back(root[i]);

			std::vector<std::vector<int> > adjRoot(numVars, std::vector<int>(numQubits_, -1));

			for (int i = 0; i < root.size(); ++i)
			{
				int rootI = root[i];
				for (int j = 0; j < numQubits_; j++)
				{
					if (tmpAdj[j][rootI] != 0)
						adjRoot[i][j] = i;
				}
			}

			std::vector<int> unusedQubit(numQubits_, 0);
			for (int i = numVars; i < r.size(); ++i)
				unusedQubit[r[i]] = 1;
			int numUnusedQubit = static_cast<int>(r.size()) - numVars;

			while (numUnusedQubit != 0)
			{
				for (int i = numVars; i < numActiveQubits_; ++i)
				{
					if (unusedQubit[r[i]])
					{
						int q = qubits_[r[i]];
						std::vector<int> rootInd;
						for (int j = 0; j < numVars; ++j)
						{
							if (adjRoot[j][q] != -1)
								rootInd.push_back(j);
						}

						if (!rootInd.empty())
						{
							//std::random_shuffle(rootInd.begin(), rootInd.end(), randomNumberGenerator); // should use
							//int ind = rootInd[0]; // should use
							// or:
							//boost::variate_generator<boost::mt19937&, boost::uniform_int<> > ui(rng, boost::uniform_int<>(0, static_cast<int>(rootInd.size()) - 1)); // should use, use this one
							//int ind = rootInd[ui()]; // should use, use this one
							//int ind = rootInd[0]; // remove randomness
							int ind;
							bool indFound = false;
							int tmpMin;
							for (int j = 0; j < rootInd.size(); ++j)
							{
								if (!indFound || static_cast<int>(tmpEmb[rootInd[j]].size()) < tmpMin)
								{
									indFound = true;
									ind = rootInd[j];
									tmpMin = static_cast<int>(tmpEmb[rootInd[j]].size());
								}
							}

							tmpEmb[ind].push_back(q);
							for (int j = 0; j < numQubits_; ++j)
							{
								if (tmpAdj[j][q] != 0)
									adjRoot[ind][j] = ind;
							}
							unusedQubit[r[i]] = 0;
							--numUnusedQubit;
						}
					}
				}
			}		

			std::vector<int> qubitGroup(numQubits_, 0);

			for (int i = 0; i < tmpEmb.size(); ++i)
				for (int j = 0; j < tmpEmb[i].size(); ++j)
					qubitGroup[tmpEmb[i][j]] = i;

			for (int i = 0; i < iIndex_.size(); ++i)
			{
				if (qubitGroup[iIndex_[i]] == qubitGroup[jIndex_[i]])
					blackBoxInternalParams.fmInd.push_back(i);
			}

			blackBoxInternalParams.emb.resize(tmpEmb.size());

			std::map<int, int> subInd;
			for (int i = 0; i < qubits_.size(); ++i)
				subInd[qubits_[i]] = i;

			for (int i = 0; i < tmpEmb.size(); ++i)
				for (int j = 0; j <tmpEmb[i].size(); ++j)
					blackBoxInternalParams.emb[i].push_back(subInd[tmpEmb[i][j]]);
		}
	}
	
	blackbox::IsingSolverPtr isingSolverPtr_;
		
	int numQubits_;
	int numActiveQubits_;
	std::vector<int> qubits_;
	std::vector<int> undefinedQubits_;
	std::vector<std::pair<int, int> > couplers_;
	std::vector<int> iIndex_;
	std::vector<int> jIndex_;
	std::vector<int> subiIndex_;
	std::vector<int> subjIndex_;
	double hMin_;
	double hMax_;
	double jMin_;
	double jMax_;
};

typedef std::shared_ptr<BlackBoxSolver> BlackBoxSolverPtr;

} // anonymous namespace

namespace blackbox
{

BlackBoxExternalParams::BlackBoxExternalParams()
{
	maxNumStateEvaluations = DEFAULT_MAX_NUM_STATE_EVALUATIONS;
	exitThresholdValue = DEFAULT_EXIT_THRESHOLD_VALUE;
	randomSeed = static_cast<unsigned int>(std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() % std::numeric_limits<unsigned int>::max());
	verbose = DEFAULT_VERBOSE;
	timeout = DEFAULT_TIMEOUT;
	isingQubo = DEFAULT_ISINGQUBO;
	drawSample = DEFAULT_DRAW_SAMPLE;
}

BlackBoxResult solveBlackBox(BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr, int numVars, IsingSolverPtr isingSolverPtr, BlackBoxExternalParams& blackBoxExternalParams)
{
	BlackBoxSolverPtr blackBoxSolverPtr(new BlackBoxSolver(isingSolverPtr));
	return blackBoxSolverPtr->solve(blackBoxObjectiveFunctionPtr, numVars, blackBoxExternalParams);
}

} // namespace sapi
