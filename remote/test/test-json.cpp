//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <limits>
#include <ostream>
#include <string>
#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/variant.hpp>

#include <json.hpp>
#include "test.hpp"

using std::string;
using std::ostream;
using std::istringstream;
using std::numeric_limits;

TEST(JsonTest, TypeNull) {
  json::Value v;
  EXPECT_TRUE(v.isNull());
  EXPECT_TRUE(!v.isBool());
  EXPECT_TRUE(!v.isInteger());
  EXPECT_TRUE(!v.isReal());
  EXPECT_TRUE(!v.isString());
  EXPECT_TRUE(!v.isArray());
  EXPECT_TRUE(!v.isObject());
  EXPECT_THROW(v.getBool(), json::TypeException);
  EXPECT_THROW(v.getInteger(), json::TypeException);
  EXPECT_THROW(v.getReal(), json::TypeException);
  EXPECT_THROW(v.getString(), json::TypeException);
  EXPECT_THROW(v.getArray(), json::TypeException);
  EXPECT_THROW(v.getObject(), json::TypeException);
}

TEST(JsonTest, TypeBool) {
  json::Value v = true;
  EXPECT_TRUE(!v.isNull());
  EXPECT_TRUE(v.isBool());
  EXPECT_TRUE(!v.isInteger());
  EXPECT_TRUE(!v.isReal());
  EXPECT_TRUE(!v.isString());
  EXPECT_TRUE(!v.isArray());
  EXPECT_TRUE(!v.isObject());
  EXPECT_EQ(v.getBool(), true);
  EXPECT_THROW(v.getInteger(), json::TypeException);
  EXPECT_THROW(v.getReal(), json::TypeException);
  EXPECT_THROW(v.getString(), json::TypeException);
  EXPECT_THROW(v.getArray(), json::TypeException);
  EXPECT_THROW(v.getObject(), json::TypeException);

  v = false;
  EXPECT_TRUE(!v.isNull());
  EXPECT_TRUE(v.isBool());
  EXPECT_TRUE(!v.isInteger());
  EXPECT_TRUE(!v.isReal());
  EXPECT_TRUE(!v.isString());
  EXPECT_TRUE(!v.isArray());
  EXPECT_TRUE(!v.isObject());
  EXPECT_EQ(v.getBool(), false);
}

TEST(JsonTest, TypeInteger) {
  json::Value v = 1;
  EXPECT_TRUE(!v.isNull());
  EXPECT_TRUE(!v.isBool());
  EXPECT_TRUE(v.isInteger());
  EXPECT_TRUE(v.isReal());
  EXPECT_TRUE(!v.isString());
  EXPECT_TRUE(!v.isArray());
  EXPECT_TRUE(!v.isObject());
  EXPECT_EQ(v.getInteger(), 1LL);
  EXPECT_EQ(v.getReal(), 1.0);
  EXPECT_THROW(v.getBool(), json::TypeException);
  EXPECT_THROW(v.getString(), json::TypeException);
  EXPECT_THROW(v.getArray(), json::TypeException);
  EXPECT_THROW(v.getObject(), json::TypeException);

  v = 2.0;
  EXPECT_TRUE(!v.isNull());
  EXPECT_TRUE(!v.isBool());
  EXPECT_TRUE(v.isInteger());
  EXPECT_TRUE(v.isReal());
  EXPECT_TRUE(!v.isString());
  EXPECT_TRUE(!v.isArray());
  EXPECT_TRUE(!v.isObject());
  EXPECT_EQ(v.getInteger(), 2LL);
  EXPECT_EQ(v.getReal(), 2.0);
  EXPECT_THROW(v.getBool(), json::TypeException);
  EXPECT_THROW(v.getString(), json::TypeException);
  EXPECT_THROW(v.getArray(), json::TypeException);
  EXPECT_THROW(v.getObject(), json::TypeException);
}

TEST(JsonTest, TypeReal) {
  json::Value v = 0.5;
  EXPECT_TRUE(!v.isNull());
  EXPECT_TRUE(!v.isBool());
  EXPECT_TRUE(!v.isInteger());
  EXPECT_TRUE(v.isReal());
  EXPECT_TRUE(!v.isString());
  EXPECT_TRUE(!v.isArray());
  EXPECT_TRUE(!v.isObject());
  EXPECT_EQ(v.getReal(), 0.5);
  EXPECT_THROW(v.getBool(), json::TypeException);
  EXPECT_THROW(v.getInteger(), json::TypeException);
  EXPECT_THROW(v.getString(), json::TypeException);
  EXPECT_THROW(v.getArray(), json::TypeException);
  EXPECT_THROW(v.getObject(), json::TypeException);
}

TEST(JsonTest, TypeString) {
  json::Value v = "hello";
  EXPECT_TRUE(!v.isNull());
  EXPECT_TRUE(!v.isBool());
  EXPECT_TRUE(!v.isInteger());
  EXPECT_TRUE(!v.isReal());
  EXPECT_TRUE(v.isString());
  EXPECT_TRUE(!v.isArray());
  EXPECT_TRUE(!v.isObject());
  EXPECT_EQ(v.getString(), "hello");
  EXPECT_THROW(v.getBool(), json::TypeException);
  EXPECT_THROW(v.getInteger(), json::TypeException);
  EXPECT_THROW(v.getReal(), json::TypeException);
  EXPECT_THROW(v.getArray(), json::TypeException);
  EXPECT_THROW(v.getObject(), json::TypeException);

  v = string("woooo");
  EXPECT_EQ(v.getString(), "woooo");
  v.getString().append("!");
  EXPECT_EQ(v.getString(), "woooo!");
}

TEST(JsonTest, TypeArray) {
  json::Value v = json::Array();
  EXPECT_TRUE(!v.isNull());
  EXPECT_TRUE(!v.isBool());
  EXPECT_TRUE(!v.isInteger());
  EXPECT_TRUE(!v.isReal());
  EXPECT_TRUE(!v.isString());
  EXPECT_TRUE(v.isArray());
  EXPECT_TRUE(!v.isObject());
  EXPECT_TRUE(v.getArray().empty());
  EXPECT_THROW(v.getBool(), json::TypeException);
  EXPECT_THROW(v.getInteger(), json::TypeException);
  EXPECT_THROW(v.getReal(), json::TypeException);
  EXPECT_THROW(v.getString(), json::TypeException);
  EXPECT_THROW(v.getObject(), json::TypeException);

  v.getArray().push_back(json::Null());
  EXPECT_EQ(v.getArray().size(), 1u);
}

TEST(JsonTest, TypeObject) {
  json::Value v = json::Object();
  EXPECT_TRUE(!v.isNull());
  EXPECT_TRUE(!v.isBool());
  EXPECT_TRUE(!v.isInteger());
  EXPECT_TRUE(!v.isReal());
  EXPECT_TRUE(!v.isString());
  EXPECT_TRUE(!v.isArray());
  EXPECT_TRUE(v.isObject());
  EXPECT_TRUE(v.getObject().empty());
  EXPECT_THROW(v.getBool(), json::TypeException);
  EXPECT_THROW(v.getInteger(), json::TypeException);
  EXPECT_THROW(v.getReal(), json::TypeException);
  EXPECT_THROW(v.getString(), json::TypeException);
  EXPECT_THROW(v.getArray(), json::TypeException);

  v.getObject()["a"] = json::Null();
  EXPECT_EQ(v.getObject().size(), 1u);
}

TEST(JsonTest, IntegerConversion) {
  json::Value h = short(1);
  json::Value hu = (unsigned short)(1);
  json::Value d = 1;
  json::Value u = 1u;
  json::Value l = 1l;
  json::Value lu = 1lu;
  json::Value ll = 1ll;
  json::Value llu = 1ull;
}

TEST(JsonTest, ParseNull) {
  EXPECT_TRUE(json::stringToJson("null").isNull());
}

TEST(JsonTest, ParseBool) {
  EXPECT_EQ(json::stringToJson("true").getBool(), true);
  EXPECT_EQ(json::stringToJson("false").getBool(), false);
}

TEST(JsonTest, ParseInteger) {
  EXPECT_EQ(json::stringToJson("1").getInteger(), 1);
  EXPECT_EQ(json::stringToJson("-2.0").getInteger(), -2);
  EXPECT_EQ(json::stringToJson("4.5e1").getInteger(), 45);
  EXPECT_EQ(json::stringToJson("-6000e-3").getInteger(), -6);
}

TEST(JsonTest, ParseReal) {
  EXPECT_EQ(json::stringToJson("1.5").getReal(), 1.5);
  EXPECT_DOUBLE_EQ(json::stringToJson("1000000000000000000000000000000").getReal(), 1e30);
  EXPECT_DOUBLE_EQ(json::stringToJson("0.000000000000000000000000000001").getReal(), 1e-30);
  EXPECT_DOUBLE_EQ(json::stringToJson("1.2345e100").getReal(), 1.2345e100);
  EXPECT_DOUBLE_EQ(json::stringToJson("1.2345E100").getReal(), 1.2345e100);
}

TEST(JsonTest, ParseString) {
  EXPECT_EQ(json::stringToJson("\"hello\"").getString(), "hello");
  EXPECT_EQ(json::stringToJson("\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"").getString(), "\"\\/\x08\x0c\x0a\x0d\x09");
  EXPECT_EQ(json::stringToJson("\"\\u0020\"").getString(), "\x20");
  EXPECT_EQ(json::stringToJson("\"\\u0080\\u07ff\"").getString(), "\xc2\x80\xdf\xbf");
  EXPECT_EQ(json::stringToJson("\"\\u0800\\uffff\"").getString(), "\xe0\xa0\x80\xef\xbf\xbf");
  EXPECT_EQ(json::stringToJson("\"\\uD834\\udd1e\"").getString(), "\xf0\x9d\x84\x9e");
}

TEST(JsonTest, ParseArray) {
  EXPECT_TRUE(json::stringToJson("[]").getArray().empty());
  json::Value v = json::stringToJson("[null,null,null]");
  json::Array expectedV(3);
  EXPECT_EQ(v.getArray(), expectedV);
}

TEST(JsonTest, ParseObject) {
  EXPECT_TRUE(json::stringToJson("{}").getObject().empty());
  json::Value v = json::stringToJson("{\"a\":null,\"b\":null}");
  json::Object expectedV;
  expectedV["a"] = json::Null();
  expectedV["b"] = json::Null();
  EXPECT_EQ(v.getObject(), expectedV);
}

TEST(JsonTest, ParseBad) {
  EXPECT_THROW(json::stringToJson(","), json::ParseException);
  EXPECT_THROW(json::stringToJson("."), json::ParseException);
  EXPECT_THROW(json::stringToJson("123e"), json::ParseException);
  EXPECT_THROW(json::stringToJson("234e45.2"), json::ParseException);
  EXPECT_THROW(json::stringToJson("[[]"), json::ParseException);
  EXPECT_THROW(json::stringToJson("[]]"), json::ParseException);
  EXPECT_THROW(json::stringToJson("[][]"), json::ParseException);
  EXPECT_THROW(json::stringToJson("{123: 456}"), json::ParseException);
  EXPECT_THROW(json::stringToJson("[}"), json::ParseException);
  EXPECT_THROW(json::stringToJson("\"hello"), json::ParseException);
  EXPECT_THROW(json::stringToJson("[\"hello]"), json::ParseException);
  EXPECT_THROW(json::stringToJson("\"hello\\\""), json::ParseException);
}

TEST(JsonTest, DumpNull) {
  EXPECT_EQ(json::jsonToString(json::Value()), "null");
}

TEST(JsonTest, DumpBool) {
  EXPECT_EQ(json::jsonToString(json::Value(true)), "true");
  EXPECT_EQ(json::jsonToString(json::Value(false)), "false");
}

TEST(JsonTest, DumpInteger) {
  EXPECT_EQ(json::jsonToString(4), "4");
}

TEST(JsonTest, DumpReal) {
  double x = 12345.6789;
  string s = json::jsonToString(x);
  istringstream ss(s);
  double px;
  ss >> px;
  EXPECT_DOUBLE_EQ(px, x);
}

TEST(JsonTest, DumpString) {
  EXPECT_EQ(json::jsonToString("hello"), "\"hello\"");
  EXPECT_EQ(json::jsonToString("\"\\\x08\x0c\x0a\x0d\x09"), "\"\\\"\\\\\\b\\f\\n\\r\\t\"");
  EXPECT_EQ(json::jsonToString("\x01\x02\x03\x04\x05\x06\x07\x0b\x0e\x0f"),
      "\"\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\u000b\\u000e\\u000f\"");
  EXPECT_EQ(json::jsonToString("\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"),
      "\"\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018"
      "\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f\"");
}

TEST(JsonTest, DumpArray) {
  EXPECT_EQ(json::jsonToString(json::Array()), "[]");
}

TEST(JsonTest, DumpObject) {
  EXPECT_EQ(json::jsonToString(json::Object()), "{}");
}

TEST(JsonTest, Complex) {
  string s0 = "[\"hello\", null, 42.5, {\"a\": [1,2,3], \"xyz\": {}, \"12\": \"\\u1234\\/\\/\\/\"}, "
      " [[[[[[null]]]]]], false, false, false]";
  json::Value v1 = json::stringToJson(s0);
  ASSERT_TRUE(v1.isArray());
  EXPECT_EQ(v1.getArray().size(), 8u);
  string s1 = json::jsonToString(v1);
  json::Value v2 = json::stringToJson(s1);
  EXPECT_EQ(v1, v2);
}

TEST(JsonTest, BadNumbers) {
  EXPECT_THROW(json::Value v(numeric_limits<double>::infinity()), json::ValueException);
  EXPECT_THROW(json::Value v(-numeric_limits<double>::infinity()), json::ValueException);
  EXPECT_THROW(json::Value v(numeric_limits<double>::quiet_NaN()), json::ValueException);
  EXPECT_THROW(json::Value v(numeric_limits<double>::signaling_NaN()), json::ValueException);
}
