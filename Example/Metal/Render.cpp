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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>

static constexpr auto VS = R"(
    #include <metal_stdlib>
    using namespace metal;

    struct Push
    {
        float4x4 projView;
        float4x4 meshMat;
    };

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
    };

    vertex VSOut vs_main(VSIn input [[stage_in]], constant Push& push [[buffer(16)]])
    {
        VSOut output;
        output.world = (push.meshMat * float4(input.in_vertex, 1.0)).xyz;
        output.normal = (push.meshMat * float4(input.in_normal, 0.0)).xyz;
        output.uv = input.in_uv;
        output.position = push.projView * float4(output.world, 1.0);
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
    };

    constant float3 LIGHT_POSITION = float3(5.0, 5.0, 5.0);
    constant float3 LIGHT_COLOR = float3(1.0, 0.98, 0.94);
    constant float LIGHT_INTENSITY = 1.5;
    constant float3 AMBIENT_COLOR = float3(0.1, 0.1, 0.1);
    constant float3 DIFFUSE_COLOR = float3(1.0, 1.0, 1.0);
    constant float3 SPECULAR_COLOR = float3(0.5, 0.5, 0.5);
    constant float SHININESS = 32.0;
    constant float3 CAMERA_POSITION = float3(0.0, 0.0, 10.0);

    fragment float4 fs_main(PSIn input [[stage_in]], texture2d<float> texture0 [[texture(0)]], sampler sampler0 [[sampler(1)]])
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(LIGHT_POSITION - input.world);
        float3 V = normalize(CAMERA_POSITION - input.world);
        float3 H = normalize(L + V);
        float3 ambient = AMBIENT_COLOR;
        float diff = max(dot(N, L), 0.0);
        float3 diffuse = diff * DIFFUSE_COLOR * texture0.sample(sampler0, input.uv).rgb;
        float spec = pow(max(dot(N, H), 0.0), SHININESS);
        float3 specular = spec * SPECULAR_COLOR;
        float3 result = ambient + LIGHT_INTENSITY * LIGHT_COLOR * (diffuse + specular);
        return float4(result, 1.0);
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
        .depth = {.write = true, .func = RT_LEQUAL,},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
    });
    static auto pass_color = rt_create_texture({
        .width = (uint32_t)width, .height = (uint32_t)height,
        .format = RT_TEXTURE_BGRA8UNORM,
        .min_filter = RT_LINEAR, .mag_filter = RT_LINEAR,
        .mipmaps = 1,
    });
    static auto pass_depth = rt_create_texture_depth(width, height);
    {
        rt_pass_render_t pass = {.module = module, .colors = {{.texture_view = pass_color.default_view, .clear = true,}}, .depth = {.texture_view = pass_depth.default_view, .clear = true,},};
        rt_begin_render(pass);

        auto projMat = glm::perspectiveRH_ZO(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
        auto viewMat = glm::lookAt(glm::vec3(0, 2, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        auto meshMat = glm::rotate(glm::rotate(glm::mat4(1), glm::radians(-23.5f), glm::vec3(0, 0, 1)), (float)SDL_GetTicks() / 2000.0f, glm::vec3(0, 1, 0));
        glm::mat4 push[2] = {projMat * viewMat, meshMat};
        rt_push_constant((const uint8_t*)push, sizeof(push));

        static auto texture0 = rt_load_texture_file("../../Earth.png", true);
        static auto sampler0 = rt_create_sampler({.min_filter = RT_LINEAR, .mag_filter = RT_LINEAR, .address_u = RT_REPEAT, .address_v = RT_REPEAT,});
        rt_bind_texture(texture0, {.binding = 0,});
        rt_bind_sampler(sampler0, {.binding = 1,});

        static auto mesh = rt_create_mesh_sphere(2, 64, 32);
        rt_draw_mesh(mesh);

        rt_end_render(pass);
    }

    rt_submit();
    present(pass_color, width, height);
}

int main()
{
    int w = 1000, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    auto window = SDL_CreateWindow("Metal-Render", w, h, SDL_WINDOW_METAL);
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
