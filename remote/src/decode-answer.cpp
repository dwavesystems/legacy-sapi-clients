//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <coding.hpp>
#include <json.hpp>

namespace {
const auto formatKey = "format";
namespace formats {
const auto qp = "qp";
} // namespace {anonymous}::formats
} // namespace {anonymous}

namespace sapiremote {

AnswerFormat answerFormat(const json::Value& answer) {
  if (!answer.isObject()) return FORMAT_NONE;
  const auto& answerObj = answer.getObject();
  auto formatIter = answerObj.find(formatKey);
  if (formatIter == answerObj.end()) return FORMAT_NONE;

  if (formatIter->second.isString() && formatIter->second.getString() == formats::qp) {
    return FORMAT_QP;
  } else {
    return FORMAT_UNKNOWN;
  }
}

} // namespace sapiremote
