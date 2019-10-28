//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <iostream>
#include <fstream>
#include <string>
#include <streambuf>
#include <chrono>

#include <json.hpp>

using std::cout;
using std::ifstream;
using std::istreambuf_iterator;
using std::string;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

int main(int argc, char* argv[]) {

  for (auto i = 1; i < argc; ++i) {
    ifstream file(argv[i]);
    auto s = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
    auto t0 = system_clock::now();
    for (auto j = 0; j < 1000; ++j) {
      auto v = json::stringToJson(s);
    }
    auto t1 = system_clock::now();
    cout << argv[i] << ": " << duration_cast<milliseconds>(t1 - t0).count() << "\n";
  }
  return 0;
}
