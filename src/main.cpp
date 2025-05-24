#include "../include/core.hpp"
#include "../include/CLI11.hpp"
#include <iostream>
int main(){
  //CLI::App ACTS("Advanced CPU Testing System", "ACTS");
  alignas(16) float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
  alignas(16) float b[4] = {5.0f, 6.0f, 7.0f, 8.0f};
  alignas(16) float out[4];
  sse(a, b, out);
  std::cout << "Result: ";
  for (float f : out) std::cout << f << " ";
  std::cout << std::endl;
  return 0;
}
