/*=================================================
* Copyright © 2020-2026 ChivenZhang.
* All Rights Reserved.
* =====================Note=========================
*
*
* ====================History=======================
* Created by chivenzhang@gmail.com.
*
* =================================================*/
#define CA_PRIVATE_IMPLEMENTATION
#define OPENRTX_IMPLEMENTATION
#include "../../OpenRTX.h"
#include "../Slang/Metal-Default.h"
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>
#include <cstdio>

static MTL::Device* device = nullptr;
static MTL::CommandQueue* queue = nullptr;
static CA::MetalLayer* layer = nullptr;

static void present(rt_texture_t& color, uint32_t width, uint32_t height)
{
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    CGSize drawableSize = {(CGFloat)width, (CGFloat)height};
    layer->setDrawableSize(drawableSize);
    CA::MetalDrawable* drawable = layer->nextDrawable();
    if (!drawable)
    {
        fprintf(stderr, "[Metal][ERROR] nextDrawable failed\n");
        pool->drain();
        return;
    }

    MTL::CommandBuffer* cmd = queue->commandBuffer();
    MTL::BlitCommandEncoder* blit = cmd->blitCommandEncoder();
    MTL::Texture* src = *reinterpret_cast<MTL::Texture**>(color.native);
    blit->copyFromTexture(src, drawable->texture());
    blit->endEncoding();
    cmd->presentDrawable(drawable);
    cmd->commit();
    cmd->waitUntilCompleted();
    pool->drain();
}

void frame(int width, int height)
{
    static auto module = rt_create_module_render({
        .vshader = {.code = (const char*)VS, .size = sizeof(VS), .entry = VS_ENTRY},
        .fshader = {.code = (const char*)FS, .size = sizeof(FS), .entry = FS_ENTRY},
        .colors = {{.format = RT_TEXTURE_BGRA8UNORM,}},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
    });
    static auto pass_color = rt_create_texture({ .width = (uint32_t)width, .height = (uint32_t)height, .format = RT_TEXTURE_BGRA8UNORM, });
    {
        rt_pass_render_t pass = {.colors = {{.texture_view = pass_color.default_view, .clear = true,}},};
        rt_begin_render(pass);
        rt_bind_module_render(module);

        static auto mesh = rt_create_mesh_triangle(1);
        rt_draw_mesh(mesh);

        rt_end_render(pass);
    }

    rt_submit();
    present(pass_color, width, height);
}

int main()
{
    int w = 600, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    auto window = SDL_CreateWindow("Metal-Default", w, h, SDL_WINDOW_METAL);
    SDL_MetalView view = SDL_Metal_CreateView(window);
    layer = (CA::MetalLayer*)SDL_Metal_GetLayer(view);
    if (!layer)
    {
        fprintf(stderr, "[Metal][ERROR] SDL window has no CAMetalLayer\n");
        return 1;
    }

    device = MTL::CreateSystemDefaultDevice();
    if (!device)
    {
        fprintf(stderr, "[Metal][ERROR] MTLCreateSystemDefaultDevice failed\n");
        return 1;
    }
    queue = device->newCommandQueue();
    layer->setDevice(device);
    layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    layer->setFramebufferOnly(false);

    rt_load_library({.backend = RT_METAL, .metal = {.device = device, .queue = queue}});

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) running = false;
        // ====================================================================

        int pw = w, ph = h;
        SDL_GetWindowSizeInPixels(window, &pw, &ph);
        frame(pw, ph);

        // ====================================================================
    }

    rt_unload_library();
    queue->release();
    device->release();
    SDL_Metal_DestroyView(view);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
