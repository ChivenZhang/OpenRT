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
#define WEBGPU_IMPLEMENTATION
#include "../../OpenRTX.h"
#include "../../WebGPU.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <emscripten.h>
#include <cstdio>
#include <cstring>

static constexpr auto VS = R"(
    struct VSOut {
        @builtin(position) position: vec4f,
        @location(0) vertex: vec3f,
        @location(1) normal: vec3f,
        @location(2) uv: vec2f,
        @location(3) color: vec3f,
    };

    @vertex
    fn main(
        @location(0) in_vertex: vec3f,
        @location(1) in_normal: vec3f,
        @location(2) in_uv: vec2f,
        @builtin(vertex_index) vertexID: u32
    ) -> VSOut {
        var colors = array<vec3f, 3>(vec3f(1, 0, 0), vec3f(0, 1, 0), vec3f(0, 0, 1));
        var output: VSOut;
        output.vertex = in_vertex;
        output.normal = in_normal;
        output.uv = in_uv;
        output.color = colors[vertexID];
        output.position = vec4f(in_vertex, 1.0);
        return output;
    }
)";

static constexpr auto FS = R"(
    @fragment
    fn main(
        @location(0) vertex: vec3f,
        @location(1) normal: vec3f,
        @location(2) uv: vec2f,
        @location(3) color: vec3f
    ) -> @location(0) vec4f {
        return vec4f(color, 1);
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

static void on_error(WGPUErrorType, char const* message, void*)
{
    fprintf(stderr, "[WebGPU][ERROR] %s\n", message ? message : "");
}

static void present(rt_texture_t& color, uint32_t width, uint32_t height)
{
    if (width < 1 || height < 1) return;
    configure_surface((int)width, (int)height);

    WGPUSurfaceTexture current = {};
    wgpuSurfaceGetCurrentTexture(surface, &current);
    if (current.status != WGPUSurfaceGetCurrentTextureStatus_Success || !current.texture)
    {
        if (current.texture) wgpuTextureRelease(current.texture);
        return;
    }

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    WGPUImageCopyTexture src = {};
    src.texture = *reinterpret_cast<WGPUTexture*>(color.native);
    src.aspect = WGPUTextureAspect_All;
    WGPUImageCopyTexture dst = {};
    dst.texture = current.texture;
    dst.aspect = WGPUTextureAspect_All;
    WGPUExtent3D size = {width, height, 1};
    wgpuCommandEncoderCopyTextureToTexture(encoder, &src, &dst, &size);

    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuSurfacePresent(surface);
    wgpuTextureRelease(current.texture);
}

void frame(int width, int height)
{
    static auto module = rt_create_module_render({
        .vshader = {.code = VS, .size = (uint32_t)strlen(VS)},
        .fshader = {.code = FS, .size = (uint32_t)strlen(FS)},
        .colors = {{.format = colorFormat,}},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
    });
    static uint32_t colorWidth = (uint32_t)width;
    static uint32_t colorHeight = (uint32_t)height;
    static auto pass_color = rt_create_texture({
        .width = colorWidth, .height = colorHeight,
        .format = colorFormat,
        .min_filter = RT_LINEAR, .mag_filter = RT_LINEAR,
        .mipmaps = 1,
    });
    {
        rt_pass_render_t pass = {.module = module, .colors = {{.texture_view = pass_color.default_view, .clear = true,}},};
        rt_begin_render(pass);

        static auto mesh = rt_create_mesh_triangle(1);
        rt_draw_mesh(mesh);

        rt_end_render(pass);
    }

    rt_submit();
    present(pass_color, colorWidth, colorHeight);
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

    int pw = 600, ph = 600;
    SDL_GetWindowSizeInPixels(window, &pw, &ph);
    frame(pw, ph);
}

static void on_device(WGPURequestDeviceStatus status, WGPUDevice dev, char const* message, void*)
{
    if (status != WGPURequestDeviceStatus_Success || !dev)
    {
        fprintf(stderr, "[WebGPU][ERROR] requestDevice failed: %s\n", message ? message : "");
        return;
    }
    device = dev;
    queue = wgpuDeviceGetQueue(device);
    wgpuDeviceSetUncapturedErrorCallback(device, on_error, nullptr);
    choose_format(wgpuSurfaceGetPreferredFormat(surface, adapter));
    wg_load_library(device, queue);
    gpuReady = true;
    if (adapter)
    {
        wgpuAdapterRelease(adapter);
        adapter = nullptr;
    }
}

static void on_adapter(WGPURequestAdapterStatus status, WGPUAdapter ad, char const* message, void*)
{
    if (status != WGPURequestAdapterStatus_Success || !ad)
    {
        fprintf(stderr, "[WebGPU][ERROR] requestAdapter failed: %s\n", message ? message : "");
        return;
    }
    adapter = ad;
    wgpuAdapterRequestDevice(adapter, nullptr, on_device, nullptr);
}

int main()
{
    int w = 600, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("WebGPU-Default", w, h, 0);
    if (!window)
    {
        fprintf(stderr, "[WebGPU][ERROR] SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    WGPUInstanceDescriptor instanceDesc = {};
    instance = wgpuCreateInstance(&instanceDesc);
    WGPUSurfaceDescriptorFromCanvasHTMLSelector canvasDesc = {};
    canvasDesc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    canvasDesc.selector = canvas_selector();
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &canvasDesc.chain;
    surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
    if (!surface)
    {
        fprintf(stderr, "[WebGPU][ERROR] wgpuInstanceCreateSurface failed\n");
        return 1;
    }

    WGPURequestAdapterOptions options = {};
    options.compatibleSurface = surface;
    wgpuInstanceRequestAdapter(instance, &options, on_adapter, nullptr);
    emscripten_set_main_loop(tick, 0, 0);
    return 0;
}
