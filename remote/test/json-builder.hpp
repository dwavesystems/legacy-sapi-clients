//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <string>
#include <utility>

#include <json.hpp>

namespace build_json {

class JsonArrayBuilder {
private:
  json::Array array_;
public:
  JsonArrayBuilder(json::Value v) : array_(1) { array_[0] = std::move(v); }
  json::Array array() { return std::move(array_); }
  json::Value value() { return array(); }
  operator json::Value() { return value(); }
  operator json::Array() { return array(); }
  JsonArrayBuilder& operator,(json::Value v) {
    array_.push_back(std::move(v));
    return *this;
  }
};

class JsonObjectBuilderV;

class JsonObjectBuilderK {
private:
  json::Object object_;
public:
  JsonObjectBuilderK() {}
  explicit JsonObjectBuilderK(json::Object object) : object_(std::move(object)) {}
  json::Object object() { return std::move(object_); }
  json::Value value() { return object(); }
  operator json::Value() { return value(); }
  operator json::Object() { return object(); }
  JsonObjectBuilderV operator,(std::string);
};

class JsonObjectBuilderV {
private:
  json::Object object_;
  std::string key_;
public:
  JsonObjectBuilderV(std::string key, json::Object object) : object_(std::move(object)), key_(std::move(key)) {}
  JsonObjectBuilderK operator,(json::Value v) {
    object_[std::move(key_)] = std::move(v);
    return JsonObjectBuilderK(std::move(object_));
  }
};

inline JsonObjectBuilderV JsonObjectBuilderK::operator,(std::string key) {
  return JsonObjectBuilderV(std::move(key), std::move(object_));
}

class JsonArrayStarter {
public:
  json::Array array() const { return json::Array(); }
  json::Value value() { return array(); }
  operator json::Value() { return value(); }
  operator json::Array() { return array(); }
  JsonArrayBuilder operator,(json::Value v) const {
    return JsonArrayBuilder(std::move(v));
  }
};

class JsonObjectStarter {
public:
  json::Object object() const { return json::Object(); }
  json::Value value() const { return object(); }
  operator json::Value() const { return value(); }
  operator json::Object() const { return object(); }
  JsonObjectBuilderV operator,(std::string key) const {
    return JsonObjectBuilderV(std::move(key), json::Object());
  }
};

} // namespace build_json

inline build_json::JsonArrayStarter jsonArray() { return build_json::JsonArrayStarter(); }
inline build_json::JsonObjectStarter jsonObject() { return build_json::JsonObjectStarter(); }
