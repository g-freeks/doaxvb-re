#include "overlay_imgui_panel.h"

#include "imgui.h"

#include <cfloat>
#include <initializer_list>

namespace {

/* The panel's text is sized for the target's height, as the game's own is:
   ImGui's 13-pixel font at kReferenceHeight lines. The font is built at that
   size once, so it stays sharp. */
constexpr float kReferenceHeight = 600.0f;
constexpr float kFontPixels = 13.0f;
constexpr float kMargin = 12.0f;
float panel_scale = 1.0f;

}  // namespace

void recomp_overlay_imgui_style(float target_height)
{
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    panel_scale = target_height > kReferenceHeight ? target_height / kReferenceHeight : 1.0f;
    ImFontConfig font{};
    font.SizePixels = kFontPixels * panel_scale;
    io.Fonts->AddFontDefault(&font);
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.06f, 0.10f, 0.62f);
    style.ScaleAllSizes(panel_scale);
}

void recomp_overlay_imgui_build(const RecompOverlayText &text, float width, float height,
    const RecompOverlayPicture *picture, const ImTextureID *icons)
{
    const float scale = panel_scale;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    if (text.title[0] != '\0' || text.count != 0u) {
        ImGui::SetNextWindowPos(ImVec2(kMargin * scale, kMargin * scale));
        ImGui::Begin("recomp-overlay-panel", nullptr, flags);
        if (text.title[0] != '\0') {
            ImGui::TextColored(ImVec4(1.0f, 0.86f, 0.35f, 1.0f), "%s", text.title);
        }
        for (unsigned i = 0u; i < text.count; ++i) ImGui::TextUnformatted(text.line[i]);
        ImGui::End();
    }
    const bool draw_icons = text.icon_count != 0u && picture != nullptr && icons != nullptr;
    if (draw_icons) {
        ImDrawList *list = ImGui::GetForegroundDrawList();
        for (unsigned i = 0u; i < text.icon_count; ++i) {
            const RecompOverlayIcon &icon = text.icon[i];
            if (icon.image >= RECOMP_ICON_COUNT || icons[icon.image] == ImTextureID{}) continue;
            const float size = icon.size * picture->height;
            const ImVec2 low(picture->x + icon.x * picture->width + icon.shift * picture->height,
                picture->y + icon.y * picture->height);
            const ImVec2 high(low.x + size, low.y + size);
            const float alpha = icon.alpha < 0.0f ? 0.0f : icon.alpha > 1.0f ? 1.0f : icon.alpha;
            list->AddImage(icons[icon.image], low, high, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                IM_COL32(255, 255, 255, static_cast<int>(alpha * 255.0f + 0.5f)));
            if (icon.label[0] != '\0') {
                /* Green for "+" and "++", red for "-" and "--", twice the
                   text's size and outlined so it reads on any background. */
                ImFont *font = ImGui::GetFont();
                const float font_size = ImGui::GetFontSize() * 2.0f;
                const ImVec2 size_text = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, icon.label);
                const float gap = 3.0f * scale;
                const ImVec2 at(icon.label_left ? low.x - gap - size_text.x : high.x + gap,
                    (low.y + high.y - size_text.y) * 0.5f);
                const ImU32 colour = icon.label[0] == '+' ? IM_COL32(90, 255, 140, 255) : IM_COL32(255, 90, 80, 255);
                const float edge = 1.5f * scale;
                for (const ImVec2 offset : {ImVec2(-edge, 0.0f), ImVec2(edge, 0.0f), ImVec2(0.0f, -edge),
                         ImVec2(0.0f, edge)}) {
                    list->AddText(font, font_size, ImVec2(at.x + offset.x, at.y + offset.y), IM_COL32(0, 0, 0, 230),
                        icon.label);
                }
                list->AddText(font, font_size, at, colour, icon.label);
            }
        }
    }
    /* Icons stand in for the hint that describes them. */
    if (text.hint[0] != '\0' && !draw_icons) {
        ImGui::SetNextWindowPos(ImVec2(width * 0.5f, height - kMargin * scale), 0, ImVec2(0.5f, 1.0f));
        ImGui::Begin("recomp-overlay-hint", nullptr, flags);
        /* Each line of the hint is centred on its own. */
        const char *line = text.hint;
        while (*line != '\0') {
            const char *end = line;
            while (*end != '\0' && *end != '\n') ++end;
            const float line_width = ImGui::CalcTextSize(line, end).x;
            const float room = ImGui::GetContentRegionAvail().x;
            if (room > line_width) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (room - line_width) * 0.5f);
            ImGui::TextUnformatted(line, end);
            line = *end == '\n' ? end + 1 : end;
        }
        ImGui::End();
    }
    if (text.keys.count != 0u) {
        /* At the picture's right edge, ending above the line the game's
           subtitles and the notice use: a line a key, its button (or its
           name in a button-sized space), its action and, dimmer, its
           detail. */
        const RecompOverlayPicture whole{0.0f, 0.0f, width, height};
        const RecompOverlayPicture &area = picture != nullptr ? *picture : whole;
        ImGui::SetNextWindowPos(ImVec2(area.x + area.width - kMargin * scale, area.y + area.height * 0.85f), 0,
            ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.45f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f * scale, 1.0f * scale));
        ImGui::Begin("recomp-overlay-keys", nullptr, flags);
        const float button = ImGui::GetTextLineHeight() * 2.0f;
        for (unsigned i = 0u; i < text.keys.count; ++i) {
            const RecompOverlayKey &key = text.keys.key[i];
            if (key.indent) ImGui::Indent(button * 0.6f);
            if (icons != nullptr && key.image < RECOMP_ICON_COUNT && icons[key.image] != ImTextureID{}) {
                /* The game's button glyphs fill the middle of their 64-pixel
                   textures, from texel 14 to 53 across. */
                ImGui::Image(icons[key.image], ImVec2(button, button), ImVec2(13.0f / 64.0f, 13.0f / 64.0f),
                    ImVec2(54.0f / 64.0f, 54.0f / 64.0f));
            } else {
                const ImVec2 at = ImGui::GetCursorScreenPos();
                const ImVec2 size = ImGui::CalcTextSize(key.name);
                ImGui::Dummy(ImVec2(button, button));
                /* A name without an image, such as a thumbstick's L or R,
                   sits in a ring like the stick's cap, so it does not read
                   as a shoulder button. */
                if (key.name[0] != '\0') {
                    const ImVec2 centre(at.x + button * 0.5f, at.y + button * 0.5f);
                    ImGui::GetWindowDrawList()->AddCircleFilled(centre, button * 0.42f, IM_COL32(20, 30, 40, 200));
                    ImGui::GetWindowDrawList()->AddCircle(centre, button * 0.42f, IM_COL32(200, 225, 235, 230), 0,
                        1.5f * scale);
                }
                ImGui::GetWindowDrawList()->AddText(
                    ImVec2(at.x + (button - size.x) * 0.5f, at.y + (button - size.y) * 0.5f),
                    IM_COL32(255, 255, 255, 255), key.name);
            }
            const float centre = (button - ImGui::GetTextLineHeight()) * 0.5f;
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + centre);
            ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.55f, 1.0f), "%s", key.action);
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + centre);
            ImGui::TextColored(ImVec4(0.85f, 0.92f, 1.0f, 0.8f), "%s", key.detail);
            if (key.indent) ImGui::Unindent(button * 0.6f);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
    if (text.keys.tip[0] != '\0') {
        /* The reminder: a pill in the picture's bottom left corner, the
           notice's twin, with its key in a lighter cap. */
        const RecompOverlayPicture whole{0.0f, 0.0f, width, height};
        const RecompOverlayPicture &area = picture != nullptr ? *picture : whole;
        ImDrawList *list = ImGui::GetForegroundDrawList();
        const ImVec2 key = ImGui::CalcTextSize(text.keys.tip_key);
        const ImVec2 tip = ImGui::CalcTextSize(text.keys.tip);
        const float pad = 6.0f * scale;
        const ImVec2 low(area.x + kMargin * scale, area.y + area.height - kMargin * scale - tip.y - pad);
        const ImVec2 high(low.x + key.x + tip.x + 5.0f * pad, low.y + tip.y + pad);
        const float round = (high.y - low.y) * 0.5f;
        list->AddRectFilled(low, high, IM_COL32(5, 15, 26, 150), round);
        list->AddRect(low, high, IM_COL32(170, 235, 255, 120), round, 0, 1.0f * scale);
        const ImVec2 cap_low(low.x + pad * 0.75f, low.y + pad * 0.3f);
        const ImVec2 cap_high(cap_low.x + key.x + pad, high.y - pad * 0.3f);
        list->AddRectFilled(cap_low, cap_high, IM_COL32(200, 225, 235, 220), (cap_high.y - cap_low.y) * 0.3f);
        list->AddText(ImVec2(cap_low.x + pad * 0.5f, low.y + pad * 0.5f), IM_COL32(10, 25, 40, 255),
            text.keys.tip_key);
        list->AddText(ImVec2(cap_high.x + pad, low.y + pad * 0.5f), IM_COL32(230, 240, 250, 230), text.keys.tip);
    }
    if (text.notice[0] != '\0') {
        /* A pill in the picture's bottom right corner, glass and green like
           the game's own panels; a busy notice leads with a turning arc. */
        const RecompOverlayPicture whole{0.0f, 0.0f, width, height};
        const RecompOverlayPicture &area = picture != nullptr ? *picture : whole;
        ImDrawList *list = ImGui::GetForegroundDrawList();
        const ImVec2 size = ImGui::CalcTextSize(text.notice);
        const float pad = 6.0f * scale;
        const float mark = text.notice_busy ? size.y + pad : 0.0f;
        const ImVec2 high(area.x + area.width - kMargin * scale, area.y + area.height - kMargin * scale);
        const ImVec2 low(high.x - size.x - mark - 3.0f * pad, high.y - size.y - pad);
        const float round = (high.y - low.y) * 0.5f;
        list->AddRectFilled(low, high, IM_COL32(5, 15, 26, 170), round);
        list->AddRect(low, high, IM_COL32(170, 235, 255, 150), round, 0, 1.0f * scale);
        if (text.notice_busy) {
            const float radius = size.y * 0.38f;
            const ImVec2 centre(low.x + 1.5f * pad + size.y * 0.5f, (low.y + high.y) * 0.5f);
            const float start = static_cast<float>(ImGui::GetTime()) * 6.0f;
            list->PathArcTo(centre, radius, start, start + 4.2f, 16);
            list->PathStroke(IM_COL32(90, 255, 140, 255), 0, 2.0f * scale);
        }
        list->AddText(ImVec2(low.x + 1.5f * pad + mark, low.y + pad * 0.5f), IM_COL32(90, 255, 140, 255),
            text.notice);
    }
}
