// Loads PNG images into SDL textures that Dear ImGui can draw.
// Adapted from ApexLabsDevSpace at commit c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0.
#pragma once

#include <cstdint>
#include <string>

#include "imgui.h"

struct SDL_Renderer;
struct SDL_Texture;

struct Image {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;

    ImTextureID id() const {
        return reinterpret_cast<ImTextureID>(texture);
    }
    explicit operator bool() const {
        return texture != nullptr;
    }
};

Image LoadImage(SDL_Renderer* renderer, const std::string& path);
void FreeImage(Image& image);
