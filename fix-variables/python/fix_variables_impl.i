//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

%define DOCSTRING
"fix_variables documentation"
%enddef

%module(docstring=DOCSTRING) fix_variables_impl
#pragma SWIG nowarn=401

%{
   #include "fix_variables_python_wrapper.hpp"
%}

%feature("compactdefaultargs") fix_variables_impl;

%exception
{
    try
    {
        $action
    }
    catch (const fix_variables_::FixVariablesException& e)
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

// fix_variables_impl
%feature("autodoc",
"answer = fix_variables_impl(Q, method=None)

Args:
   Q: must be a dictionary object (key is a tuple and value is an integer or double)
      for Q defining a QUBO problem x' * Q * x.

   method: determines the algorithm used to infer values. [optional]
           If it is not None, it must be a string of \"optimized\" or \"standard\"
           \"optimized\": roof-duality & strongly connected components (default value)
           \"standard\": roof-duality
           If it is None, it will use its default value \"optimized\"           
      
	       fix_variables uses maximum flow in the implication network to correctly
		   fix variables (that is, one can find an assignment for the other variables
		   that attains the optimal value). The variables that roof duality fixes will
		   take the same values in all optimal solutions.

           Using strongly connected components can fix more variables, but in some
		   optimal solutions these variables may take different values.

		   In summary:
             * All the variables fixed by method = \"standard\" will also be fixed
			   by method = \"optimized\" (reverse is not true)
             * All the variables fixed by method = \"standard\" will take the same
			   value in every optimal solution
             * There exists at least one optimal solution that has the fixed values
			   as given by method = \"optimized\"

           Thus, method = \"standard\" is a subset of method = \"optimized\" as any variable
		   that is fixed by method = \"standard\" will also be fixed by method = \"optimized\"
		   and additionally, method = \"optimized\" may fix some variables that method = \"standard\"
		   could not. For this reason, method = \"optimized\" takes longer than method = \"standard\".

Returns:
   answer is a dictionary which has the following keys: 
   \"fixed_variables\": a dictionary of variables that can be fixed with value 1 or 0
   \"new_Q\": a dictionary of new Q of unfixed variables
   \"offset\": a double, x' * new_Q * x + offset = x' * Q * x

Raises:
   ValueError: bad parameter value") fix_variables_impl;

%include "fix_variables_python_wrapper.hpp"
