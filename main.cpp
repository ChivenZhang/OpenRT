#include "OpenGL.h"
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

        uniform mat4 mvp;
        uniform mat4 modelMat;

        void main()
        {
            vertex = vec3(modelMat * vec4(in_vertex, 1.0));
            normal = vec3(modelMat * vec4(in_normal, 0.0));
            uv = in_uv;
            gl_Position = mvp * vec4(in_vertex, 1.0);
        }
    )";
    constexpr auto FS = R"(
        #version 460
        in vec3 vertex;
        in vec3 normal;
        in vec2 uv;

        out vec4 final;

        layout(binding = 0) uniform sampler2D texture0;

        // 光照参数
        uniform vec3 lightPosition;    // 光源位置
        uniform vec3 lightColor;       // 光源颜色
        uniform vec3 cameraPosition;   // 相机位置
        uniform vec3 ambientColor;     // 环境光颜色
        uniform float ambientStrength; // 环境光强度

        // 材质参数
        uniform vec3 materialColor;    // 材质颜色
        uniform float shininess;       // 高光强度（镜面反射系数）

        void main()
        {
            // 归一化法线
            vec3 N = normalize(normal);

            // 光照方向（从片段到光源）
            vec3 L = normalize(lightPosition - vertex);

            // 视线方向（从片段到相机）
            vec3 V = normalize(cameraPosition - vertex);

            // 半程向量（Blinn-Phong 核心）
            vec3 H = normalize(L + V);

            // 环境光
            vec3 ambient = ambientStrength * ambientColor;

            // 漫反射
            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = diff * lightColor;

            // 镜面反射（Blinn-Phong）
            float spec = pow(max(dot(N, H), 0.0), shininess);
            vec3 specular = spec * lightColor;

            // 从纹理采样颜色
            vec3 textureColor = vec3(1,1,1);// texture(texture0, uv).rgb;

            // 组合最终颜色
            vec3 result = (ambient + diffuse + specular) * materialColor * textureColor;

            // 输出
            final = vec4(result, 1.0);
        }
    )";
    static auto program = gl_create_program_graphics(VS, FS);
    static auto pass1_color = gl_create_texture_color(width, height, nullptr);
    static auto pass1_depth = gl_create_texture_depth(width, height, nullptr);
    static auto pass1 = gl_create_pipeline(width, height, pass1_color, pass1_depth);
    gl_begin_render(pass1, {.program = program, .clear_color = true, .clear_depth = true, .depth_test = true, .depth_func = GL_LEQUAL, });

    auto project = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
    auto view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    auto model = glm::mat4(1.0f);
    auto mvp = project * view * model;
    gl_set_uniform_mat4("mvp", &mvp[0][0]);
    gl_set_uniform_mat4("modelMat", &model[0][0]);
    gl_set_uniform_vec3("lightPosition", &glm::vec3(10.0f, 10.0f, 10.0f)[0]);
    gl_set_uniform_vec3("lightColor", &glm::vec3(1.0f, 1.0f, 1.0f)[0]);
    gl_set_uniform_vec3("cameraPosition", &glm::vec3(0.0f, 0.0f, 5.0f)[0]);
    gl_set_uniform_vec3("ambientColor", &glm::vec3(0.2f, 0.2f, 0.2f)[0]);
    gl_set_uniform_float("ambientStrength", 0.1f);
    gl_set_uniform_vec3("materialColor", &glm::vec3(1.0f, 0.5f, 0.3f)[0]);
    gl_set_uniform_float("shininess", 32.0f);
    gl_set_viewport(0, 0, width, height);
    gl_set_scissor(width * 0.5-10, height * 0.5-10, 20, 20);

    static auto sphere = gl_create_mesh_sphere(1, 16, 32);
    gl_draw_mesh(sphere);
    gl_end_render(pass1);

    gl_draw_screen(width, height, pass1_color);
}