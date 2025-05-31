#include "core.hpp"
#include "pcg_random.hpp"
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <chrono>
#include <vector>

class acts {
public:
    void init() {
        std::string op_mode;
        std::unordered_map<std::string, std::function<void()>> command_map = {
            {"exit", [this]() { running = false; }},
            {"menu", [this]() { this->showMenu(); }},
            {"avx",  [this]() { this->initAvx(); }}
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
            if (op_mode.empty()){} else {
                std::cout << "Invalid command: " << op_mode << std::endl;
            }
        }
    }

private:
    bool running = true;
    unsigned int num_threads = std::thread::hardware_concurrency();
    static constexpr auto APP_VERSION = "1.0";
    static constexpr int AVX_ARRAY_SIZE = 8;

    void showMenu() { // Non-static
        std::cout << "...\n";
    }

    void initAvx() {
        unsigned long iterations = 0;
        int upper_lm = 0;
        int lower_lm = 0;

        std::cout << "Iterations?: ";
        std::cin >> iterations;
        if (std::cin.fail() || iterations == 0) {
            std::cout << "Invalid iterations. Please enter a positive integer." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }

        std::cout << "Upper limit?: ";
        std::cin >> upper_lm;
        if (std::cin.fail()) {
            std::cout << "Invalid upper limit." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }

        std::cout << "Lower limit?: ";
        std::cin >> lower_lm;
        if (std::cin.fail()) {
            std::cout << "Invalid lower limit." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }

        if (lower_lm > upper_lm) {
            std::cout << "Error: Lower limit cannot be greater than upper limit." << std::endl;
            return;
        }

        std::vector<std::thread> current_avx_threads;
        for (unsigned int i = 0; i < num_threads; ++i) {
            current_avx_threads.emplace_back(avxWorker, iterations / num_threads, lower_lm, upper_lm, i);
        }

        unsigned int remaining_iterations = iterations % num_threads;
        if (remaining_iterations > 0) {
            current_avx_threads.emplace_back(avxWorker, remaining_iterations, lower_lm, upper_lm, num_threads);
        }


        for (auto& thread : current_avx_threads) {
            thread.join();
        }

    }

    // This function will be executed by each thread
    static void avxWorker(unsigned long iterations_per_thread, int lower_lm, int upper_lm, int thread_id) {
        std::cout << "AVX test! (Thread " << thread_id << ")\n";

        // Seed the random number generator differently for each thread
        // Using thread_id as part of the seed for simplicity, but a more robust
        // seeding strategy might involve std::random_device.
        pcg32 gen(42u + thread_id, 54u + thread_id);
        std::uniform_real_distribution<float> gen_float_lm(static_cast<float>(lower_lm), static_cast<float>(upper_lm));

        alignas(32) float n1[AVX_ARRAY_SIZE];
        alignas(32) float n2[AVX_ARRAY_SIZE];
        alignas(32) float out[AVX_ARRAY_SIZE]; // Each thread has its own output array

        for (unsigned long iter = 0; iter < iterations_per_thread; ++iter) {
            for (int i = 0; i < AVX_ARRAY_SIZE; ++i) {
                n1[i] = gen_float_lm(gen);
                n2[i] = gen_float_lm(gen);
            }
            avx(n1, n2, out);
        }

        std::cout << "Thread " << thread_id << " Result: ";
        for (const float i : out) { std::cout << i << " "; }
        std::cout << std::endl;
    }
};

int main() {
    acts test;
    test.init();
    return 0;
}