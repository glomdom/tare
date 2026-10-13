#pragma once

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace harness {

struct Failure {
  std::string expr;
  std::string expected;
  std::string received;
  std::string detail;

  bool has_values = false;
  const char* file = nullptr;
  int line = 0;
};

struct State {
  int depth = 0;
  int tests_passed = 0;
  int tests_failed = 0;
  int checks_passed = 0;
  int checks_failed = 0;
  int checks_in_test = 0;

  std::vector<const char*> scopes;
  std::vector<Failure> failures;
  std::vector<std::string> failed_tests;
};

inline State& state() {
  static State s;

  return s;
}

inline void indent(int extra) {
  const int total = state().depth + extra;

  for (int i = 0; i < total; i += 1) {
    std::printf("  ");
  }
}

inline std::string quote(std::string_view text, char delim) {
  std::string out;
  out.reserve(text.size() + 2);
  out += delim;

  for (const char ch : text) {
    switch (ch) {
      case '\n': {
        out += "\\n";

        break;
      }

      case '\t': {
        out += "\\t";

        break;
      }

      case '\r': {
        out += "\\r";

        break;
      }

      case '\0': {
        out += "\\0";

        break;
      }

      default: {
        const auto byte = static_cast<unsigned char>(ch);

        if (ch == delim || ch == '\\') {
          out += '\\';
          out += ch;
        } else if (byte < 0x20 || byte == 0x7f) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\x%02x", byte);
          out += buf;
        } else {
          out += ch;
        }

        break;
      }
    }
  }

  out += delim;

  return out;
}

template <typename T>
std::string show(const T& v) {
  using U = std::remove_cvref_t<T>;

  if constexpr (std::is_same_v<U, bool>) {
    return v ? "true" : "false";
  } else if constexpr (std::is_same_v<U, char>) {
    return quote(std::string_view{&v, 1}, '\'');
  } else if constexpr (std::is_integral_v<U>) {
    return std::to_string(v);
  } else if constexpr (std::is_floating_point_v<U>) {
    char buf[64];
    const auto result = std::to_chars(buf, buf + sizeof(buf), v);

    return std::string(buf, result.ptr);
  } else if constexpr (std::is_null_pointer_v<U>) {
    return "nullptr";
  } else if constexpr (std::is_pointer_v<U>) {
    if (v == nullptr) {
      return "nullptr";
    }

    if constexpr (std::is_same_v<std::remove_cv_t<std::remove_pointer_t<U>>, char>) {
      return quote(v, '"');
    } else {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%p", static_cast<const volatile void*>(v));

      return buf;
    }
  } else if constexpr (std::is_convertible_v<const U&, std::string_view>) {
    return quote(std::string_view{v}, '"');
  } else if constexpr (std::is_enum_v<U>) {
    return show(static_cast<std::underlying_type_t<U>>(v));
  } else if constexpr (requires(std::ostream& os, const U& x) { os << x; }) {
    std::ostringstream os;
    os << v;

    return os.str();
  } else {
    return "<unprintable>";
  }
}

inline bool check(bool ok, const char* expr, const char* file, int line) {
  State& s = state();
  s.checks_in_test += 1;

  if (ok) {
    s.checks_passed += 1;

    return true;
  }

  Failure f;
  f.expr = expr;
  f.file = file;
  f.line = line;

  s.checks_failed += 1;
  s.failures.push_back(std::move(f));

  return false;
}

template <typename TLeft, typename TRight>
inline bool check_eq(const TLeft& a, const TRight& b, const char* ea, const char* eb, const char* file, int line) {
  State& s = state();
  s.checks_in_test += 1;

  if (a == b) {
    s.checks_passed += 1;

    return true;
  }

  Failure f;
  f.expr = std::string{ea} + " == " + eb;
  f.expected = show(b);
  f.received = show(a);
  f.has_values = true;
  f.file = file;
  f.line = line;

  s.checks_failed += 1;
  s.failures.push_back(std::move(f));

  return false;
}

inline void print_failure(const Failure& f) {
  indent(3);
  std::printf("\x1b[31m%s\x1b[0m\n", f.expr.c_str());

  if (f.has_values) {
    indent(4);
    std::printf("\x1b[90mexpected\x1b[0m \x1b[32m%s\x1b[0m\n", f.expected.c_str());
    indent(4);
    std::printf("\x1b[90mreceived\x1b[0m \x1b[31m%s\x1b[0m\n", f.received.c_str());
  } else if (!f.detail.empty()) {
    indent(4);
    std::printf("\x1b[90mwhat\x1b[0m     \x1b[31m%s\x1b[0m\n", f.detail.c_str());
  } else {
    indent(4);
    std::printf("\x1b[90mwas false\x1b[0m\n");
  }

  if (f.file != nullptr) {
    indent(4);
    std::printf("\x1b[90mat %s:%d\x1b[0m\n", f.file, f.line);
  }
}

template <typename TBody>
void describe(const char* name, TBody&& body) {
  State& s = state();

  indent(1);
  std::printf("\x1b[4m%s\x1b[0m\n", name);

  s.scopes.push_back(name);
  s.depth += 1;
  body();
  s.depth -= 1;
  s.scopes.pop_back();
}

template <typename TBody>
void it(const char* name, TBody&& body) {
  State& s = state();

  const std::size_t mark = s.failures.size();
  s.checks_in_test = 0;
  const auto start = std::chrono::steady_clock::now();

  try {
    body();
  } catch (const std::exception& e) {
    Failure f;
    f.expr = "uncaught exception";
    f.detail = e.what();

    s.checks_in_test += 1;
    s.checks_failed += 1;
    s.failures.push_back(std::move(f));
  } catch (...) {
    Failure f;
    f.expr = "uncaught exception";
    f.detail = "<not a std::exception>";

    s.checks_in_test += 1;
    s.checks_failed += 1;
    s.failures.push_back(std::move(f));
  }

  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  const std::size_t failed = s.failures.size() - mark;

  indent(1);

  if (failed == 0) {
    s.tests_passed += 1;
    std::printf("\x1b[32m+\x1b[0m %s \x1b[90m(%d checks, %.3f ms)\x1b[0m\n", name, s.checks_in_test, ms);

    return;
  }

  s.tests_failed += 1;
  std::printf("\x1b[31m- %s\x1b[0m \x1b[90m(%zu of %d checks failed, %.3f ms)\x1b[0m\n", name, failed, s.checks_in_test, ms);

  std::string path;

  for (const char* scope : s.scopes) {
    path += scope;
    path += " > ";
  }

  path += name;
  s.failed_tests.push_back(std::move(path));

  for (std::size_t i = mark; i < s.failures.size(); i += 1) {
    print_failure(s.failures[i]);
  }
}

inline int finish() {
  State& s = state();
  const int tests = s.tests_passed + s.tests_failed;
  const int checks = s.checks_passed + s.checks_failed;

  if (!s.failed_tests.empty()) {
    std::printf("\n  Failed:\n");

    for (const std::string& path : s.failed_tests) {
      std::printf("    \x1b[31m- %s\x1b[0m\n", path.c_str());
    }
  }

  if (s.tests_failed == 0) {
    std::printf("\n\x1b[42;30m PASS \x1b[0m \n\n");
  } else {
    std::printf("\n\x1b[41;37m FAIL \x1b[0m \n\n");
  }

  std::printf("  Tests   \x1b[32m%d passed\x1b[0m", s.tests_passed);

  if (s.tests_failed > 0) {
    std::printf(", \x1b[31m%d failed\x1b[0m", s.tests_failed);
  }

  std::printf(", %d total\n", tests);
  std::printf("  Checks  \x1b[32m%d passed\x1b[0m", s.checks_passed);

  if (s.checks_failed > 0) {
    std::printf(", \x1b[31m%d failed\x1b[0m", s.checks_failed);
  }

  std::printf(", %d total\n\n", checks);

  return s.tests_failed == 0 ? 0 : 1;
}

} // namespace harness

#define CHECK(...) harness::check((__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)
#define CHECK_EQ(a, b) harness::check_eq((a), (b), #a, #b, __FILE__, __LINE__)