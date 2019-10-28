//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef _WIN32
#include <crtdbg.h>
void preventWindowsDebugWindow() {
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
}

#else
void preventWindowsDebugWindow() {}
#endif

int main(int argc, char* argv[]) {
  preventWindowsDebugWindow();
  testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
