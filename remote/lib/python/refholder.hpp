//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef REFHOLDER_HPP_INCLUDED
#define REFHOLDER_HPP_INCLUDED

#include <Python.h>

class RefHolder {
private:
  PyObject *obj_;
  bool dec_;
  RefHolder(const RefHolder&);
  RefHolder& operator=(const RefHolder&);
public:
  RefHolder(PyObject *obj) : obj_(obj), dec_(true) {}
  ~RefHolder() { if (dec_) Py_DECREF(obj_); }
  void release() { dec_ = false; }
};

#endif
