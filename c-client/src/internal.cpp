//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cstring>
#include <exception>

#include <exceptions.hpp>
#include <sapi-local/exceptions.hpp>
#include <blackbox.hpp>
#include <fix_variables.hpp>
#include <find_embedding.hpp>

#include "dwave_sapi.h"
#include "internal.hpp"

using std::exception_ptr;
using std::rethrow_exception;

namespace sapi {

void writeErrorMessage(char* destination, const char* source) {
  if (destination) {
    std::strncpy(destination, source, SAPI_ERROR_MESSAGE_MAX_SIZE - 1);
    destination[SAPI_ERROR_MESSAGE_MAX_SIZE - 1] = '\0';
  }
}

sapi_Code handleException(std::exception_ptr e, char* errMsg) {
  try {
    rethrow_exception(e);

  } catch (const blackbox::BlackBoxException& e) {
    writeErrorMessage(errMsg, e.what().c_str());
    return SAPI_ERR_SOLVE_FAILED;

  } catch (const fix_variables_::FixVariablesException& e) {
    writeErrorMessage(errMsg, e.what().c_str());
    return SAPI_ERR_SOLVE_FAILED;

  } catch (const find_embedding_::FindEmbeddingException& e) {
    writeErrorMessage(errMsg, e.what().c_str());
    return SAPI_ERR_SOLVE_FAILED;

  } catch (sapiremote::AuthenticationException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_AUTHENTICATION;

  } catch (sapiremote::CommunicationException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_COMMUNICATION;

  } catch (sapiremote::DecodingException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_COMMUNICATION;

  } catch (sapiremote::EncodingException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_INVALID_PARAMETER;

  } catch (sapiremote::NetworkException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_NETWORK;

  } catch (sapiremote::NoAnswerException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_ASYNC_NOT_DONE;

  } catch (sapiremote::ProblemCancelledException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_PROBLEM_CANCELLED;

  } catch (sapiremote::SolveException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_SOLVE_FAILED;

  } catch (sapiremote::ServiceShutdownException&) {
    writeErrorMessage(errMsg, sapi::NotInitializedException().what());
    return SAPI_ERR_NO_INIT;

  } catch (sapiremote::Exception& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_SOLVE_FAILED;

  } catch (sapilocal::Exception& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_SOLVE_FAILED;

  } catch (sapi::InvalidParameterException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_INVALID_PARAMETER;

  } catch (sapi::NotInitializedException& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_NO_INIT;

  } catch (std::bad_alloc& e) {
    writeErrorMessage(errMsg, "out of memory");
    return SAPI_ERR_OUT_OF_MEMORY;

  } catch (std::exception& e) {
    writeErrorMessage(errMsg, e.what());
    return SAPI_ERR_SOLVE_FAILED;

  } catch (...) {
    writeErrorMessage(errMsg, "unknown error");
    return SAPI_ERR_SOLVE_FAILED;
  }
}

} // namespace sapi
