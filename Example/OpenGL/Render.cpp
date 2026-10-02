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

void frame(int width, int height)
{
    constexpr auto VS = R"(
        #version 460
        layout(location = 0) in vec3 in_vertex;
        layout(location = 1) in vec3 in_normal;
        layout(location = 2) in vec2 in_uv;
        out vec3 vertex;
        out vec3 normal;
        out vec2 uv;

        uniform mat4 projMat;
        uniform mat4 viewMat;
        uniform mat4 meshMat;

        void main()
        {
            vertex = vec3(meshMat * vec4(in_vertex, 1));
            normal = vec3(meshMat * vec4(in_normal, 0));
            uv = in_uv;
            gl_Position = projMat * viewMat * vec4(vertex, 1.0);
        }
    )";
    constexpr auto FS = R"(
        #version 460
        in vec3 vertex;
        in vec3 normal;
        in vec2 uv;
        out vec4 final;

        layout(binding = 0) uniform sampler2D texture0;

        const vec3 LIGHT_POSITION = vec3(5.0, 5.0, 5.0);
        const vec3 LIGHT_COLOR = vec3(1.0, 0.98, 0.94);
        const float LIGHT_INTENSITY = 1.5;

        const vec3 AMBIENT_COLOR = vec3(0.1, 0.1, 0.1);
        const vec3 DIFFUSE_COLOR = vec3(1.0, 1.0, 1.0);
        const vec3 SPECULAR_COLOR = vec3(0.5, 0.5, 0.5);
        const float SHININESS = 32.0;

        const vec3 CAMERA_POSITION = vec3(0.0, 0.0, 10.0);

        void main()
        {
            vec3 N = normalize(normal);

            vec3 L = normalize(LIGHT_POSITION - vertex);

            vec3 V = normalize(CAMERA_POSITION - vertex);

            vec3 H = normalize(L + V);

            vec3 ambient = AMBIENT_COLOR;

            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = diff * DIFFUSE_COLOR * texture(texture0, uv).rgb;

            float spec = pow(max(dot(N, H), 0.0), SHININESS);
            vec3 specular = spec * SPECULAR_COLOR;

            vec3 result = ambient + LIGHT_INTENSITY * LIGHT_COLOR * (diffuse + specular);

            final = vec4(result, 1.0);
        }
    )";

    static auto module = rt_create_module_render({
        .vshader = {VS}, .fshader = {FS},
        .colors = {{.format = RT_TEXTURE_RGBA8UNORM,}},
        .depth = {.write = true, .func = RT_LEQUAL,},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
        .binding = {{.binding = 0, .type = RT_BINDING_TEXTURE,}, {.binding = 1, .type = RT_BINDING_SAMPLER,},},
    });
    static auto pass_color = rt_create_texture_color(width, height);
    static auto pass_depth = rt_create_texture_depth(width, height);
    {
        rt_pass_render_t pass = {.module = module, .colors = {{.texture_view = pass_color.default_view, .clear = true,}}, .depth = {.texture_view = pass_depth.default_view, .clear = true,},};
        rt_begin_render(pass);

        auto projMat = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
        auto viewMat = glm::lookAt(glm::vec3(0, 2, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        auto meshMat = glm::rotate(glm::rotate(glm::mat4(1), glm::radians(-23.5f), glm::vec3(0, 0, 1)), (float)SDL_GetTicks() / 2000.0f, glm::vec3(0, 1, 0));

        rt_push_const_mat4("projMat", &projMat[0][0]);
        rt_push_const_mat4("viewMat", &viewMat[0][0]);
        rt_push_const_mat4("meshMat", &meshMat[0][0]);

        static auto texture0 = rt_load_texture_file("../../Earth.png", true);
        static auto sampler0 = rt_create_sampler({.min_filter = RT_LINEAR, .mag_filter = RT_LINEAR, .address_u = RT_REPEAT, .address_v = RT_REPEAT,});
        rt_bind_texture(texture0, {.binding = 0,});
        rt_bind_sampler(sampler0, {.binding = 0,});

        static auto mesh = rt_create_mesh_sphere(2, 64, 32);
        // rt_draw_mesh(mesh);
        rt_draw_index(mesh.vertex, 3, mesh.index, mesh.index.size / sizeof(uint32_t), 1);

        rt_end_render(pass);
    }

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
    auto window = SDL_CreateWindow("OpenGL-Render", w, h, SDL_WINDOW_OPENGL);
    auto context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    rt_load_library();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) running = false;
        // ====================================================================

        frame(w, h);

        // ====================================================================
        SDL_GL_SwapWindow(window);
    }

    rt_unload_library();

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}