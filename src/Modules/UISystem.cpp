#include "Modules/UISystem.hpp"
#include "Modules/BlockData.hpp"
#include "Modules/ChatSystem.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstdint>

namespace UISystem {
    void render(GameState& state, RenderSystem::RenderResources& res, Framebuffer& viewportBuffer, Vec2& viewportSize) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        
        ImVec2 vSize = ImGui::GetContentRegionAvail();
        if (vSize.x != viewportSize.x || vSize.y != viewportSize.y) {
            viewportSize = {vSize.x, vSize.y};
            viewportBuffer.resize((int)vSize.x, (int)vSize.y);
            state.camera.setAspect(vSize.x / vSize.y);
        }

        ImVec2 screenPos = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)(intptr_t)viewportBuffer.getTexture(), vSize, ImVec2(0, 1), ImVec2(1, 0));

        // HUD
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 center = ImVec2(screenPos.x + vSize.x * 0.5f, screenPos.y + vSize.y * 0.5f);
            drawList->AddLine(ImVec2(center.x - 10, center.y), ImVec2(center.x + 10, center.y), IM_COL32(255, 255, 255, 200), 2.0f);
            drawList->AddLine(ImVec2(center.x, center.y - 10), ImVec2(center.x, center.y + 10), IM_COL32(255, 255, 255, 200), 2.0f);

            // Hotbar
            float slotSize = 40.0f;
            float hotbarW = slotSize * 9;
            ImVec2 hbPos = ImVec2(screenPos.x + (vSize.x - hotbarW) * 0.5f, screenPos.y + vSize.y - 60.0f);
            ImTextureID texId = (ImTextureID)(intptr_t)res.atlas.getID();

            for (int i = 1; i <= 9; ++i) {
                ImVec2 sPos = ImVec2(hbPos.x + (i - 1) * slotSize, hbPos.y);
                ImVec2 sEnd = ImVec2(sPos.x + slotSize, sPos.y + slotSize);
                drawList->AddRectFilled(sPos, sEnd, IM_COL32(40, 40, 40, 255), 2.0f);
                
                if (state.playerState.selectedBlock == i) {
                    drawList->AddRect(sPos, sEnd, IM_COL32(255, 255, 255, 255), 2.0f, 0, 3.0f);
                }

                auto info = BlockData::getInfo(i);
                ImVec2 uv0 = ImVec2(info.tx / 16.0f, info.ty / 16.0f);
                ImVec2 uv1 = ImVec2((info.tx + 1) / 16.0f, (info.ty + 1) / 16.0f);
                drawList->AddImage(texId, ImVec2(sPos.x + 4, sPos.y + 4), ImVec2(sEnd.x - 4, sEnd.y - 4), uv0, uv1);
            }

            // Debug Info
            ImGui::SetCursorScreenPos(ImVec2(screenPos.x + 10, screenPos.y + 10));
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Voxel-Sim Architect v0.8");
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Pos: %.1f, %.1f, %.1f", state.camera.position().x, state.camera.position().y, state.camera.position().z);
        }

        state.viewportHovered = ImGui::IsWindowHovered();
        ImGui::End();
        ImGui::PopStyleVar();

        if (state.showWorldEditor) {
            ImGui::Begin("World Editor", &state.showWorldEditor);
            ImGui::Text("Mode: %s (F10)", state.menuMode ? "MENU" : "WORLD");
            if (ImGui::SliderFloat("Time", &state.worldTime, 0.0f, 24000.0f)) {}
            
            if (ImGui::SliderInt("Render Distance", &state.renderDistance, 2, 16)) {
                state.world.setRenderDistance(state.renderDistance);
            }

            const char* weatherNames[] = { "Clear", "Rain", "Snow" };
            int w = (int)state.currentWeather;
            if (ImGui::Combo("Weather", &w, weatherNames, 3)) state.currentWeather = (WeatherType)w;

            if (ImGui::Button("Save World")) state.world.save("world.dat");
            ImGui::SameLine();
            if (ImGui::Button("Load World")) state.world.load("world.dat");

            ImGui::Separator();
            for (int i = 1; i < 26; ++i) {
                auto info = BlockData::getInfo(i);
                if (ImGui::RadioButton(info.name.c_str(), state.playerState.selectedBlock == i)) state.playerState.selectedBlock = i;
                if (i % 4 != 0) ImGui::SameLine();
            }
            ImGui::End();
        }

        if (state.playerState.inventoryOpen) {
            ImGui::Begin("Inventory", &state.playerState.inventoryOpen);
            ImTextureID texId = (ImTextureID)(intptr_t)res.atlas.getID();
            for (int i = 1; i < BLOCK_COUNT; ++i) {
                auto info = BlockData::getInfo(i);
                ImVec2 uv0 = ImVec2(info.tx / 16.0f, info.ty / 16.0f);
                ImVec2 uv1 = ImVec2((info.tx + 1) / 16.0f, (info.ty + 1) / 16.0f);
                if (ImGui::ImageButton(info.name.c_str(), texId, ImVec2(40, 40), uv0, uv1)) {
                    state.playerState.selectedBlock = i;
                }
                if (i % 8 != 0) ImGui::SameLine();
            }
            ImGui::End();
        }

        ChatSystem::render(state.chatHistory, state.chatInput, state.chatOpen);
    }
}
