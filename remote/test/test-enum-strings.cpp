//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <gtest/gtest.h>

#include <types.hpp>

using sapiremote::toString;

namespace submittedstates = sapiremote::submittedstates;
namespace remotestatuses = sapiremote::remotestatuses;
namespace errortypes = sapiremote::errortypes;



TEST(ToStringTest, RemoteStatuses) {
  EXPECT_EQ("PENDING", toString(remotestatuses::PENDING));
  EXPECT_EQ("IN_PROGRESS", toString(remotestatuses::IN_PROGRESS));
  EXPECT_EQ("COMPLETED", toString(remotestatuses::COMPLETED));
  EXPECT_EQ("FAILED", toString(remotestatuses::FAILED));
  EXPECT_EQ("CANCELED", toString(remotestatuses::CANCELED));
  EXPECT_EQ("UNKNOWN", toString(remotestatuses::UNKNOWN));
  EXPECT_EQ("UNKNOWN", toString(static_cast<remotestatuses::Type>(-999)));
}



TEST(ToStringTest, SubmittedStates) {
  EXPECT_EQ("SUBMITTING", toString(submittedstates::SUBMITTING));
  EXPECT_EQ("SUBMITTED", toString(submittedstates::SUBMITTED));
  EXPECT_EQ("DONE", toString(submittedstates::DONE));
  EXPECT_EQ("RETRYING", toString(submittedstates::RETRYING));
  EXPECT_EQ("FAILED", toString(submittedstates::FAILED));
  EXPECT_EQ("UNKNOWN", toString(static_cast<submittedstates::Type>(-999)));
}



TEST(ToStringTest, ErrorTypes) {
  EXPECT_EQ("NETWORK", toString(errortypes::NETWORK));
  EXPECT_EQ("PROTOCOL", toString(errortypes::PROTOCOL));
  EXPECT_EQ("AUTH", toString(errortypes::AUTH));
  EXPECT_EQ("SOLVE", toString(errortypes::SOLVE));
  EXPECT_EQ("MEMORY", toString(errortypes::MEMORY));
  EXPECT_EQ("INTERNAL", toString(errortypes::INTERNAL));
  EXPECT_EQ("INTERNAL", toString(static_cast<errortypes::Type>(-999)));
}
