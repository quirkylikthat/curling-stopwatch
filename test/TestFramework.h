// Minimal single-header test framework (no dependencies).
//
//   TEST(my_test) { CHECK(1 + 1 == 2); CHECK_EQ(a, b); CHECK_STR(s, "x"); }
//
// Build & run:  make -C test      (see test/Makefile)
#pragma once

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace tf {

struct Case {
    const char* name;
    void (*fn)();
};

inline std::vector<Case>& registry() {
    static std::vector<Case> r;
    return r;
}

inline int& failures() {
    static int f = 0;
    return f;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline int runAll() {
    int failedCases = 0;
    for (const Case& c : registry()) {
        const int before = failures();
        c.fn();
        const bool ok = failures() == before;
        if (!ok) ++failedCases;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", c.name);
    }
    std::printf("\n%zu tests, %d failed\n", registry().size(), failedCases);
    return failedCases == 0 ? 0 : 1;
}

}  // namespace tf

#define TEST(name)                                         \
    static void name();                                    \
    static tf::Registrar name##_registrar(#name, &name);   \
    static void name()

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("    %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond); \
            ++tf::failures();                                                    \
        }                                                                        \
    } while (0)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        if (!((a) == (b))) {                                                               \
            std::printf("    %s:%d: CHECK_EQ(%s, %s) failed\n", __FILE__, __LINE__, #a, #b); \
            ++tf::failures();                                                              \
        }                                                                                  \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                                                   \
    do {                                                                                        \
        if (std::fabs(static_cast<double>(a) - static_cast<double>(b)) > (eps)) {               \
            std::printf("    %s:%d: CHECK_NEAR(%s, %s) failed: %f vs %f\n", __FILE__, __LINE__, \
                        #a, #b, static_cast<double>(a), static_cast<double>(b));                \
            ++tf::failures();                                                                   \
        }                                                                                       \
    } while (0)

#define CHECK_STR(a, b)                                                                       \
    do {                                                                                      \
        if (std::strcmp((a), (b)) != 0) {                                                     \
            std::printf("    %s:%d: CHECK_STR failed: \"%s\" vs \"%s\"\n", __FILE__, __LINE__, \
                        (a), (b));                                                            \
            ++tf::failures();                                                                 \
        }                                                                                     \
    } while (0)
