//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/foreach.hpp>

#include <base64.hpp>
#include <exceptions.hpp>

using std::string;
using std::vector;

using sapiremote::Base64Exception;

namespace {
namespace base64 {

const unsigned int ign = 0x100;
const unsigned int bad = 0x101;
const unsigned int pad = 0x102;
const unsigned int decode[128] = {
    /*   0 */ bad, bad, bad, bad, bad, bad, bad, bad,   bad, bad, ign, bad, bad, ign, bad, bad,
    /*  16 */ bad, bad, bad, bad, bad, bad, bad, bad,   bad, bad, bad, bad, bad, bad, bad, bad,
    /*  32 */ bad, bad, bad, bad, bad, bad, bad, bad,   bad, bad, bad,  62, bad, bad, bad,  63,
    /*  48 */  52,  53,  54,  55,  56,  57,  58,  59,    60,  61, bad, bad, bad, pad, bad, bad,
    /*  64 */ bad,   0,   1,   2,   3,   4,   5,   6,     7,   8,   9,  10,  11,  12,  13,  14,
    /*  80 */  15,  16,  17,  18,  19,  20,  21,  22,    23,  24,  25, bad, bad, bad, bad, bad,
    /*  96 */ bad,  26,  27,  28,  29,  30,  31,  32,    33,  34,  35,  36,  37,  38,  39,  40,
    /* 112 */  41,  42,  43,  44,  45,  46,  47,  48,    49,  50,  51, bad, bad, bad, bad, bad,
};
const int decodeSize = sizeof(decode) / sizeof(decode[0]);

const int blockSize = 4;
const int encodedBits = 6;
const int minPaddingIndex = 2;

const auto encode = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const auto padChar = '=';

} // namespace {anonymous}::base64
} // namespace {anonymous}

namespace sapiremote {

string encodeBase64(const void* vdata, size_t len) {
  auto data = static_cast<const unsigned char*>(vdata);

  if (len > (string::npos - 1) / 4 * 3) throw std::length_error("sapiremote::encodeBase64");
  auto b64size = (len + 2) / 3 * 4;
  auto b64 = string(b64size, '\0');
  auto out = b64.begin();

  auto numFullBlocks = len / 3;
  for (auto i = 0u; i < numFullBlocks; ++i) {
    unsigned block = (static_cast<unsigned int>(data[0]) << 16)
        | (static_cast<unsigned int>(data[1]) << 8) | data[2];
    *out++ = base64::encode[(block >> 18) & 0x3f];
    *out++ = base64::encode[(block >> 12) & 0x3f];
    *out++ = base64::encode[(block >> 6) & 0x3f];
    *out++ = base64::encode[block & 0x3f];
    data += 3;
  }

  switch (len % 3) {
    case 0: break;
    case 1:
      {
        unsigned block = (static_cast<unsigned int>(data[0]) << 16);;
        *out++ = base64::encode[(block >> 18) & 0x3f];
        *out++ = base64::encode[(block >> 12) & 0x3f];
        *out++ = base64::padChar;
        *out++ = base64::padChar;
      }
      break;
    case 2:
      {
        unsigned block = (static_cast<unsigned int>(data[0]) << 16)
            | (static_cast<unsigned int>(data[1]) << 8);
        *out++ = base64::encode[(block >> 18) & 0x3f];
        *out++ = base64::encode[(block >> 12) & 0x3f];
        *out++ = base64::encode[(block >> 6) & 0x3f];
        *out++ = base64::padChar;
      }
      break;
  }

  assert(out == b64.end());
  return b64;
}

vector<unsigned char> decodeBase64(const string& b64data) {
  vector<unsigned char> data;
  data.reserve(b64data.size() * 3 / 4);

  auto byteIndex = 0;
  auto padding = 0;
  auto finished = false;
  unsigned int block = 0;
  BOOST_FOREACH( char c, b64data ) {
    if (c < 0 || c >= base64::decodeSize) throw Base64Exception();
    auto d = base64::decode[c];
    if (d == base64::ign) continue;
    if (finished) throw Base64Exception();

    switch (d) {
      case base64::bad: throw Base64Exception();
      case base64::pad:
        if (padding == 0 && byteIndex < base64::minPaddingIndex) throw Base64Exception();
        block <<= base64::encodedBits;
        ++padding;
        ++byteIndex;
        break;
      default:
        if (padding) throw Base64Exception();
        block = (block << base64::encodedBits) | d;
        ++byteIndex;
        break;
    }

    if (byteIndex == base64::blockSize) {
      data.push_back(static_cast<unsigned char>(block >> 16));
      if (padding < 2) data.push_back(static_cast<unsigned char>((block >> 8) & 0xff));
      if (padding < 1) data.push_back(static_cast<unsigned char>(block & 0xff));
      finished = padding > 0;
      byteIndex = 0;
      block = 0;
    }
  }

  if (byteIndex != 0) throw Base64Exception();
  return data;
}

} // namespace sapiremote
