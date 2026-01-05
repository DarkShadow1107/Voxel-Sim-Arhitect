#include "Modules/ChatSystem.hpp"
#include "imgui.h"
#include <algorithm>

namespace ChatSystem {
    void addMessage(std::vector<std::string>& history, const std::string& msg) {
        history.push_back(msg);
        if (history.size() > 50) history.erase(history.begin());
    }

    void update(std::vector<std::string>& history, char* input, bool& chatOpen, WeatherType& weather, float& worldTime, World& world, int& seed) {
        if (!chatOpen) return;

        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            std::string cmd = input;
            if (!cmd.empty()) {
                addMessage(history, "> " + cmd);
                
                if (cmd == "/weather clear") {
                    weather = WEATHER_CLEAR;
                    addMessage(history, "Weather set to clear.");
                } else if (cmd == "/weather rain") {
                    weather = WEATHER_RAIN;
                    addMessage(history, "Weather set to rain.");
                } else if (cmd == "/weather snow") {
                    weather = WEATHER_SNOW;
                    addMessage(history, "Weather set to snow.");
                } else if (cmd.substr(0, 6) == "/time ") {
                    worldTime = (float)std::atof(cmd.substr(6).c_str());
                    addMessage(history, "Time set to " + std::to_string(worldTime));
                } else if (cmd == "/save") {
                    world.save("world.dat");
                    addMessage(history, "World saved to world.dat");
                } else if (cmd == "/load") {
                    world.load("world.dat");
                    addMessage(history, "World loaded from world.dat");
                } else if (cmd.substr(0, 6) == "/seed ") {
                    seed = std::atoi(cmd.substr(6).c_str());
                    world.clear();
                    addMessage(history, "Created new world with seed " + std::to_string(seed));
                } else {
                    addMessage(history, "Unknown command: " + cmd);
                }
                
                input[0] = '\0';
                chatOpen = false;
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            chatOpen = false;
        }
    }

    void render(const std::vector<std::string>& history, char* input, bool chatOpen) {
        if (!chatOpen && history.empty()) return;

        ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - (chatOpen ? 200 : 150)));
        ImGui::SetNextWindowSize(ImVec2(400, chatOpen ? 180 : 120));
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        if (!chatOpen) flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;

        ImGui::Begin("Chat", nullptr, flags);
        
        ImGui::BeginChild("Messages", ImVec2(0, chatOpen ? -30 : 0));
        for (const auto& msg : history) {
            ImGui::TextUnformatted(msg.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();

        if (chatOpen) {
            ImGui::Separator();
            ImGui::SetKeyboardFocusHere();
            ImGui::InputText("##Input", input, 256);
        }

        ImGui::End();
    }
}
