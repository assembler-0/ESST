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
            {"avx",  [this]() { this->initAvx(); }},
            {"3np1",  [this]() { this->init3np1(); }}
        };

        std::cout << "ACTS version " << APP_VERSION << std::endl;

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
            if (op_mode.empty()){}
        }
    }

private:
    bool running = true;
    unsigned int num_threads = std::thread::hardware_concurrency();
    static constexpr auto APP_VERSION = "0.1";
    static constexpr int AVX_ARRAY_SIZE = 8;

    void showMenu() { // Non-static
        std::cout << "...\n";
    }
    void init3np1(){
        unsigned long upper_lm = 0;
        unsigned long lower_lm = 0;
        unsigned long iterations = 0;
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
        std::vector<std::thread> current_cp_threads;
        for (unsigned int i = 0; i < num_threads; ++i) {
            current_cp_threads.emplace_back(p3np1, iterations, upper_lm, lower_lm, i);
        }
        for (auto& thread : current_cp_threads) {
            thread.join();
        }
    }
    volatile static void p3np1(const long iterations, const long upper_lm, const long lower_lm, const int thread_id){
        const auto startTime = std::chrono::steady_clock::now();
        pcg32 gen(42u + thread_id, 54u + thread_id);
        std::uniform_int_distribution<unsigned long> gen_long_lm(lower_lm, upper_lm);
        unsigned long total_steps = 0;
        for (int i = 0; i < iterations; ++i) {
            unsigned long steps = 0;
            p3np1E(gen_long_lm(gen), &steps);
            total_steps += steps;
        }
        const auto endTime = std::chrono::steady_clock::now();
        const auto duration = endTime - startTime;
        const long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        std::cout << "Time taken: " << milliseconds << " ms" << ". For thread " << thread_id << std::endl;
    }
    void initAvx() {
        unsigned long iterations = 0;
        unsigned long upper_lm = 0;
        unsigned long lower_lm = 0;
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

        if (unsigned int remaining_iterations = iterations % num_threads; remaining_iterations > 0) {
            current_avx_threads.emplace_back(avxWorker, remaining_iterations, lower_lm, upper_lm, num_threads);
        }


        for (auto& thread : current_avx_threads) {
            thread.join();
        }

    }

    volatile static void avxWorker(const unsigned long iterations_per_thread, const int lower_lm, const int upper_lm, const int thread_id) {
        const auto startTime = std::chrono::steady_clock::now();
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(thread_id % std::thread::hardware_concurrency(), &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

        pcg32 gen(42u + thread_id, 54u + thread_id);
        std::uniform_real_distribution<float> gen_float_lm(static_cast<float>(lower_lm), static_cast<float>(upper_lm));

        constexpr int AVX_BUFFER_SIZE = 32;  // 32 floats = 128 bytes
        alignas(32) float n1[AVX_BUFFER_SIZE];
        alignas(32) float n2[AVX_BUFFER_SIZE];
        alignas(32) float n3[AVX_BUFFER_SIZE];
        alignas(32) float out[AVX_BUFFER_SIZE];

        for (unsigned long iter = 0; iter < iterations_per_thread; ++iter) {
            for (int i = 0; i < AVX_BUFFER_SIZE; ++i) {
                n1[i] = gen_float_lm(gen);
                n2[i] = gen_float_lm(gen);
                n3[i] = gen_float_lm(gen);
            }
            for (int offset = 0; offset < AVX_BUFFER_SIZE; offset += 8) {
                avx(&n1[offset], &n2[offset], &n3[offset], &out[offset]);
            }
            volatile float sink = 0;
            for (int i = 0; i < AVX_BUFFER_SIZE; ++i) {
                sink += out[i];  // Prevent optimization
            }
        }
        const auto endTime = std::chrono::steady_clock::now();
        const auto duration = endTime - startTime;
        const long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        std::cout << "Time taken: " << milliseconds << " ms" << ". For thread " << thread_id << std::endl;

    }
};

int main() {
    acts test;
    test.init();
    return 0;
}