// SPDX-License-Identifier: BSL-1.0

#include "harness.hpp"

void run_optional_tests();
void run_result_tests();
void run_span_tests();

int main() {
  run_optional_tests();
  run_result_tests();
  run_span_tests();

  return harness::finish();
}