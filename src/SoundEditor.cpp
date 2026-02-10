#include "SoundEditor.hpp"
#include "AudioManager.hpp"

#include "imgui.h"

#include <string>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

namespace {
    std::string openFileDialog() {
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = 260;
        ofn.lpstrFilter = "Audio Files (*.wav;*.mp3)\0*.wav;*.mp3\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameA(&ofn) == TRUE) return std::string(szFile);
        return "";
    }
}
#else
namespace { std::string openFileDialog() { return ""; } }
#endif

void SoundEditor::show(bool* open) {
    if (!*open) return;

    auto& am = AudioManager::getInstance();

    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Sound Editor", open)) {
        ImGui::Text("Volume Mixers");
        ImGui::Separator();

        float master = am.getMasterVolume();
        if (ImGui::SliderFloat("Master", &master, 0.0f, 1.0f)) am.setMasterVolume(master);

        float music = am.getMusicVolume();
        if (ImGui::SliderFloat("Music", &music, 0.0f, 1.0f)) am.setMusicVolume(music);

        float mobs = am.getMobVolume();
        if (ImGui::SliderFloat("Mobs", &mobs, 0.0f, 1.0f)) am.setMobVolume(mobs);

        float blocks = am.getBlockVolume();
        if (ImGui::SliderFloat("Blocks", &blocks, 0.0f, 1.0f)) am.setBlockVolume(blocks);

        ImGui::Spacing();
        ImGui::Text("Environmental Sounds");
        ImGui::Separator();

        bool musicOn = am.isMusicEnabled();
        if (ImGui::Checkbox("Background Music", &musicOn)) am.setMusicEnabled(musicOn);

        if (ImGui::Button("Next Track")) {
            am.setMusicEnabled(false);
            am.setMusicEnabled(true);
        }

        bool mobsOn = am.isMobSoundsEnabled();
        if (ImGui::Checkbox("Mob Sounds", &mobsOn)) am.setMobSoundsEnabled(mobsOn);

        bool blocksOn = am.isBlockSoundsEnabled();
        if (ImGui::Checkbox("Block Breaking", &blocksOn)) am.setBlockSoundsEnabled(blocksOn);

        ImGui::Separator();
        ImGui::Text("Sound Studio");
        static char soundPath[256] = "";
        static float pitch = 1.0f;
        static float vol = 1.0f;
        ImGui::InputText("File Path", soundPath, 256);
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            std::string path = openFileDialog();
            if (!path.empty()) {
                strncpy(soundPath, path.c_str(), 255);
            }
        }
        ImGui::SliderFloat("Pitch Alteration", &pitch, 0.5f, 2.0f);
        ImGui::SliderFloat("Volume Alteration", &vol, 0.0f, 1.0f);

        if (ImGui::Button("Preview Adjusted sound")) {
            if (strlen(soundPath) > 0) {
                am.playSoundWithPitch(soundPath, vol, pitch);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Set as Block Break")) {
            // Mapping logic
        }

        ImGui::Separator();
        if (ImGui::Button("Close")) *open = false;
    }
    ImGui::End();
}
