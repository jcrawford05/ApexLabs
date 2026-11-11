// Apex Lab desktop foundation: SDL3 window + SDL_GPU + Dear ImGui.

#include <cstdio>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "app.h"
#include "theme.h"

namespace {

constexpr int kWindowWidth = 1440;
constexpr int kWindowHeight = 900;
constexpr int kMinimumWindowWidth = 1280;
constexpr int kMinimumWindowHeight = 760;

void Shutdown(SDL_Window* window, SDL_GPUDevice* device) {
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (device && window) {
        SDL_ReleaseWindowFromGPUDevice(device, window);
    }
    if (device) {
        SDL_DestroyGPUDevice(device);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

}  // namespace

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Apex Lab",
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(
        window,
        kMinimumWindowWidth,
        kMinimumWindowHeight);

    SDL_GPUDevice* device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL |
            SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB,
        true,
        nullptr);
    if (!device) {
        std::fprintf(stderr, "SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        std::fprintf(
            stderr,
            "SDL_ClaimWindowForGPUDevice failed: %s\n",
            SDL_GetError());
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetGPUSwapchainParameters(
        device,
        window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        SDL_GPU_PRESENTMODE_VSYNC);

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    const char* base_path = SDL_GetBasePath();
    const std::string base_directory = base_path ? base_path : "";
    float pixel_scale = SDL_GetWindowPixelDensity(window);
    if (pixel_scale <= 0) {
        pixel_scale = 1;
    }
    if (!theme::LoadFonts(base_directory + "assets/fonts", pixel_scale)) {
        std::fprintf(stderr, "Apex Lab fonts could not be loaded.\n");
        Shutdown(window, device);
        return 1;
    }
    theme::ApplyStyle();

    if (!ImGui_ImplSDL3_InitForSDLGPU(window)) {
        std::fprintf(stderr, "Dear ImGui SDL3 backend initialization failed.\n");
        Shutdown(window, device);
        return 1;
    }

    ImGui_ImplSDLGPU3_InitInfo init_info{};
    init_info.Device = device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    if (!ImGui_ImplSDLGPU3_Init(&init_info)) {
        std::fprintf(stderr, "Dear ImGui SDL_GPU backend initialization failed.\n");
        Shutdown(window, device);
        return 1;
    }

    App app;
    std::string error;
    if (!app.Init(window, device, base_directory, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        Shutdown(window, device);
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }
            // .json dropped on the window, or picked in the Import dialog.
            if (event.type == SDL_EVENT_DROP_FILE && event.drop.data) {
                app.ImportFile(event.drop.data);
            }
            if (app.import_event != 0 && event.type == app.import_event &&
                event.user.data1) {
                app.ImportFile(static_cast<const char*>(event.user.data1));
                SDL_free(event.user.data1);
            }
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        // Dear ImGui 1.91.9b falls back to SDL_GetGlobalMouseState() whenever the
        // app is focused. On macOS fullscreen that disagrees with the client
        // area (notch / safe-area), so hit boxes sit above the drawn controls.
        // Prefer window-relative coordinates while the cursor is in our window
        // (same fix as imgui 2025-09-24 / #7919).
        if (SDL_GetMouseFocus() == window) {
            float mouse_x = 0.0F;
            float mouse_y = 0.0F;
            SDL_GetMouseState(&mouse_x, &mouse_y);
            io.AddMousePosEvent(mouse_x, mouse_y);
        }
        ImGui::NewFrame();

        app.Frame();

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized =
            draw_data->DisplaySize.x <= 0.0F || draw_data->DisplaySize.y <= 0.0F;

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
        if (!command_buffer) {
            std::fprintf(
                stderr,
                "SDL_AcquireGPUCommandBuffer failed: %s\n",
                SDL_GetError());
            continue;
        }

        app.RenderViewport(command_buffer);

        SDL_GPUTexture* swapchain_texture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                command_buffer,
                window,
                &swapchain_texture,
                nullptr,
                nullptr)) {
            std::fprintf(
                stderr,
                "SDL_WaitAndAcquireGPUSwapchainTexture failed: %s\n",
                SDL_GetError());
            SDL_CancelGPUCommandBuffer(command_buffer);
            continue;
        }

        if (swapchain_texture != nullptr && !is_minimized) {
            // Mandatory: upload ImGui vertex/index data before the render pass.
            Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            SDL_GPUColorTargetInfo target_info{};
            target_info.texture = swapchain_texture;
            target_info.clear_color = SDL_FColor{18 / 255.0F, 18 / 255.0F, 22 / 255.0F, 1.0F};
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            SDL_GPURenderPass* render_pass =
                SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
        if (app.request_quit) {
            running = false;
        }
    }

    app.Shutdown();
    SDL_WaitForGPUIdle(device);
    Shutdown(window, device);
    return 0;
}
