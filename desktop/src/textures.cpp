#include "textures.h"

#include <cstdio>

#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb/stb_image.h"

Image LoadImage(SDL_Renderer* renderer, const std::string& path) {
    Image image;
    int channels = 0;
    unsigned char* pixels =
        stbi_load(path.c_str(), &image.width, &image.height, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "Could not load image %s\n", path.c_str());
        return image;
    }

    image.texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STATIC,
        image.width,
        image.height);
    if (image.texture) {
        SDL_UpdateTexture(image.texture, nullptr, pixels, image.width * 4);
        SDL_SetTextureBlendMode(image.texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(image.texture, SDL_SCALEMODE_LINEAR);
    }
    stbi_image_free(pixels);
    return image;
}

void FreeImage(Image& image) {
    if (image.texture) {
        SDL_DestroyTexture(image.texture);
    }
    image = Image{};
}
