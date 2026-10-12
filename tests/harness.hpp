// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace harness {

struct Failure {
    const char* file;
    int line;

    std::string message;
};

struct State {
    int depth = 0;
    int tests_passed = 0;
    int tests_failed = 0;
    int checks_passed = 0;
    int checks_failed = 0;
    int checks_in_test = 0;

    std::vector<Failure> failures;
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

inline std::string show(bool v) {
    return v ? "true" : "false";
}

inline std::string show(char v) {
    return std::string{"'"} + v + "'";
}

inline std::string show(signed char v) {
    return std::to_string(static_cast<int>(v));
}

inline std::string show(unsigned char v) {
    return std::to_string(static_cast<unsigned>(v));
}

inline std::string show(short v) {
    return std::to_string(v);
}

inline std::string show(unsigned short v) {
    return std::to_string(v);
}

inline std::string show(int v) {
    return std::to_string(v);
}

inline std::string show(unsigned v) {
    return std::to_string(v);
}

inline std::string show(long v) {
    return std::to_string(v);
}

inline std::string show(unsigned long v) {
    return std::to_string(v);
}

inline std::string show(long long v) {
    return std::to_string(v);
}

inline std::string show(unsigned long long v) {
    return std::to_string(v);
}

inline std::string show(double v) {
    return std::to_string(v);
}

inline std::string show(const char* v) {
    return std::string{v};
}

template <typename T>
std::string show(const T&) {
    return "<unprintable>";
}

inline bool check(bool ok, const char* expr, const char* file, int line) {
    State& s = state();

    s.checks_in_test += 1;

    if (ok) {
        s.checks_passed += 1;

        return true;
    }

    s.checks_failed += 1;
    s.failures.push_back(Failure{file, line, std::string{expr} + "  →  was false"});

    return false;
}

template <typename TLeft, typename TRight>
bool check_eq(const TLeft& a, const TRight& b, const char* ea, const char* eb, const char* file, int line) {
    State& s = state();

    s.checks_in_test += 1;

    if (a == b) {
        s.checks_passed += 1;

        return true;
    }

    s.checks_failed += 1;
    s.failures.push_back(Failure{
        file,
        line,
        std::string{ea} + " == " + eb + "  →  expected " + show(b) + ", got " + show(a),
    });

    return false;
}

template <typename TBody>
void describe(const char* name, TBody&& body) {
    State& s = state();

    indent(1);
    std::printf("\x1b[1m%s\x1b[0m\n", name);

    s.depth += 1;
    body();
    s.depth -= 1;
}

template <typename TBody>
void it(const char* name, TBody&& body) {
    State& s = state();

    const std::size_t mark = s.failures.size();
    s.checks_in_test = 0;

    const auto start = std::chrono::steady_clock::now();
    body();

    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    const bool ok = s.failures.size() == mark;

    indent(1);

    if (ok) {
        s.tests_passed += 1;
        std::printf("\x1b[32mY\x1b[0m %s \x1b[90m(%d checks, %.3f ms)\x1b[0m\n", name, s.checks_in_test, ms);

        return;
    }

    s.tests_failed += 1;
    std::printf("\x1b[31mN %s\x1b[0m \x1b[90m(%d checks, %.3f ms)\x1b[0m\n", name, s.checks_in_test, ms);

    for (std::size_t i = mark; i < s.failures.size(); i += 1) {
        const Failure& f = s.failures[i];

        indent(3);
        std::printf("\x1b[31m%s\x1b[0m\n", f.message.c_str());
        indent(3);
        std::printf("\x1b[90mat %s:%d\x1b[0m\n", f.file, f.line);
    }
}

inline void begin(const char* label) {
    std::printf("\x1b[90mRUNS\x1b[0m  %s\n\n", label);
}

inline int finish(const char* label) {
    State& s = state();
    const int tests = s.tests_passed + s.tests_failed;
    const int checks = s.checks_passed + s.checks_failed;

    if (s.tests_failed == 0) {
        std::printf("\n\x1b[42;30m PASS \x1b[0m %s\n\n", label);
    } else {
        std::printf("\n\x1b[41;37m FAIL \x1b[0m %s\n\n", label);
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

}

#define CHECK(...) harness::check((__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)
#define CHECK_EQ(a, b) harness::check_eq((a), (b), #a, #b, __FILE__, __LINE__)