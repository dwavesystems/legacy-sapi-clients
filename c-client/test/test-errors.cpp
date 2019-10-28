//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <exception>
#include <new>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <exceptions.hpp>
#include <sapi-local/exceptions.hpp>
#include <blackbox.hpp>
#include <find_embedding.hpp>
#include <fix_variables.hpp>

#include <dwave_sapi.h>
#include <sapi-impl.hpp>
#include <internal.hpp>

using std::string;

using sapi::handleException;

#define MAKE_EXECPTION_PTR(var_, expr_) std::exception_ptr var_; \
  try { throw (expr_); } catch (...) { var_ = std::current_exception(); }

namespace {

struct UnknownException{};
struct UnknownRemoteException : public sapiremote::Exception {
  UnknownRemoteException() : sapiremote::Exception("unknown") {}
};
struct UnknownLocalException : public sapilocal::Exception {
  UnknownLocalException() : sapilocal::Exception("unknown") {}
};

} // namespace {anonymous}


TEST(ErrorsTest, OutOfMemeory) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, std::bad_alloc());
  EXPECT_EQ(SAPI_ERR_OUT_OF_MEMORY, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, NoInit) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapi::NotInitializedException());
  EXPECT_EQ(SAPI_ERR_NO_INIT, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, InvalidParameter) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapi::InvalidParameterException("something"));
  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, UnsupportedAnswerFormat) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapi::UnsupportedAnswerFormatException("something"));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}




TEST(ErrorsTest, RemoteAuthentication) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::AuthenticationException());
  EXPECT_EQ(SAPI_ERR_AUTHENTICATION, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteNetwork) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::NetworkException("something"));
  EXPECT_EQ(SAPI_ERR_NETWORK, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteCommunication) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::CommunicationException("something", "test://test"));
  EXPECT_EQ(SAPI_ERR_COMMUNICATION, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteEncoding) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::EncodingException("something"));
  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteDecoding) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::DecodingException("something"));
  EXPECT_EQ(SAPI_ERR_COMMUNICATION, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteSolve) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::SolveException("something"));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteAsyncNotDone) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::NoAnswerException());
  EXPECT_EQ(SAPI_ERR_ASYNC_NOT_DONE, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteCancelled) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::ProblemCancelledException());
  EXPECT_EQ(SAPI_ERR_PROBLEM_CANCELLED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteServiceShutDown) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::ServiceShutdownException());
  EXPECT_EQ(SAPI_ERR_NO_INIT, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteInternal) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapiremote::InternalException("something"));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, RemoteBase) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, UnknownRemoteException());
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, Json) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, json::Exception("something"));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}




TEST(ErrorsTest, LocalSolve) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapilocal::SolveException("something"));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, LocalParams) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapilocal::InvalidParameterException("something"));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, LocalProblem) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapilocal::InvalidProblemException("something"));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, LocalProblemType) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, sapilocal::UnsupportedProblemTypeException());
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}


TEST(ErrorsTest, LocalBase) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, UnknownLocalException());
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}




TEST(ErrorsTest, QSage) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, blackbox::BlackBoxException("something"));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}




TEST(ErrorsTest, FindEmbedding) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, find_embedding_::FindEmbeddingException());
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}




TEST(ErrorsTest, FixVariables) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, fix_variables_::FixVariablesException());
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_NE(string("unknown error"), errMsg);
}




TEST(ErrorsTest, Other) {
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE] = {0};
  MAKE_EXECPTION_PTR(e, UnknownException());
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, errMsg));
  EXPECT_EQ(string("unknown error"), errMsg);
}




TEST(ErrorsTest, NullMessage) {
  MAKE_EXECPTION_PTR(e, UnknownException());
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, handleException(e, 0));
}
