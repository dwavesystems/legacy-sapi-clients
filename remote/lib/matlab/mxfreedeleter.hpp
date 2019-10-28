//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef MXFREEDELETER_HPP_INCLUDED
#define MXFREEDELETER_HPP_INCLUDED

#include <mex.h>

struct MxFreeDeleter {
  void operator()(void* p) { mxFree(p); }
};

#endif
