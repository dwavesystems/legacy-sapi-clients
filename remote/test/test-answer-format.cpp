//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <gtest/gtest.h>

#include <coding.hpp>

#include "json-builder.hpp"

using sapiremote::answerFormat;

namespace {
auto o = jsonObject();
auto a = jsonArray();
}

TEST(AnswerFormatTest, none) {
  EXPECT_EQ(sapiremote::FORMAT_NONE, answerFormat(json::Null()));
  EXPECT_EQ(sapiremote::FORMAT_NONE, answerFormat(123));
  EXPECT_EQ(sapiremote::FORMAT_NONE, answerFormat("stuff"));
  EXPECT_EQ(sapiremote::FORMAT_NONE, answerFormat( (o, "no-format", "here").value() ));
}

TEST(AnswerFormatTest, unknown) {
  EXPECT_EQ(sapiremote::FORMAT_UNKNOWN, answerFormat( (o, "format", "who knows?").value() ));
}

TEST(AnswerFormatTest, qp) {
  EXPECT_EQ(sapiremote::FORMAT_QP, answerFormat( (o, "format", "qp").value() ));
}
