#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

int main() {
    std::string os;
    std::cout << "Enter host OS (linux/windows): ";
    std::cin >> os;

    // Select compiler and assembly format based on OS
    std::string compiler = (os == "linux") ? "x86_64-w64-mingw32-g++" : "g++";
    std::string asm_format =  "win64";

    std::cout << "Using compiler: " << compiler << "\n";
    std::cout << "Assembly format: " << asm_format << "\n\n";

    // Create obj directory
    if (!fs::exists("obj")) {
        fs::create_directory("obj");
        std::cout << "Created obj/ directory\n";
    }

    // Clean old object files
    for (const auto& file : fs::directory_iterator("obj")) {
        if (file.path().extension() == ".o" || file.path().extension() == ".obj") {
            fs::remove(file);
            std::cout << "Cleaned: " << file.path().filename().string() << "\n";
        }
    }

    std::vector<std::string> objects;
    std::vector<std::string> failed_asm;

    // Assemble all .asm files (continue even if some fail)
    std::cout << "\n=== Assembling .asm files ===\n";
    for (const auto& file : fs::directory_iterator("asm")) {
        if (file.path().extension() == ".asm") {
            std::string obj_name = "obj/" + file.path().stem().string() + ".o";
            std::string cmd = "nasm -f " + asm_format + " \"" + file.path().string() + "\" -o \"" + obj_name + "\"";

            std::cout << "Assembling: " << file.path().filename().string() << " ... ";

            if (std::system(cmd.c_str()) == 0) {
                objects.push_back(obj_name);
                std::cout << "OK\n";
            } else {
                failed_asm.push_back(file.path().filename().string());
                std::cout << "FAILED\n";
            }
        }
    }

    if (!failed_asm.empty()) {
        std::cout << "\nWarning: Some assembly files failed to compile:\n";
        for (const auto& failed : failed_asm) {
            std::cout << "  - " << failed << "\n";
        }
        std::cout << "Continuing with available objects...\n";
    }

    if (objects.empty()) {
        std::cout << "No assembly objects available, building C++ only...\n";
    }

    // Build object list string
    std::string obj_list;
    for (const auto& obj : objects) {
        obj_list += "\"" + obj + "\" ";
    }

    // Check if mainCLI.cpp exists, fallback to main.cpp
    std::string main_file = "src/mainCLI.cpp";
    if (!fs::exists(main_file)) {
        main_file = "src/main.cpp";
        std::cout << "mainCLI.cpp not found, using main.cpp\n";
    }

    // Compile final executable with proper linking order
    std::string compile_cmd = compiler + " -O3 -std=c++20 -static -Wall -Wextra -pthread \"" +
                             main_file + "\" " + obj_list + "-o esst.exe";

    // Add Windows-specific flags if cross-compiling from Linux
    if (os == "linux") {
        compile_cmd += " -static-libgcc -static-libstdc++";
    }

    std::cout << "\n=== Final Compilation ===\n";
    std::cout << "Command: " << compile_cmd << "\n";

    if (std::system(compile_cmd.c_str()) != 0) {
        std::cerr << "\nCompilation failed! Trying without assembly objects...\n";

        // Retry without assembly objects
        std::string fallback_cmd = compiler + " -O3 -std=c++20 -static -Wall -Wextra -pthread \"" +
                                  main_file + "\" -o esst.exe";

        if (os == "linux") {
            fallback_cmd += " -static-libgcc -static-libstdc++";
        }

        std::cout << "Fallback: " << fallback_cmd << "\n";

        if (std::system(fallback_cmd.c_str()) != 0) {
            std::cerr << "Fallback compilation also failed!\n";
            return 1;
        }

        std::cout << "\nBuild complete (C++ only)! Output: esst.exe\n";
    } else {
        std::cout << "\nBuild complete! Output: esst.exe\n";
        std::cout << "Assembly objects included: " << objects.size() << "\n";
    }

    return 0;
}