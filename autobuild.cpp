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

    // Select compiler based on OS
    std::string compiler = (os == "linux") ? "x86_64-w64-mingw32-g++" : "g++";
    std::string asm_format = (os == "linux") ? "elf64" : "win64";

    // Create obj directory
    if (!fs::exists("obj")) {
        fs::create_directory("obj");
        std::cout << "Created obj/ directory\n";
    }

    // Clean old object files
    for (const auto& file : fs::directory_iterator("obj")) {
        if (file.path().extension() == ".o" || file.path().extension() == ".obj") {
            fs::remove(file);
        }
    }

    std::vector<std::string> objects;

    // Assemble all .asm files
    for (const auto& file : fs::directory_iterator("asm")) {
        if (file.path().extension() == ".asm") {
            std::string obj_name = "obj/" + file.path().stem().string() + ".o";
            std::string cmd = "nasm -f " + asm_format + " " + file.path().string() + " -o " + obj_name;

            std::cout << "Assembling: " << file.path().filename().string() << "\n";
            if (std::system(cmd.c_str()) != 0) {
                std::cerr << "Assembly failed for " << file.path().string() << "\n";
                return 1;
            }
            objects.push_back(obj_name);
        }
    }

    // Build object list string
    std::string obj_list;
    for (const auto& obj : objects) {
        obj_list += obj + " ";
    }

    // Compile final executable
    std::string compile_cmd = compiler + " -O3 -std=c++20 -static src/main.cpp " + obj_list + "-o esst.exe";
    std::cout << "Compiling final executable...\n";

    if (std::system(compile_cmd.c_str()) != 0) {
        std::cerr << "Compilation failed!\n";
        return 1;
    }

    std::cout << "Build complete! Output: esst.exe\n";
    return 0;
}