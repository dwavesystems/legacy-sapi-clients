//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef PYTHON_EXCEPTION_HPP_INCLUDED
#define PYTHON_EXCEPTION_HPP_INCLUDED

#include <stdexcept>

class PythonException : public std::runtime_error {
public:
  PythonException() : std::runtime_error("Python exception raised") {}
};

#endif
