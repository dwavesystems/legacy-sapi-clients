//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <vector>

#include <gtest/gtest.h>

#include <base64.hpp>
#include <exceptions.hpp>

using std::vector;

using sapiremote::encodeBase64;
using sapiremote::decodeBase64;

TEST(DecodeBase64Test, fullRange) {
  auto fullRange = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  auto expected = vector<unsigned char>{
        0,  16, 131,  16,  81, 135,  32, 146, 139,  48, 211, 143,  65,  20, 147,  81,
       85, 151,  97, 150, 155, 113, 215, 159, 130,  24, 163, 146,  89, 167, 162, 154,
      171, 178, 219, 175, 195,  28, 179, 211,  93, 183, 227, 158, 187, 243, 223, 191};

  EXPECT_EQ(expected, decodeBase64(fullRange));
}

TEST(DecodeBase64Test, padding) {
  auto zeros3 = vector<unsigned char>{0, 0, 0};
  auto zeros2 = vector<unsigned char>{0, 0};
  auto zeros1 = vector<unsigned char>{0};
  auto ones3 = vector<unsigned char>{0xff, 0xff, 0xff};
  auto ones2 = vector<unsigned char>{0xff, 0xff};
  auto ones1 = vector<unsigned char>{0xff};
  EXPECT_EQ(zeros3, decodeBase64("AAAA"));
  EXPECT_EQ(zeros2, decodeBase64("AAA="));
  EXPECT_EQ(zeros1, decodeBase64("AA=="));
  EXPECT_EQ(ones3, decodeBase64("////"));
  EXPECT_EQ(ones2, decodeBase64("///="));
  EXPECT_EQ(ones1, decodeBase64("//=="));
}

TEST(DecodeBase64Test, empty) {
  EXPECT_EQ(vector<unsigned char>{}, decodeBase64(""));
}

TEST(DecodeBase64Test, ignore) {
  auto z = vector<unsigned char>{0, 0};
  EXPECT_EQ(z, decodeBase64("\nA\r\r\nAA=\n\n\n\r\n"));
}

TEST(DecodeBase64Test, badChars) {
  EXPECT_THROW(decodeBase64("AAA?"), sapiremote::Base64Exception);
}

TEST(DecodeBase64Test, badPadding) {
  EXPECT_THROW(decodeBase64("A==="), sapiremote::Base64Exception);
  EXPECT_THROW(decodeBase64("AAA"), sapiremote::Base64Exception);
  EXPECT_THROW(decodeBase64("AAAA===="), sapiremote::Base64Exception);
  EXPECT_THROW(decodeBase64("AAA=AAAA"), sapiremote::Base64Exception);
}

TEST(EncodeBase64Test, fullRangeRaw) {
  auto data = vector<unsigned char>{
        0,  16, 131,  16,  81, 135,  32, 146, 139,  48, 211, 143,  65,  20, 147,  81,
       85, 151,  97, 150, 155, 113, 215, 159, 130,  24, 163, 146,  89, 167, 162, 154,
      171, 178, 219, 175, 195,  28, 179, 211,  93, 183, 227, 158, 187, 243, 223, 191};
  auto expected = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  EXPECT_EQ(expected, encodeBase64(data.data(), data.size()));
}

TEST(EncodeBase64Test, empty) {
  EXPECT_EQ("", encodeBase64(0, 0));
}

TEST(EncodeBase64Test, padding) {
  auto data = "123456";
  EXPECT_EQ("MQ==", encodeBase64(data, 1));
  EXPECT_EQ("MTI=", encodeBase64(data, 2));
  EXPECT_EQ("MTIz", encodeBase64(data, 3));
  EXPECT_EQ("MTIzNA==", encodeBase64(data, 4));
  EXPECT_EQ("MTIzNDU=", encodeBase64(data, 5));
  EXPECT_EQ("MTIzNDU2", encodeBase64(data, 6));
}

TEST(EncodeBase64Test, vector) {
  EXPECT_EQ("AAAAAADgXsAAAAAAADiPQH3DlCWtSbJU", encodeBase64(vector<double>{-123.5, 999, 1e100}));
  EXPECT_EQ("n4YBAHUnAACgWwAA/////w==", encodeBase64(vector<int>{99999, 10101, 23456, -1}));
}
