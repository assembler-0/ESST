#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

int main() {
    system("mkdir obj");
    std::vector<std::string> obj_files;
    std::string compile_flags = "-O3 -march=native -mtune=native -Wall -Wextra -pthread -std=c++20";
    std::string output_exe = "esst.exe";

    // Step 1: Compile all ASM files with NASM
    std::cout << "Compiling ASM files...\n";
    for (const auto& entry : fs::directory_iterator("asm")) {
        if (entry.path().extension() == ".asm") {
            std::string asm_file = entry.path().string();
            std::string obj_file = "obj/" + entry.path().stem().string() + ".o";

            std::string command = "nasm -f win64 " + asm_file + " -o " + obj_file;
            std::cout << "Running: " << command << "\n";

            int result = system(command.c_str());
            if (result != 0) {
                std::cerr << "Error compiling " << asm_file << "\n";
                return 1;
            }

            obj_files.push_back(obj_file);
        }
    }

    // Step 2: Compile all CPP files
    std::cout << "\nCompiling CPP files...\n";
    for (const auto& entry : fs::directory_iterator("src")) {
        if (entry.path().extension() == ".cpp" || entry.path().extension() == ".module.cpp") {
            std::string cpp_file = entry.path().string();
            std::string obj_file = "obj/" + entry.path().stem().string() + ".o";

            std::string command = "x86_64-w64-mingw32-g++ " + compile_flags + " -c " + cpp_file + " -o " + obj_file;
            std::cout << "Running: " << command << "\n";

            int result = system(command.c_str());
            if (result != 0) {
                std::cerr << "Error compiling " << cpp_file << "\n";
                return 1;
            }

            obj_files.push_back(obj_file);
        }
    }

    // Step 3: Link everything
    std::cout << "\nLinking executable...\n";
    std::string link_command = "x86_64-w64-mingw32-g++ -static -o " + output_exe;
    for (const auto& obj : obj_files) {
        link_command += " " + obj;
    }

    std::cout << "Running: " << link_command << "\n";
    int result = system(link_command.c_str());
    if (result != 0) {
        std::cerr << "Error linking executable\n";
        return 1;
    }

    std::cout << "\nBuild successful! Output: " << output_exe << "\n";
    return 0;
}