//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef BASE64_HPP_INCLUDED
#define BASE64_HPP_INCLUDED

#include <cstddef>
#include <string>
#include <vector>

namespace sapiremote {

std::string encodeBase64(const void* data, std::size_t len);

template<typename T>
std::string encodeBase64(const std::vector<T> data) {
  return encodeBase64(data.data(), data.size() * sizeof(T));
}

std::vector<unsigned char> decodeBase64(const std::string& b64data);

} // namespace sapiremote

#endif
