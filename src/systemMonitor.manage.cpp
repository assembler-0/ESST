#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <array>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

namespace esst {

struct Metrics {
    float cpu_temp{0};
    float gpu_temp{0};
    float cpu_usage{0};
    float memory_usage{0};
    int cpu_freq{0};
    int gpu_freq{0};
};

struct History {
    static constexpr size_t MAX_SAMPLES = 120;
    std::array<float, MAX_SAMPLES> cpu_temp{};
    std::array<float, MAX_SAMPLES> gpu_temp{};
    std::array<float, MAX_SAMPLES> cpu_usage{};
    size_t index{0};

    void push(float cpu_t, float gpu_t, float cpu_u) {
        cpu_temp[index] = cpu_t;
        gpu_temp[index] = gpu_t;
        cpu_usage[index] = cpu_u;
        index = (index + 1) % MAX_SAMPLES;
    }
};

class Monitor {
    GLFWwindow* window_{nullptr};
    bool running_{false};
    Metrics metrics_;
    History history_;
    std::string cpu_sensor_;
    std::string gpu_sensor_;

    void setup_theme() {
        auto& style = ImGui::GetStyle();
        auto* colors = style.Colors;

        // Futuristic dark theme
        colors[ImGuiCol_WindowBg] = {0.05f, 0.05f, 0.08f, 1.0f};
        colors[ImGuiCol_Text] = {0.9f, 0.95f, 1.0f, 1.0f};
        colors[ImGuiCol_TextDisabled] = {0.4f, 0.5f, 0.6f, 1.0f};
        colors[ImGuiCol_FrameBg] = {0.1f, 0.12f, 0.18f, 1.0f};
        colors[ImGuiCol_Button] = {0.15f, 0.2f, 0.3f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = {0.2f, 0.25f, 0.35f, 1.0f};
        colors[ImGuiCol_PlotLines] = {0.2f, 0.8f, 1.0f, 1.0f};
        colors[ImGuiCol_Separator] = {0.2f, 0.25f, 0.35f, 1.0f};

        style.WindowRounding = 8.0f;
        style.FrameRounding = 4.0f;
        style.WindowPadding = {16, 16};
        style.ItemSpacing = {12, 8};
    }

    std::string find_sensor(const std::string& pattern) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator("/sys/class/hwmon")) {
                auto name_path = entry.path() / "name";
                std::ifstream file(name_path);
                std::string name;
                if (file >> name && name.find(pattern) != std::string::npos) {
                    for (const auto& temp_file : std::filesystem::directory_iterator(entry.path())) {
                        if (temp_file.path().filename().string().contains("temp1_input")) {
                            return temp_file.path();
                        }
                    }
                }
            }
        } catch (...) {}
        return {};
    }

    float read_temp(const std::string& path) {
        if (path.empty()) return 0;
        std::ifstream file(path);
        int temp;
        return file >> temp ? temp / 1000.0f : 0;
    }

    float read_cpu_usage() {
        static long last_idle{0}, last_total{0};
        std::ifstream file("/proc/stat");
        std::string line;
        std::getline(file, line);

        long user, nice, system, idle, iowait, irq, softirq, steal;
        sscanf(line.c_str(), "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

        long total = user + nice + system + idle + iowait + irq + softirq + steal;
        long diff_total = total - last_total;
        long diff_idle = idle - last_idle;

        last_total = total;
        last_idle = idle;

        return diff_total > 0 ? 100.0f * (diff_total - diff_idle) / diff_total : 0;
    }

    float read_memory_usage() {
        std::ifstream file("/proc/meminfo");
        std::string line;
        long mem_total{0}, mem_available{0};

        while (std::getline(file, line)) {
            if (line.starts_with("MemTotal:")) {
                sscanf(line.c_str(), "MemTotal: %ld", &mem_total);
            } else if (line.starts_with("MemAvailable:")) {
                sscanf(line.c_str(), "MemAvailable: %ld", &mem_available);
            }
        }

        return mem_total > 0 ? 100.0f * (mem_total - mem_available) / mem_total : 0;
    }

    void update_metrics() {
        metrics_.cpu_temp = read_temp(cpu_sensor_);
        metrics_.gpu_temp = read_temp(gpu_sensor_);
        metrics_.cpu_usage = read_cpu_usage();
        metrics_.memory_usage = read_memory_usage();

        history_.push(metrics_.cpu_temp, metrics_.gpu_temp, metrics_.cpu_usage);
    }

    void render_ui() {
        ImGui::Begin("NEXUS", nullptr, ImGuiWindowFlags_NoDecoration);

        // Header
        ImGui::TextColored({0.2f, 0.8f, 1.0f, 1.0f}, "SYSTEM NEXUS");
        ImGui::Separator();
        ImGui::Spacing();

        // Metrics grid
        if (ImGui::BeginTable("metrics", 4, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableNextColumn();
            ImGui::TextDisabled("CPU");
            ImGui::Text("%.0f°C", metrics_.cpu_temp);
            ImGui::Text("%.0f%%", metrics_.cpu_usage);

            ImGui::TableNextColumn();
            ImGui::TextDisabled("GPU");
            ImGui::Text("%.0f°C", metrics_.gpu_temp);
            ImGui::Text("--");

            ImGui::TableNextColumn();
            ImGui::TextDisabled("MEM");
            ImGui::Text("%.0f%%", metrics_.memory_usage);
            ImGui::ProgressBar(metrics_.memory_usage / 100.0f, {60, 0}, "");

            ImGui::TableNextColumn();
            ImGui::TextDisabled("SYS");
            ImGui::Text("ACTIVE");
            ImGui::TextColored({0.2f, 0.8f, 0.2f, 1.0f}, "◉ ONLINE");

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Graphs
        ImGui::PlotLines("CPU THERMAL", history_.cpu_temp.data(), History::MAX_SAMPLES,
                        history_.index, nullptr, 0, 100, {0, 60});

        ImGui::PlotLines("CPU LOAD", history_.cpu_usage.data(), History::MAX_SAMPLES,
                        history_.index, nullptr, 0, 100, {0, 60});

        ImGui::Spacing();
        if (ImGui::Button("DISCONNECT", {120, 0})) {
            running_ = false;
        }

        ImGui::End();
    }

public:
    bool init() {
        if (!glfwInit()) return false;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        window_ = glfwCreateWindow(480, 400, "NEXUS", nullptr, nullptr);
        if (!window_) {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;

        setup_theme();
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        cpu_sensor_ = find_sensor("coretemp");
        gpu_sensor_ = find_sensor("amdgpu");

        return true;
    }

    void run() {
        if (!init()) return;
        running_ = true;

        while (running_ && !glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            update_metrics();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

            render_ui();

            ImGui::Render();
            int w, h;
            glfwGetFramebufferSize(window_, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window_);

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        cleanup();
    }

    void cleanup() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        if (window_) glfwDestroyWindow(window_);
        glfwTerminate();
    }

    void stop() { running_ = false; }
};

} // namespace esst

// Simple C API
static std::unique_ptr<esst::Monitor> g_monitor;
static std::thread g_thread;

extern "C" void spawn_system_monitor() {
    if (g_monitor) return;

    g_monitor = std::make_unique<esst::Monitor>();
    g_thread = std::thread([]{ g_monitor->run(); });
}

extern "C" void stop_system_monitor() {
    if (!g_monitor) return;

    g_monitor->stop();
    if (g_thread.joinable()) g_thread.join();
    g_monitor.reset();
}