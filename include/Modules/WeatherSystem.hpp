#pragma once
#include "Modules/Components.hpp"
#include "Camera.hpp"
#include <vector>

namespace WeatherSystem {
    void update(std::vector<SnowParticle>& particles, const Camera& camera, double dt, WeatherType weather);
    void render(const std::vector<SnowParticle>& particles, const Camera& camera);
}
