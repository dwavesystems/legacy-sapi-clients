//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef JSON_TO_MATLAB_HPP_INCLUDED
#define JSON_TO_MATLAB_HPP_INCLUDED

#include <mex.h>

#include <json.hpp>

mxArray* jsonToMatlab(const json::Value& v);
json::Value matlabToJson(const mxArray* a);

#endif
