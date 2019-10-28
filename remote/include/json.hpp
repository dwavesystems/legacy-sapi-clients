//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef JSON_HPP_INCLUDED
#define JSON_HPP_INCLUDED

#include <cmath>
#include <exception>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/variant.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

namespace json {

class Exception : public std::runtime_error {
public:
  Exception(const std::string& msg) : std::runtime_error(msg) {}
};

class TypeException : public Exception {
public:
  TypeException() : Exception("json::TypeException") {}
};

class ParseException : public Exception {
public:
  ParseException(const std::string& msg) : Exception("JSON parsing error: " + msg) {}
};

class ValueException : public Exception {
public:
  ValueException() : Exception("json::ValueException") {}
};

class Value;

typedef boost::blank Null;
typedef long long Integer;
typedef std::vector<Value> Array;
typedef std::map<std::string, Value> Object;

class Value {
public:
  typedef boost::variant<
      Null,
      bool,
      Integer,
      double,
      std::string,
      boost::recursive_wrapper<Array>,
      boost::recursive_wrapper<Object>
  > Variant;

private:
  Variant v_;

public:
  Value() : v_(Null()) {}
  Value(const Null&) : v_(Null()) {}
  Value(bool b) : v_(b) {}
  Value(int n) : v_(static_cast<Integer>(n)) {}
  Value(long n) : v_(static_cast<Integer>(n)) {}
  Value(long long n) : v_(static_cast<Integer>(n)) {}
  Value(unsigned n) : v_(static_cast<double>(n)) {}
  Value(unsigned long n) : v_(static_cast<double>(n)) {}
  Value(unsigned long long n) : v_(static_cast<double>(n)) {}
  Value(const std::string& s) : v_(s) {}
  Value(std::string&& s) : v_(std::move(s)) {}
  Value(const char* c) : v_(std::string(c)) {}
  Value(const Array& a) : v_(a) {}
  Value(Array&& a) : v_(std::move(a)) {}
  Value(const Object& o) : v_(o) {}
  Value(Object&& o) : v_(std::move(o)) {}

  // store double as long long if no precision loss
  Value(double d) : v_(d) {
    if (!boost::math::isfinite(d)) throw ValueException();
    if (std::floor(d) == d
        && d + 1.0 > d
        && d - 1.0 < d
        && d <= static_cast<double>(std::numeric_limits<Integer>::max())
        && d >= static_cast<double>(std::numeric_limits<Integer>::min())) {
      v_ = static_cast<Integer>(d);
    }
  }

  Value(const Value& other) : v_(other.v_) {}
  Value& operator=(const Value& other) { v_ = other.v_; return *this; }
  Value(Value&& other) : v_(std::move(other.v_)) {} // not provided automatically bt MS VC11
  Value& operator=(Value&& other) { v_.swap(other.v_); return *this; } // same ^

  void swap(Value& other) { v_.swap(other.v_); }

  template<typename T>
  bool is() const { return !!boost::get<const T>(&v_); }
  bool isNull() const { return is<Null>(); }
  bool isBool() const { return is<bool>(); }
  bool isInteger() const { return is<Integer>(); }
  bool isReal() const { return is<double>() || isInteger(); }
  bool isString() const { return is<std::string>(); }
  bool isArray() const { return is<Array>(); }
  bool isObject() const { return is<Object>(); }

  template<typename T>
  const T& get() const {
    const T* t = boost::get<const T>(&v_);
    if (!t) throw TypeException();
    return *t;
  }
  bool getBool() const { return get<bool>(); }
  Integer getInteger() const { return get<long long>(); }
  double getReal() const {
    const double* d = boost::get<const double>(&v_);
    return d ? *d : static_cast<double>(getInteger());
  }
  const std::string& getString() const { return get<std::string>(); }
  const Array& getArray() const { return get<Array>(); }
  const Object& getObject() const { return get<Object>(); }

  template<typename T>
  T& get() {
    T* t = boost::get<T>(&v_);
    if (!t) throw TypeException();
    return *t;
  }
  std::string& getString() { return get<std::string>(); }
  Array& getArray() { return get<Array>(); }
  Object& getObject() { return get<Object>(); }

  const Variant& variant() const { return v_; }
  Variant& variant() { return v_; }
};

std::string jsonToString(const Value& jsonValue);
std::string jsonToString(const Array& jsonArray);
std::string jsonToString(const Object& jsonObject);

Value stringToJson(const std::string& s);


} // namespace json

#endif
