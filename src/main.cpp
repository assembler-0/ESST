#include "core.hpp"
#include "pcg_random.hpp"
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <chrono>
class acts {
public:
	void init() {
		std::string op_mode;
		std::unordered_map<std::string, std::function<void()>> command_map = {
			{"exit", [this]() { running = false; }},
			{"menu", [this]() { intermediate(val_intermediate[0]); }},
			{"avx",  [this]() { intermediate(val_intermediate[1]); }}
		};

		std::cout << "Welcome to ACTS version " << APP_VERSION << std::endl;

		while (running) {
			std::cout << "[ACTS] >> ";
			std::getline(std::cin, op_mode);

			if (std::cin.eof()) {
				std::cout << "Exiting..." << std::endl;
				return;
			}
			if (auto it = command_map.find(op_mode); it != command_map.end()) {
				it->second();
			}
		}
	}
private:
	bool running = true;
	char val_intermediate[8] = {'m', 'a', 's'};
	unsigned int threads = std::thread::hardware_concurrency();
	std::vector<std::thread> thread_pool;
	static constexpr auto APP_VERSION = "1.0";
	static void intermediate(char func){
		switch (func){
			case 'm': {
				menu();
				break;
			}
			case 'a': {
				for (int i = 0; i < threads; ++i) thread_pool.emplace_back(avx_init, i);
				for (auto& thread : thread_pool) thread.join();
				break;
			}
			default:{
				break;
			}
		}
	}
	static void menu(){
		std::cout << "...\n";
	}
	static void avx_init(){
		std::cout << "AVX test!\n";
		int upper_lm = 0;
		int lower_lm = 0;
		static constexpr int size = 8;
		unsigned int iterations = 0;
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
		std::uniform_int_distribution<float> gen_float_lm(lower_lm, upper_lm);
		while (iterations > 0){
			for (int i = 0; i < size; ++i){
				n1[i] = gen_float_lm(gen);
				n2[i] = gen_float_lm(gen);
			}
			avx(n1, n2, out);
			iterations -= 1;
		}
		std::cout << "Result: ";
		for (const float i : out){std::cout << i << " ";}
		std::cout << std::endl;
	}
};
int main(){
	acts test;
		test.init();
  return 0;
}
