//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef FIND_EMBEDDING_HPP_INCLUDED
#define FIND_EMBEDDING_HPP_INCLUDED

#include <vector>
#include <memory>

#include "compressed_matrix.hpp"

namespace find_embedding_
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

struct FindEmbeddingExternalParams
{
	FindEmbeddingExternalParams();

	bool fastEmbedding;
	// actually not controled by user, not initialized here, but initialized in Python, MATLAB, C wrappers level
	LocalInteractionPtr localInteractionPtr;
	int maxNoImprovement;	
	unsigned int randomSeed;
	double timeout;
	int tries;
	int verbose;
};

class FindEmbeddingException
{
public:
	FindEmbeddingException(const std::string& m = "find embedding exception") : message(m) {}
	const std::string& what() const {return message;}
private:
	std::string message;
};

class ProblemCancelledException : public FindEmbeddingException
{
public:
	ProblemCancelledException(const std::string& m = "problem cancelled exception") : FindEmbeddingException(m) {}
	const std::string& what() const {return message;}
private:
	std::string message;
};

//std::vector<std::vector<int> > findEmbedding(const compressed_matrix::CompressedMatrix<int>& Q, const compressed_matrix::CompressedMatrix<int>& A, const FindEmbeddingExternalParams& feExternalParams);

std::vector<std::vector<int> > findEmbedding(const compressed_matrix::CompressedMatrix<int>& Q, const compressed_matrix::CompressedMatrix<int>& A, const FindEmbeddingExternalParams& feExternalParams);

} // namespace find_embedding_

#endif  // FIND_EMBEDDING_HPP_INCLUDED
