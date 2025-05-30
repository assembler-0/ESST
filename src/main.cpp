#include "core.hpp"
#include "pcg_random.hpp"
#include <iostream>
#include <random>
class acts {
public:
	void init(){

	}
private:
	char menu(){

	}
	void sse_init(){
		int upper_lm = 0;
		int lower_lm = 0;
		constexpr int size = 8;
		unsigned int iterations = 0;
		unsigned int loop_start = 0;
		pcg32 gen(42u, 54u);
		std::cout << "Iterations?: ";
		std::cin >> iterations;
		std::cout << "Upper limit?: ";
		std::cin >> upper_lm;
		std::cout << "Lower limit?: ";
		std::cin >> lower_lm;
		alignas(32) float n1[size];
		alignas(32) float n2[size];
		alignas(32) float out[size];
		std::uniform_int_distribution gen_int_lm(lower_lm, upper_lm);
		while (iterations > 0){
			for (int i = 0; i < size; ++i){
				n1[i] = gen_int_lm(gen);
				n2[i] = gen_int_lm(gen);
			}
			avx(n1, n2, out);
			iterations -= 1;
		}
		std::cout << "Result: ";
		for (const float f : out) std::cout << f << " ";
		std::cout << std::endl;
	}
};
int main(){
  acts test;
	test.init();

  return 0;
}
