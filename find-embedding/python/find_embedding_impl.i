//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

%module find_embedding_impl

%{
   #include "find_embedding_python_wrapper.hpp"
%}

%exception
{
    try
    {
        $action
    }
    catch (const find_embedding_::FindEmbeddingException& e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what().c_str());
        return NULL;
    }
    catch (...)
    {
        PyErr_SetString(PyExc_RuntimeError, "unknown exception caught");
        return NULL;
    }
}

%include "find_embedding_python_wrapper.hpp"
