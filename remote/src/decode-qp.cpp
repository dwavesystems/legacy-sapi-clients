//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <vector>
#include <string>

#include <boost/foreach.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <exceptions.hpp>
#include <json.hpp>
#include <base64.hpp>
#include <coding.hpp>

using std::numeric_limits;
using std::string;
using std::vector;

using boost::numeric_cast;

using sapiremote::decodeBase64;
using sapiremote::DecodingException;

namespace {

const auto formatName = "qp";
namespace problemtypes {
const auto ising = "ising";
const auto qubo = "qubo";
}
const char unusedIsingVariable = 3;

namespace answerkeys {
const char* format = "format";
const char* activeVars = "active_variables";
const char* numVars = "num_variables";
const char* solutions = "solutions";
const char* energies = "energies";
const char* numOccurrences = "num_occurrences";
}

class MissingAnswerKeyException : public DecodingException {
public:
  MissingAnswerKeyException(const string& key) : DecodingException("missing value: " + key) {}
};

class BadAnswerTypeException : public DecodingException {
public:
  BadAnswerTypeException(const string& key) : DecodingException("bad value type: " + key) {}
};

class BadAnswerValueException : public DecodingException {
public:
  BadAnswerValueException(const string& key) : DecodingException("bad value: " + key) {}
};


template<typename T>
vector<T> decodeBinary(const json::Object& answer, const string& key) {
  try {
    auto bytes = decodeBase64(answer.at(key).getString());
    if (bytes.size() % sizeof(T) != 0) throw BadAnswerValueException(key);
    vector<T> v(bytes.size() / sizeof(T));
    std::memcpy(v.data(), bytes.data(), bytes.size());
    return v;
  } catch (json::TypeException&) {
    throw BadAnswerTypeException(key);
  } catch (std::out_of_range&) {
    throw MissingAnswerKeyException(key);
  }
}

vector<unsigned char> decodeSolutionBits(const json::Object& answer) {
  try {
    return decodeBase64(answer.at(answerkeys::solutions).getString());
  } catch (json::TypeException&) {
    throw BadAnswerTypeException(answerkeys::solutions);
  } catch (std::out_of_range&) {
    throw MissingAnswerKeyException(answerkeys::solutions);
  }
}

int decodeNumVars(const json::Object& answer) {
  try {
    auto n = numeric_cast<int>(answer.at(answerkeys::numVars).getInteger());
    if (n < 0) throw BadAnswerValueException(answerkeys::numVars);
    return n;
  } catch (json::TypeException&) {
    throw BadAnswerTypeException(answerkeys::numVars);
  } catch (std::out_of_range&) {
    throw MissingAnswerKeyException(answerkeys::numVars);
  } catch (boost::bad_numeric_cast&) {
    throw BadAnswerValueException(answerkeys::numVars);
  }
}

vector<char> decodeSolutions(const json::Object& answer, size_t numSols, char zero) {
  auto solBits = decodeSolutionBits(answer);
  auto activeVars = decodeBinary<int>(answer, answerkeys::activeVars);
  auto numActiveVars = activeVars.size();
  auto numVars = decodeNumVars(answer);

  auto last = -1;
  BOOST_FOREACH( auto av, activeVars ) {
    if (av <= last) throw BadAnswerValueException(answerkeys::activeVars);
    last = av;
  }
  if (last >= numVars) throw BadAnswerValueException(answerkeys::activeVars);

  if (numSols * ((numActiveVars + 7) / 8) != solBits.size()) throw BadAnswerValueException(answerkeys::solutions);
  if (numVars > 0 && numeric_limits<size_t>::max() / static_cast<unsigned>(numVars) < numSols) {
    throw DecodingException("solution data too large");
  }

  auto solutions = vector<char>(numSols * numVars, unusedIsingVariable);
  if (!solBits.empty()) {
    auto solIter = solutions.begin();
    auto solBitsIter = solBits.begin();
    auto bitIndex = 7;
    for (size_t i = 0; i < numSols; ++i) {
      BOOST_FOREACH(auto av, activeVars) {
        if (bitIndex < 0) {
          ++solBitsIter;
          bitIndex = 7;
        }
        solIter[av] = ((*solBitsIter >> bitIndex) & 1) ? 1 : zero;
        --bitIndex;
      }
      ++solBitsIter;
      bitIndex = 7;
      solIter += numVars;
    }
  }

  return solutions;
}

} // namespace {anonymous}

namespace sapiremote {

QpAnswer decodeQpAnswer(const std::string& problemType, const json::Object& answer) {
  char zero;
  if (problemType == problemtypes::ising) {
    zero = -1;
  } else if (problemType == problemtypes::qubo) {
    zero = 0;
  } else {
    throw DecodingException("invalid problem type for qp decoding");
  }

  try {
    if (answer.at(answerkeys::format).getString() != formatName) {
      throw DecodingException("invalid format name for qp decoding");
    }
  } catch (json::TypeException&) {
    throw BadAnswerTypeException(answerkeys::format);
  } catch (std::out_of_range&) {
    throw MissingAnswerKeyException(answerkeys::format);
  }

  QpAnswer qpAnswer;
  qpAnswer.energies = decodeBinary<double>(answer, answerkeys::energies);
  if (answer.find(answerkeys::numOccurrences) != answer.end()) {
    qpAnswer.numOccurrences = decodeBinary<int>(answer, answerkeys::numOccurrences);
    if (!qpAnswer.numOccurrences.empty() && qpAnswer.numOccurrences.size() != qpAnswer.energies.size()) {
      throw DecodingException("inconsistent energies and num_occurrences sizes");
    }
  }
  qpAnswer.solutions = decodeSolutions(answer, qpAnswer.energies.size(), zero);
  return qpAnswer;
}

} // namespace sapiremote
