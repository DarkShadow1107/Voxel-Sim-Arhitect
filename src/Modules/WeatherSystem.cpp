#include "Modules/WeatherSystem.hpp"
#include "imgui.h"
#include <algorithm>

namespace WeatherSystem {
    void update(std::vector<SnowParticle>& particles, const Camera& camera, double dt, WeatherType weather) {
        if (weather == WEATHER_SNOW && particles.size() < 500) {
            for (int i = 0; i < 5; ++i) {
                SnowParticle p;
                p.position = { (float)(rand() % 60 - 30), (float)(rand() % 20 + 10), (float)(rand() % 60 - 30) };
                p.velocity = { (float)(rand() % 10 - 5) * 0.1f, -2.0f - (float)(rand() % 10) * 0.1f, (float)(rand() % 10 - 5) * 0.1f };
                p.life = 5.0f + (float)(rand() % 50) * 0.1f;
                particles.push_back(p);
            }
        }

        for (auto it = particles.begin(); it != particles.end(); ) {
            it->position.x += it->velocity.x * (float)dt;
            it->position.y += it->velocity.y * (float)dt;
            it->position.z += it->velocity.z * (float)dt;
            it->life -= (float)dt;

            if (it->position.y < -10.0f) it->position.y = 20.0f;
            
            if (it->life <= 0) {
                it = particles.erase(it);
            } else {
                ++it;
            }
        }
    }

    void render(const std::vector<SnowParticle>& particles, const Camera& camera) {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        ImVec2 s = ImGui::GetIO().DisplaySize;
        Mat4 viewProj = camera.projectionMatrix() * camera.viewMatrix();

        for (const auto& p : particles) {
            Vec3 worldPos = camera.position() + p.position;
            Vec4 clipPos = viewProj * Vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
            
            if (clipPos.w > 0.1f) {
                float ndcX = clipPos.x / clipPos.w;
                float ndcY = clipPos.y / clipPos.w;
                if (ndcX >= -1.0f && ndcX <= 1.0f && ndcY >= -1.0f && ndcY <= 1.0f) {
                    float screenX = (ndcX * 0.5f + 0.5f) * s.x;
                    float screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * s.y;
                    float size = 3.0f / clipPos.w * 15.0f;
                    size = std::clamp(size, 1.5f, 6.0f);
                    draw->AddCircleFilled(ImVec2(screenX, screenY), size, IM_COL32(255, 255, 255, 200));
                }
            }
        }
    }
}
