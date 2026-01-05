#pragma once
#include <vector>
#include <string>
#include "Modules/Components.hpp"
#include "World.hpp"

namespace ChatSystem {
    void addMessage(std::vector<std::string>& history, const std::string& msg);
    void update(std::vector<std::string>& history, char* input, bool& chatOpen, WeatherType& weather, float& worldTime, World& world, int& seed);
    void render(const std::vector<std::string>& history, char* input, bool chatOpen);
}
