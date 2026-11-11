// Offscreen SDL_GPU viewport for the Car Setup preview.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "imgui.h"
#include "vehicle.h"

enum class ViewportFocus {
    FullCar,
    FrontWing,
    RearWing,
    FrontRideHeight,
    RearRideHeight,
    Tires,
};

struct Viewport3D {
    bool Init(SDL_GPUDevice* device, const std::string& glb_path);
    void Shutdown();

    void EnsureSize(int width, int height);

    // Sync setup state into the preview (wings, ride height, tires).
    void UpdateFromVehicle(const Vehicle& vehicle);

    // Sticky focus: only changes when explicitly set or reset.
    void SetFocus(ViewportFocus focus);
    void ResetView();

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
        bool is_tire = false;
        // For tires: interleaved x,y,z,shade per vertex (compound tint applied later).
        std::vector<float> tire_lit;
    };

    bool CreatePipeline();
    bool LoadGlbOrCube(const std::string& glb_path);
    bool CreateCubeMesh();
    bool CreateGpuVertexBuffer(
        const void* bytes,
        size_t byte_count,
        SDL_GPUBuffer** out_buffer);
    bool WriteGpuVertexBuffer(
        SDL_GPUBuffer* buffer,
        const void* bytes,
        size_t byte_count);
    void ApplyTireCompound(int compound);
    void DestroyDrawables();
    void DestroyTargets();
    void RebuildWorldMatrices();
    void ResolveFocusTarget(float out_center[3], float* out_distance) const;
    int FindNodeIndex(const char* name) const;
    static bool IsWheelAssemblyName(const std::string& name);

    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUGraphicsPipeline* pipeline_ = nullptr;
    SDL_GPUTexture* color_ = nullptr;
    SDL_GPUTexture* depth_ = nullptr;
    SDL_GPUSampler* sampler_ = nullptr;
    SDL_GPUTextureSamplerBinding binding_{};

    std::vector<SceneNode> nodes_;
    std::vector<int> root_nodes_;
    std::vector<Drawable> drawables_;
    int world_node_ = -1;
    int front_wing_pivot_ = -1;
    int rear_wing_pivot_ = -1;

    float front_wing_deg_ = 8.0F;
    float rear_wing_deg_ = 12.0F;
    float front_ride_height_mm_ = 31.0F;
    float rear_ride_height_mm_ = 78.0F;
    int tire_compound_ = 1;

    float rest_front_wing_deg_ = 8.0F;
    float rest_rear_wing_deg_ = 12.0F;
    float rest_front_ride_mm_ = 31.0F;
    float rest_rear_ride_mm_ = 78.0F;

    ViewportFocus focus_ = ViewportFocus::FullCar;
    float focus_center_[3] = {0.0F, 0.0F, 0.0F};
    float target_distance_ = 7.5F;
    float full_car_distance_ = 7.5F;
    float anim_center_[3] = {0.0F, 0.0F, 0.0F};
    float anim_distance_ = 7.5F;

    float default_yaw_deg_ = 38.0F;
    float default_pitch_deg_ = 22.0F;

    float model_radius_ = 1.0F;
    float min_distance_ = 1.5F;
    float max_distance_ = 14.0F;
    int width_ = 0;
    int height_ = 0;
    bool ready_ = false;
};
