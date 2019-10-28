//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef EXCEPTIONS_HPP_INCLUDED
#define EXCEPTIONS_HPP_INCLUDED

#include <stdexcept>
#include <string>

namespace sapiremote {

class Exception : public std::runtime_error {
protected:
  Exception(const std::string& msg) : runtime_error(msg) {}
};



class SolveException : public Exception {
protected:
  SolveException(const std::string& msg, int) : Exception(msg) {}
public:
  SolveException(const std::string& msg) : Exception("Problem failed: " + msg) {}
};

class ProblemCancelledException : public SolveException {
public:
  ProblemCancelledException() : SolveException("Problem cancelled", 0) {}
};

class NoAnswerException : public SolveException {
public:
  NoAnswerException() : SolveException("answer not available") {}
};



class NetworkException : public Exception {
public:
  NetworkException(const std::string& msg) : Exception("Network error: " + msg) {}
};



class AuthenticationException : public Exception {
public:
  AuthenticationException() : Exception("authentication failed") {}
};



class CommunicationException : public Exception {
public:
  CommunicationException(const std::string& msg, const std::string& url) :
    Exception("Bad server response from <" + url + ">: " + msg) {}
};

class TooManyProblemIdsException : public CommunicationException {
public:
  TooManyProblemIdsException(const std::string& url) :
    CommunicationException("too many problem IDs requested", url) {}
};



class ServiceShutdownException : public Exception {
public:
  ServiceShutdownException() : Exception("Service shut down") {}
};



class DecodingException : public Exception {
public:
  DecodingException(const std::string& msg) : Exception("Decoding error: " + msg) {}
};

class Base64Exception : public DecodingException {
public:
  Base64Exception() : DecodingException("base64 decoding error") {}
};



class EncodingException : public Exception {
public:
  EncodingException(const std::string& msg) : Exception("Encoding error: " + msg) {}
};



class InternalException : public Exception {
public:
  InternalException(const std::string& msg) : Exception("Internal error: " + msg) {}
};

} // namespace sapiremote

#endif
