#ifndef DOAXBV_RECOMP_OVERLAY_IMGUI_PANEL_H
#define DOAXBV_RECOMP_OVERLAY_IMGUI_PANEL_H

#include "imgui.h"
#include "island_icons.h"
#include "overlay_text.h"

/* The overlay's look and layout in Dear ImGui, shared by every presenter's
   drawer: call the style once after creating the context, with the height
   of the target it will draw to, and the build between ImGui::NewFrame and
   ImGui::Render. */
void recomp_overlay_imgui_style(float target_height);
/* The game's picture inside the target, in pixels; the overlay's icons are
   placed on it. icons holds one texture per island_icons.h image, or is
   null when the presenter has none, and the hint is drawn instead. */
struct RecompOverlayPicture {
    float x, y, width, height;
};
void recomp_overlay_imgui_build(const RecompOverlayText &text, float width, float height,
    const RecompOverlayPicture *picture = nullptr, const ImTextureID *icons = nullptr);

#endif
