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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../OpenGL.h"

static void present(int width, int height, rt_texture_t& color)
{
    gl_draw_screen(width, height, {}, color);
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

void frame(int width, int height)
{
    constexpr auto VS = R"(
        #version 460
        #extension GL_NV_gpu_shader5 : require
        #extension GL_NV_shader_buffer_load : require
        #define buffer(HANDLE) packUint2x32(HANDLE)
        out vec3 vertex;
        out vec3 normal;
        out vec2 uv;

        layout(std430, binding = 0) readonly buffer Bindless
        {
            uvec2 textures[4];
            uvec2 positions;
            uvec2 normals;
            uvec2 uvs;
            uvec2 indices;
        };

        uniform mat4 projMat;
        uniform mat4 viewMat;
        uniform mat4 meshMat;

        void main()
        {
            uint vertexId = ((uint*)buffer(indices))[uint(gl_VertexID)];
            float* positionData = (float*)buffer(positions);
            float* normalData = (float*)buffer(normals);
            float* uvData = (float*)buffer(uvs);
            vec3 inVertex = vec3(positionData[vertexId * 3 + 0], positionData[vertexId * 3 + 1], positionData[vertexId * 3 + 2]);
            vec3 inNormal = vec3(normalData[vertexId * 3 + 0], normalData[vertexId * 3 + 1], normalData[vertexId * 3 + 2]);
            vec2 inUv = vec2(uvData[vertexId * 2 + 0], uvData[vertexId * 2 + 1]);
            vertex = vec3(meshMat * vec4(inVertex, 1));
            normal = vec3(meshMat * vec4(inNormal, 0));
            uv = inUv;
            gl_Position = projMat * viewMat * vec4(vertex, 1.0);
        }
    )";
    constexpr auto FS = R"(
        #version 460
        #extension GL_ARB_bindless_texture : require
        in vec3 vertex;
        in vec3 normal;
        in vec2 uv;
        out vec4 final;

        layout(std430, binding = 0) readonly buffer Bindless
        {
            uvec2 textures[4];
            uvec2 positions;
            uvec2 normals;
            uvec2 uvs;
            uvec2 indices;
        };

        uniform uint albedo;

        const vec3 LIGHT_POSITION = vec3(5.0, 5.0, 5.0);
        const vec3 LIGHT_COLOR = vec3(1.0, 0.98, 0.94);
        const float LIGHT_INTENSITY = 1.5;

        const vec3 AMBIENT_COLOR = vec3(0.1, 0.1, 0.1);
        const vec3 DIFFUSE_COLOR = vec3(1.0, 1.0, 1.0);
        const vec3 SPECULAR_COLOR = vec3(0.5, 0.5, 0.5);
        const float SHININESS = 32.0;

        const vec3 CAMERA_POSITION = vec3(0.0, 2.0, 8.0);

        void main()
        {
            vec3 N = normalize(normal);
            vec3 L = normalize(LIGHT_POSITION - vertex);
            vec3 V = normalize(CAMERA_POSITION - vertex);
            vec3 H = normalize(L + V);

            vec3 ambient = AMBIENT_COLOR;
            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = diff * DIFFUSE_COLOR * texture(sampler2D(textures[albedo]), uv).rgb;
            float spec = pow(max(dot(N, H), 0.0), SHININESS);
            vec3 specular = spec * SPECULAR_COLOR;

            final = vec4(ambient + LIGHT_INTENSITY * LIGHT_COLOR * (diffuse + specular), 1.0);
        }
    )";

    static auto layout = rt_create_layout({
        .entry = {{.binding = 0, .type = RT_BINDING_STORAGE_BUFFER}},
    });
    static auto module = rt_create_module_render({
        .layout = layout,
        .vshader = {VS}, .fshader = {FS},
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
    static auto positions = rt_create_buffer({.size = sizeof(cubePositions), .usages = RT_BUFFER_USAGE_BINDLESS, .data = cubePositions});
    static auto normals = rt_create_buffer({.size = sizeof(cubeNormals), .usages = RT_BUFFER_USAGE_BINDLESS, .data = cubeNormals});
    static auto uvs = rt_create_buffer({.size = sizeof(cubeUvs), .usages = RT_BUFFER_USAGE_BINDLESS, .data = cubeUvs});
    static auto indices = rt_create_buffer({.size = sizeof(cubeIndices), .usages = RT_BUFFER_USAGE_BINDLESS, .data = cubeIndices});
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
        .size = (size_t)limits.max_bindless_handle_size * 8,
        .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_MAP_WRITE,
    });
    static auto binding = rt_create_binding({
        .layout = layout,
        .entry = {{.binding = 0, .buffer = table}},
    });
    if (auto* memory = rt_map_buffer(table, RT_WRITE_ONLY, 0, table.size))
    {
        if (limits.max_bindless_handle_size == sizeof(uint32_t))
        {
            auto* handles = (uint32_t*)memory;
            for (int i = 0; i < 4; ++i)
                handles[i] = (uint32_t)textures[i].default_view.bindless;
        }
        else if (limits.max_bindless_handle_size == sizeof(uint64_t))
        {
            auto* handles = (uint64_t*)memory;
            for (int i = 0; i < 4; ++i)
                handles[i] = textures[i].default_view.bindless;
            handles[4] = positions.bindless;
            handles[5] = normals.bindless;
            handles[6] = uvs.bindless;
            handles[7] = indices.bindless;
        }
        rt_unmap_buffer(table);
    }

    rt_pass_render_t pass = {.colors = {{.texture = pass_color.default_view, .clear = true,}}, .depth = {.texture = pass_depth.default_view, .clear = true,},};
    rt_begin_render(pass);
    rt_bind_module_render(module);
    rt_bind_binding(binding);

    auto projMat = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    auto viewMat = glm::lookAt(glm::vec3(0, 2, 8), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    rt_push_const_mat4("projMat", &projMat[0][0]);
    rt_push_const_mat4("viewMat", &viewMat[0][0]);

    float spin = (float)SDL_GetTicks() / 2000.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto meshMat = glm::translate(glm::mat4(1), places[i]);
        meshMat = glm::rotate(meshMat, spin + i * 0.4f, glm::vec3(0, 1, 0));
        meshMat = glm::rotate(meshMat, glm::radians(-18.0f), glm::vec3(1, 0, 0));
        rt_push_const_mat4("meshMat", &meshMat[0][0]);
        rt_push_const_uint("albedo", (uint32_t)i);
        rt_draw_array(nullptr, 0, 36, 1, 0, 0);
    }

    rt_end_render(pass);
    present(width, height, pass_color);
    rt_submit();
}

int main()
{
    int w = 1000, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    auto window = SDL_CreateWindow("OpenGL-Bindless", w, h, SDL_WINDOW_OPENGL);
    auto context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    rt_load_library();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) running = false;

        frame(w, h);

        SDL_GL_SwapWindow(window);
    }

    rt_unload_library();

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
