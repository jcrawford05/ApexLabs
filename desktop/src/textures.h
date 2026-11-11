// Loads PNG images into SDL_GPU textures that Dear ImGui can draw.
#pragma once

#include <cstdint>
#include <string>

#include <SDL3/SDL.h>

#include "imgui.h"

struct Image {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUTextureSamplerBinding binding{};
    int width = 0;
    int height = 0;

    ImTextureID id() const {
        return reinterpret_cast<ImTextureID>(
            const_cast<SDL_GPUTextureSamplerBinding*>(&binding));
    }
    explicit operator bool() const {
        return texture != nullptr && binding.texture != nullptr &&
            binding.sampler != nullptr;
    }
};

Image LoadImage(
    SDL_GPUDevice* device,
    SDL_GPUSampler* sampler,
    const std::string& path);
void FreeImage(SDL_GPUDevice* device, Image& image);
