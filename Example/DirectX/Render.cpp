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
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define OPENRTX_IMPLEMENTATION
#include "../../OpenRTX.h"
#include "../../DirectX.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstring>
#include <vector>

using Microsoft::WRL::ComPtr;

static constexpr auto VS = R"(
    cbuffer Push : register(b16)
    {
        column_major float4x4 projView;
        column_major float4x4 meshMat;
    };

    struct VSIn
    {
        float3 in_vertex : TEXCOORD0;
        float3 in_normal : TEXCOORD1;
        float2 in_uv : TEXCOORD2;
    };

    struct VSOut
    {
        float4 position : SV_Position;
        float3 vertex : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };

    VSOut main(VSIn input)
    {
        VSOut output;
        output.vertex = mul(meshMat, float4(input.in_vertex, 1)).xyz;
        output.normal = mul(meshMat, float4(input.in_normal, 0)).xyz;
        output.uv = input.in_uv;
        output.position = mul(projView, float4(output.vertex, 1));
        return output;
    }
)";

static constexpr auto FS = R"(
    Texture2D texture0 : register(t0);
    SamplerState sampler0 : register(s1);

    struct PSIn
    {
        float4 position : SV_Position;
        float3 vertex : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };

    static const float3 LIGHT_POSITION = float3(5.0, 5.0, 5.0);
    static const float3 LIGHT_COLOR = float3(1.0, 0.98, 0.94);
    static const float LIGHT_INTENSITY = 1.5;
    static const float3 AMBIENT_COLOR = float3(0.1, 0.1, 0.1);
    static const float3 DIFFUSE_COLOR = float3(1.0, 1.0, 1.0);
    static const float3 SPECULAR_COLOR = float3(0.5, 0.5, 0.5);
    static const float SHININESS = 32.0;
    static const float3 CAMERA_POSITION = float3(0.0, 0.0, 10.0);

    float4 main(PSIn input) : SV_Target0
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(LIGHT_POSITION - input.vertex);
        float3 V = normalize(CAMERA_POSITION - input.vertex);
        float3 H = normalize(L + V);

        float3 ambient = AMBIENT_COLOR;
        float diff = max(dot(N, L), 0.0);
        float3 diffuse = diff * DIFFUSE_COLOR * texture0.Sample(sampler0, input.uv).rgb;
        float spec = pow(max(dot(N, H), 0.0), SHININESS);
        float3 specular = spec * SPECULAR_COLOR;
        float3 result = ambient + LIGHT_INTENSITY * LIGHT_COLOR * (diffuse + specular);
        return float4(result, 1);
    }
)";

static ComPtr<ID3D12Device> device;
static ComPtr<ID3D12CommandQueue> queue;
static ComPtr<IDXGISwapChain3> swapchain;
static ComPtr<ID3D12Resource> backBuffers[2];
static D3D12_RESOURCE_STATES backStates[2] = {D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON};
static ComPtr<ID3D12CommandAllocator> presentAllocator;
static ComPtr<ID3D12GraphicsCommandList> presentCmd;
static ComPtr<ID3D12Fence> presentFence;
static HANDLE presentEvent = nullptr;
static UINT64 presentValue = 0;

static std::vector<uint8_t> compile_shader(const char* source, const char* target)
{
    ComPtr<ID3DBlob> code, error;
    HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, "main", target, 0, 0, code.GetAddressOf(), error.GetAddressOf());
    if (FAILED(hr))
    {
        fprintf(stderr, "[DirectX][ERROR] shader compile failed:\n%s\n", error ? (const char*)error->GetBufferPointer() : "");
        abort();
    }
    auto* bytes = (const uint8_t*)code->GetBufferPointer();
    return {bytes, bytes + code->GetBufferSize()};
}

static void wait_present()
{
    if (!presentFence || presentValue == 0) return;
    if (presentFence->GetCompletedValue() < presentValue)
    {
        presentFence->SetEventOnCompletion(presentValue, presentEvent);
        WaitForSingleObject(presentEvent, INFINITE);
    }
}

static void present(rt_texture_t& color)
{
    wait_present();
    presentAllocator->Reset();
    presentCmd->Reset(presentAllocator.Get(), nullptr);

    UINT index = swapchain->GetCurrentBackBufferIndex();
    ID3D12Resource* src = *reinterpret_cast<ID3D12Resource**>(color.native);
    ID3D12Resource* dst = backBuffers[index].Get();

    D3D12_RESOURCE_BARRIER toCopy[2] = {};
    toCopy[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toCopy[0].Transition.pResource = src;
    toCopy[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    toCopy[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    toCopy[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    toCopy[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toCopy[1].Transition.pResource = dst;
    toCopy[1].Transition.StateBefore = backStates[index];
    toCopy[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    toCopy[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    presentCmd->ResourceBarrier(2, toCopy);

    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = src;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = dst;
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    presentCmd->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    D3D12_RESOURCE_BARRIER toPresent[2] = {};
    toPresent[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent[0].Transition.pResource = src;
    toPresent[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    toPresent[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    toPresent[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    toPresent[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent[1].Transition.pResource = dst;
    toPresent[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    toPresent[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    toPresent[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    presentCmd->ResourceBarrier(2, toPresent);
    backStates[index] = D3D12_RESOURCE_STATE_PRESENT;

    presentCmd->Close();
    ID3D12CommandList* lists[] = {presentCmd.Get()};
    queue->ExecuteCommandLists(1, lists);
    presentValue++;
    queue->Signal(presentFence.Get(), presentValue);
    swapchain->Present(1, 0);
}

void frame(int width, int height)
{
    wait_present();
    static auto vs = compile_shader(VS, "vs_5_1");
    static auto fs = compile_shader(FS, "ps_5_1");
    static auto module = rt_create_module_render({
        .vshader = {.code = (const char*)vs.data(), .size = (uint32_t)vs.size()},
        .fshader = {.code = (const char*)fs.data(), .size = (uint32_t)fs.size()},
        .colors = {{.format = RT_TEXTURE_RGBA8UNORM,}},
        .depth = {.write = true, .func = RT_LEQUAL,},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
        .binding = {{.binding = 0, .type = RT_BINDING_TEXTURE}, {.binding = 1, .type = RT_BINDING_SAMPLER}},
    });
    static auto pass_color = rt_create_texture_color(width, height, nullptr);
    static auto pass_depth = rt_create_texture_depth(width, height, nullptr);
    {
        rt_pass_render_t pass = {.module = module, .colors = {{.texture_view = pass_color.default_view, .clear = true,}}, .depth = {.texture_view = pass_depth.default_view, .clear = true,},};
        rt_begin_render(pass);

        auto projMat = glm::perspectiveRH_ZO(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
        auto viewMat = glm::lookAt(glm::vec3(0, 2, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        auto meshMat = glm::rotate(glm::rotate(glm::mat4(1), glm::radians(-23.5f), glm::vec3(0, 0, 1)), (float)SDL_GetTicks() / 2000.0f, glm::vec3(0, 1, 0));
        glm::mat4 push[2] = {projMat * viewMat, meshMat};
        rt_push_constant((const uint8_t*)push, sizeof(push));

        static auto texture0 = rt_load_texture_file("../../Earth.png", true);
        rt_bind_texture(texture0, {.binding = 0,});
        static auto sampler0 = rt_create_sampler({.min_filter = RT_LINEAR, .mag_filter = RT_LINEAR, .wrap_s = RT_REPEAT, .wrap_t = RT_REPEAT,});
        rt_bind_sampler(sampler0, {.binding = 1,});

        static auto mesh = rt_create_mesh_sphere(2, 64, 32);
        rt_draw_mesh(mesh);

        rt_end_render(pass);
    }

    rt_submit();
    present(pass_color);
}

int main()
{
    int w = 1000, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    auto window = SDL_CreateWindow("DirectX-Render", w, h, 0);
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!hwnd)
    {
        fprintf(stderr, "[DirectX][ERROR] SDL window has no HWND\n");
        return 1;
    }

    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        debug->EnableDebugLayer();

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))))
    {
        fprintf(stderr, "[DirectX][ERROR] CreateDXGIFactory2 failed\n");
        return 1;
    }
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
            break;
        device.Reset();
    }
    if (!device)
    {
        fprintf(stderr, "[DirectX][ERROR] D3D12CreateDevice failed\n");
        return 1;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue))))
    {
        fprintf(stderr, "[DirectX][ERROR] CreateCommandQueue failed\n");
        return 1;
    }

    DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
    swapDesc.Width = (UINT)w;
    swapDesc.Height = (UINT)h;
    swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.BufferCount = 2;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    ComPtr<IDXGISwapChain1> swap1;
    if (FAILED(factory->CreateSwapChainForHwnd(queue.Get(), hwnd, &swapDesc, nullptr, nullptr, &swap1)))
    {
        fprintf(stderr, "[DirectX][ERROR] CreateSwapChainForHwnd failed\n");
        return 1;
    }
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    swap1.As(&swapchain);
    for (UINT i = 0; i < 2; ++i)
        swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&presentAllocator));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, presentAllocator.Get(), nullptr, IID_PPV_ARGS(&presentCmd));
    presentCmd->Close();
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&presentFence));
    presentEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    dx_load_library(device.Get(), queue.Get());

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) running = false;
        // ====================================================================

        frame(w, h);

        // ====================================================================
    }

    wait_present();
    rt_unload_library();

    if (presentEvent) CloseHandle(presentEvent);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
