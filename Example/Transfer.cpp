#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define OPENRTX_IMPLEMENTATION
#include "../OpenRTX.h"

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

        void main()
        {
            vertex = in_vertex;
            normal = in_normal;
            uv = in_uv;
            gl_Position = vec4(in_vertex, 1.0);
        }
    )";
    constexpr auto FS = R"(
        #version 460
        in vec3 vertex;
        in vec3 normal;
        in vec2 uv;
        out vec4 final;

        layout(binding = 0) uniform sampler2D texture0;

        void main()
        {
            final = texture(texture0, uv);
        }
    )";

    static auto module = rt_create_module_render({.vshader = VS, .fshader = FS, .depth = {.write = true, .func = GL_LEQUAL,}, .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,}, });

    static auto texture0 = []()
    {
        auto texture = rt_load_texture_file("../../Earth.png");

        // Copy Texture To Buffer

        auto buffer = rt_create_buffer({.size = texture.width * texture.height * 3,});
        {
            rt_pass_transfer_t pass = {};
            rt_begin_transfer(pass);
            rt_copy_buffer_texture(
                {.texture = texture, },
                {.buffer = buffer, .bytesPerRow = texture.width * 3, .rowsPerImage = texture.height,},
                {texture.width, texture.height, 1});
            rt_end_transfer(pass);
        }
        rt_destroy_texture(texture);

        // Copy Buffer To Texture

        texture = rt_create_texture({.width = texture.width, .height = texture.height, .format = GL_RGB, .internal_format = GL_RGB8, });
        {
            rt_pass_transfer_t pass = {};
            rt_begin_transfer(pass);
            rt_copy_texture_buffer(
                {.buffer = buffer, .bytesPerRow = texture.width * 3, .rowsPerImage = texture.height,},
                {.texture = texture,},
                {texture.width, texture.height, 1});
            rt_end_transfer(pass);
        }
        rt_destroy_buffer(buffer);

        return texture;
    }();

    static auto pass_color = rt_create_texture_color(width, height, nullptr);
    {
        rt_pass_render_t pass = {.module = module, .colors = {{.texture = pass_color, .clear = true,}},};
        rt_begin_render(pass);

        rt_bind_texture(texture0, {.binding = 0,});

        static auto mesh = rt_create_mesh_screen();
        rt_draw_mesh(mesh);

        rt_end_render(pass);
    }

    rt_draw_screen(width, height, {}, pass_color);

    rt_submit();
}

int main()
{
    int w = 1000, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    auto window = SDL_CreateWindow( "Transfer", w, h, SDL_WINDOW_OPENGL);
    auto context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

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