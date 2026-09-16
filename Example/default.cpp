#define OPENGLX_IMPLEMENTATION
#include "../OpenGLX.h"
#include "../OpenGL.h"
#include <SDL3/SDL.h>

void frame(int width, int height);

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    auto window = SDL_CreateWindow( "OpenGL Demo", 1000, 600, SDL_WINDOW_OPENGL);
    auto context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

// ====================================================================

    gl_load_library();

    SDL_Event event;
    bool running = true;
    while (running)
    {
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) running = false;

        int w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        frame(w, h);

        SDL_GL_SwapWindow(window);
    }

// ====================================================================

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

        // 光源属性
        const vec3 LIGHT_POSITION = vec3(5.0, 5.0, 5.0);
        const vec3 LIGHT_COLOR = vec3(1.0, 0.98, 0.94);
        const float LIGHT_INTENSITY = 1.5;

        // 材质属性
        const vec3 AMBIENT_COLOR = vec3(0.1, 0.1, 0.1);
        const vec3 DIFFUSE_COLOR = vec3(1.0, 1.0, 1.0);
        const vec3 SPECULAR_COLOR = vec3(0.5, 0.5, 0.5);
        const float SHININESS = 32.0;

        // 相机位置（用于计算观察方向）
        const vec3 CAMERA_POSITION = vec3(0.0, 0.0, 10.0);

        void main()
        {
            // 归一化法线
            vec3 N = normalize(normal);

            // 计算光照方向
            vec3 L = normalize(LIGHT_POSITION - vertex);

            // 计算观察方向
            vec3 V = normalize(CAMERA_POSITION - vertex);

            // 计算半程向量（Blinn-Phong 的核心）
            vec3 H = normalize(L + V);

            // ===== 环境光 =====
            vec3 ambient = AMBIENT_COLOR;

            // ===== 漫反射 =====
            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = diff * DIFFUSE_COLOR * texture(texture0, uv).rgb;

            // ===== 镜面反射（Blinn-Phong）=====
            float spec = pow(max(dot(N, H), 0.0), SHININESS);
            vec3 specular = spec * SPECULAR_COLOR;

            // ===== 组合光照 =====
            vec3 result = ambient + LIGHT_INTENSITY * LIGHT_COLOR * (diffuse + specular);

            // 输出最终颜色
            final = vec4(result, 1.0);
        }
    )";

    auto projMat = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
    auto viewMat = glm::lookAt(glm::vec3(0, 2, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    auto meshMat = glm::rotate(glm::mat4(1.0f), (float)SDL_GetTicks() / 2000.0f, glm::vec3(0, 1, 0));

    static auto module = rhi_create_module_render(VS, FS, {
        .depth = {.write = true, .func = GL_LEQUAL,},
        .layout = {{.binding = 0, .type = GL_BINDING_TEXTURE, }},
        .vertex = {rhi_vertex_layout, rhi_normal_layout, rhi_uv_layout,},
    });
    static auto pass_color = rhi_create_texture_color(width, height, nullptr);
    static auto pass_depth = rhi_create_texture_depth(width, height, nullptr);
    {
        static auto texture0 = []()
        {
            auto texture = rhi_load_texture_file("../../Earth.png");

            rhi_pass_t pass = {};
            rhi_begin_transfer(pass);
            auto buffer = rhi_create_buffer({.size = texture.width * texture.height * 3,});
            rhi_copy_buffer_texture(
                {.texture = texture, },
                {.buffer = buffer, .bytesPerRow = texture.width * 3, .rowsPerImage = texture.height,},
                {texture.width, texture.height, 1});
            rhi_end_transfer(pass);

            rhi_destroy_texture(texture);
            texture = rhi_create_texture({.width = texture.width, .height = texture.height, .format = GL_RGB, .internal_format = GL_RGB8, });

            rhi_begin_transfer(pass);
            rhi_copy_texture_buffer(
                {.buffer = buffer, .bytesPerRow = texture.width * 3, .rowsPerImage = texture.height,},
                {.texture = texture,},
                {texture.width, texture.height, 1});
            rhi_end_transfer(pass);

            rhi_destroy_buffer(buffer);
            return texture;
        }();

        rhi_pass_t pass = {.module = module, .colors = {{.texture = pass_color, .clear = true,}}, .depth = {.texture = pass_depth, .clear = true,},};
        rhi_begin_render(pass);

        rhi_push_const_mat4("projMat", &projMat[0][0]);
        rhi_push_const_mat4("viewMat", &viewMat[0][0]);
        rhi_push_const_mat4("meshMat", &meshMat[0][0]);

        rhi_bind_texture(texture0, {.binding = 0,});

        static auto mesh = rhi_create_mesh_sphere(2, 64, 32);
        rhi_draw_mesh(mesh);

        rhi_end_render(pass);
    }

    rhi_draw_screen(width, height, pass_color, {});
}