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
        float4 c0;
        float4 c1;
        float4 c2;
        packed_float3 translation;
        uint albedo;
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
        float4x4 meshMat = float4x4(push.c0, push.c1, push.c2, float4(float3(push.translation), 1.0));
        VSOut output;
        output.world = (meshMat * float4(input.in_vertex, 1.0)).xyz;
        output.normal = (meshMat * float4(input.in_normal, 0.0)).xyz;
        output.uv = input.in_uv;
        output.position = push.projView * float4(output.world, 1.0);
        return output;
    }
)";

static constexpr auto FS = R"(
    #include <metal_stdlib>
    using namespace metal;

    struct Push
    {
        float4x4 projView;
        float4 c0;
        float4 c1;
        float4 c2;
        packed_float3 translation;
        uint albedo;
    };

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
    constant float3 CAMERA_POSITION = float3(0.0, 2.0, 8.0);

    fragment float4 fs_main(PSIn input [[stage_in]], device uint64_t* handles [[buffer(0)]], constant Push& push [[buffer(16)]], sampler samp [[sampler(1)]])
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(LIGHT_POSITION - input.world);
        float3 V = normalize(CAMERA_POSITION - input.world);
        float3 H = normalize(L + V);
        float3 ambient = AMBIENT_COLOR;
        float diff = max(dot(N, L), 0.0);
        texture2d<float> tex = texture2d<float>(handles[push.albedo]);
        float3 sampled = tex.sample(samp, input.uv).rgb;
        float3 diffuse = diff * DIFFUSE_COLOR * sampled;
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
        fprintf(stderr, "[Metal][ERROR] nextDrawable failed");
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

struct Push
{
    glm::mat4 projView;
    glm::vec4 c0;
    glm::vec4 c1;
    glm::vec4 c2;
    glm::vec3 translation;
    uint32_t albedo;
};
static_assert(sizeof(Push) == 128, "bindless push constant is 128 bytes");

static Push make_push(glm::mat4 const& projView, glm::mat4 const& mesh, uint32_t albedo)
{
    Push push = {};
    push.projView = projView;
    push.c0 = mesh[0];
    push.c1 = mesh[1];
    push.c2 = mesh[2];
    push.translation = glm::vec3(mesh[3]);
    push.albedo = albedo;
    return push;
}

static rt_texture_t load_bindless(const char* filename)
{
    rt_image_t image = rt_load_image_file(filename, true);
    rt_texture_t texture = rt_create_texture({
        .width = image.width,
        .height = image.height,
        .depth = 1,
        .mipmaps = 1,
        .target = RT_TEXTURE_2D,
        .format = image.format,
        .usages = RT_TEXTURE_USAGE_COPY_SRC | RT_TEXTURE_USAGE_COPY_DST | RT_TEXTURE_USAGE_TEXTURE_BINDING | RT_TEXTURE_USAGE_STORAGE_BINDING | RT_TEXTURE_USAGE_RENDER_ATTACHMENT | RT_TEXTURE_USAGE_TEXTURE_BINDLESS,
        .min_filter = RT_LINEAR,
        .mag_filter = RT_LINEAR,
        .address_u = RT_REPEAT,
        .address_v = RT_REPEAT,
        .address_w = RT_REPEAT,
        .data = image.data,
    });
    rt_destroy_image(image);
    return texture;
}

static void write_handles(void* memory, uint32_t stride, rt_texture_t const* textures, int count)
{
    if (!memory || stride == 0) return;
    if (stride == sizeof(uint32_t))
    {
        auto* handles = (uint32_t*)memory;
        for (int i = 0; i < count; ++i)
            handles[i] = (uint32_t)textures[i].default_view.bindless;
    }
    else if (stride == sizeof(uint64_t))
    {
        auto* handles = (uint64_t*)memory;
        for (int i = 0; i < count; ++i)
            handles[i] = textures[i].default_view.bindless;
    }
}

void frame(int width, int height)
{
    static auto layout = rt_create_layout({
        .entry = {{.binding = 0, .type = RT_BINDING_STORAGE_BUFFER}, {.binding = 1, .type = RT_BINDING_SAMPLER}},
    });
    static auto module = rt_create_module_render({
        .layout = layout,
        .vshader = {.code = VS, .size = (uint32_t)strlen(VS), .entry = "vs_main"},
        .fshader = {.code = FS, .size = (uint32_t)strlen(FS), .entry = "fs_main"},
        .colors = {{.format = RT_TEXTURE_BGRA8UNORM,}},
        .depth = {.write = true, .func = RT_LEQUAL,},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
    });
    static auto pass_color = rt_create_texture({
        .width = (uint32_t)width, .height = (uint32_t)height,
        .mipmaps = 1,
        .format = RT_TEXTURE_BGRA8UNORM,
        .min_filter = RT_LINEAR, .mag_filter = RT_LINEAR,
    });
    static auto pass_depth = rt_create_texture_depth(width, height);
    static auto cube = rt_create_mesh_cube(1.4f, 1.4f, 1.4f);
    static const char* files[] = {
        "../../Example/OpenGL/Default.png",
        "../../Example/OpenGL/Example.png",
        "../../Example/OpenGL/Meshlet.png",
        "../../Example/OpenGL/Render.png",
    };
    static rt_texture_t textures[4] = {
        load_bindless(files[0]),
        load_bindless(files[1]),
        load_bindless(files[2]),
        load_bindless(files[3]),
    };
    static const glm::vec3 places[4] = {
        {-3.3f, 0.0f, 0.0f},
        {-1.1f, 0.0f, 0.0f},
        { 1.1f, 0.0f, 0.0f},
        { 3.3f, 0.0f, 0.0f},
    };
    static auto limits = rt_get_limits();
    static auto table = rt_create_buffer({
        .size = (size_t)limits.max_bindless_handle_size * 4,
        .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_MAP_WRITE,
    });
    static auto samp = rt_create_sampler({.min_filter = RT_LINEAR, .mag_filter = RT_LINEAR, .address_u = RT_REPEAT, .address_v = RT_REPEAT,});
    static auto binding = rt_create_binding({
        .layout = layout,
        .entry = {
            {.binding = 0, .buffer = table},
            {.binding = 1, .sampler = samp},
        },
    });
    if (auto* memory = rt_map_buffer(table, RT_WRITE_ONLY, 0, table.size))
    {
        write_handles(memory, limits.max_bindless_handle_size, textures, 4);
        rt_unmap_buffer(table);
    }
    {
        rt_pass_render_t pass = {.colors = {{.texture = pass_color.default_view, .clear = true,}}, .depth = {.texture = pass_depth.default_view, .clear = true,},};
        rt_begin_render(pass);
        rt_bind_module_render(module);
        rt_bind_binding(binding);

        auto projView = glm::perspectiveRH_ZO(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f) * glm::lookAt(glm::vec3(0, 2, 8), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        float spin = (float)SDL_GetTicks() / 2000.0f;
        for (int i = 0; i < 4; ++i)
        {
            auto meshMat = glm::translate(glm::mat4(1), places[i]);
            meshMat = glm::rotate(meshMat, spin + i * 0.4f, glm::vec3(0, 1, 0));
            meshMat = glm::rotate(meshMat, glm::radians(-18.0f), glm::vec3(1, 0, 0));
            Push push = make_push(projView, meshMat, (uint32_t)i);
            rt_push_constant((const uint8_t*)&push, sizeof(push));
            rt_draw_index(cube.vertex, 3, cube.index, cube.index.size / sizeof(uint32_t), 1, 0, 0, 0);
        }

        rt_end_render(pass);
    }

    rt_submit();
    present(pass_color, width, height);
}

int main()
{
    int w = 1000, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    auto window = SDL_CreateWindow("Metal-Bindless", w, h, SDL_WINDOW_METAL);
    SDL_MetalView view = SDL_Metal_CreateView(window);
    layer = (CA::MetalLayer*)SDL_Metal_GetLayer(view);
    if (!layer)
    {
        fprintf(stderr, "[Metal][ERROR] SDL window has no CAMetalLayer");
        return 1;
    }

    device = MTL::CreateSystemDefaultDevice();
    if (!device)
    {
        fprintf(stderr, "[Metal][ERROR] MTLCreateSystemDefaultDevice failed");
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
