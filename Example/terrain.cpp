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

    SDL_GL_SetSwapInterval(0);

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
    constexpr auto MS = R"(
        #version 460
        #extension GL_NV_mesh_shader : require

        layout(local_size_x = 3) in;
        layout(max_vertices=3, max_primitives=1) out;
        layout(triangles) out;

        uniform mat4 projMat, viewMat, modelMat;

        layout(std430, binding = 0) buffer Vertices
        {
            vec3 vertices[];
        };

        layout(std430, binding = 1) buffer Indices
        {
            uint indices[];
        };

        void main()
        {
            uint meshlet_id = gl_WorkGroupID.x;

            uint thread_id = gl_LocalInvocationID.x;

            uint index = indices[meshlet_id * 3 + thread_id];

            vec3 point = vertices[index];

            gl_MeshVerticesNV[thread_id].gl_Position = projMat * viewMat * modelMat * vec4(point, 1);

            gl_PrimitiveIndicesNV[thread_id] = thread_id;

            gl_PrimitiveCountNV = 1;
        }
    )";
    constexpr auto FS = R"(
        #version 460
        out vec4 final;

        void main()
        {
            final = vec4(1,1,0,1);
        }
    )";
    static auto program = gl_create_program_meshlet(nullptr, MS, FS);
    static auto pass1_color = gl_create_texture_color(width, height, nullptr);
    static auto pass1_depth = gl_create_texture_depth(width, height, nullptr);
    static auto pass1 = gl_create_pipeline(width, height, pass1_color, pass1_depth);

    auto projMat = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
    auto viewMat = glm::lookAt(glm::vec3(0, 2, 4), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    auto modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));

    gl_begin_render(pass1, {.program = program, .clear_color = true, .clear_depth = true, .depth_test = true, .depth_func = GL_LEQUAL, .fill_mode = GL_LINE, });

    gl_set_uniform_mat4("projMat", &projMat[0][0]);
    gl_set_uniform_mat4("viewMat", &viewMat[0][0]);
    gl_set_uniform_mat4("modelMat", &modelMat[0][0]);
    gl_set_viewport(0, 0, width, height);
    static auto meshlet = gl_create_meshlet_plane(10, 4);

    gl_buffer_t vertices
    {
        .handle = meshlet.vertex_vbo,
        .size = meshlet.vertex_count * sizeof(glm::vec4),
    };
    gl_buffer_t indices
    {
        .handle = meshlet.index_vbo,
        .size = meshlet.index_count * sizeof(uint32_t),
    };
    gl_bind_buffer(vertices, {.binding = 0, .target = GL_SHADER_STORAGE_BUFFER,});
    gl_bind_buffer(indices, {.binding = 1, .target = GL_SHADER_STORAGE_BUFFER,});
    gl_draw_mesh_task(meshlet.index_count / 3, 1, 1);

    gl_end_render(pass1);

    gl_draw_screen(width, height, pass1_color);
}