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
#include "../Slang/WebGPU-Default.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <emscripten.h>
#include <webgpu/webgpu.h>
#include <cstdio>
#include <cstring>

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
    fprintf(stderr, "[WebGPU][ERROR] %s\n", message.data ? message.data : "");
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

void frame(int width, int height)
{
    static auto module = rt_create_module_render({
        .vshader = {.code = VS, .size = (uint32_t)strlen(VS), .entry = VS_ENTRY},
        .fshader = {.code = FS, .size = (uint32_t)strlen(FS), .entry = FS_ENTRY},
        .colors = {{.format = colorFormat,}},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
    });
    static auto pass_color = rt_create_texture({
        .width = (uint32_t)width, .height = (uint32_t)height,
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

    int pw = 600, ph = 600;
    SDL_GetWindowSizeInPixels(window, &pw, &ph);
    frame(pw, ph);
}

static void on_device(WGPURequestDeviceStatus status, WGPUDevice dev, WGPUStringView message, void*, void*)
{
    if (status != WGPURequestDeviceStatus_Success || !dev)
    {
        fprintf(stderr, "[WebGPU][ERROR] requestDevice failed: %s\n", message.data ? message.data : "");
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
        fprintf(stderr, "[WebGPU][ERROR] requestAdapter failed: %s\n", message.data ? message.data : "");
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
    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {};
    canvasDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
    canvasDesc.selector.data = canvas_selector();
    canvasDesc.selector.length = WGPU_STRLEN;
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
    WGPURequestAdapterCallbackInfo request = {};
    request.mode = WGPUCallbackMode_AllowSpontaneous;
    request.callback = on_adapter;
    wgpuInstanceRequestAdapter(instance, &options, request);
    emscripten_set_main_loop(tick, 0, 0);
    return 0;
}
