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
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../OpenGL.h"

static constexpr uint32_t INSTANCE_COUNT = 100;
static constexpr uint32_t TEXTURE_COUNT = 4;

struct Instance
{
    glm::mat4 mesh;
    uint32_t albedo;
    uint32_t pad[3] = {};
};

static rt_vertex_t rt_vertex_instance{
    .stride = sizeof(Instance),
    .instance = true,
    .attrib = {
        {.location = 3, .offset = 0, .format = RT_VERTEX_FLOAT32X4},
        {.location = 4, .offset = 16, .format = RT_VERTEX_FLOAT32X4},
        {.location = 5, .offset = 32, .format = RT_VERTEX_FLOAT32X4},
        {.location = 6, .offset = 48, .format = RT_VERTEX_FLOAT32X4},
        {.location = 7, .offset = offsetof(Instance, albedo), .format = RT_VERTEX_UINT32},
    },
};

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
        layout(location = 0) in vec3 in_vertex;
        layout(location = 1) in vec3 in_normal;
        layout(location = 2) in vec2 in_uv;
        layout(location = 3) in vec4 in_mesh0;
        layout(location = 4) in vec4 in_mesh1;
        layout(location = 5) in vec4 in_mesh2;
        layout(location = 6) in vec4 in_mesh3;
        layout(location = 7) in uint in_albedo;
        out vec3 vertex;
        out vec3 normal;
        out vec2 uv;
        flat out uint albedo;

        uniform mat4 projMat;
        uniform mat4 viewMat;

        void main()
        {
            mat4 meshMat = mat4(in_mesh0, in_mesh1, in_mesh2, in_mesh3);
            vertex = vec3(meshMat * vec4(in_vertex, 1));
            normal = vec3(meshMat * vec4(in_normal, 0));
            uv = in_uv;
            albedo = in_albedo;
            gl_Position = projMat * viewMat * vec4(vertex, 1.0);
        }
    )";
    constexpr auto FS = R"(
        #version 460
        #extension GL_ARB_bindless_texture : require
        in vec3 vertex;
        in vec3 normal;
        in vec2 uv;
        flat in uint albedo;
        out vec4 final;

        layout(std430, binding = 0) readonly buffer BindlessTextures
        {
            sampler2D handles[];
        };

        const vec3 LIGHT_POSITION = vec3(5.0, 12.0, 8.0);
        const vec3 LIGHT_COLOR = vec3(1.0, 0.98, 0.94);
        const float LIGHT_INTENSITY = 1.5;

        const vec3 AMBIENT_COLOR = vec3(0.1, 0.1, 0.1);
        const vec3 DIFFUSE_COLOR = vec3(1.0, 1.0, 1.0);
        const vec3 SPECULAR_COLOR = vec3(0.5, 0.5, 0.5);
        const float SHININESS = 32.0;

        const vec3 CAMERA_POSITION = vec3(0.0, 14.0, 22.0);

        void main()
        {
            vec3 N = normalize(normal);
            vec3 L = normalize(LIGHT_POSITION - vertex);
            vec3 V = normalize(CAMERA_POSITION - vertex);
            vec3 H = normalize(L + V);

            vec3 ambient = AMBIENT_COLOR;
            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = diff * DIFFUSE_COLOR * texture(handles[albedo], uv).rgb;
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
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv, rt_vertex_instance,},
    });
    static auto pass_color = rt_create_texture_color(width, height);
    static auto pass_depth = rt_create_texture_depth(width, height);
    static auto cube = rt_create_mesh_cube(1.2f, 1.2f, 1.2f);
    static const char* files[] = {
        "../../Example/OpenGL/Default.png",
        "../../Example/OpenGL/Example.png",
        "../../Example/OpenGL/Meshlet.png",
        "../../Example/OpenGL/Render.png",
    };
    static rt_texture_t textures[TEXTURE_COUNT] = {
        load_bindless(files[0]),
        load_bindless(files[1]),
        load_bindless(files[2]),
        load_bindless(files[3]),
    };
    static auto limits = rt_get_limits();
    static auto table = rt_create_buffer({
        .size = (size_t)limits.max_bindless_handle_size * TEXTURE_COUNT,
        .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_MAP_WRITE,
    });
    static auto binding = rt_create_binding({
        .layout = layout,
        .entry = {{.binding = 0, .buffer = table}},
    });
    static auto instances = rt_create_buffer({
        .size = sizeof(Instance) * INSTANCE_COUNT,
        .usages = RT_BUFFER_USAGE_VERTEX | RT_BUFFER_USAGE_MAP_WRITE,
    });
    if (auto* memory = rt_map_buffer(table, RT_WRITE_ONLY, 0, table.size))
    {
        if (limits.max_bindless_handle_size == sizeof(uint32_t))
        {
            auto* handles = (uint32_t*)memory;
            for (uint32_t i = 0; i < TEXTURE_COUNT; ++i)
                handles[i] = (uint32_t)textures[i].default_view.bindless;
        }
        else if (limits.max_bindless_handle_size == sizeof(uint64_t))
        {
            auto* handles = (uint64_t*)memory;
            for (uint32_t i = 0; i < TEXTURE_COUNT; ++i)
                handles[i] = textures[i].default_view.bindless;
        }
        rt_unmap_buffer(table);
    }
    if (auto* memory = (Instance*)rt_map_buffer(instances, RT_WRITE_ONLY, 0, instances.size))
    {
        float spin = (float)SDL_GetTicks() / 2000.0f;
        for (uint32_t i = 0; i < INSTANCE_COUNT; ++i)
        {
            uint32_t col = i % 10;
            uint32_t row = i / 10;
            glm::vec3 place((float)col - 4.5f, 0.0f, (float)row - 4.5f);
            place *= 2.4f;
            glm::mat4 mesh = glm::translate(glm::mat4(1), place);
            mesh = glm::rotate(mesh, spin + i * 0.15f, glm::vec3(0, 1, 0));
            mesh = glm::rotate(mesh, glm::radians(-18.0f), glm::vec3(1, 0, 0));
            memory[i].mesh = mesh;
            memory[i].albedo = i % TEXTURE_COUNT;
        }
        rt_unmap_buffer(instances);
    }

    rt_pass_render_t pass = {.colors = {{.texture = pass_color.default_view, .clear = true,}}, .depth = {.texture = pass_depth.default_view, .clear = true,},};
    rt_begin_render(pass);
    rt_bind_module_render(module);
    rt_bind_binding(binding);

    auto projMat = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 200.0f);
    auto viewMat = glm::lookAt(glm::vec3(0, 14, 22), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    rt_push_const_mat4("projMat", &projMat[0][0]);
    rt_push_const_mat4("viewMat", &viewMat[0][0]);

    rt_buffer_view_t vertex[] = {
        {.buffer = cube.vertex[0]},
        {.buffer = cube.vertex[1]},
        {.buffer = cube.vertex[2]},
        {.buffer = instances},
    };
    rt_buffer_view_t index = {.buffer = cube.index};
    rt_draw_index(vertex, 4, index, cube.index.size / sizeof(uint32_t), INSTANCE_COUNT, 0, 0, 0);

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
    auto window = SDL_CreateWindow("OpenGL-Instance", w, h, SDL_WINDOW_OPENGL);
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
