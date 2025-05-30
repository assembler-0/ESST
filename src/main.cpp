#include "core.hpp"
#include "pcg_random.hpp"
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <vector>
#include <chrono>
class acts {
public:
	void init() {
		std::string op_mode;
		std::unordered_map<std::string, std::function<void()>> command_map = {
			{"exit", [this]() { running = false; }},
			{"mode", [this]() { menu(); }},
			{"avx",  [this]() { avx_init(); }}
		};

		std::cout << "Welcome to ACTS version " << APP_VERSION << std::endl;

		while (running) {
			std::cout << "[ACTS] >> ";
			std::getline(std::cin, op_mode);

			if (std::cin.eof()) {
				std::cout << "Exiting..." << std::endl;
				return;
			}

			auto it = command_map.find(op_mode);
			if (it != command_map.end()) {
				it->second();  //Call the function
			}
		}
	}
private:
	bool running = true;
	static constexpr auto APP_VERSION = "1.0";

	void menu(){
		std::cout << "...\n";
	}
	void avx_init(){
		std::cout << "AVX test!\n";
		int upper_lm = 0;
		int lower_lm = 0;
		static constexpr int size = 8;
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
		for (int i : out){std::cout << i << " ";}
		std::cout << std::endl;
	}
};
int main(){
	acts test;
		test.init();

  return 0;
}
