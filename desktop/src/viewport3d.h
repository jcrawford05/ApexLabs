// Offscreen SDL_GPU viewport for the Car Setup preview.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "imgui.h"

enum class ViewportFocus {
    FullCar,
    FrontWing,
    RearWing,
};

struct Viewport3D {
    bool Init(SDL_GPUDevice* device, const std::string& glb_path);
    void Shutdown();

    void EnsureSize(int width, int height);

    // Apply setup angles / camera focus, then draw into the offscreen target.
    void UpdateAero(float front_wing_deg, float rear_wing_deg);
    void SetFocus(ViewportFocus focus);
    void Render(SDL_GPUCommandBuffer* command_buffer);

    ImTextureID TextureId() const;
    bool HasTexture() const;

    void Orbit(float dx_pixels, float dy_pixels);
    void Zoom(float wheel);

    float yaw_deg = 38.0F;
    float pitch_deg = 22.0F;
    float distance = 7.5F;
    std::string status = "Loading vehicle preview…";

private:
    struct SceneNode {
        std::string name;
        float local[16]{};
        float world[16]{};
        int parent = -1;
        std::vector<int> children;
    };

    struct Drawable {
        SDL_GPUBuffer* vertex_buffer = nullptr;
        uint32_t vertex_count = 0;
        int node_index = -1;
    };

    bool CreatePipeline();
    bool LoadGlbOrCube(const std::string& glb_path);
    bool CreateCubeMesh();
    bool UploadDrawable(
        const void* bytes,
        size_t byte_count,
        uint32_t vertex_count,
        int node_index);
    void DestroyDrawables();
    void DestroyTargets();
    void RebuildWorldMatrices();
    void ResolveFocusTarget(float out_center[3], float* out_distance) const;
    int FindNodeIndex(const char* name) const;

    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUGraphicsPipeline* pipeline_ = nullptr;
    SDL_GPUTexture* color_ = nullptr;
    SDL_GPUTexture* depth_ = nullptr;
    SDL_GPUSampler* sampler_ = nullptr;
    SDL_GPUTextureSamplerBinding binding_{};

    std::vector<SceneNode> nodes_;
    std::vector<int> root_nodes_;
    std::vector<Drawable> drawables_;
    int front_wing_pivot_ = -1;
    int rear_wing_pivot_ = -1;

    float front_wing_deg_ = 8.0F;
    float rear_wing_deg_ = 12.0F;
    float rest_front_wing_deg_ = 8.0F;
    float rest_rear_wing_deg_ = 12.0F;

    ViewportFocus focus_ = ViewportFocus::FullCar;
    float focus_center_[3] = {0.0F, 0.0F, 0.0F};
    float target_distance_ = 7.5F;
    float full_car_distance_ = 7.5F;
    float anim_center_[3] = {0.0F, 0.0F, 0.0F};
    float anim_distance_ = 7.5F;

    float model_radius_ = 1.0F;
    float min_distance_ = 1.5F;
    float max_distance_ = 14.0F;
    int width_ = 0;
    int height_ = 0;
    bool ready_ = false;
};
