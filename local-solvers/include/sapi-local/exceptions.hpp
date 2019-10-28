//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPILOCAL_EXCEPTIONS_HPP_INCLUDED
#define SAPILOCAL_EXCEPTIONS_HPP_INCLUDED

#include <stdexcept>
#include <string>

namespace sapilocal {

class Exception : public std::runtime_error {
protected:
  Exception(const std::string& msg) : std::runtime_error(msg) {}
};

class UnsupportedProblemTypeException : public Exception {
public:
  UnsupportedProblemTypeException() : Exception("Unsupported problem type") {}
};

class InvalidProblemException : public Exception {
public:
  InvalidProblemException(const std::string& msg) : Exception(msg) {}
};

class InvalidParameterException : public Exception {
public:
  InvalidParameterException(const std::string& msg) : Exception(msg) {}
};

class SolveException : public Exception {
public:
  SolveException(const std::string& msg): Exception(msg) {}
};

} // namespace sapilocal

#endif
