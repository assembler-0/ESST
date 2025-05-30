#include "../include/core.hpp"
#include <iostream>
#include <cstdlib>

void sse_init(){
	int upper_lm = 0;
	int lower_lm = 0;
	constexpr int size = 8;
	std::cout << "Please enter upperz limit: ";
	std::cin >> upper_lm;
	std::cout << "Please enter lower limit: ";
	std::cin >> lower_lm;
	alignas(32) float n1[size];
	alignas(32) float n2[size];
	alignas(32) float out[size];
	for (int i = 0; i < size; ++i){
		n1[i] = rand() % upper_lm + lower_lm;
		n2[i] = rand() % upper_lm + lower_lm;
	}
	sse(n1, n2, out);
	std::cout << "Result: ";
	for (const float f : out) std::cout << f << " ";
	std::cout << std::endl;
}
int main(){
  sse_init();
  return 0;
}
