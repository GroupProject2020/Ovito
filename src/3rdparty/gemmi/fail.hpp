// Copyright 2017 Global Phasing Ltd.
//
// fail() and unreachable()

#ifndef GEMMI_FAIL_HPP_
#define GEMMI_FAIL_HPP_

#include <stdexcept>  // for runtime_error
#include <string>

namespace gemmi {

[[noreturn]]
inline void fail(const std::string& msg) { throw std::runtime_error(msg); }

// unreachable() is used to silence GCC -Wreturn-type and hint the compiler
[[noreturn]] inline void unreachable() {
#if defined(__GNUC__) || defined(__clang__)
  __builtin_unreachable();
#elif defined(_MSC_VER)
  __assume(0);
#endif
}

} // namespace gemmi
#endif
