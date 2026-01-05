#include "Modules/RenderSystem.hpp"
#include "MeshBuilder.hpp"
#include "Modules/MobSystem.hpp"
#include "Modules/WeatherSystem.hpp"
#include "Modules/BlockData.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace RenderSystem {
    static GLMesh buildCubeMeshForTile(int tx, int ty) {
        GLMesh mesh;
        MeshBuilder mb;
        float u = (float)tx / 16.0f;
        float v = (float)ty / 16.0f;
        float s = 1.0f / 16.0f;
        mb.addFace({0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}, {0,0,-1}, u, v, u+s, v+s, 1.0f); // -Z
        mb.addFace({0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}, {0,0,1},  u, v, u+s, v+s, 1.0f); // +Z
        mb.addFace({0,0,0}, {0,0,1}, {0,1,1}, {0,1,0}, {-1,0,0}, u, v, u+s, v+s, 1.0f); // -X
        mb.addFace({1,0,0}, {1,0,1}, {1,1,1}, {1,1,0}, {1,0,0},  u, v, u+s, v+s, 1.0f); // +X
        mb.addFace({0,0,0}, {1,0,0}, {1,0,1}, {0,0,1}, {0,-1,0}, u, v, u+s, v+s, 1.0f); // -Y
        mb.addFace({0,1,0}, {1,1,0}, {1,1,1}, {0,1,1}, {0,1,0},  u, v, u+s, v+s, 1.0f); // +Y
        mesh.upload(mb.getVertices());
        return mesh;
    }

    bool init(RenderResources& res) {
        const auto findProjectRoot = []() -> std::filesystem::path {
            std::filesystem::path p = std::filesystem::current_path();
            for (int i = 0; i < 6; ++i) {
                if (std::filesystem::exists(p / "assets" / "shaders" / "voxel.vert")) return p;
                if (!p.has_parent_path()) break;
                p = p.parent_path();
            }
            return std::filesystem::current_path();
        };
        const std::filesystem::path root = findProjectRoot();
        const std::string voxelVert = (root / "assets" / "shaders" / "voxel.vert").string();
        const std::string voxelFrag = (root / "assets" / "shaders" / "voxel.frag").string();

        if (!res.voxelShader.loadFromFiles(voxelVert, voxelFrag)) {
            std::cerr << "Failed to load voxel shader.\n";
            return false;
        }

        res.atlas.generateAtlas();

        res.mobMeshes.reserve((size_t)MOB_COUNT);
        for (int i = 0; i < (int)MOB_COUNT; ++i) res.mobMeshes.push_back(buildCubeMeshForTile(i, 3));

        res.crackMeshes.reserve(10);
        for (int i = 0; i < 10; ++i) res.crackMeshes.push_back(buildCubeMeshForTile(i, 2));

        res.sunMesh = buildCubeMeshForTile(14, 2);
        res.moonMesh = buildCubeMeshForTile(13, 2);
        res.cloudMesh = buildCubeMeshForTile(15, 2);
        res.starMesh = buildCubeMeshForTile(12, 2);

        return true;
    }

    void render(GameState& state, RenderResources& res, Framebuffer& viewportBuffer, const Vec2& viewportSize) {
        float dayProgress = state.worldTime / 24000.0f;
        float sunAngle = dayProgress * 2.0f * 3.14159f;
        float sunY = std::sin(sunAngle);
        Vec3 lightDir = normalize(Vec3{std::cos(sunAngle), sunY, -0.25f});
        Vec3 skyColor = Vec3{0.4f, 0.6f, 0.9f} * std::max(0.1f, sunY);
        if (sunY < 0) skyColor = Vec3{0.05f, 0.05f, 0.1f};

        viewportBuffer.bind();
        glClearColor(skyColor.x, skyColor.y, skyColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        res.voxelShader.use();
        res.voxelShader.setMat4("uView", state.camera.viewMatrix());
        res.voxelShader.setMat4("uProjection", state.camera.projectionMatrix());
        res.voxelShader.setVec3("uLightDir", lightDir);
        res.voxelShader.setVec3("uViewPos", state.camera.position());
        res.voxelShader.setFloat("uTime", (float)glfwGetTime());
        res.voxelShader.setVec2("uResolution", {viewportSize.x, viewportSize.y});
        
        res.atlas.bind(0);
        res.voxelShader.setInt("uTexture", 0);

        res.voxelShader.setMat4("uModel", Mat4::identity());
        state.world.render(res.voxelShader, state.camera.projectionMatrix() * state.camera.viewMatrix());

        MobSystem::render(state.registry, res.voxelShader, res.mobMeshes.data());

        // Sky Objects
        {
            Vec3 sunDir = normalize(Vec3{std::cos(sunAngle), std::sin(sunAngle), -0.25f});
            Vec3 moonDir = sunDir * -1.0f;
            Vec3 sunPos = state.camera.position() + sunDir * 220.0f + Vec3{0.0f, 40.0f, 0.0f};
            Vec3 moonPos = state.camera.position() + moonDir * 220.0f + Vec3{0.0f, 40.0f, 0.0f};
            Mat4 face = rotateY(radians(state.camera.yawDegrees() + 90.0f)) * rotateX(radians(-state.camera.pitchDegrees()));

            res.voxelShader.setMat4("uModel", translate(sunPos) * face * scale({18.0f, 18.0f, 1.0f}));
            res.sunMesh.draw();
            res.voxelShader.setMat4("uModel", translate(moonPos) * face * scale({14.0f, 14.0f, 1.0f}));
            res.moonMesh.draw();
        }

        if (sunY < 0.1f) {
            Mat4 face = rotateY(radians(state.camera.yawDegrees() + 90.0f)) * rotateX(radians(-state.camera.pitchDegrees()));
            srand(42);
            for (int i = 0; i < 150; ++i) {
                float az = (float)(rand() % 360) * 0.01745f;
                float el = (float)(rand() % 180) * 0.01745f;
                Vec3 sDir = {std::cos(az) * std::cos(el), std::sin(el), std::sin(az) * std::cos(el)};
                Vec3 sPos = state.camera.position() + sDir * 200.0f;
                res.voxelShader.setMat4("uModel", translate(sPos) * face * scale({1.5f, 1.5f, 1.0f}));
                res.starMesh.draw();
            }
        }

        // Clouds
        {
            float t = (float)glfwGetTime();
            float offset = std::fmod(t * 1.2f, 256.0f);
            for (int cz = -12; cz <= 12; ++cz) {
                for (int cx = -12; cx <= 12; ++cx) {
                    float gridScale = 96.0f;
                    float px = std::floor(state.camera.position().x / gridScale) * gridScale + (float)cx * gridScale + offset;
                    float pz = std::floor(state.camera.position().z / gridScale) * gridScale + (float)cz * gridScale;
                    float n1 = std::sin(px * 0.005f) * std::cos(pz * 0.005f);
                    float n2 = std::sin(px * 0.015f + pz * 0.01f);
                    float noiseVal = (n1 + n2 * 0.5f);
                    if (noiseVal > 0.65f) {
                        Vec3 p = {px, 140.0f + std::sin(px * 0.02f) * 2.0f, pz};
                        res.voxelShader.setMat4("uModel", translate(p) * scale({64.0f + (noiseVal-0.65f)*120.0f, 6.0f + (noiseVal-0.65f)*15.0f, 64.0f + (noiseVal-0.65f)*120.0f}));
                        res.cloudMesh.draw();
                    }
                }
            }
        }

        if (state.playerState.breaking) {
            int stage = std::clamp((int)(state.playerState.breakProgress * 10.0f), 0, 9);
            Mat4 model = translate({(float)state.playerState.breakX, (float)state.playerState.breakY, (float)state.playerState.breakZ}) * scale({1.02f, 1.02f, 1.02f});
            res.voxelShader.setMat4("uModel", model);
            res.crackMeshes[stage].draw();
        }

        WeatherSystem::render(state.snowParticles, state.camera);
        viewportBuffer.unbind();
    }
}
