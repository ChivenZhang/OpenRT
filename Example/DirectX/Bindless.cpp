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
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdio>
#include <vector>

using Microsoft::WRL::ComPtr;

static constexpr auto VS = R"(
    cbuffer Push : register(b16)
    {
        column_major float4x4 projView;
        float4 c0;
        float4 c1;
        float4 c2;
        float3 translation;
        uint albedo;
    };

    RWByteAddressBuffer indices : register(u2);
    RWByteAddressBuffer positions : register(u3);
    RWByteAddressBuffer normals : register(u4);
    RWByteAddressBuffer uvs : register(u5);

    struct VSOut
    {
        float4 position : SV_Position;
        float3 vertex : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };

    VSOut main(uint vid : SV_VertexID)
    {
        uint vertexId = indices.Load(vid * 4);
        float3 inVertex = asfloat(positions.Load3(vertexId * 12));
        float3 inNormal = asfloat(normals.Load3(vertexId * 12));
        float2 inUv = asfloat(uvs.Load2(vertexId * 8));
        float4x4 meshMat = float4x4(
            float4(c0.x, c1.x, c2.x, translation.x),
            float4(c0.y, c1.y, c2.y, translation.y),
            float4(c0.z, c1.z, c2.z, translation.z),
            float4(c0.w, c1.w, c2.w, 1.0));
        VSOut output;
        output.vertex = mul(meshMat, float4(inVertex, 1)).xyz;
        output.normal = mul(meshMat, float4(inNormal, 0)).xyz;
        output.uv = inUv;
        output.position = mul(projView, float4(output.vertex, 1));
        return output;
    }
)";

static constexpr auto FS = R"(
    cbuffer BindlessTextures : register(b0)
    {
        uint4 handles;
    };
    Texture2D bindlessTextures[16] : register(t0, space1);
    SamplerState samp : register(s1);

    cbuffer Push : register(b16)
    {
        column_major float4x4 projView;
        float4 c0;
        float4 c1;
        float4 c2;
        float3 translation;
        uint albedo;
    };

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
    static const float3 CAMERA_POSITION = float3(0.0, 2.0, 8.0);

    float4 main(PSIn input) : SV_Target0
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(LIGHT_POSITION - input.vertex);
        float3 V = normalize(CAMERA_POSITION - input.vertex);
        float3 H = normalize(L + V);

        float3 ambient = AMBIENT_COLOR;
        float diff = max(dot(N, L), 0.0);
        uint index = handles[0];
        if (albedo == 1) index = handles[1];
        else if (albedo == 2) index = handles[2];
        else if (albedo == 3) index = handles[3];
        float3 sampled = bindlessTextures[index].Sample(samp, input.uv).rgb;
        float3 diffuse = diff * DIFFUSE_COLOR * sampled;
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

#ifndef D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES
#define D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES (1u << 20)
#endif

static std::vector<uint8_t> compile_shader(const char* source, const char* target)
{
    ComPtr<ID3DBlob> code, error;
    UINT flags = D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
    HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, "main", target, flags, 0, code.GetAddressOf(), error.GetAddressOf());
    if (FAILED(hr))
    {
        fprintf(stderr, "[DirectX][ERROR] shader compile failed:\n%s", error ? (const char*)error->GetBufferPointer() : "");
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
    wait_present();
    static auto vs = compile_shader(VS, "vs_5_1");
    static auto fs = compile_shader(FS, "ps_5_1");
        static auto layout = rt_create_layout({
        .entry = {
            {.binding = 0, .type = RT_BINDING_UNIFORM_BUFFER},
            {.binding = 1, .type = RT_BINDING_SAMPLER},
            {.binding = 2, .stages = RT_STAGE_VERTEX, .type = RT_BINDING_STORAGE_BUFFER},
            {.binding = 3, .stages = RT_STAGE_VERTEX, .type = RT_BINDING_STORAGE_BUFFER},
            {.binding = 4, .stages = RT_STAGE_VERTEX, .type = RT_BINDING_STORAGE_BUFFER},
            {.binding = 5, .stages = RT_STAGE_VERTEX, .type = RT_BINDING_STORAGE_BUFFER},
        },
    });
    static auto module = rt_create_module_render({
        .layout = layout,
        .vshader = {.code = (const char*)vs.data(), .size = (uint32_t)vs.size()},
        .fshader = {.code = (const char*)fs.data(), .size = (uint32_t)fs.size()},
        .colors = {{.format = RT_TEXTURE_RGBA8UNORM,}},
        .depth = {.write = true, .func = RT_LEQUAL,},
    });
    static auto pass_color = rt_create_texture_color(width, height);
    static auto pass_depth = rt_create_texture_depth(width, height);
    static const float cubePositions[24 * 3] = {
        -0.7f, -0.7f,  0.7f,  0.7f, -0.7f,  0.7f,  0.7f,  0.7f,  0.7f, -0.7f,  0.7f,  0.7f,
         0.7f, -0.7f, -0.7f, -0.7f, -0.7f, -0.7f, -0.7f,  0.7f, -0.7f,  0.7f,  0.7f, -0.7f,
        -0.7f,  0.7f,  0.7f,  0.7f,  0.7f,  0.7f,  0.7f,  0.7f, -0.7f, -0.7f,  0.7f, -0.7f,
        -0.7f, -0.7f, -0.7f,  0.7f, -0.7f, -0.7f,  0.7f, -0.7f,  0.7f, -0.7f, -0.7f,  0.7f,
         0.7f, -0.7f,  0.7f,  0.7f, -0.7f, -0.7f,  0.7f,  0.7f, -0.7f,  0.7f,  0.7f,  0.7f,
        -0.7f, -0.7f, -0.7f, -0.7f, -0.7f,  0.7f, -0.7f,  0.7f,  0.7f, -0.7f,  0.7f, -0.7f,
    };
    static const float cubeNormals[24 * 3] = {
        0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
        0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
        0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
        0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,
        1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
        -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
    };
    static const float cubeUvs[24 * 2] = {
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
    };
    static const uint32_t cubeIndices[36] = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };
    static auto positions = rt_create_buffer({.size = sizeof(cubePositions), .usages = RT_BUFFER_USAGE_STORAGE});
    static auto normals = rt_create_buffer({.size = sizeof(cubeNormals), .usages = RT_BUFFER_USAGE_STORAGE});
    static auto uvs = rt_create_buffer({.size = sizeof(cubeUvs), .usages = RT_BUFFER_USAGE_STORAGE});
    static auto indices = rt_create_buffer({.size = sizeof(cubeIndices), .usages = RT_BUFFER_USAGE_STORAGE});
    static bool geometryReady = false;
    if (!geometryReady)
    {
        auto upload = [](const void* data, size_t size, rt_buffer_t& buffer)
        {
            rt_buffer_data_t source = {};
            source.data = (const uint8_t*)data;
            source.size = size;
            rt_buffer_copy_t destination = {.buffer = buffer};
            rt_copy_buffer_data(source, destination, size);
        };
        rt_pass_transfer_t transfer = {};
        rt_begin_transfer(transfer);
        upload(cubePositions, sizeof(cubePositions), positions);
        upload(cubeNormals, sizeof(cubeNormals), normals);
        upload(cubeUvs, sizeof(cubeUvs), uvs);
        upload(cubeIndices, sizeof(cubeIndices), indices);
        rt_end_transfer(transfer);
        rt_submit();
        geometryReady = true;
    }
    struct DrawArrayIndirect
    {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };
    static const DrawArrayIndirect draw = {36, 1, 0, 0};
    static auto indirect = rt_create_buffer({.size = sizeof(draw), .usages = RT_BUFFER_USAGE_INDIRECT, .data = &draw});
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
        .usages = RT_BUFFER_USAGE_UNIFORM | RT_BUFFER_USAGE_MAP_WRITE,
    });
    static auto samp = rt_create_sampler({.min_filter = RT_LINEAR, .mag_filter = RT_LINEAR, .address_u = RT_REPEAT, .address_v = RT_REPEAT,});
    static auto binding = rt_create_binding({
        .layout = layout,
        .entry = {
            {.binding = 0, .buffer = table},
            {.binding = 1, .sampler = samp},
            {.binding = 2, .buffer = indices},
            {.binding = 3, .buffer = positions},
            {.binding = 4, .buffer = normals},
            {.binding = 5, .buffer = uvs},
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
            rt_draw_array_indirect(nullptr, 0, indirect, 0, 1, 0);
        }

        rt_end_render(pass);
    }

    rt_submit();
    present(pass_color);
}

int main()
{
    int w = 1000, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    auto window = SDL_CreateWindow("DirectX-Bindless", w, h, 0);
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!hwnd)
    {
        fprintf(stderr, "[DirectX][ERROR] SDL window has no HWND");
        return 1;
    }

    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        debug->EnableDebugLayer();

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))))
    {
        fprintf(stderr, "[DirectX][ERROR] CreateDXGIFactory2 failed");
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
        fprintf(stderr, "[DirectX][ERROR] D3D12CreateDevice failed");
        return 1;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue))))
    {
        fprintf(stderr, "[DirectX][ERROR] CreateCommandQueue failed");
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
        fprintf(stderr, "[DirectX][ERROR] CreateSwapChainForHwnd failed");
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

    rt_load_library({.backend = RT_DIRECTX, .directx = {.device = device.Get(), .queue = queue.Get()}});

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
