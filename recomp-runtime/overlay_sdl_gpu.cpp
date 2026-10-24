#include "overlay_sdl_gpu.h"
#include "overlay_imgui_panel.h"

#include "imgui.h"
#include "backends/imgui_impl_sdlgpu3.h"

#include <SDL3/SDL_timer.h>

#include <cstdio>
#include <cstring>

namespace {

bool ready;
bool failed;
SDL_GPUDevice *owner;
Uint64 last_ticks;
Uint32 styled_height;
SDL_GPUTexture *icon_textures[RECOMP_ICON_COUNT];
SDL_GPUSampler *icon_sampler;
SDL_GPUTextureSamplerBinding icon_bindings[RECOMP_ICON_COUNT];
ImTextureID icon_ids[RECOMP_ICON_COUNT];
bool icons_uploaded;

bool start(SDL_GPUDevice *device, SDL_Window *window, Uint32 height)
{
    if (ready) return true;
    if (failed) return false;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    recomp_overlay_imgui_style(static_cast<float>(height));
    styled_height = height;

    ImGui_ImplSDLGPU3_InitInfo info{};
    info.Device = device;
    info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
    info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    if (!ImGui_ImplSDLGPU3_Init(&info)) {
        std::fprintf(stderr, "[overlay] ImGui SDL GPU backend failed to start\n");
        ImGui::DestroyContext();
        failed = true;
        return false;
    }
    owner = device;
    ready = true;
    last_ticks = SDL_GetTicks();
    return true;
}

// The window is resizable: size the panel for the new height. The style and
// font start over so the scale is not applied twice; the backend rebuilds
// the font texture on the next frame.
void restyle(Uint32 height)
{
    if (height == styled_height) return;
    styled_height = height;
    ImGui::GetStyle() = ImGuiStyle();
    ImGui::GetIO().Fonts->Clear();
    recomp_overlay_imgui_style(static_cast<float>(height));
    ImGui_ImplSDLGPU3_DestroyFontsTexture();
}

// Uploads the icon images the game side handed over, once, in a copy pass
// before the overlay's render pass. False until there are images to upload.
bool upload_icons(SDL_GPUDevice *device, SDL_GPUCommandBuffer *command_buffer)
{
    if (icons_uploaded) return true;
    const RecompIconImage *images = recomp_overlay_icon_images();
    if (images == nullptr) return false;
    Uint32 total = 0u;
    for (unsigned i = 0u; i < RECOMP_ICON_COUNT; ++i) total += images[i].width * images[i].height * 4u;
    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = total;
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (transfer == nullptr) return false;
    auto *bytes = static_cast<Uint8 *>(SDL_MapGPUTransferBuffer(device, transfer, false));
    if (bytes == nullptr) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }
    for (unsigned i = 0u, at = 0u; i < RECOMP_ICON_COUNT; at += images[i].width * images[i].height * 4u, ++i) {
        std::memcpy(bytes + at, images[i].rgba, images[i].width * images[i].height * 4u);
    }
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(command_buffer);
    for (unsigned i = 0u, at = 0u; i < RECOMP_ICON_COUNT; at += images[i].width * images[i].height * 4u, ++i) {
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = images[i].width;
        info.height = images[i].height;
        info.layer_count_or_depth = 1u;
        info.num_levels = 1u;
        icon_textures[i] = SDL_CreateGPUTexture(device, &info);
        if (icon_textures[i] == nullptr || copy == nullptr) continue;
        SDL_GPUTextureTransferInfo source{};
        source.transfer_buffer = transfer;
        source.offset = at;
        source.pixels_per_row = images[i].width;
        source.rows_per_layer = images[i].height;
        SDL_GPUTextureRegion region{};
        region.texture = icon_textures[i];
        region.w = images[i].width;
        region.h = images[i].height;
        region.d = 1u;
        SDL_UploadToGPUTexture(copy, &source, &region, false);
    }
    if (copy != nullptr) SDL_EndGPUCopyPass(copy);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_GPUSamplerCreateInfo sampler{};
    sampler.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    icon_sampler = SDL_CreateGPUSampler(device, &sampler);
    for (unsigned i = 0u; i < RECOMP_ICON_COUNT; ++i) {
        icon_bindings[i].texture = icon_textures[i];
        icon_bindings[i].sampler = icon_sampler;
        icon_ids[i] = icon_textures[i] != nullptr && icon_sampler != nullptr
            ? reinterpret_cast<ImTextureID>(&icon_bindings[i]) : ImTextureID{};
    }
    icons_uploaded = true;
    return true;
}

}  // namespace

void recomp_overlay_sdl_gpu_draw(SDL_GPUDevice *device, SDL_Window *window,
    SDL_GPUCommandBuffer *command_buffer, SDL_GPUTexture *target,
    Uint32 width, Uint32 height, float picture_x, float picture_y, float picture_width,
    float picture_height, const RecompOverlayText *text)
{
    if (device == nullptr || window == nullptr || command_buffer == nullptr || target == nullptr ||
        width == 0u || height == 0u || recomp_overlay_empty(text)) {
        return;
    }
    if (!start(device, window, height)) return;
    restyle(height);

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    const Uint64 now = SDL_GetTicks();
    io.DeltaTime = now > last_ticks ? static_cast<float>(now - last_ticks) / 1000.0f : 1.0f / 60.0f;
    last_ticks = now;

    const bool icons = (text->icon_count != 0u || text->keys.count != 0u) && upload_icons(device, command_buffer);
    const RecompOverlayPicture picture{picture_x, picture_y, picture_width, picture_height};

    ImGui_ImplSDLGPU3_NewFrame();
    ImGui::NewFrame();
    recomp_overlay_imgui_build(*text, io.DisplaySize.x, io.DisplaySize.y, &picture, icons ? icon_ids : nullptr);
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    if (draw_data == nullptr || draw_data->CmdListsCount == 0) return;

    Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
    SDL_GPUColorTargetInfo color{};
    color.texture = target;
    color.load_op = SDL_GPU_LOADOP_LOAD;
    color.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(command_buffer, &color, 1, nullptr);
    if (pass == nullptr) return;
    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, pass);
    SDL_EndGPURenderPass(pass);
}

void recomp_overlay_sdl_gpu_shutdown(void)
{
    if (!ready) return;
    SDL_WaitForGPUIdle(owner);
    for (SDL_GPUTexture *&texture : icon_textures) {
        if (texture != nullptr) SDL_ReleaseGPUTexture(owner, texture);
        texture = nullptr;
    }
    if (icon_sampler != nullptr) SDL_ReleaseGPUSampler(owner, icon_sampler);
    icon_sampler = nullptr;
    icons_uploaded = false;
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();
    ready = false;
}
