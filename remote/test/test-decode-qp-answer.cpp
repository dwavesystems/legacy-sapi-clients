//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <vector>

#include <gtest/gtest.h>

#include <exceptions.hpp>
#include <json.hpp>
#include <coding.hpp>

#include "json-builder.hpp"
#include "test.hpp"

using std::vector;

using sapiremote::decodeQpAnswer;

namespace {
auto o = jsonObject();
auto a = jsonArray();
}

TEST(QpDecoderTest, isingPad0) {
  auto encodedAnswer = (o,
      "format", "qp",
      "energies", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
      "num_variables", 20,
      "active_variables", "AQAAAAIAAAADAAAABQAAAAcAAAAJAAAADAAAAA8AAAA=",
      "solutions", "AFUzD/8=").object();

  auto expectedSolutions = vector<char>{
       3,-1,-1,-1, 3,-1, 3,-1, 3,-1, 3, 3,-1, 3, 3,-1, 3, 3, 3, 3,
       3,-1, 1,-1, 3, 1, 3,-1, 3, 1, 3, 3,-1, 3, 3, 1, 3, 3, 3, 3,
       3,-1,-1, 1, 3, 1, 3,-1, 3,-1, 3, 3, 1, 3, 3, 1, 3, 3, 3, 3,
       3,-1,-1,-1, 3,-1, 3, 1, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 3, 3,
       3, 1, 1, 1, 3, 1, 3, 1, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 3, 3,
     };

  auto decodedAnswer = decodeQpAnswer("ising", encodedAnswer);
  EXPECT_EQ(expectedSolutions, decodedAnswer.solutions);
}

TEST(QpDecoderTest, isingPad1) {
  auto encodedAnswer0 = (o,
      "format", "qp",
      "energies", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
      "num_variables", 20,
      "active_variables", "AQAAAAIAAAADAAAABQAAAAcAAAAJAAAADAAAAA8AAAASAAAA",
      "solutions", "AAAkgEkAkgD/gA==").object();
  auto encodedAnswer1 = (o,
      "format", "qp",
      "energies", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
      "num_variables", 20,
      "active_variables", "AQAAAAIAAAADAAAABQAAAAcAAAAJAAAADAAAAA8AAAASAAAA",
      "solutions", "AH8k/0l/kn///w==").object();

  auto expectedSolutions = vector<char>{
       3,-1,-1,-1, 3,-1, 3,-1, 3,-1, 3, 3,-1, 3, 3,-1, 3, 3,-1, 3,
       3,-1,-1, 1, 3,-1, 3,-1, 3, 1, 3, 3,-1, 3, 3,-1, 3, 3, 1, 3,
       3,-1, 1,-1, 3,-1, 3, 1, 3,-1, 3, 3,-1, 3, 3, 1, 3, 3,-1, 3,
       3, 1,-1,-1, 3, 1, 3,-1, 3,-1, 3, 3, 1, 3, 3,-1, 3, 3,-1, 3,
       3, 1, 1, 1, 3, 1, 3, 1, 3, 1, 3, 3, 1, 3, 3, 1, 3, 3, 1, 3,
     };

  auto decodedAnswer = decodeQpAnswer("ising", encodedAnswer0);
  EXPECT_EQ(expectedSolutions, decodedAnswer.solutions);
  decodedAnswer = decodeQpAnswer("ising", encodedAnswer1);
  EXPECT_EQ(expectedSolutions, decodedAnswer.solutions);
}

TEST(QpDecoderTest, isingPad7) {
  auto encodedAnswer0 = (o,
      "format", "qp",
      "energies", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
      "num_variables", 20,
      "active_variables", "AQAAAAIAAAADAAAABQAAAAcAAAAJAAAADAAAAA==",
      "solutions", "AA444P4=").object();
  auto encodedAnswer1 = (o,
      "format", "qp",
      "energies", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
      "num_variables", 20,
      "active_variables", "AQAAAAIAAAADAAAABQAAAAcAAAAJAAAADAAAAA==",
      "solutions", "AQ854f8=").object();

  auto expectedSolutions = vector<char>{
       3,-1,-1,-1, 3,-1, 3,-1, 3,-1, 3, 3,-1, 3, 3, 3, 3, 3, 3, 3,
       3,-1,-1,-1, 3,-1, 3, 1, 3, 1, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3,
       3,-1,-1, 1, 3, 1, 3, 1, 3,-1, 3, 3,-1, 3, 3, 3, 3, 3, 3, 3,
       3, 1, 1, 1, 3,-1, 3,-1, 3,-1, 3, 3,-1, 3, 3, 3, 3, 3, 3, 3,
       3, 1, 1, 1, 3, 1, 3, 1, 3, 1, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3,
     };

  auto decodedAnswer = decodeQpAnswer("ising", encodedAnswer0);
  EXPECT_EQ(expectedSolutions, decodedAnswer.solutions);
  decodedAnswer = decodeQpAnswer("ising", encodedAnswer1);
  EXPECT_EQ(expectedSolutions, decodedAnswer.solutions);
}

TEST(QpDecoderTest, trivial) {
  auto encodedAnswer = (o, "format", "qp", "energies", "", "num_variables", 0,
      "active_variables", "", "solutions", "").object();

  auto decodedAnswer = decodeQpAnswer("ising", encodedAnswer);
  EXPECT_TRUE(decodedAnswer.energies.empty());
  EXPECT_TRUE(decodedAnswer.numOccurrences.empty());
  EXPECT_TRUE(decodedAnswer.solutions.empty());
}

TEST(QpDecoderTest, empty) {
  auto encodedAnswer = (o, "format", "qp", "energies", "AAAAAAAAAMAAAAAAAADwvw==",
    "num_variables", 8, "active_variables", "", "solutions", "").object();

  auto expectedEnergies = vector<double>{-2.0, -1.0};
  auto expectedNumOccurrences = vector<int>{};
  auto expectedSolutions = vector<char>{3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

  auto decodedAnswer = decodeQpAnswer("ising", encodedAnswer);
  EXPECT_EQ(decodedAnswer.energies, expectedEnergies);
  EXPECT_EQ(decodedAnswer.numOccurrences, expectedNumOccurrences);
  EXPECT_EQ(decodedAnswer.solutions, expectedSolutions);
}

TEST(QpDecoderTest, energies) {
  auto encodedAnswer = (o, "format", "qp",
      "energies", "AAAAAADAWMAAAAAAAMBIwAAAAAAAwDjAAAAAAAAAAAAAAAAAAOBeQH3DlCWtSbJU",
      "num_variables", 4, "active_variables", "", "solutions", "").object();

  auto expectedEnergies = vector<double>{-99.0, -49.5, -24.75, 0.0, 123.5, 1e100};

  auto decodedAnswer = decodeQpAnswer("ising", encodedAnswer);
  EXPECT_EQ(decodedAnswer.energies, expectedEnergies);
}

TEST(QpDecoderTest, numOccurrences) {
  auto encodedAnswer = (o, "format", "qp", "energies", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
      "num_variables", 4, "active_variables", "", "solutions", "",
      "num_occurrences", "AQAAAAwAAAB7AAAA0gQAADkwAAA=").object();

  auto expectedNumOccurrences = vector<int>{1, 12, 123, 1234, 12345};

  auto decodedAnswer = decodeQpAnswer("ising", encodedAnswer);
  EXPECT_EQ(decodedAnswer.numOccurrences, expectedNumOccurrences);
}

TEST(QpDecoderTest, noNumOccurrences) {
  auto encodedAnswer = (o, "format", "qp", "energies", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
      "num_variables", 4, "active_variables", "", "solutions", "").object();

  auto decodedAnswer = decodeQpAnswer("ising", encodedAnswer);
  EXPECT_TRUE(decodedAnswer.numOccurrences.empty());
}

TEST(QpDecoderTest, quboHist) {
  auto encodedAnswer = (o, "format", "qp", "energies", "AAAAAADAXsAAAAAAAAAowAAAAAAAAAjAAAAAAAAAEkAAAAAAAMBQQA==",
      "num_variables", 13, "active_variables", "AAAAAAEAAAACAAAAAwAAAAYAAAAHAAAACAAAAAoAAAAMAAAA",
      "num_occurrences", "n4YBALgiAAAJAwAAQgAAAAUAAAA=", "solutions", "kgVVf8oA8YAEDw==").object();

  auto expectedEnergies = vector<double>{-123, -12, -3, 4.5, 67};
  auto expectedNumOccurrences = vector<int>{99999, 8888, 777, 66, 5};
  auto expectedSolutions = vector<char>{
      1, 0, 0, 1, 3, 3, 0, 0, 1, 3, 0, 3, 0,
      0, 1, 0, 1, 3, 3, 0, 1, 0, 3, 1, 3, 0,
      1, 1, 0, 0, 3, 3, 1, 0, 1, 3, 0, 3, 0,
      1, 1, 1, 1, 3, 3, 0, 0, 0, 3, 1, 3, 1,
      0, 0, 0, 0, 3, 3, 0, 1, 0, 3, 0, 3, 0,
  };

  auto decodedAnswer = decodeQpAnswer("qubo", encodedAnswer);
  EXPECT_EQ(expectedEnergies, decodedAnswer.energies);
  EXPECT_EQ(expectedNumOccurrences, decodedAnswer.numOccurrences);
  EXPECT_EQ(expectedSolutions, decodedAnswer.solutions);
}

TEST(QpDecoderTest, quboRaw) {
  auto encodedAnswer = (o, "format", "qp", "energies", "AAAAAAAAAAAAAAAAAEqTQA==", "num_variables", 16,
      "active_variables", "AQAAAAIAAAADAAAACwAAAAwAAAANAAAADwAAAA==", "solutions", "/gA=").object();

  auto expectedEnergies = vector<double>{0, 1234.5};
  auto expectedSolutions = vector<char>{
      3, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 3, 1,
      3, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 3, 0,
  };

  auto decodedAnswer = decodeQpAnswer("qubo", encodedAnswer);
  EXPECT_EQ(expectedEnergies, decodedAnswer.energies);
  EXPECT_TRUE(decodedAnswer.numOccurrences.empty());
  EXPECT_EQ(expectedSolutions, decodedAnswer.solutions);
}

TEST(QpDecoderTest, missingFields) {
  auto goodEncodedAnswer = (o, "format", "qp", "energies", "", "num_variables", 0,
      "active_variables", "", "solutions", "").object();

  auto missingFormat = goodEncodedAnswer;
  missingFormat.erase("format");
  auto missingEnergies = goodEncodedAnswer;
  missingEnergies.erase("energies");
  auto missingNumVars = goodEncodedAnswer;
  missingNumVars.erase("num_variables");
  auto missingActiveVars = goodEncodedAnswer;
  missingActiveVars.erase("active_variables");
  auto missingSolutions = goodEncodedAnswer;
  missingSolutions.erase("solutions");

  decodeQpAnswer("ising", goodEncodedAnswer);
  EXPECT_THROW(decodeQpAnswer("ising", missingFormat), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", missingEnergies), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", missingNumVars), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", missingActiveVars), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", missingSolutions), sapiremote::DecodingException);
}

TEST(QpDecoderTest, badFieldTypes) {
  auto goodEncodedAnswer = (o, "format", "qp", "energies", "", "num_variables", 0,
      "active_variables", "", "solutions", "").object();

  auto badFormat = goodEncodedAnswer;
  badFormat["format"] = json::Null();
  auto badEnergies = goodEncodedAnswer;
  badEnergies["energies"] = json::Null();
  auto badNumVars = goodEncodedAnswer;
  badNumVars["num_variables"] = json::Null();
  auto badActiveVars = goodEncodedAnswer;
  badActiveVars["active_variables"] = json::Null();
  auto badSolutions = goodEncodedAnswer;
  badSolutions["solutions"] = json::Null();

  decodeQpAnswer("ising", goodEncodedAnswer);
  EXPECT_THROW(decodeQpAnswer("ising", badFormat), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", badEnergies), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", badNumVars), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", badActiveVars), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", badSolutions), sapiremote::DecodingException);
}

TEST(QpDecoderTest, badNumVars) {
  auto encodedAnswer = (o, "format", "qp", "energies", "", "num_variables", -1,
      "active_variables", "", "solutions", "").object();
  EXPECT_THROW(decodeQpAnswer("ising", encodedAnswer), sapiremote::DecodingException);
}

TEST(QpDecoderTest, badActiveVars) {
  auto goodAnswer = (o, "format", "qp", "energies", "AAAAAAAAAAA=", "num_variables", 10,
      "active_variables", "AQAAAAIAAAADAAAA", "solutions", "AA==").object();
  auto badAnswer1 = (o, "format", "qp", "energies", "AAAAAAAAAAA=", "num_variables", 10,
      "active_variables", "/////wIAAAADAAAA", "solutions", "AA==").object();
  auto badAnswer2 = (o, "format", "qp", "energies", "AAAAAAAAAAA=", "num_variables", 10,
      "active_variables", "AQAAAAEAAAADAAAA", "solutions", "AA==").object();
  auto badAnswer3 = (o, "format", "qp", "energies", "AAAAAAAAAAA=", "num_variables", 10,
      "active_variables", "AgAAAAEAAAADAAAA", "solutions", "AA==").object();
  auto badAnswer4 = (o, "format", "qp", "energies", "AAAAAAAAAAA=", "num_variables", 10,
      "active_variables", "AQAAAAIAAAAeAAAA", "solutions", "AA==").object();

  decodeQpAnswer("ising", goodAnswer);
  EXPECT_THROW(decodeQpAnswer("ising", badAnswer1), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", badAnswer2), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", badAnswer3), sapiremote::DecodingException);
  EXPECT_THROW(decodeQpAnswer("ising", badAnswer4), sapiremote::DecodingException);
}

TEST(QpDecoderTest, badSolutionsSize) {
  auto encodedAnswer = (o, "format", "qp", "energies", "AAAAAAAAAAA=", "num_variables", 10,
      "active_variables", "AQAAAAIAAAADAAAA", "solutions", "AAAA").object();
  EXPECT_THROW(decodeQpAnswer("ising", encodedAnswer), sapiremote::DecodingException);
}

TEST(QpDecoderTest, badEnergiesNumOccSizes) {
  auto encodedAnswer = (o, "format", "qp", "energies", "AAAAAAAAAAA=", "num_variables", 10,
      "num_occurrences", "ZAAAAAoAAAA=", "active_variables", "AQAAAAIAAAADAAAA", "solutions", "AA==").object();
  EXPECT_THROW(decodeQpAnswer("ising", encodedAnswer), sapiremote::DecodingException);
}
