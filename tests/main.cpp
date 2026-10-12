#include "harness.hpp"

void run_optional_tests();
void run_result_tests();

int main() {
  run_optional_tests();
  run_result_tests();

  return harness::finish();
}