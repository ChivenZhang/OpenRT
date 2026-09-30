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
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>
#include <cstdio>

static constexpr auto VS = R"(
    #include <metal_stdlib>
    using namespace metal;

    struct VSIn
    {
        float3 in_vertex [[attribute(0)]];
        float3 in_normal [[attribute(1)]];
        float2 in_uv [[attribute(2)]];
    };

    struct VSOut
    {
        float4 position [[position]];
        float3 world;
        float3 normal;
        float2 uv;
        float3 color;
    };

    vertex VSOut vs_main(VSIn input [[stage_in]], uint vertexID [[vertex_id]])
    {
        float3 colors[3] = {float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1)};
        VSOut output;
        output.world = input.in_vertex;
        output.normal = input.in_normal;
        output.uv = input.in_uv;
        output.color = colors[vertexID];
        output.position = float4(input.in_vertex, 1.0);
        return output;
    }
)";

static constexpr auto FS = R"(
    #include <metal_stdlib>
    using namespace metal;

    struct PSIn
    {
        float4 position [[position]];
        float3 world;
        float3 normal;
        float2 uv;
        float3 color;
    };

    fragment float4 fs_main(PSIn input [[stage_in]])
    {
        return float4(input.color, 1);
    }
)";

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
        .vshader = {.code = VS, .size = (uint32_t)strlen(VS), .entry = "vs_main"},
        .fshader = {.code = FS, .size = (uint32_t)strlen(FS), .entry = "fs_main"},
        .colors = {{.format = RT_TEXTURE_BGRA8UNORM,}},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
    });
    static auto pass_color = rt_create_texture({ .width = (uint32_t)width, .height = (uint32_t)height, .format = RT_TEXTURE_BGRA8UNORM, });
    {
        rt_pass_render_t pass = {.module = module, .colors = {{.texture_view = pass_color.default_view, .clear = true,}},};
        rt_begin_render(pass);

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

    rt_load_library("metal", {.metal = {.device = device, .queue = queue}});

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
