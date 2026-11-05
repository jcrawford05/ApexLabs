// Start Screen and Car & Track Setup colors, fonts, and ImGui style for Apex Lab.
// Adapted from ApexLabsDevSpace at commit c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0.
#pragma once

#include <cstdint>
#include <string>

#include "imgui.h"

namespace theme {

constexpr ImU32 Hex(uint32_t rgb, uint8_t alpha = 255) {
    return (ImU32(alpha) << 24) | ((rgb & 0xFF) << 16) | (rgb & 0x00FF00) |
           ((rgb >> 16) & 0xFF);
}

inline ImVec4 ToVec4(ImU32 color) {
    return ImGui::ColorConvertU32ToFloat4(color);
}

inline constexpr ImU32 kRed = Hex(0xE10600);
inline constexpr ImU32 kRedDark = Hex(0xB80500);
inline constexpr ImU32 kRedBorder = Hex(0xFF413C);
inline constexpr ImU32 kWhite = Hex(0xFFFFFF);

inline constexpr ImU32 kNavBg = Hex(0x000000);
inline constexpr ImU32 kNavActive = Hex(0xF4F4F5);
inline constexpr ImU32 kNavIdle = Hex(0x9A9AA3);

// Start + Car & Track Setup ("setup" palette)
namespace setup {
inline constexpr ImU32 kBg = Hex(0x090A0D);
inline constexpr ImU32 kPanel = Hex(0x0E1014);
inline constexpr ImU32 kPanelBorder = Hex(0x262A31);
inline constexpr ImU32 kDivider = Hex(0x252930);
inline constexpr ImU32 kCard = Hex(0x13151A);
inline constexpr ImU32 kCardBorder = Hex(0x22262C);
inline constexpr ImU32 kCardActive = Hex(0x1B1111);
inline constexpr ImU32 kCardActiveBorder = Hex(0x681D1A);
inline constexpr ImU32 kTitle = Hex(0xF4F5F6);
inline constexpr ImU32 kCardTitle = Hex(0xC7CBD1);
inline constexpr ImU32 kActiveSub = Hex(0xD98681);
inline constexpr ImU32 kMuted = Hex(0x6D737D);
inline constexpr ImU32 kMuted3 = Hex(0x656C76);
inline constexpr ImU32 kTagline = Hex(0xD6D8DD);
inline constexpr ImU32 kFooter = Hex(0x737984);
inline constexpr ImU32 kMenuCard = Hex(0x13151A);
inline constexpr ImU32 kMenuBorder = Hex(0x2B2E35);
inline constexpr ImU32 kMenuIcon = Hex(0x1E2127);
inline constexpr ImU32 kMenuIconText = Hex(0xC7CAD0);
inline constexpr ImU32 kMenuTitle = Hex(0xF0F1F3);
inline constexpr ImU32 kMenuSub = Hex(0x737985);
inline constexpr ImU32 kMenuArrow = Hex(0x777D87);
inline constexpr ImU32 kMenuSubActive = Hex(0xFFD5D3);
}  // namespace setup

enum class Face {
    SpaceBold,
    SpaceMedium,
    PlexRegular,
    PlexSemiBold,
};

bool LoadFonts(const std::string& font_dir, float pixel_scale);
ImFont* Font(Face face, float size);
float LineHeight(Face face, float size);
void ApplyStyle();

}  // namespace theme
