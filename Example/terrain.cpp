#include "../OpenGL.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void frame(int width, int height);

int main()
{
    // 初始化 SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    // 设置 OpenGL 属性（可选，使用默认值也可以）
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // 创建窗口
    SDL_Window* window = SDL_CreateWindow("OpenGL Demo", 1000, 600, SDL_WINDOW_OPENGL);

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    // 创建 OpenGL 上下文
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        fprintf(stderr, "GL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // 初始化 GLEW
    gl_hello_world();

    // 主循环
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        frame(w, h);

        // 交换缓冲
        SDL_GL_SwapWindow(window);
    }

    // 清理
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
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

        uniform mat4 projMat, viewMat, modelMat;

        void main()
        {
            vertex = vec3(modelMat * vec4(in_vertex, 1.0));
            normal = vec3(modelMat * vec4(in_normal, 0.0));
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

        void main()
        {
            final = vec4(1, 0.5, 0.3, 1.0);
        }
    )";
    static auto program = gl_create_program_graphics(VS, FS);
    static auto pass1_color = gl_create_texture_color(width, height, nullptr);
    static auto pass1_depth = gl_create_texture_depth(width, height, nullptr);
    static auto pass1 = gl_create_pipeline(width, height, pass1_color, pass1_depth);

    auto projMat = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
    auto viewMat = glm::lookAt(glm::vec3(0, 1.8, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    auto modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-2, 0, 0));

    gl_begin_render(pass1, {.program = program, .clear_color = true, .clear_depth = true, .depth_test = true, .depth_func = GL_LEQUAL, .fill_mode = GL_LINE, });

    gl_set_uniform_mat4("projMat", &projMat[0][0]);
    gl_set_uniform_mat4("viewMat", &viewMat[0][0]);
    gl_set_uniform_mat4("modelMat", &modelMat[0][0]);
    gl_set_viewport(0, 0, width, height);
    static auto plane = gl_create_mesh_plane(10, 4);
    gl_draw_mesh(plane);

    gl_end_render(pass1);

    gl_draw_screen(width, height, pass1_color);
}