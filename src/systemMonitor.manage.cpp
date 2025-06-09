#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <memory>

// ImGui includes
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

class SystemMonitor {
private:
    bool running = false;
    GLFWwindow* window = nullptr;

    // System data
    float cpu_temp = 0.0f;
    float gpu_temp = 0.0f;
    int cpu_usage = 0;
    int cpu_freq = 0;
    int gpu_freq = 0;
    int memory_usage = 0;

    // History for graphs (120 samples for a 60-second history at 500ms refresh)
    const size_t history_size = 120;
    std::vector<float> cpu_temp_history;
    std::vector<float> gpu_temp_history;
    std::vector<float> cpu_usage_history;

    std::string cpu_temp_path;
    std::string gpu_temp_path;

    // --- Robust Sensor Detection ---

    // Find the best CPU temperature sensor path
    std::string find_cpu_temp_path() {
        // Priority 1: hwmon with 'coretemp' (Intel) or 'k10temp' (AMD)
        try {
            for (const auto& dir_entry : std::filesystem::directory_iterator("/sys/class/hwmon")) {
                if (dir_entry.is_directory()) {
                    std::string name_path = dir_entry.path().string() + "/name";
                    std::ifstream name_file(name_path);
                    if (name_file.is_open()) {
                        std::string name;
                        name_file >> name;
                        if (name == "coretemp" || name == "k10temp" || name == "zenpower") {
                             // Find the first temp input in this directory
                            for(const auto& file_entry : std::filesystem::directory_iterator(dir_entry.path())){
                                std::string filename = file_entry.path().filename().string();
                                if(filename.find("temp") != std::string::npos && filename.find("_input") != std::string::npos){
                                    std::cout << "Using CPU temp sensor: " << file_entry.path().string() << " (driver: " << name << ")" << std::endl;
                                    return file_entry.path().string();
                                }
                            }
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) { /* Ignore if dir doesn't exist */ }

        // Priority 2: Fallback to generic thermal zones, preferring 'x86_pkg_temp'
        try {
            for (const auto& dir_entry : std::filesystem::directory_iterator("/sys/class/thermal")) {
                 if (dir_entry.is_directory() && dir_entry.path().filename().string().find("thermal_zone") != std::string::npos) {
                    std::string type_path = dir_entry.path().string() + "/type";
                    std::string temp_path = dir_entry.path().string() + "/temp";
                    std::ifstream type_file(type_path);
                    if (type_file.is_open()) {
                        std::string type;
                        std::getline(type_file, type);
                        if (type == "x86_pkg_temp") {
                            std::cout << "Using CPU temp sensor: " << temp_path << " (type: " << type << ")" << std::endl;
                            return temp_path;
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) { /* Ignore */ }

        std::cerr << "Warning: Could not find a reliable CPU temperature sensor." << std::endl;
        return "";
    }

    // Find the best GPU temperature sensor path
    std::string find_gpu_temp_path() {
        try {
            for (const auto& card_entry : std::filesystem::directory_iterator("/sys/class/drm")) {
                if (card_entry.is_directory() && card_entry.path().filename().string().find("card") != std::string::npos) {
                    std::string hwmon_path = card_entry.path().string() + "/device/hwmon";
                    if (std::filesystem::exists(hwmon_path)) {
                        for (const auto& hwmon_entry : std::filesystem::directory_iterator(hwmon_path)) {
                            if (hwmon_entry.is_directory()) {
                                std::string temp_input_path = hwmon_entry.path().string() + "/temp1_input";
                                if (std::filesystem::exists(temp_input_path)) {
                                    std::cout << "Using GPU temp sensor: " << temp_input_path << std::endl;
                                    return temp_input_path;
                                }
                            }
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) { /* Ignore */ }
        return "";
    }


    // --- Data Reading Functions ---

    float read_temp_from_path(const std::string& path) {
        if (path.empty()) return 0.0f;
        std::ifstream file(path);
        if (file.is_open()) {
            int temp_millic;
            file >> temp_millic;
            if(file.good()){
                 return static_cast<float>(temp_millic) / 1000.0f;
            }
        }
        return 0.0f; // Return 0 on failure
    }

    int read_cpu_freq() {
        std::ifstream freq_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
        if (freq_file.is_open()) {
            int freq_khz;
            freq_file >> freq_khz;
            return freq_khz / 1000;
        }
        // Fallback for older systems
        std::ifstream file("/proc/cpuinfo");
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("cpu MHz") != std::string::npos) {
                return static_cast<int>(std::stof(line.substr(line.find(':') + 1)));
            }
        }
        return 0;
    }

    int read_gpu_freq() { // Note: This is primarily for AMD GPUs.
        std::ifstream file("/sys/class/drm/card0/device/pp_dpm_sclk");
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.back() == '*') {
                    size_t start = line.find(':') + 2;
                    size_t end = line.find("Mhz");
                    if (start != std::string::npos && end != std::string::npos) {
                        return std::stoi(line.substr(start, end - start));
                    }
                }
            }
        }
        return 0;
    }

    int read_cpu_usage() {
        static long long last_idle = 0, last_total = 0;
        std::ifstream file("/proc/stat");
        std::string line;
        std::getline(file, line);
        std::istringstream ss(line);
        std::string cpu_label;
        long long user, nice, system, idle, iowait, irq, softirq, steal;
        ss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        long long current_total = user + nice + system + idle + iowait + irq + softirq + steal;
        long long total_diff = current_total - last_total;
        long long idle_diff = idle - last_idle;
        last_total = current_total;
        last_idle = idle;
        if (total_diff > 0) {
            return 100 * (total_diff - idle_diff) / total_diff;
        }
        return 0;
    }

    int read_memory_usage() {
        std::ifstream file("/proc/meminfo");
        std::string line;
        long long mem_total = 0, mem_available = 0;
        while (std::getline(file, line)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                mem_total = std::stoll(line.substr(line.find(':') + 1));
            } else if (line.rfind("MemAvailable:", 0) == 0) {
                mem_available = std::stoll(line.substr(line.find(':') + 1));
            }
        }
        if (mem_total > 0) {
            return 100 * (mem_total - mem_available) / mem_total;
        }
        return 0;
    }

    void update_system_data() {
        cpu_temp = read_temp_from_path(cpu_temp_path);
        gpu_temp = read_temp_from_path(gpu_temp_path);
        cpu_usage = read_cpu_usage();
        cpu_freq = read_cpu_freq();
        gpu_freq = read_gpu_freq();
        memory_usage = read_memory_usage();

        auto update_history = [](std::vector<float>& history, float value, size_t max_size) {
            history.push_back(value);
            if (history.size() > max_size) {
                history.erase(history.begin());
            }
        };

        update_history(cpu_temp_history, cpu_temp, history_size);
        update_history(gpu_temp_history, gpu_temp, history_size);
        update_history(cpu_usage_history, (float)cpu_usage, history_size);
    }

    // --- GUI Setup ---

    void setup_minimalist_theme() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.12f, 0.12f, 0.12f, 0.94f);
        colors[ImGuiCol_Border]                = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.20f, 0.20f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.30f, 0.30f, 0.30f, 0.40f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.35f, 0.35f, 0.35f, 0.67f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]             = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        colors[ImGuiCol_SliderGrab]            = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_Button]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.47f, 0.47f, 0.47f, 0.67f);
        colors[ImGuiCol_Separator]             = ImVec4(0.32f, 0.32f, 0.32f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.32f, 0.32f, 0.32f, 0.78f);
        colors[ImGuiCol_SeparatorActive]       = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
        colors[ImGuiCol_Tab]                   = ImVec4(0.18f, 0.18f, 0.18f, 0.86f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.26f, 0.26f, 0.80f);
        colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TabUnfocused]          = ImVec4(0.15f, 0.15f, 0.15f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines]             = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram]         = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

        style.WindowPadding     = ImVec2(8.0f, 8.0f);
        style.FramePadding      = ImVec2(5.0f, 3.0f);
        style.CellPadding       = ImVec2(4.0f, 2.0f);
        style.ItemSpacing       = ImVec2(6.0f, 6.0f);
        style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
        style.IndentSpacing     = 21.0f;
        style.ScrollbarSize     = 14.0f;
        style.GrabMinSize       = 10.0f;
        style.WindowBorderSize  = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.FrameBorderSize   = 0.0f; // No border on frames
        style.TabBorderSize     = 0.0f;
        style.WindowRounding    = 4.0f;
        style.ChildRounding     = 4.0f;
        style.FrameRounding     = 2.0f;
        style.PopupRounding     = 4.0f;
        style.ScrollbarRounding = 9.0f;
        style.GrabRounding      = 2.0f;
        style.TabRounding       = 4.0f;
    }

public:
    bool init_window() {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW\n";
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(620, 520, "ESST System Monitor", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr; // Disable .ini file saving

        // Custom font loading has been REMOVED.

        setup_minimalist_theme();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        // Initial sensor discovery
        cpu_temp_path = find_cpu_temp_path();
        gpu_temp_path = find_gpu_temp_path();

        return true;
    }

    void run() {
        if (!init_window()) {
            return;
        }
        running = true;

        while (running && !glfwWindowShouldClose(window)) {
            glfwPollEvents();
            update_system_data();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); // No rounding for main window
            ImGui::Begin("System Monitor", nullptr,
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
            ImGui::PopStyleVar();

            // --- Header ---
            ImGui::Text("ESST System Monitor");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            // --- Main Layout ---
            ImGui::Columns(2, "MainLayout", false);
            ImGui::SetColumnWidth(0, 280.0f);

            // --- Left Column: Current Stats ---
            {
                ImGui::SeparatorText("CPU");
                if (ImGui::BeginTable("CPU_Stats", 2)) {
                    ImGui::TableNextColumn(); ImGui::TextDisabled("Usage");
                    ImGui::TableNextColumn(); ImGui::Text("%d %%", cpu_usage);
                    ImGui::TableNextColumn(); ImGui::TextDisabled("Temperature");
                    ImGui::TableNextColumn(); ImGui::Text("%.1f °C", cpu_temp);
                    ImGui::TableNextColumn(); ImGui::TextDisabled("Frequency");
                    ImGui::TableNextColumn(); ImGui::Text("%d MHz", cpu_freq);
                    ImGui::EndTable();
                }

                // Only show GPU if a sensor was found
                if (!gpu_temp_path.empty()) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::SeparatorText("GPU");
                    if (ImGui::BeginTable("GPU_Stats", 2)) {
                        ImGui::TableNextColumn(); ImGui::TextDisabled("Temperature");
                        ImGui::TableNextColumn(); ImGui::Text("%.1f °C", gpu_temp);
                        ImGui::TableNextColumn(); ImGui::TextDisabled("Frequency");
                        ImGui::TableNextColumn(); ImGui::Text("%d MHz", gpu_freq);
                        ImGui::EndTable();
                    }
                }

                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::SeparatorText("Memory");
                if (ImGui::BeginTable("Mem_Stats", 2)) {
                    ImGui::TableNextColumn(); ImGui::TextDisabled("Usage");
                    ImGui::TableNextColumn(); ImGui::Text("%d %%", memory_usage);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::ProgressBar(memory_usage / 100.0f, ImVec2(-1, 0), "");
                    ImGui::EndTable();
                }
            }

            // --- Right Column: Graphs ---
            ImGui::NextColumn();
            {
                ImGui::SeparatorText("History");

                ImGui::TextDisabled("CPU Usage (%%)");
                ImGui::PlotLines("##CPUUsage", cpu_usage_history.data(), cpu_usage_history.size(),
                               0, nullptr, 0.0f, 100.0f, ImVec2(0, 80));

                ImGui::TextDisabled("CPU Temperature (°C)");
                ImGui::PlotLines("##CPUTemp", cpu_temp_history.data(), cpu_temp_history.size(),
                               0, nullptr, 0.0f, 110.0f, ImVec2(0, 80));

                if (!gpu_temp_path.empty()) {
                    ImGui::TextDisabled("GPU Temperature (°C)");
                    ImGui::PlotLines("##GPUTemp", gpu_temp_history.data(), gpu_temp_history.size(),
                                   0, nullptr, 0.0f, 110.0f, ImVec2(0, 80));
                }
            }

            ImGui::Columns(1);
            ImGui::Separator();

            // --- Footer ---
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 30.0f)); // Push button to bottom
            if (ImGui::Button("Close Monitor", ImVec2(120, 0))) {
                running = false;
            }

            ImGui::End();

            // --- Rendering ---
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            ImVec4 clear_color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        cleanup();
    }

    void cleanup() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        if(window) glfwDestroyWindow(window);
        glfwTerminate();
    }

    void stop() {
        running = false;
    }
};


// --- Public C-style API ---

static std::unique_ptr<SystemMonitor> monitor_instance = nullptr;
static std::thread monitor_thread;

extern "C" void spawn_system_monitor() {
    if (monitor_instance) {
        std::cout << "System monitor is already running." << std::endl;
        return;
    }

    monitor_instance = std::make_unique<SystemMonitor>();

    monitor_thread = std::thread([](){
        monitor_instance->run();
        // The instance will be reset once the thread fully joins
    });

    std::cout << "System monitor spawned." << std::endl;
}

extern "C" void stop_system_monitor() {
    if (monitor_instance) {
        std::cout << "Stopping system monitor..." << std::endl;
        monitor_instance->stop();
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }
        monitor_instance.reset();
        std::cout << "System monitor stopped and cleaned up." << std::endl;
    } else {
        std::cout << "System monitor is not running." << std::endl;
    }
}