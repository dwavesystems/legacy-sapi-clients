//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef CONVERSION_HPP_INCLUDED
#define CONVERSION_HPP_INCLUDED

#include <Python.h>

#include <string>

#include <coding.hpp>
#include <json.hpp>

json::Value pythonToJson(PyObject* pyobj);

PyObject* jsonToPython(const json::Value& v);
PyObject* jsonToPython(const json::Object& v);

sapiremote::QpProblem pythonToQpProblem(PyObject* pyobj);
PyObject* qpAnswerToPython(json::Object answer, const sapiremote::QpAnswer& qpAnswer);

PyObject* submittedProblemInfoToPython(const sapiremote::SubmittedProblemInfo& status);

#endif
