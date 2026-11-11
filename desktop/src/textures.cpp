#include "textures.h"

#include <cstdio>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb/stb_image.h"

Image LoadImage(
    SDL_GPUDevice* device,
    SDL_GPUSampler* sampler,
    const std::string& path) {
    Image image;
    if (!device || !sampler) {
        return image;
    }

    int channels = 0;
    unsigned char* pixels =
        stbi_load(path.c_str(), &image.width, &image.height, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "Could not load image %s\n", path.c_str());
        return image;
    }

    const Uint32 upload_size =
        static_cast<Uint32>(image.width * image.height * 4);

    SDL_GPUTextureCreateInfo texture_info{};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.width = static_cast<Uint32>(image.width);
    texture_info.height = static_cast<Uint32>(image.height);
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = 1;

    image.texture = SDL_CreateGPUTexture(device, &texture_info);
    if (!image.texture) {
        std::fprintf(
            stderr,
            "Could not create GPU texture for %s: %s\n",
            path.c_str(),
            SDL_GetError());
        stbi_image_free(pixels);
        image = Image{};
        return image;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = upload_size;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer) {
        std::fprintf(
            stderr,
            "Could not create transfer buffer for %s: %s\n",
            path.c_str(),
            SDL_GetError());
        SDL_ReleaseGPUTexture(device, image.texture);
        stbi_image_free(pixels);
        image = Image{};
        return image;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(mapped, pixels, upload_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    stbi_image_free(pixels);

    SDL_GPUTextureTransferInfo source{};
    source.transfer_buffer = transfer;
    source.offset = 0;

    SDL_GPUTextureRegion destination{};
    destination.texture = image.texture;
    destination.w = static_cast<Uint32>(image.width);
    destination.h = static_cast<Uint32>(image.height);
    destination.d = 1;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    SDL_UploadToGPUTexture(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    image.binding.texture = image.texture;
    image.binding.sampler = sampler;
    return image;
}

void FreeImage(SDL_GPUDevice* device, Image& image) {
    if (device && image.texture) {
        SDL_ReleaseGPUTexture(device, image.texture);
    }
    image = Image{};
}
