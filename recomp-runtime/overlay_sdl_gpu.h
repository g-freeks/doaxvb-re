#ifndef DOAXBV_RECOMP_OVERLAY_SDL_GPU_H
#define DOAXBV_RECOMP_OVERLAY_SDL_GPU_H

#include <SDL3/SDL_gpu.h>

#include "overlay_text.h"

/* Draws the runtime's overlay text (overlay_text.h) with Dear ImGui into a
   colour target of the window's swapchain format, after the game's frame.
   Created on first use; nothing is drawn while the overlay is empty. The
   picture is where the game's frame sits in the target, for the icons. */
void recomp_overlay_sdl_gpu_draw(SDL_GPUDevice *device, SDL_Window *window,
    SDL_GPUCommandBuffer *command_buffer, SDL_GPUTexture *target,
    Uint32 width, Uint32 height, float picture_x, float picture_y, float picture_width,
    float picture_height, const RecompOverlayText *text);
void recomp_overlay_sdl_gpu_shutdown(void);

#endif
