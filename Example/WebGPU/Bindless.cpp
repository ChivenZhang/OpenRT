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
#define OPENRTX_IMPLEMENTATION
#include "../../OpenRTX.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <emscripten.h>
#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cstring>

static constexpr auto VS = R"(
    struct Push {
        projView: mat4x4f,
        c0: vec4f,
        c1: vec4f,
        c2: vec4f,
        translation: vec3f,
        albedo: u32,
    }
    @group(0) @binding(16) var<uniform> push: Push;

    struct VSOut {
        @builtin(position) position: vec4f,
        @location(0) world: vec3f,
        @location(1) normal: vec3f,
        @location(2) uv: vec2f,
    };

    @vertex
    fn vs_main(@location(0) in_vertex: vec3f, @location(1) in_normal: vec3f, @location(2) in_uv: vec2f) -> VSOut {
        let meshMat = mat4x4f(push.c0, push.c1, push.c2, vec4f(push.translation, 1.0));
        var output: VSOut;
        output.world = (meshMat * vec4f(in_vertex, 1.0)).xyz;
        output.normal = (meshMat * vec4f(in_normal, 0.0)).xyz;
        output.uv = in_uv;
        output.position = push.projView * vec4f(output.world, 1.0);
        return output;
    }
)";

static constexpr auto FS = R"(
    struct Push {
        projView: mat4x4f,
        c0: vec4f,
        c1: vec4f,
        c2: vec4f,
        translation: vec3f,
        albedo: u32,
    }
    @group(0) @binding(16) var<uniform> push: Push;
    @group(0) @binding(0) var<storage, read_write> handles: array<u32>;
    @group(0) @binding(1) var samp: sampler;
    @group(0) @binding(2) var tex0: texture_2d<f32>;
    @group(0) @binding(3) var tex1: texture_2d<f32>;
    @group(0) @binding(4) var tex2: texture_2d<f32>;
    @group(0) @binding(5) var tex3: texture_2d<f32>;

    fn sample_slot(slot: u32, uv: vec2f) -> vec3f {
        switch slot {
            case 0u: { return textureSample(tex0, samp, uv).rgb; }
            case 1u: { return textureSample(tex1, samp, uv).rgb; }
            case 2u: { return textureSample(tex2, samp, uv).rgb; }
            case 3u: { return textureSample(tex3, samp, uv).rgb; }
            default: { return textureSample(tex0, samp, uv).rgb; }
        }
    }

    @fragment
    fn fs_main(@location(0) world: vec3f, @location(1) normal: vec3f, @location(2) uv: vec2f) -> @location(0) vec4f {
        let light_position = vec3f(5.0, 5.0, 5.0);
        let light_color = vec3f(1.0, 0.98, 0.94);
        let light_intensity = 1.5;
        let ambient_color = vec3f(0.1, 0.1, 0.1);
        let diffuse_color = vec3f(1.0, 1.0, 1.0);
        let specular_color = vec3f(0.5, 0.5, 0.5);
        let shininess = 32.0;
        let camera_position = vec3f(0.0, 2.0, 8.0);

        let N = normalize(normal);
        let L = normalize(light_position - world);
        let V = normalize(camera_position - world);
        let H = normalize(L + V);
        let ambient = ambient_color;
        let diff = max(dot(N, L), 0.0);
        let stored = handles[push.albedo];
        let slot = select(push.albedo, stored, stored != 0u);
        let sampled = sample_slot(slot, uv);
        let diffuse = diff * diffuse_color * sampled;
        let spec = pow(max(dot(N, H), 0.0), shininess);
        let specular = spec * specular_color;
        let result = ambient + light_intensity * light_color * (diffuse + specular);
        return vec4f(result, 1.0);
    }
)";

static SDL_Window* window = nullptr;
static WGPUInstance instance = nullptr;
static WGPUAdapter adapter = nullptr;
static WGPUDevice device = nullptr;
static WGPUQueue queue = nullptr;
static WGPUSurface surface = nullptr;
static WGPUTextureFormat canvasFormat = WGPUTextureFormat_BGRA8Unorm;
static rt_texture_format_t colorFormat = RT_TEXTURE_BGRA8UNORM;
static bool gpuReady = false;

static const char* canvas_selector()
{
    static char selector[128] = "#canvas";
#ifdef SDL_PROP_WINDOW_EMSCRIPTEN_CANVAS_ID_STRING
    const char* id = SDL_GetStringProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_EMSCRIPTEN_CANVAS_ID_STRING, "canvas");
#else
    const char* id = "canvas";
#endif
    if (id && id[0] == '#')
        snprintf(selector, sizeof(selector), "%s", id);
    else
        snprintf(selector, sizeof(selector), "#%s", (id && id[0]) ? id : "canvas");
    return selector;
}

static void choose_format(WGPUTextureFormat preferred)
{
    switch (preferred)
    {
        case WGPUTextureFormat_RGBA8Unorm:
            canvasFormat = preferred;
            colorFormat = RT_TEXTURE_RGBA8UNORM;
            break;
        case WGPUTextureFormat_RGBA8UnormSrgb:
            canvasFormat = preferred;
            colorFormat = RT_TEXTURE_RGBA8UNORM_SRGB;
            break;
        case WGPUTextureFormat_BGRA8UnormSrgb:
            canvasFormat = preferred;
            colorFormat = RT_TEXTURE_BGRA8UNORM_SRGB;
            break;
        case WGPUTextureFormat_BGRA8Unorm:
        default:
            canvasFormat = WGPUTextureFormat_BGRA8Unorm;
            colorFormat = RT_TEXTURE_BGRA8UNORM;
            break;
    }
}

static uint32_t configuredWidth = 0;
static uint32_t configuredHeight = 0;

static void configure_surface(int width, int height)
{
    uint32_t w = width < 1 ? 1u : (uint32_t)width;
    uint32_t h = height < 1 ? 1u : (uint32_t)height;
    if (w == configuredWidth && h == configuredHeight) return;
    WGPUSurfaceConfiguration config = {};
    config.device = device;
    config.format = canvasFormat;
    config.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    config.width = w;
    config.height = h;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(surface, &config);
    configuredWidth = w;
    configuredHeight = h;
}

static void on_error(WGPUDevice const*, WGPUErrorType, WGPUStringView message, void*, void*)
{
    fprintf(stderr, "[WebGPU][ERROR] %s", message.data ? message.data : "");
}

static void present(rt_texture_t& color, uint32_t width, uint32_t height)
{
    if (width < 1 || height < 1) return;
    configure_surface((int)width, (int)height);

    WGPUSurfaceTexture current = {};
    wgpuSurfaceGetCurrentTexture(surface, &current);
    if ((current.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
         current.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) || !current.texture)
    {
        if (current.texture) wgpuTextureRelease(current.texture);
        return;
    }

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    WGPUTexelCopyTextureInfo src = {};
    src.texture = *reinterpret_cast<WGPUTexture*>(color.native);
    src.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyTextureInfo dst = {};
    dst.texture = current.texture;
    dst.aspect = WGPUTextureAspect_All;
    WGPUExtent3D size = {width, height, 1};
    wgpuCommandEncoderCopyTextureToTexture(encoder, &src, &dst, &size);

    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(surface);
#endif
    wgpuTextureRelease(current.texture);
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
        .target = RT_TEXTURE_2D,
        .format = image.format,
        .usages = RT_TEXTURE_USAGE_TEXTURE_BINDLESS,
        .min_filter = RT_LINEAR,
        .mag_filter = RT_LINEAR,
        .address_u = RT_REPEAT,
        .address_v = RT_REPEAT,
        .address_w = RT_REPEAT,
        .mipmaps = 1,
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
        .entry = {
            {.binding = 0, .type = RT_BINDING_STORAGE_BUFFER},
            {.binding = 1, .type = RT_BINDING_SAMPLER},
            {.binding = 2, .type = RT_BINDING_TEXTURE},
            {.binding = 3, .type = RT_BINDING_TEXTURE},
            {.binding = 4, .type = RT_BINDING_TEXTURE},
            {.binding = 5, .type = RT_BINDING_TEXTURE},
        },
    });
    static auto module = rt_create_module_render({
        .layout = layout,
        .vshader = {.code = VS, .size = (uint32_t)strlen(VS), .entry = "vs_main"},
        .fshader = {.code = FS, .size = (uint32_t)strlen(FS), .entry = "fs_main"},
        .colors = {{.format = colorFormat,}},
        .depth = {.write = true, .func = RT_LEQUAL,},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
        .wind_mode = RT_CCW,
    });
    static auto pass_color = rt_create_texture({
        .width = (uint32_t)width, .height = (uint32_t)height,
        .format = colorFormat,
        .min_filter = RT_LINEAR, .mag_filter = RT_LINEAR,
        .mipmaps = 1,
    });
    static auto pass_depth = rt_create_texture_depth(width, height);
    static auto cube = rt_create_mesh_cube(1.4f, 1.4f, 1.4f);
    static const char* files[] = {
        "Example/OpenGL/Default.png",
        "Example/OpenGL/Example.png",
        "Example/OpenGL/Meshlet.png",
        "Example/OpenGL/Render.png",
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
        .size = limits.max_bindless_handle_size ? (size_t)limits.max_bindless_handle_size * 4 : 16,
        .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_MAP_WRITE,
    });
    static auto samp = rt_create_sampler({.min_filter = RT_LINEAR, .mag_filter = RT_LINEAR, .address_u = RT_REPEAT, .address_v = RT_REPEAT,});
    static auto binding = rt_create_binding({
        .layout = layout,
        .entry = {
            {.binding = 0, .buffer = table},
            {.binding = 1, .sampler = samp},
            {.binding = 2, .texture = textures[0].default_view},
            {.binding = 3, .texture = textures[1].default_view},
            {.binding = 4, .texture = textures[2].default_view},
            {.binding = 5, .texture = textures[3].default_view},
        },
    });
    if (auto* memory = rt_map_buffer(table, RT_WRITE_ONLY, 0, table.size))
    {
        write_handles(memory, limits.max_bindless_handle_size, textures, 4);
        rt_unmap_buffer(table);
    }
    {
        rt_pass_render_t pass = {
            .colors = {{.texture_view = pass_color.default_view, .clear = true,}},
            .depth = {.texture_view = pass_depth.default_view, .clear = true,},
        };
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
            rt_draw_index(cube.vertex, 3, cube.index, cube.index.size / sizeof(uint32_t), 1, 0, 0);
        }

        rt_end_render(pass);
    }

    rt_submit();
    present(pass_color, width, height);
}

static void tick()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            emscripten_cancel_main_loop();
            if (gpuReady) rt_unload_library();
            SDL_DestroyWindow(window);
            SDL_Quit();
            return;
        }
    }
    if (!gpuReady) return;

    int pw = 1000, ph = 600;
    SDL_GetWindowSizeInPixels(window, &pw, &ph);
    frame(pw, ph);
}

static void on_device(WGPURequestDeviceStatus status, WGPUDevice dev, WGPUStringView message, void*, void*)
{
    if (status != WGPURequestDeviceStatus_Success || !dev)
    {
        fprintf(stderr, "[WebGPU][ERROR] requestDevice failed: %s", message.data ? message.data : "");
        return;
    }
    device = dev;
    queue = wgpuDeviceGetQueue(device);
    WGPUSurfaceCapabilities caps = {};
    if (wgpuSurfaceGetCapabilities(surface, adapter, &caps) == WGPUStatus_Success && caps.formatCount)
        choose_format(caps.formats[0]);
    wgpuSurfaceCapabilitiesFreeMembers(caps);
    rt_load_library({.backend = RT_WEBGPU, .webgpu = {.device = device, .queue = queue}});
    gpuReady = true;
    if (adapter)
    {
        wgpuAdapterRelease(adapter);
        adapter = nullptr;
    }
}

static void on_adapter(WGPURequestAdapterStatus status, WGPUAdapter ad, WGPUStringView message, void*, void*)
{
    if (status != WGPURequestAdapterStatus_Success || !ad)
    {
        fprintf(stderr, "[WebGPU][ERROR] requestAdapter failed: %s", message.data ? message.data : "");
        return;
    }
    adapter = ad;
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.uncapturedErrorCallbackInfo.callback = on_error;
    WGPURequestDeviceCallbackInfo request = {};
    request.mode = WGPUCallbackMode_AllowSpontaneous;
    request.callback = on_device;
    wgpuAdapterRequestDevice(adapter, &deviceDesc, request);
}

int main()
{
    int w = 1000, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("WebGPU-Bindless", w, h, 0);
    if (!window)
    {
        fprintf(stderr, "[WebGPU][ERROR] SDL_CreateWindow failed: %s", SDL_GetError());
        return 1;
    }

    WGPUInstanceDescriptor instanceDesc = {};
    instance = wgpuCreateInstance(&instanceDesc);
    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {};
    canvasDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
    canvasDesc.selector.data = canvas_selector();
    canvasDesc.selector.length = WGPU_STRLEN;
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &canvasDesc.chain;
    surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
    if (!surface)
    {
        fprintf(stderr, "[WebGPU][ERROR] wgpuInstanceCreateSurface failed");
        return 1;
    }

    WGPURequestAdapterOptions options = {};
    options.compatibleSurface = surface;
    WGPURequestAdapterCallbackInfo request = {};
    request.mode = WGPUCallbackMode_AllowSpontaneous;
    request.callback = on_adapter;
    wgpuInstanceRequestAdapter(instance, &options, request);
    emscripten_set_main_loop(tick, 0, 0);
    return 0;
}
