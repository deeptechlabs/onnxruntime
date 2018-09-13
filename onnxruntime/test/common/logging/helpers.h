#pragma once

#include <sstream>

#include "date/date.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "core/common/logging/capture.h"
#include "core/common/logging/isink.h"

class MockSink : public ::onnxruntime::Logging::ISink {
 public:
  MOCK_METHOD3(SendImpl, void(const ::onnxruntime::Logging::Timestamp& timestamp,
                              const std::string& logger_id,
                              const ::onnxruntime::Logging::Capture& message));
};

// The ACTION*() macros trigger warning C4100 (unreferenced formal
// parameter) in MSVC with -W4.  Unfortunately they cannot be fixed in
// the macro definition, as the warnings are generated when the macro
// is expanded and macro expansion cannot contain #pragma.  Therefore
// we suppress them here.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#endif

ACTION(PrintArgs) {
  using date::operator<<;

  // const Timestamp &timestamp, const std::string &logger_id, const Message &message
  //                  arg0                          arg1                        arg2
  std::cout << arg1 << "@" << arg0 << " "
            << arg2.SeverityPrefix() << ":" << arg2.Category() << ":"
            << arg2.Location().ToString(::onnxruntime::CodeLocation::kFilenameAndPath) << " " << arg2.Message() << std::endl;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif