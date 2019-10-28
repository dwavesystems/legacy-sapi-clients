//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "find_embedding.hpp"

#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <string>
#include <chrono>
#include <stack>
#include <sstream>
#include <boost/random.hpp>

namespace
{

bool isNaN(double a)
{
	return (a != a);
}

// default external paramter values
const int DEFAULT_FE_VERBOSE = 0;
const double DEFAULT_FE_TIMEOUT = 1000.0;
const int DEFAULT_TRIES = 10;
const int DEFAULT_MAXNOIMPROVEMENT = 10;
const bool DEFAULT_FASTEMBEDDING = false;

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

// default internal paramter values
const int DEFAULT_ALPHA = 5;
const int DEFAULT_MAXWIDTH = std::numeric_limits<int>::max();
const std::string DEFAULT_ORDER = "bfs";
const int DEFAULT_MAXROUNDS = std::numeric_limits<int>::max();
const int DEFAULT_MAXSHUFFLE = 5;

struct FindEmbeddingInternalParams
{
	FindEmbeddingInternalParams()
	{
		alpha = DEFAULT_ALPHA;
		maxWidth = DEFAULT_MAXWIDTH;
		order = DEFAULT_ORDER;
		maxRounds = DEFAULT_MAXROUNDS;
		maxShuffle = DEFAULT_MAXSHUFFLE;
	}

	int alpha;
	int maxWidth;
	std::string order;
	int maxRounds;
	int maxShuffle;
};

bool compareColSum(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
	if (a.first != b.first)
		return a.first > b.first;
	else
		return a.second < b.second;
}

void stronglyConnectedComponents(const compressed_matrix::CompressedMatrix<int>& Q, std::vector<std::vector<int> >& connectedComponents)
{
	int numVertices = Q.numRows();
	std::vector<int> visited(numVertices, 0);
	std::queue<int> q;
	for (int i = 0; i < numVertices; ++i)
	{
		if (!visited[i])
		{
			connectedComponents.push_back(std::vector<int>());
			q.push(i);
			visited[i] = 1;
			connectedComponents.back().push_back(i);
			while (!q.empty())
			{
				int currNode = q.front();
				q.pop();
				int start = Q.rowOffsets()[currNode];
				int end = Q.rowOffsets()[currNode + 1];
				for (int k = start; k < end; ++k)
				{
					int neighbour = Q.colIndices()[k];
					if (!visited[neighbour])
					{
						q.push(neighbour);
						visited[neighbour] = 1;
						connectedComponents.back().push_back(neighbour);
					}
				}
			}
		}
	}
}

void dfs(const compressed_matrix::CompressedMatrix<int>& Q, int start, std::vector<int>& order, bool& hasUnreachable)
{
	int len = Q.numRows();
	std::vector<int> visited(len, 0);
	std::stack<int> s;
	s.push(start);
	visited[start] = 1;
	order.push_back(start);
	int cnt = 1;

	while (!s.empty())
	{
		int curr = s.top();
		s.pop();
		int start = Q.rowOffsets()[curr];
		int end = Q.rowOffsets()[curr + 1];
		for (int k = start; k < end; ++k)
		{
			int i = Q.colIndices()[k];
			if (!visited[i])
			{
				s.push(i);
				visited[i] = 1;
				order.push_back(i);
				++cnt;
			}
		}
	}

	if (cnt == len)
		hasUnreachable = false;
	else
		hasUnreachable = true;
}

void bfs(const compressed_matrix::CompressedMatrix<int>& Q, int start, std::vector<int>& order, bool& hasUnreachable, boost::mt19937& rng)
{
	RandomNumberGenerator randomNumberGenerator(rng);
	int len = Q.numRows();
	std::vector<int> visited(len, 0);
	std::queue<int> q;
	q.push(start);
	visited[start] = 1;
	order.push_back(start);
	int cnt = 1;

	while (!q.empty())
	{
		int curr = q.front();
		q.pop();
		int start = Q.rowOffsets()[curr];
		int end = Q.rowOffsets()[curr + 1];
		std::vector<int> unVisitedNeighbours;
		unVisitedNeighbours.reserve(end - start);
		for (int k = start; k < end; ++k)
		{
			int i = Q.colIndices()[k];
			if (!visited[i])
				unVisitedNeighbours.push_back(i);
		}
		
		// need to comment this line if compared to results of MATLAB codes since in MATLAB codes, bfs doesn't randomly pick the unvisited vertices
		std::random_shuffle(unVisitedNeighbours.begin(), unVisitedNeighbours.end(), randomNumberGenerator);

		for (int t = 0; t < unVisitedNeighbours.size(); ++t)
		{
			int i = unVisitedNeighbours[t];
			q.push(i);
			visited[i] = 1;
			order.push_back(i);
			++cnt;
		}
	}

	if (cnt == len)
		hasUnreachable = false;
	else
		hasUnreachable = true;
}

struct GraphNode
{
	long long int distance;
	int at;
};

struct GraphNodeComparison
{
	bool operator()(const GraphNode& a, const GraphNode& b)
	{
		if (a.distance != b.distance)
			return a.distance > b.distance;
		else
			return a.at > b.at;
	}
};

void dijkstraShortestPath(compressed_matrix::CompressedMatrix<long long int>& g, int numVertices, int source,
						  std::vector<long long int>& distances, std::vector<int>& parents, const find_embedding_::FindEmbeddingExternalParams& feExternalParams)
{
	if (feExternalParams.localInteractionPtr->cancelled())
		throw find_embedding_::ProblemCancelledException();

	distances.resize(numVertices, std::numeric_limits<long long int>::max());
	parents.resize(numVertices, -1);

	std::priority_queue<GraphNode, std::vector<GraphNode>, GraphNodeComparison> pq;
	GraphNode curr;
	curr.distance = 0;
	curr.at = source;
	distances[source] = 0;
	pq.push(curr);
	std::vector<int> visited(g.numRows(), 0);
	int numVisited = 0;
	while (!pq.empty() && numVisited != g.numRows())
	{
		curr = pq.top();
		pq.pop();

		if (!visited[curr.at])
		{
			++numVisited;
			visited[curr.at] = 1;
			int start = g.rowOffsets()[curr.at];
			int end = g.rowOffsets()[curr.at + 1];
			for (int j = start; j < end; ++j)
			{
				int c = g.colIndices()[j];
				if (   curr.at != c
					&& !visited[c]
					&& (curr.distance + g.values()[j] < distances[c]) )
				{
					distances[c] = curr.distance + g.values()[j];
					parents[c] = curr.at;
					GraphNode tmp;
					tmp.distance = distances[c];
					tmp.at = c;
					pq.push(tmp);
				}
			}
		}
	}
}

void findClosestVertex(const compressed_matrix::CompressedMatrix<int>& A, const std::vector<int>& I, const std::vector<int>& J, const std::vector<std::vector<int> >& neighbourSets,
					   const std::vector<int>& avoidanceSet, const std::vector<long long int>& weight, boost::mt19937& rng, long long int& minDist, std::vector<int>& uTree,
					   std::vector<std::vector<int> >& vpath, std::vector<std::vector<int> >& longvpath, std::vector<int>& longuTree, const find_embedding_::FindEmbeddingExternalParams& feExternalParams)
{
	int n = A.numRows();

	std::vector<std::vector<int> > parents(neighbourSets.size());
	std::vector<long long int> totalDist(n, 0);

	std::vector<int> degree(neighbourSets.size());
	for (int i = 0; i < neighbourSets.size(); ++i)
		degree[i] = static_cast<int>(neighbourSets[i].size());

	compressed_matrix::CompressedMatrix<long long int> g(2 * n + 1, 2 * n + 1);
	std::vector<int>& rowOffsets = const_cast<std::vector<int>& >(g.rowOffsets());
	std::vector<int>& colIndices = const_cast<std::vector<int>& >(g.colIndices());
	std::vector<long long int>& values = const_cast<std::vector<long long int>& >(g.values());
	colIndices.reserve(I.size() + n + n);
	values.reserve(I.size() + n + n);

	for (int k = 0; k < neighbourSets.size(); ++k)
	{
		std::vector<long long int> kWeight = weight;
		for (int i = 0; i < degree[k]; ++i)
			kWeight[neighbourSets[k][i]] = 1;

		std::vector<long long int> distances;

		if (!avoidanceSet.empty())
		{
			std::set<int> kAvoidanceSet;
			std::set<int> tmpSet(neighbourSets[k].begin(), neighbourSets[k].end());
			for (int i = 0; i < avoidanceSet.size(); ++i)
			{
				if (tmpSet.find(avoidanceSet[i]) == tmpSet.end())
					kAvoidanceSet.insert(avoidanceSet[i]);
			}

			int offset = 0;
			int currRow = 0;
			for (int i = 0; i < I.size(); ++i)
			{
				if (kAvoidanceSet.find(I[i]) == kAvoidanceSet.end())
				{
					if (currRow <= I[i])
					{
						for (int j = currRow; j <= I[i]; ++j)
							rowOffsets[j] = offset;
						currRow = I[i] + 1;
					}

					colIndices.push_back(J[i] + n);
					values.push_back(0);
					++offset;
				}
			}

			for (int i = 0; i < n; ++i)
			{
				if (currRow <= i + n)
				{
					for (int j = currRow; j <= i + n; ++j)
						rowOffsets[j] = offset;
					currRow = i + n + 1;
				}

				colIndices.push_back(i);
				values.push_back(kWeight[i]);
				++offset;
			}

			for (int i = 0; i < degree[k]; ++i)
			{
				if (currRow <= 2 * n)
				{
					for (int j = currRow; j <= 2 * n; ++j)
						rowOffsets[j] = offset;
					currRow = 2 * n + 1;
				}

				colIndices.push_back(neighbourSets[k][i]);
				values.push_back(0);
				++offset;
			}
			for (int i = currRow; i < rowOffsets.size(); ++i)
				rowOffsets[i] = offset;
		}
		else
		{
			int offset = 0;
			int currRow = 0;
			for (int i = 0; i < I.size(); ++i)
			{
				if (currRow <= I[i])
				{
					for (int j = currRow; j <= I[i]; ++j)
						rowOffsets[j] = offset;
					currRow = I[i] + 1;
				}

				colIndices.push_back(J[i] + n);
				values.push_back(0);
				++offset;
			}

			for (int i = 0; i < n; ++i)
			{
				if (currRow <= i + n)
				{
					for (int j = currRow; j <= i + n; ++j)
						rowOffsets[j] = offset;
					currRow = i + n + 1;
				}

				colIndices.push_back(i);
				values.push_back(kWeight[i]);
				++offset;
			}

			for (int i = 0; i < neighbourSets[k].size(); ++i)
			{
				if (currRow <= 2 * n)
				{
					for (int j = currRow; j <= 2 * n; ++j)
						rowOffsets[j] = offset;
					currRow = 2 * n + 1;
				}

				colIndices.push_back(neighbourSets[k][i]);
				values.push_back(0);
				++offset;
			}
			for (int i = currRow; i < rowOffsets.size(); ++i)
				rowOffsets[i] = offset;
		}
				
		dijkstraShortestPath(g, 2 * n + 1, 2 * n, distances, parents[k], feExternalParams);

		for (int i = 0; i < n; ++i)
		{
			// here distances for unreachable vertices are std::numeric_limits<long long int>::max(), so here set totalDist[i] as std::numeric_limits<long long int>::max() if
			// distance[i] is std::numeric_limits<long long int>::max() !!!
			// Also if both totalDist[i] and distance[i] are not std::numeric_limits<long long int>::max(), they can be added !!!
			// that's what the else branch does !!!
			if (distances[i] == std::numeric_limits<long long int>::max())
				totalDist[i] = std::numeric_limits<long long int>::max();
			else if (totalDist[i] != std::numeric_limits<long long int>::max())
				totalDist[i] += distances[i];
		}

		// punish v in neighbour set
		for (int i = 0; i < neighbourSets[k].size(); ++i)
		{
			// only if totalDist[neighbourSets[k][i]] is not std::numeric_limits<long long int>::max(), weight[neighbourSets[k][i]] can be added to it !!!
			if (totalDist[neighbourSets[k][i]] != std::numeric_limits<long long int>::max())
				totalDist[neighbourSets[k][i]] += weight[neighbourSets[k][i]];
		}

		colIndices.clear();
		values.clear();
	}

	for (int i = 0; i < avoidanceSet.size(); ++i)
		totalDist[avoidanceSet[i]] = std::numeric_limits<long long int>::max();

	minDist = std::numeric_limits<long long int>::max();
	std::vector<int> minList;
	for (int i = 0; i < totalDist.size(); ++i)
	{
		if (totalDist[i] < minDist)
		{
			minDist = totalDist[i];
			minList.clear();
			minList.push_back(i);
		}
		else if (totalDist[i] == minDist)
			minList.push_back(i);
	}
	
	boost::variate_generator<boost::mt19937&, boost::uniform_int<> > uiForUVertex(rng, boost::uniform_int<>(0, static_cast<int>(minList.size()) - 1));
	int uVertex = minList[uiForUVertex()];

	vpath.resize(neighbourSets.size());
	longvpath.resize(neighbourSets.size());

	std::vector<int> allpaths(n, 0);
	std::vector<int> doubles(n, 0);
	for (int k = 0; k < neighbourSets.size(); ++k)
	{
		int curr = uVertex;
		// 2 * n is the source
		while (curr != -1)
		{
			vpath[k].push_back(curr);
			curr = parents[k][curr];
		}
		std::reverse(vpath[k].begin(), vpath[k].end());
		
		for (int i = 1; i < vpath[k].size(); i += 2)
			longvpath[k].push_back(vpath[k][i]);

		std::vector<int> tmp;
		for (int i = 3; i < static_cast<int>(vpath[k].size()) - 2; i += 2)
			tmp.push_back(vpath[k][i]);
		vpath[k] = tmp;

		for (int i = 0; i < vpath[k].size(); ++i)
		{
			doubles[vpath[k][i]] |= allpaths[vpath[k][i]];
			allpaths[vpath[k][i]] = 1;
		}
	}

	// in each path find the first occurence of a double. 
    // vertices before that stay in vpath, 
    // vertices after that are added to uTree until an existing uTree
    // vertex is reached
	
	uTree.assign(n, 0);
	uTree[uVertex] = 1;

	longuTree = uTree;

	for (int k = 0; k < neighbourSets.size(); ++k)
	{
		int idx = -1;
		std::vector<int> tmp;
		for (int i = 0; i < vpath[k].size(); ++i)
			tmp.push_back(doubles[vpath[k][i]]);

		for (int i = 0; i < tmp.size(); ++i)
		{
			if (tmp[i] != 0)
			{
				idx = i;
				break;
			}
		}

		if (idx != -1)
		{
			std::vector<int> upath;
			for (int i = idx; i < vpath[k].size(); ++i)
				upath.push_back(vpath[k][i]);

			int idx2 = -1;
			std::vector<int> tmp2;
			for (int i = 0; i < upath.size(); ++i)
				tmp2.push_back(uTree[upath[i]]);

			for (int i = 0; i < tmp2.size(); ++i)
			{
				if (tmp2[i] != 0)
				{
					idx2 = i;
					break;
				}
			}

			if (idx2 != -1)
			{
				for (int i = 0; i <= idx2 - 1; ++i)
					uTree[upath[i]] = 1;
			}
			else
			{
				for (int i = 0; i < upath.size(); ++i)
					uTree[upath[i]] = 1;
			}

			for (int i = 0; i < upath.size(); ++i)
				longuTree[upath[i]] = 1;

			std::vector<int> tmp3;
			for (int i = 0; i <= idx - 1; ++i)
				tmp3.push_back(vpath[k][i]);

			vpath[k] = tmp3;
		}
	}

	std::vector<int> longuTreeIndex;
	for (int i = 0; i < longuTree.size(); ++i)
	{
		if (longuTree[i] != 0)
			longuTreeIndex.push_back(i);
	}

	longuTree = longuTreeIndex;
			
	std::vector<int> uTreeIndex;
	for (int i = 0; i < uTree.size(); ++i)
	{
		if (uTree[i] != 0)
			uTreeIndex.push_back(i);
	}

	uTree = uTreeIndex;
}

void minimizeChainWidthWithPaths(const compressed_matrix::CompressedMatrix<int>& Q, const compressed_matrix::CompressedMatrix<int>& A, const std::vector<int>& I, const std::vector<int>& J, 
								 const std::vector<std::vector<int> >& vertexEmbeddingConst, const find_embedding_::FindEmbeddingExternalParams& feExternalParams, const FindEmbeddingInternalParams& feInternalParams,
								 boost::mt19937& rng, std::vector<std::vector<int> >& bestEmbedding)
{
	std::vector<std::vector<int> > vertexEmbedding = vertexEmbeddingConst;

	int m = Q.numRows();
	int n = A.numRows();
	
	std::vector<std::vector<std::vector<int> > > edgePaths(m, std::vector<std::vector<int> >(m));
	std::vector<int> rootVertex(m, 0);

	bestEmbedding = vertexEmbedding;

	int bestWidth = 0;
	
	for (int j = 0; j < n; ++j)
	{
		int tmp = 0;
		for (int i = 0; i < m; ++i)
			tmp += vertexEmbedding[i][j];
		bestWidth = std::max(bestWidth, tmp);
	}

	int bestChainSize = 0;
	int bestNumMaxChains = 0;
	int bestEmbeddingSum = 0;

	for (int i = 0; i < m; ++i)
	{
		int tmp = std::accumulate(vertexEmbedding[i].begin(), vertexEmbedding[i].end(), 0);
		bestEmbeddingSum += tmp;
		if (bestChainSize < tmp)
		{
			bestChainSize = tmp;
			bestNumMaxChains = 1;
		}
		else if (bestChainSize == tmp)
			++bestNumMaxChains;
	}

	if (feExternalParams.verbose >= 1)
	{
		std::stringstream ss;
		ss << "max chain size = " << bestChainSize << ", num max chains = " << bestNumMaxChains << ", qubits used = " << bestEmbeddingSum << "\n";
		feExternalParams.localInteractionPtr->displayOutput(ss.str());
	}
	
	for (int u = 0; u < m; ++u)
	{
		std::vector<int> neighbourhood(Q.colIndices().begin() + Q.rowOffsets()[u], Q.colIndices().begin() + Q.rowOffsets()[u + 1]);

		std::vector<std::vector<int> > neighbourSets(neighbourhood.size());

		for (int i = 0; i < neighbourhood.size(); ++i)
			for (int j = 0; j < n; ++j)
			{
				if (vertexEmbedding[neighbourhood[i]][j] != 0)
					neighbourSets[i].push_back(j);
			}

		long long int minDist;
		std::vector<int> uChain;
		std::vector<std::vector<int> > vpath;
		std::vector<std::vector<int> > longvpath;
		std::vector<int> longuTree;

		if (!neighbourhood.empty())
		{
			for (int i = 0; i < vertexEmbedding[u].size(); ++i)
				vertexEmbedding[u][i] = 0;

			std::vector<int> colSumVertexEmbedding(n, 0);
			for (int j = 0; j < n; ++j)
				for (int i = 0; i < m; ++i)
					colSumVertexEmbedding[j] += vertexEmbedding[i][j];

			std::vector<long long int> weight(n);
			std::vector<int> fullBags;
			for (int i = 0; i < colSumVertexEmbedding.size(); ++i)
			{
				weight[i] = (1LL << (colSumVertexEmbedding[i] * feInternalParams.alpha));
				if (colSumVertexEmbedding[i] >= bestWidth)
					fullBags.push_back(i);
			}

			findClosestVertex(A, I, J, neighbourSets, fullBags, weight, rng, minDist, uChain, vpath, longvpath, longuTree, feExternalParams);

			if (minDist == std::numeric_limits<long long int>::max())
				return;
		}
		else
		{
			for (int i = 0; i < vertexEmbedding[u].size(); ++i)
			{
				if (vertexEmbedding[u][i] != 0)
					uChain.push_back(i);
			}

			if (!longvpath.empty())
				longvpath[0] = uChain;
			else
				longvpath.push_back(uChain);
		}
		
		for (int i = 0; i < neighbourhood.size(); ++i)
		{
			int pushIndex = std::max(bestChainSize - static_cast<int>(neighbourSets[i].size()) - 2, -1);
			pushIndex = std::min(pushIndex, static_cast<int>(vpath[i].size()) - 1);
			std::vector<int> uChainSorted = uChain;
			std::sort(uChainSorted.begin(), uChainSorted.end());
			std::vector<int> vpathPartSoted(vpath[i].begin() + (pushIndex + 1), vpath[i].end());
			std::sort(vpathPartSoted.begin(), vpathPartSoted.end());
			std::vector<int> tmp(uChainSorted.size() + vpathPartSoted.size());
			std::vector<int>::iterator it = std::set_union(uChainSorted.begin(), uChainSorted.end(), vpathPartSoted.begin(), vpathPartSoted.end(), tmp.begin());
			tmp.resize(it - tmp.begin());
			uChain = tmp;
			vpath[i].erase(vpath[i].begin() + (pushIndex + 1), vpath[i].end());
			for (int k = 0; k < vpath[i].size(); ++k)
				vertexEmbedding[neighbourhood[i]][vpath[i][k]] = 1;

			if (neighbourhood[i] < u)
			{
				std::vector<int> iEmbedding;
				for (int k = 0; k < vertexEmbedding[neighbourhood[i]].size(); ++k)
				{
					if (vertexEmbedding[neighbourhood[i]][k] != 0)
						iEmbedding.push_back(k);
				}

				int rootIndex;
				for (int k = 0; k < iEmbedding.size(); ++k)
				{
					if (iEmbedding[k] == rootVertex[neighbourhood[i]])
						rootIndex = k;
				}
				int endIndex;
				for (int k = 0; k < iEmbedding.size(); ++k)
				{
					if (iEmbedding[k] == longvpath[i][0])
						endIndex = k;
				}

				std::vector<long long int> distances;
				std::vector<int> parent;

				compressed_matrix::CompressedMatrix<long long int> g(static_cast<int>(iEmbedding.size()), static_cast<int>(iEmbedding.size()));
				// Add weighted edges
				std::vector<int>& rowOffsets = const_cast<std::vector<int>& >(g.rowOffsets());
				std::vector<int>& colIndices = const_cast<std::vector<int>& >(g.colIndices());
				std::vector<long long int>& values = const_cast<std::vector<long long int>& >(g.values());
				colIndices.reserve(iEmbedding.size() * iEmbedding.size());
				values.reserve(iEmbedding.size() * iEmbedding.size());
				int offset = 0;
				for (int k = 0; k < iEmbedding.size(); ++k)
				{
					rowOffsets[k] = offset;
					for (int t = 0; t < iEmbedding.size(); ++t)
					{
						int tmpAValue = A.get(iEmbedding[k], iEmbedding[t]);
						if (tmpAValue != 0)
						{
							colIndices.push_back(t);
							values.push_back(tmpAValue);
							++offset;
						}
					}
				}
				rowOffsets.back() = offset;

				dijkstraShortestPath(g, static_cast<int>(iEmbedding.size()), rootIndex, distances, parent, feExternalParams);

				std::vector<int> ipath;
				int curr = endIndex;
				while (curr != -1)
				{
					ipath.push_back(curr);
					curr = parent[curr];
				}
				std::reverse(ipath.begin(), ipath.end());

				std::vector<int> iEmbeddingipath;
				for (int k = 0; k < ipath.size(); ++k)
					iEmbeddingipath.push_back(iEmbedding[ipath[k]]);
				std::sort(iEmbeddingipath.begin(), iEmbeddingipath.end());
				std::vector<int> longvpathSorted = longvpath[i];
				std::sort(longvpathSorted.begin(), longvpathSorted.end());
				std::vector<int> tmp(iEmbeddingipath.size() + longvpath[i].size());
				std::vector<int>::iterator it = std::set_union(iEmbeddingipath.begin(), iEmbeddingipath.end(), longvpathSorted.begin(), longvpathSorted.end(), tmp.begin());
				tmp.resize(it - tmp.begin());
				edgePaths[neighbourhood[i]][u] = tmp;
			}
		}

		for (int i = 0; i < uChain.size(); ++i)
			vertexEmbedding[u][uChain[i]] = 1;

		rootVertex[u] = longvpath[0].back();

		int bagWidth = 0;
		for (int j = 0; j < n; ++j)
		{
			int tmp = 0;
			for (int i = 0; i < m; ++i)
				tmp += vertexEmbedding[i][j];
			bagWidth = std::max(bagWidth, tmp);
		}

		int maxChainSize = 0;
		int numMaxChains = 0;

		for (int i = 0; i < m; ++i)
		{
			int tmp = std::accumulate(vertexEmbedding[i].begin(), vertexEmbedding[i].end(), 0);
			if (maxChainSize < tmp)
			{
				maxChainSize = tmp;
				numMaxChains = 1;
			}
			else if (maxChainSize == tmp)
				++numMaxChains;
		}

		int embeddingSum = 0;
		for (int i = 0; i < m; ++i)
			embeddingSum += std::accumulate(vertexEmbedding[i].begin(), vertexEmbedding[i].end(), 0);
		if (bagWidth <= bestWidth && (maxChainSize < bestChainSize || (maxChainSize <= bestChainSize && numMaxChains < bestNumMaxChains)))
		{
			bestChainSize = maxChainSize;
			bestNumMaxChains = numMaxChains;
			bestWidth = bagWidth;
			bestEmbedding = vertexEmbedding;
			bestEmbeddingSum = embeddingSum;
		}
	}

	int notImproved = 0;
	int maxNoImprovement = 2;

	int out_while_cnt = 0;

	while (notImproved <= maxNoImprovement)
	{
		if (feExternalParams.verbose >= 1)
		{
			std::stringstream ss;
			ss << "max chain size = " << bestChainSize << ", num max chains = " << bestNumMaxChains << ", qubits used = " << bestEmbeddingSum << "\n";
			feExternalParams.localInteractionPtr->displayOutput(ss.str());
		}
		
		for (int u = 0; u < m; ++u)
		{
			std::vector<int> neighbourhood(Q.colIndices().begin() + Q.rowOffsets()[u], Q.colIndices().begin() + Q.rowOffsets()[u + 1]);

			long long int minDist;
			std::vector<int> uChain;
			std::vector<std::vector<int> > vpath;
			std::vector<std::vector<int> > longvpath;
			std::vector<int> longuTree;

			std::vector<std::vector<int> > neighbourSets(neighbourhood.size());

			if (!neighbourhood.empty())
			{
				for (int i = 0; i < vertexEmbedding[u].size(); ++i)
					vertexEmbedding[u][i] = 0;
				for (int k = 0; k < neighbourhood.size(); ++k)
				{
					int i = neighbourhood[k];
					std::vector<int> iNeighbours;
					int start = Q.rowOffsets()[i];
					int end = Q.rowOffsets()[i + 1];
					for (int j = start; j < end; ++j)
					{
						int t = Q.colIndices()[j];
						if (t != u)
							iNeighbours.push_back(t);
					}
								
					std::vector<int> rowEmbedding(n, 0);
					for (int t = 0; t < iNeighbours.size(); ++t)
					{
						int j = iNeighbours[t];
						if (j < i)
						{
							for (int n = 0; n < edgePaths[j][i].size(); ++n)
								rowEmbedding[edgePaths[j][i][n]] |= vertexEmbedding[i][edgePaths[j][i][n]];
						}
						else
						{
							for (int n = 0; n < edgePaths[i][j].size(); ++n)
								rowEmbedding[edgePaths[i][j][n]] |= vertexEmbedding[i][edgePaths[i][j][n]];
						}
					}

					vertexEmbedding[i] = rowEmbedding;
				}

				for (int i = 0; i < neighbourhood.size(); ++i)
				{
					for (int j = 0; j < n; ++j)
					{
						if (vertexEmbedding[neighbourhood[i]][j] != 0)
							neighbourSets[i].push_back(j);
					}
				}
				std::vector<int> colSumVertexEmbedding(n, 0);
				for (int j = 0; j < n; ++j)
					for (int i = 0; i < m; ++i)
						colSumVertexEmbedding[j] += vertexEmbedding[i][j];

				std::vector<long long int> weight(n);
				std::vector<int> fullBags;
				for (int i = 0; i < colSumVertexEmbedding.size(); ++i)
				{
					weight[i] = (1LL << (colSumVertexEmbedding[i] * feInternalParams.alpha));
					if (colSumVertexEmbedding[i] >= bestWidth)
						fullBags.push_back(i);
				}

				findClosestVertex(A, I, J, neighbourSets, fullBags, weight, rng, minDist, uChain, vpath, longvpath, longuTree, feExternalParams);

				if (minDist == std::numeric_limits<long long int>::max())
					return;
			}
			else
			{
				for (int i = 0; i < vertexEmbedding[u].size(); ++i)
				{
					if (vertexEmbedding[u][i] != 0)
						uChain.push_back(i);
				}
				
				if (!longvpath.empty())
					longvpath[0] = uChain;
				else
					longvpath.push_back(uChain);
			}

			for (int i = 0; i < neighbourhood.size(); ++i)
			{
				int pushIndex = std::max(bestChainSize - static_cast<int>(neighbourSets[i].size()) - 2, -1);
				pushIndex = std::min(pushIndex, static_cast<int>(vpath[i].size()) - 1);
				std::vector<int> uChainSorted = uChain;
				std::sort(uChainSorted.begin(), uChainSorted.end());
				std::vector<int> vpathPartSorted(vpath[i].begin() + (pushIndex + 1), vpath[i].end());
				std::sort(vpathPartSorted.begin(), vpathPartSorted.end());
				std::vector<int> tmp(uChainSorted.size() + vpathPartSorted.size());
				std::vector<int>::iterator it = std::set_union(uChainSorted.begin(), uChainSorted.end(), vpathPartSorted.begin(), vpathPartSorted.end(), tmp.begin());
				tmp.resize(it - tmp.begin());
				uChain = tmp;
				vpath[i].erase(vpath[i].begin() + (pushIndex + 1), vpath[i].end());
				for (int k = 0; k < vpath[i].size(); ++k)
					vertexEmbedding[neighbourhood[i]][vpath[i][k]] = 1;
								
				std::vector<int> iEmbedding;
				for (int k = 0; k < vertexEmbedding[neighbourhood[i]].size(); ++k)
				{
					if (vertexEmbedding[neighbourhood[i]][k] != 0)
						iEmbedding.push_back(k);
				}

				int rootIndex;
				for (int k = 0; k < iEmbedding.size(); ++k)
				{
					if (iEmbedding[k] == rootVertex[neighbourhood[i]])
						rootIndex = k;
				}

				int endIndex;
				for (int k = 0; k < iEmbedding.size(); ++k)
				{
					if (iEmbedding[k] == longvpath[i][0])
						endIndex = k;
				}

				std::vector<long long int> distances;
				std::vector<int> parent;

				compressed_matrix::CompressedMatrix<long long int> g(static_cast<int>(iEmbedding.size()), static_cast<int>(iEmbedding.size()));
				// Add weighted edges
				std::vector<int>& rowOffsets = const_cast<std::vector<int>& >(g.rowOffsets());
				std::vector<int>& colIndices = const_cast<std::vector<int>& >(g.colIndices());
				std::vector<long long int>& values = const_cast<std::vector<long long int>& >(g.values());
				colIndices.reserve(iEmbedding.size() * iEmbedding.size());
				values.reserve(iEmbedding.size() * iEmbedding.size());
				int offset = 0;
				for (int k = 0; k < iEmbedding.size(); ++k)
				{
					rowOffsets[k] = offset;
					for (int t = 0; t < iEmbedding.size(); ++t)
					{
						int tmpAValue = A.get(iEmbedding[k], iEmbedding[t]);
						if (tmpAValue != 0)
						{
							colIndices.push_back(t);
							values.push_back(tmpAValue);
							++offset;
						}
					}
				}
				rowOffsets.back() = offset;

				dijkstraShortestPath(g, static_cast<int>(iEmbedding.size()), rootIndex, distances, parent, feExternalParams);

				std::vector<int> ipath;
				int curr = endIndex;
				while (curr != -1)
				{
					ipath.push_back(curr);
					curr = parent[curr];
				}
				std::reverse(ipath.begin(), ipath.end());
				
				std::vector<int> iEmbeddingipath;
				for (int k = 0; k < ipath.size(); ++k)
					iEmbeddingipath.push_back(iEmbedding[ipath[k]]);

				std::sort(iEmbeddingipath.begin(), iEmbeddingipath.end());
				std::vector<int> longvpathSorted = longvpath[i];
				std::sort(longvpathSorted.begin(), longvpathSorted.end());
				std::vector<int> tmp2(iEmbeddingipath.size() + longvpath[i].size());
				std::vector<int>::iterator it2 = std::set_union(iEmbeddingipath.begin(), iEmbeddingipath.end(), longvpathSorted.begin(), longvpathSorted.end(), tmp2.begin());
				tmp2.resize(it2 - tmp2.begin());
				
				if (u < neighbourhood[i])
					edgePaths[u][neighbourhood[i]] = tmp2;
				else
					edgePaths[neighbourhood[i]][u] = tmp2;
			}

			for (int i = 0; i < uChain.size(); ++i)
				vertexEmbedding[u][uChain[i]] = 1;

			rootVertex[u] = longvpath[0].back();

			int bagWidth = 0;
			for (int j = 0; j < n; ++j)
			{
				int tmp = 0;
				for (int i = 0; i < m; ++i)
					tmp += vertexEmbedding[i][j];
				bagWidth = std::max(bagWidth, tmp);
			}
		
			int maxChainSize = 0;
			int numMaxChains = 0;

			for (int i = 0; i < m; ++i)
			{
				int tmp = std::accumulate(vertexEmbedding[i].begin(), vertexEmbedding[i].end(), 0);
				if (maxChainSize < tmp)
				{
					maxChainSize = tmp;
					numMaxChains = 1;
				}
				else if (maxChainSize == tmp)
					++numMaxChains;
			}

			int embeddingSum = 0;
			for (int i = 0; i < m; ++i)
				embeddingSum += std::accumulate(vertexEmbedding[i].begin(), vertexEmbedding[i].end(), 0);

			if (bagWidth <= bestWidth && (maxChainSize < bestChainSize || (maxChainSize <= bestChainSize && numMaxChains < bestNumMaxChains) || (maxChainSize <= bestChainSize && numMaxChains <= bestNumMaxChains && embeddingSum < bestEmbeddingSum)))
			{
				notImproved = 0;
				bestChainSize = maxChainSize;
				bestNumMaxChains = numMaxChains;
				bestWidth = bagWidth;
				bestEmbedding = vertexEmbedding;
				bestEmbeddingSum = embeddingSum;
			}
		}

		++notImproved;
		++out_while_cnt;
	}
}

void initialVarOrder(const compressed_matrix::CompressedMatrix<int>& Q, const FindEmbeddingInternalParams& feInternalParams, int initialQ, boost::mt19937& rng, std::vector<int>& orderQ)
{
	RandomNumberGenerator randomNumberGenerator(rng);
	int m = Q.numRows();

	if (feInternalParams.order == "bfs")
	{
		std::vector<int> order;
		bool hasUnreachable;
		bfs(Q, initialQ, order, hasUnreachable, rng);
		if (hasUnreachable)
		{
			for (int i = 0; i < m; ++i)
				orderQ[i] = i;
			std::random_shuffle(orderQ.begin(), orderQ.end(), randomNumberGenerator);
		}
		else
		{
			for (int i = 0; i < order.size(); ++i)
				orderQ[order[i]] = i;
		}
	}
	else if (feInternalParams.order == "random")
	{
		for (int i = 0; i < m; ++i)
			orderQ[i] = i;
		std::random_shuffle(orderQ.begin(), orderQ.end(), randomNumberGenerator);
	}
	else if (feInternalParams.order == "degree")
	{
		std::vector<std::pair<int, int> > colSum(m);
		for (int j = 0; j < m; ++j)
		{
			colSum[j].first = 0;
			colSum[j].second = j;
			for (int i = 0; i < m; ++i)
				colSum[j].first += Q.get(i, j);
		}

		// sort in descending order, if same value, order it by index (to be the same as MATLAB sort)
		std::sort(colSum.begin(), colSum.end(), compareColSum);
		for (int i = 0; i < m; ++i)
			orderQ[i] = colSum[i].second;
	}
	else if (feInternalParams.order == "dfs")
	{
		std::vector<int> order;
		bool hasUnreachable;
		dfs(Q, initialQ, order, hasUnreachable);
		std::set<int> orderSet(order.begin(), order.end());
		int index = 0;
		for (int i = 0; i < m; ++i)
		{
			if (orderSet.find(i) == orderSet.end())
				orderQ[index++] = i;
		}
		for (int i = 0; i < order.size(); ++i)
			orderQ[index++] = order[i];
	}
}

int vertexAdditionHeuristic(const compressed_matrix::CompressedMatrix<int>& QInput, const compressed_matrix::CompressedMatrix<int>& A, const std::vector<int>& I, const std::vector<int>& J,
							const find_embedding_::FindEmbeddingExternalParams& feExternalParams, const FindEmbeddingInternalParams& feInternalParams, const std::chrono::high_resolution_clock::time_point& startTime, boost::mt19937& rng,
							std::vector<std::vector<int> >& vertexBags, std::vector<std::vector<int> >& vertexChains)
{
	int isInterrupted = 0;

	RandomNumberGenerator randomNumberGenerator(rng);
	
	int m = QInput.numRows();
	int n = A.numRows();


	std::vector<int> rp;
	for (int i = 0; i < m; ++i)
		rp.push_back(i);
	std::random_shuffle(rp.begin(), rp.end(), randomNumberGenerator);

	std::map<std::pair<int, int>, int> QMap;
	std::vector<int> rpIndexMapping(m);
	for (int i = 0; i < m; ++i)
		rpIndexMapping[rp[i]] = i;

	for (int i = 0; i < m; ++i)
	{
		int start = QInput.rowOffsets()[i];
		int end = QInput.rowOffsets()[i + 1];
		for (int j = start; j < end; ++j)
			QMap.insert(std::make_pair(std::make_pair(rpIndexMapping[i], rpIndexMapping[QInput.colIndices()[j]]), QInput.values()[j]));
	}

	compressed_matrix::CompressedMatrix<int> Q(m, m, QMap);

	boost::variate_generator<boost::mt19937&, boost::uniform_int<> > uiForInitialQ(rng, boost::uniform_int<>(0, m - 1));
	int initialQ = 0;
	std::vector<int> orderQ(m);
	
	initialVarOrder(Q, feInternalParams, initialQ, rng, orderQ);

	boost::variate_generator<boost::mt19937&, boost::uniform_int<> > uiForInitialA(rng, boost::uniform_int<>(0, n - 1));
	int initialA = uiForInitialA();

	std::vector<std::vector<int> > vertexEmbedding(m, std::vector<int>(n, 0));
	vertexEmbedding[orderQ[0]][initialA] = 1;

	for (int k = 1; k < m; ++k)
	{
		int u = orderQ[k];
		std::vector<int> neighbourhood;
		std::set<int> tmpSet(Q.colIndices().begin() + Q.rowOffsets()[u], Q.colIndices().begin() + Q.rowOffsets()[u + 1]);

		for (int i = 0; i < k; ++i)
		{
			if (tmpSet.find(orderQ[i]) != tmpSet.end())
				neighbourhood.push_back(orderQ[i]);
		}
		// In MATLAB, intersect sorts the result, so here also sort the neighbourhood
		std::sort(neighbourhood.begin(), neighbourhood.end());

		std::vector<std::vector<int> > neighbourSets(neighbourhood.size());

		for (int i = 0; i < neighbourhood.size(); ++i)
			for (int j = 0; j < n; ++j)
			{
				if (vertexEmbedding[neighbourhood[i]][j] != 0)
					neighbourSets[i].push_back(j);
			}

		std::vector<int> colSumVertexEmbedding(n, 0);
		for (int j = 0; j < n; ++j)
			for (int i = 0; i < m; ++i)
				colSumVertexEmbedding[j] += vertexEmbedding[i][j];

		std::vector<long long int> weight(n);
		std::vector<int> fullBags;
		for (int i = 0; i < colSumVertexEmbedding.size(); ++i)
		{
			weight[i] = (1LL << (colSumVertexEmbedding[i] * feInternalParams.alpha));
			if (colSumVertexEmbedding[i] >= feInternalParams.maxWidth)
				fullBags.push_back(i);
		}

		long long int minDist;
		std::vector<int> uChain;
		std::vector<std::vector<int> > vpath;
		std::vector<std::vector<int> > longvpath;
		std::vector<int> longuTree;

		if (!neighbourhood.empty())
		{
			try
			{
				findClosestVertex(A, I, J, neighbourSets, fullBags, weight, rng, minDist, uChain, vpath, longvpath, longuTree, feExternalParams);
			}
			catch (const find_embedding_::ProblemCancelledException&)
			{
				isInterrupted = 1;
				break;
			}

			if (minDist == std::numeric_limits<long long int>::max())
				return 0;
		}
		else
		{
			long long int minWeight = std::numeric_limits<long long int>::max();
			std::vector<int> minList;
			for (int i = 0; i < weight.size(); ++i)
			{
				if (weight[i] < minWeight)
				{
					minWeight = weight[i];
					minList.clear();
					minList.push_back(i);
				}
				else if (weight[i] == minWeight)
					minList.push_back(i);
			}

			boost::variate_generator<boost::mt19937&, boost::uniform_int<> > uiForUChain(rng, boost::uniform_int<>(0, static_cast<int>(minList.size()) - 1));
			uChain.push_back(minList[uiForUChain()]);
		}

		for (int i = 0; i < uChain.size(); ++i)
			vertexEmbedding[u][uChain[i]] = 1;

		for (int i = 0; i < neighbourhood.size(); ++i)
			for (int j = 0; j < vpath[i].size(); ++j)
				vertexEmbedding[neighbourhood[i]][vpath[i][j]] = 1;
	}

	std::vector<std::vector<int> > bestEmbedding = vertexEmbedding;
	int bestWidth = 0;
	int bestNumMaxBags = 0;

	if (!isInterrupted)
	{
		for (int j = 0; j < n; ++j)
		{
			int tmp = 0;
			for (int i = 0; i < m; ++i)
				tmp += vertexEmbedding[i][j];
			if (bestWidth < tmp)
			{
				bestWidth = tmp;
				bestNumMaxBags = 1;
			}
			else if (bestWidth == tmp)
				++bestNumMaxBags;
		}
	}

	int bestChainSize = 0;
	int bestNumMaxChains = 0;

	if (!isInterrupted)
	{
		for (int i = 0; i < m; ++i)
		{
			int tmp = std::accumulate(vertexEmbedding[i].begin(), vertexEmbedding[i].end(), 0);
			if (bestChainSize < tmp)
			{
				bestChainSize = tmp;
				bestNumMaxChains = 1;
			}
			else if (bestChainSize == tmp)
				++bestNumMaxChains;
		}
	}

	// shuffle variables
	int shuffle = 0;
	while (!isInterrupted && shuffle < feInternalParams.maxShuffle)
	{
		++shuffle;
		if (shuffle > 1)
		{
			// restart with best so far, shuffling variables.
			if (feExternalParams.verbose >= 1)
				feExternalParams.localInteractionPtr->displayOutput("shuffling variables...\n");

			initialQ = uiForInitialQ();
			initialVarOrder(Q, feInternalParams, initialQ, rng, orderQ);
			vertexEmbedding = bestEmbedding;
		}

		int notImproved = 1;
		int maxNoImprovement = feExternalParams.maxNoImprovement;
		int rounds = 1;
		if (feExternalParams.fastEmbedding && bestWidth == 1)
			rounds = feInternalParams.maxRounds;

		while (   !isInterrupted && notImproved <= maxNoImprovement && rounds < feInternalParams.maxRounds
			   && (static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0) < feExternalParams.timeout)
		{
			if (feExternalParams.verbose >= 1)
			{
				std::stringstream ss;
				ss << "max overfill = " << bestWidth << ", num max overfills = " << bestNumMaxBags << "\n";
				feExternalParams.localInteractionPtr->displayOutput(ss.str());
			}

			std::vector<std::vector<int> > previousEmbedding = vertexEmbedding;

			for (int k = 0; k < m; ++k)
			{
				int u = orderQ[k];

				std::vector<int> neighbourhood(Q.colIndices().begin() + Q.rowOffsets()[u], Q.colIndices().begin() + Q.rowOffsets()[u + 1]);
			
				std::vector<std::vector<int> > neighbourSets(neighbourhood.size());

				for (int i = 0; i < neighbourhood.size(); ++i)
					for (int j = 0; j < n; ++j)
					{
						if (vertexEmbedding[neighbourhood[i]][j] != 0)
							neighbourSets[i].push_back(j);
					}

				for (int i = 0; i < vertexEmbedding[u].size(); ++i)
					vertexEmbedding[u][i] = 0;

				std::vector<int> colSumVertexEmbedding(n, 0);
				for (int j = 0; j < n; ++j)
					for (int i = 0; i < m; ++i)
						colSumVertexEmbedding[j] += vertexEmbedding[i][j];

				std::vector<long long int> weight(n);
				std::vector<int> fullBags;
				for (int i = 0; i < colSumVertexEmbedding.size(); ++i)
				{
					weight[i] = (1LL << (colSumVertexEmbedding[i] * feInternalParams.alpha));
					if (colSumVertexEmbedding[i] >= feInternalParams.maxWidth)
						fullBags.push_back(i);
				}

				long long int minDist;
				std::vector<int> uChain;
				std::vector<std::vector<int> > vpath;
				std::vector<std::vector<int> > longvpath;
				std::vector<int> longuTree;

				if (!neighbourhood.empty())
				{
					try
					{
						findClosestVertex(A, I, J, neighbourSets, fullBags, weight, rng, minDist, uChain, vpath, longvpath, longuTree, feExternalParams);
					}
					catch (const find_embedding_::ProblemCancelledException&)
					{
						isInterrupted = 1;
						break;
					}

					if (minDist == std::numeric_limits<long long int>::max())
						return 0;
				}
				else
				{
					long long int minWeight = std::numeric_limits<long long int>::max();
					std::vector<int> minList;
					for (int i = 0; i < weight.size(); ++i)
					{
						if (weight[i] < minWeight)
						{
							minWeight = weight[i];
							minList.clear();
							minList.push_back(i);
						}
						else if (weight[i] == minWeight)
							minList.push_back(i);
					}

					boost::variate_generator<boost::mt19937&, boost::uniform_int<> > uiForUChain(rng, boost::uniform_int<>(0, static_cast<int>(minList.size()) - 1));
					uChain.push_back(minList[uiForUChain()]);
				}

				for (int i = 0; i < uChain.size(); ++i)
					vertexEmbedding[u][uChain[i]] = 1;
			
				for (int i = 0; i < neighbourhood.size(); ++i)
					for (int j = 0; j < vpath[i].size(); ++j)
						vertexEmbedding[neighbourhood[i]][vpath[i][j]] = 1;
			
				int bagWidth = 0;
				int numMaxBags = 0;

				for (int j = 0; j < n; ++j)
				{
					int tmp = 0;
					for (int i = 0; i < m; ++i)
						tmp += vertexEmbedding[i][j];
					if (bagWidth < tmp)
					{
						bagWidth = tmp;
						numMaxBags = 1;
					}
					else if (bagWidth == tmp)
						++numMaxBags;
				}

				int maxChainSize = 0;
				int numMaxChains = 0;

				for (int i = 0; i < m; ++i)
				{
					int tmp = std::accumulate(vertexEmbedding[i].begin(), vertexEmbedding[i].end(), 0);
					if (maxChainSize < tmp)
					{
						maxChainSize = tmp;
						numMaxChains = 1;
					}
					else if (maxChainSize == tmp)
						++numMaxChains;
				}

				bool currentImprovement;
				if (bestWidth == 1)
				{
					currentImprovement = (bagWidth == 1 && (maxChainSize < bestChainSize || (maxChainSize <= bestChainSize && numMaxChains < bestNumMaxChains)));
					maxNoImprovement = 2;
				}
				else
					currentImprovement = (bagWidth < bestWidth || (bagWidth <= bestWidth && numMaxBags < bestNumMaxBags) || (bagWidth <= bestWidth && numMaxBags <= bestNumMaxBags && maxChainSize < bestChainSize));
			
				if (currentImprovement)
				{
					notImproved = 0;
					bestWidth = bagWidth;
					bestNumMaxBags = numMaxBags;
					bestChainSize = maxChainSize;
					bestNumMaxChains = numMaxChains;
					bestEmbedding = vertexEmbedding;
				}

				if (rounds <= 1)
				{
					if (bagWidth > bestWidth || (bagWidth >= bestWidth && numMaxBags > bestNumMaxBags))
						vertexEmbedding = bestEmbedding;
				}
			}

			++notImproved;

			if (previousEmbedding == vertexEmbedding)
			{
				// no change from previous cycle
				notImproved = maxNoImprovement + 1;
				shuffle = feInternalParams.maxShuffle;
			}

			++rounds;
			if (feExternalParams.fastEmbedding && bestWidth == 1)
				rounds = feInternalParams.maxRounds;
		}

		// don't bother shuffling vertices here, you're not even close or bestWidth == 1 (embedding found).
		if (bestWidth == 1 || bestWidth > 2 || (bestWidth == 2 && bestNumMaxBags > 5) )
			shuffle = feInternalParams.maxShuffle;
	}

	if (isInterrupted && bestWidth == 1)
	{
		if (feExternalParams.verbose >= 1)
			feExternalParams.localInteractionPtr->displayOutput("Embedding found.\n");
	}

	if (   !isInterrupted && bestWidth == 1 && !feExternalParams.fastEmbedding
		&& (static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0) < feExternalParams.timeout)
	{
		if (feExternalParams.verbose >= 1)
			feExternalParams.localInteractionPtr->displayOutput("Embedding found. Minimizing chains...\n");
		std::vector<std::vector<int> > bestEmbeddingConst = bestEmbedding;
		try
		{
			minimizeChainWidthWithPaths(Q, A, I, J, bestEmbeddingConst, feExternalParams, feInternalParams, rng, bestEmbedding);
		}
		catch (const find_embedding_::ProblemCancelledException&)
		{
			isInterrupted = 1;
		}
	}

	vertexBags.resize(n);
	vertexChains.resize(m);	

	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < bestEmbedding.size(); ++j)
		{
			if (bestEmbedding[j][i] != 0)
				vertexBags[i].push_back(rp[j]);
		}
	}

	for (int i = 0; i < m; ++i)
	{
		for (int j = 0; j < bestEmbedding[i].size(); ++j)
		{
			if (bestEmbedding[i][j] != 0)
				vertexChains[rp[i]].push_back(j);
		}
	}

	return isInterrupted;
}

// assume A and Q are both symmetric
bool verifyEmbedding(const compressed_matrix::CompressedMatrix<int>& A, const compressed_matrix::CompressedMatrix<int>& Q, const std::vector<std::vector<int> >& embeddings)
{
	int numVertices = Q.numRows();

	// check if dimension match
	if (numVertices != static_cast<int>(embeddings.size()))
		return false;

	// make sure chains don't overlap
	std::set<int> usedQubits;
	for (int i = 0; i < numVertices; ++i)
	{
		for (int j = 0; j < embeddings[i].size(); ++j)
		{
			if (usedQubits.find(embeddings[i][j]) != usedQubits.end())
				return false;

			usedQubits.insert(embeddings[i][j]);
		}
	}

	// check whether a target node is mapped to a connected component
	for (int i = 0; i < numVertices; ++i)
	{
		if (embeddings[i].empty())
			return false;

		int start = embeddings[i][0];
		std::vector<int> order;
		bool dummyVariable;
		dfs(A, start, order, dummyVariable);
		std::sort(order.begin(), order.end());
		for (int j = 0; j < embeddings[i].size(); ++j)
		{
			if (!std::binary_search(order.begin(), order.end(), embeddings[i][j]))
				return false;
		}
	}

	// check edges
	for (int i = 0; i < numVertices; ++i)
	{
		int start = Q.rowOffsets()[i];
		int end = Q.rowOffsets()[i + 1];
		if (start == end)
			continue;

		bool flag = false;
		for (int k = start; k < end && !flag; ++k)
		{
			int j = Q.colIndices()[k];
			for (int r = 0; r < embeddings[i].size() && !flag; ++r)
			{
				for (int t = 0; t < embeddings[j].size() && !flag; ++t)
				{
					if (A.get(embeddings[i][r], embeddings[j][t]))
						flag = true;
				}
			}
		}

		if (!flag)
			return false;
	}

	return true;
}

bool compareVectorSize(const std::vector<int>& a, const std::vector<int>& b)
{
	return a.size() > b.size();
}

} // anonymous namespace

namespace find_embedding_
{

FindEmbeddingExternalParams::FindEmbeddingExternalParams()
{
	verbose = DEFAULT_FE_VERBOSE;
	timeout = DEFAULT_FE_TIMEOUT;
	tries = DEFAULT_TRIES;
	maxNoImprovement = DEFAULT_MAXNOIMPROVEMENT;
	fastEmbedding = DEFAULT_FASTEMBEDDING;		
	randomSeed = static_cast<unsigned int>(std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() % std::numeric_limits<unsigned int>::max());
}

// Q and A are not necessarily symmetric, and it may include diagonal elements !!!
// but its values are all 1s
std::vector<std::vector<int> > findEmbedding(const compressed_matrix::CompressedMatrix<int>& Q, const compressed_matrix::CompressedMatrix<int>& A, const FindEmbeddingExternalParams& feExternalParams)
{
	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

	// check parameters
	if (Q.numRows() != Q.numCols())
		throw find_embedding_::FindEmbeddingException("Q must be square matrix");

	if (A.numRows() != A.numCols())
		throw find_embedding_::FindEmbeddingException("A must be square matrix");

	if (!feExternalParams.localInteractionPtr)
		throw find_embedding_::FindEmbeddingException("localInteractionPtr parameter is NULL");

	if (!(feExternalParams.maxNoImprovement >= 0))
		throw find_embedding_::FindEmbeddingException("max_no_improvement must be an integer >= 0");

	if (isNaN(feExternalParams.timeout))
			throw find_embedding_::FindEmbeddingException("timeout parameter is NaN");

	if (!(feExternalParams.timeout >= 0.0))
		throw find_embedding_::FindEmbeddingException("timeout parameter must be a number >= 0.0");

	if (!(feExternalParams.tries >= 0))
		throw find_embedding_::FindEmbeddingException("tries parameter must be an integer >= 0");

	if (!(feExternalParams.verbose == 0 || feExternalParams.verbose == 1))
		throw find_embedding_::FindEmbeddingException("verbose parameter must be an integer [0, 1]");

	FindEmbeddingInternalParams feInternalParams;

	boost::mt19937 rng(feExternalParams.randomSeed);

	boost::variate_generator<boost::mt19937&, boost::uniform_int<> > uiForAlpha(rng, boost::uniform_int<>(2, 10));

	// convert Q to symmetric matrix
	std::map<std::pair<int, int>, int> QAdjMap;
	int QNumRows = Q.numRows();
	for (int i = 0; i < QNumRows; ++i)
	{
		int start = Q.rowOffsets()[i];
		int end = Q.rowOffsets()[i + 1];
		for (int k = start; k < end; ++k)
		{
			int j = Q.colIndices()[k];
			// make QAdjMap symmetric
			QAdjMap.insert(std::make_pair(std::make_pair(i, j), 1));
			QAdjMap.insert(std::make_pair(std::make_pair(j, i), 1));
		}
	}
	compressed_matrix::CompressedMatrix<int> QAdj(QNumRows, QNumRows, QAdjMap);

	// convert A to symmetric matrix
	int ANumRows = A.numRows();
	std::map<std::pair<int, int>, int> AAdjMap;
	std::set<int> workingSet;
	for (int i = 0; i < ANumRows; ++i)
	{
		int start = A.rowOffsets()[i];
		int end = A.rowOffsets()[i + 1];
		for (int k = start; k < end; ++k)
		{
			int j = A.colIndices()[k];
			workingSet.insert(i);
			workingSet.insert(j);
			// make AAdjMap symmetric
			AAdjMap.insert(std::make_pair(std::make_pair(i, j), 1));
			AAdjMap.insert(std::make_pair(std::make_pair(j, i), 1));
		}
	}
	compressed_matrix::CompressedMatrix<int> AAdj(ANumRows, ANumRows, AAdjMap);

	// If AWorking's nnz == 0, the function should return an emtpy vector
	//This allows a case where unconnected problem can still be embedded.
	if (AAdj.nnz() == 0)
		return std::vector<std::vector<int> >();

	// compute the strongly connected components and sort those components by their size in non-increasing order
	std::vector<std::vector<int> > connectedComponents;
	stronglyConnectedComponents(QAdj, connectedComponents);
	std::sort(connectedComponents.begin(), connectedComponents.end(), compareVectorSize);

	bool success = true;
	int isInterrupted = 0;
	int numComponents = static_cast<int>(connectedComponents.size());
	int componentIndex = 0;
	std::vector<std::vector<int> > embeddings(QNumRows);

	while (   success && !isInterrupted && componentIndex < numComponents
		   && (static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0) < feExternalParams.timeout)
	{
		// bail if not enough target vertices remain
		if (connectedComponents[componentIndex].size() > workingSet.size())
		{
			if (feExternalParams.verbose >= 1)
			{
				std::ostringstream ss;
				ss << "Failed to find embedding. Current component has " << connectedComponents[componentIndex].size()
				  << " vertices but only " << workingSet.size() << " target vertices remain.\n";
				feExternalParams.localInteractionPtr->displayOutput(ss.str());
			}
			return std::vector<std::vector<int> >();
		}


		// construct QComponent
		std::vector<int> component = connectedComponents[componentIndex];
		int componentLen = static_cast<int>(component.size());
		std::vector<int> componentIndexMapping(QNumRows);
		for (int i = 0; i < componentLen; ++i)
			componentIndexMapping[component[i]] = i;

		std::set<int> componentSet(component.begin(), component.end());

		std::map<std::pair<int, int>, int> QComponentMap;
		for (std::map<std::pair<int, int>, int>::const_iterator it = QAdjMap.begin(), end = QAdjMap.end(); it != end; ++it)
		{
			int i = it->first.first;
			int j = it->first.second;
			if (componentSet.find(i) != componentSet.end() && componentSet.find(j) != componentSet.end())
				QComponentMap.insert(std::make_pair(std::make_pair(componentIndexMapping[i], componentIndexMapping[j]), 1));
		}
		compressed_matrix::CompressedMatrix<int> QComponent(componentLen, componentLen, QComponentMap);

		// construct AWorking
		std::vector<int> working;
		std::vector<int> workingIndexMapping(ANumRows);
		int newIndex = 0;
		for (std::set<int>::const_iterator cit = workingSet.begin(), end = workingSet.end(); cit != end; ++cit)
		{
			working.push_back(*cit);
			workingIndexMapping[*cit] = newIndex++;
		}

		std::vector<int> I;
		std::vector<int> J;
		std::map<std::pair<int, int>, int> AWorkingMap;
		for (std::map<std::pair<int, int>, int>::const_iterator it = AAdjMap.begin(), end = AAdjMap.end(); it != end; ++it)
		{
			int i = it->first.first;
			int j = it->first.second;
			if (workingSet.find(i) != workingSet.end() && workingSet.find(j) != workingSet.end())
			{
				I.push_back(workingIndexMapping[i]);
				J.push_back(workingIndexMapping[j]);
				AWorkingMap.insert(std::make_pair(std::make_pair(workingIndexMapping[i], workingIndexMapping[j]), 1));
			}
		}
		compressed_matrix::CompressedMatrix<int> AWorking(newIndex, newIndex, AWorkingMap);

		bool finished = false;
		int tryCount = 0;
		std::vector<std::vector<int> > vertexBags;
		std::vector<std::vector<int> > vertexChains;
		int W = std::numeric_limits<int>::max();

		while (   !finished && !isInterrupted && tryCount < feExternalParams.tries
			   && (static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count()) / 1000.0) < feExternalParams.timeout)
		{
			feInternalParams.alpha = uiForAlpha();
			std::vector<std::vector<int> > vertexBags1;
			std::vector<std::vector<int> > vertexChains1;
			if (feExternalParams.verbose >= 1)
			{
				std::stringstream ss;
				ss << "component " << componentIndex << ", try " << tryCount << ":\n";
				feExternalParams.localInteractionPtr->displayOutput(ss.str());
			}

			isInterrupted = vertexAdditionHeuristic(QComponent, AWorking, I, J, feExternalParams, feInternalParams, startTime, rng, vertexBags1, vertexChains1);

			if (!vertexBags1.empty())
			{
				std::vector<int> tmp;
				for (int i = 0; i < vertexBags1.size(); ++i)
					tmp.push_back(static_cast<int>(vertexBags1[i].size()));

				int W1 = *std::max_element(tmp.begin(), tmp.end());
				if (W1 < W)
				{
					W = W1;
					vertexBags = vertexBags1;
					vertexChains = vertexChains1;
					if (W1 == 1)
						finished = true;
				}
			}

			++tryCount;
		}

		if (isInterrupted)
		{
			if (feExternalParams.verbose >= 1)
				feExternalParams.localInteractionPtr->displayOutput("\nfind embedding interrupted by Ctrl-C.\n");
		}

		if (W > 1)
		{
			int targetWidth = 1;

			if (feExternalParams.verbose >= 1)
			{
				int cnt = 0;
				for (int i = 0; i < vertexBags.size(); ++i)
				{
					if (vertexBags[i].size() > targetWidth)
						++cnt;
				}

				std::stringstream ss;
				ss << "Failed to find embedding. Number of overfull qubits is " << cnt << "\n";
				feExternalParams.localInteractionPtr->displayOutput(ss.str());
			}
			embeddings.resize(0);
			success = false;
		}
		else
		{
			for (int i = 0; i < componentLen; ++i)
			{
				int variableOriginalIndex = component[i];
				for (int j = 0; j < vertexChains[i].size(); ++j)
				{
					int qubitOriginalIndex = working[vertexChains[i][j]];
					embeddings[variableOriginalIndex].push_back(qubitOriginalIndex);
					workingSet.erase(qubitOriginalIndex);
				}
			}
		}

		++componentIndex;
	}

	// verify embedding
	if (!verifyEmbedding(AAdj, QAdj, embeddings))
	{
		if (feExternalParams.verbose >= 1)
		{
			feExternalParams.localInteractionPtr->displayOutput("Failed to find embedding. Embeddings are invalid.\n");
			
		embeddings.clear();
		}
	}
	return embeddings;
}

} // namespace find_embedding
