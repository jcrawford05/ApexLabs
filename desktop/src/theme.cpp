#include "theme.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <utility>
#include <vector>

namespace theme {
namespace {

struct FontFile {
    Face face;
    const char* file;
    float line_ratio;
};

constexpr FontFile kFiles[] = {
    {Face::SpaceBold, "SpaceGrotesk-Bold.ttf", 1.276F},
    {Face::SpaceMedium, "SpaceGrotesk-Medium.ttf", 1.276F},
    {Face::PlexRegular, "IBMPlexSans-Regular.ttf", 1.3F},
    {Face::PlexSemiBold, "IBMPlexSans-SemiBold.ttf", 1.3F},
};

const std::map<Face, std::vector<float>> kSizes = {
    {Face::SpaceBold, {15, 17, 18, 24}},
    {Face::SpaceMedium, {18}},
    {Face::PlexRegular, {8, 9, 10, 11, 14}},
    {Face::PlexSemiBold, {9, 10, 11, 12, 14, 20}},
};

std::map<std::pair<Face, int>, ImFont*> g_fonts;

float Ratio(Face face) {
    for (const FontFile& font : kFiles) {
        if (font.face == face) {
            return font.line_ratio;
        }
    }
    return 1.25F;
}

int Key(float size) {
    return static_cast<int>(std::lround(size * 10.0F));
}

}  // namespace

bool LoadFonts(const std::string& font_dir, float pixel_scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    g_fonts.clear();

    bool ok = true;
    for (const FontFile& font_file : kFiles) {
        const std::string path = font_dir + "/" + font_file.file;
        for (float size : kSizes.at(font_file.face)) {
            ImFontConfig config;
            config.OversampleH = 3;
            config.OversampleV = 1;
            config.PixelSnapH = false;
            std::snprintf(config.Name, sizeof(config.Name), "%s %.0fpx", font_file.file, size);
            ImFont* font = io.Fonts->AddFontFromFileTTF(
                path.c_str(), size * font_file.line_ratio * pixel_scale, &config);
            if (!font) {
                std::fprintf(stderr, "Could not load font %s\n", path.c_str());
                ok = false;
                continue;
            }
            g_fonts[{font_file.face, Key(size)}] = font;
        }
    }

    io.FontGlobalScale = 1.0F / pixel_scale;
    if (ImFont* body = Font(Face::PlexRegular, 14)) {
        io.FontDefault = body;
    }
    return ok;
}

ImFont* Font(Face face, float size) {
    auto exact = g_fonts.find({face, Key(size)});
    if (exact != g_fonts.end()) {
        return exact->second;
    }

    ImFont* best = nullptr;
    int best_difference = 1 << 30;
    for (const auto& [key, font] : g_fonts) {
        if (key.first != face) {
            continue;
        }
        const int difference = std::abs(key.second - Key(size));
        if (difference < best_difference) {
            best_difference = difference;
            best = font;
        }
    }
    return best ? best : ImGui::GetIO().Fonts->Fonts[0];
}

float LineHeight(Face face, float size) {
    return size * Ratio(face);
}

void ApplyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(0, 0);
    style.WindowBorderSize = 0;
    style.WindowRounding = 0;
    style.ChildBorderSize = 1;
    style.ChildRounding = 12;
    style.FrameRounding = 8;
    style.FramePadding = ImVec2(12, 9);
    style.PopupRounding = 8;
    style.PopupBorderSize = 1;
    style.ItemSpacing = ImVec2(8, 8);
    style.ScrollbarSize = 8;
    style.ScrollbarRounding = 8;
    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ToVec4(setup::kBg);
    colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_PopupBg] = ToVec4(setup::kPanel);
    colors[ImGuiCol_Border] = ToVec4(setup::kPanelBorder);
    colors[ImGuiCol_Text] = ToVec4(kWhite);
    colors[ImGuiCol_TextDisabled] = ToVec4(setup::kFooter);
    colors[ImGuiCol_FrameBg] = ToVec4(setup::kMenuCard);
    colors[ImGuiCol_FrameBgHovered] = ToVec4(Hex(0x181B21));
    colors[ImGuiCol_FrameBgActive] = ToVec4(Hex(0x1F2229));
    colors[ImGuiCol_Button] = ToVec4(kRed);
    colors[ImGuiCol_ButtonHovered] = ToVec4(Hex(0xF2160F));
    colors[ImGuiCol_ButtonActive] = ToVec4(kRedDark);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ScrollbarGrab] = ToVec4(Hex(0x2A2E35));
    colors[ImGuiCol_ScrollbarGrabHovered] = ToVec4(Hex(0x3A3E46));
    colors[ImGuiCol_ScrollbarGrabActive] = ToVec4(Hex(0x4A4E56));
    colors[ImGuiCol_NavHighlight] = ToVec4(kRed);
}

}  // namespace theme
