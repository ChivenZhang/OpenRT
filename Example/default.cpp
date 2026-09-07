#define OPENGL_IMPLEMENTATION
#define OPENGLX_IMPLEMENTATION
#include "../OpenGLX.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void frame(int width, int height);

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("OpenGL Demo", 1000, 600, SDL_WINDOW_OPENGL);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        fprintf(stderr, "GL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    gl_hello_world();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) running = false;

        int w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        frame(w, h);

        SDL_GL_SwapWindow(window);
    }

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
        layout(max_vertices=64, max_primitives=126) out;
        layout(triangles) out;

        out PerVertexData
        {
            vec3 vertex;
            vec3 normal;
            vec2 uv;
        }  ms_out[];

        uniform mat4 projMat, viewMat, modelMat;

        layout(std430, binding = 0) buffer Vertices
        {
            vec3 vertices[];
        };

        layout(std430, binding = 1) buffer Normals
        {
            vec3 normals[];
        };

        layout(std430, binding = 2) buffer UVs
        {
            vec2 uvs[];
        };

        layout(std430, binding = 3) buffer Indices
        {
            uint indices[];
        };

        void main()
        {
            uint meshlet_id = gl_WorkGroupID.x;

            uint thread_id = gl_LocalInvocationID.x;

            uint index = indices[meshlet_id * 3 + thread_id];

            ms_out[thread_id].vertex = vec3(modelMat * vec4(vertices[index], 1));
            ms_out[thread_id].normal = vec3(modelMat * vec4(normals[index], 0));
            ms_out[thread_id].uv = uvs[index];

            gl_MeshVerticesNV[thread_id].gl_Position = projMat * viewMat * modelMat * vec4(vertices[index], 1);

            gl_PrimitiveIndicesNV[thread_id] = thread_id;

            gl_PrimitiveCountNV = 1;
        }
    )";
    constexpr auto FS = R"(
        #version 460

        in PerVertexData
        {
            vec3 vertex;
            vec3 normal;
            vec2 uv;
        }  fs_in;
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
            vec3 N = normalize(fs_in.normal);

            // 计算光照方向
            vec3 L = normalize(LIGHT_POSITION - fs_in.vertex);

            // 计算观察方向
            vec3 V = normalize(CAMERA_POSITION - fs_in.vertex);

            // 计算半程向量（Blinn-Phong 的核心）
            vec3 H = normalize(L + V);

            // ===== 环境光 =====
            vec3 ambient = AMBIENT_COLOR;

            // ===== 漫反射 =====
            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = diff * DIFFUSE_COLOR * texture(texture0, fs_in.uv).rgb;

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
    auto modelMat = glm::rotate(glm::mat4(1.0f), (float)SDL_GetTicks() / 2000.0f, glm::vec3(0, 1, 0));

    static auto module = gl_create_module_meshlet(nullptr, MS, FS);
    static auto pass1_color = gl_create_texture_color(width, height, nullptr);
    static auto pass1_depth = gl_create_texture_depth(width, height, nullptr);

    gl_pass_t pass1 = {.module = module, .color = {{.texture = pass1_color, .clear = true, }}, .depth = {.texture = pass1_depth, .clear = true, .write = true, .func = GL_LEQUAL, }, };
    gl_begin_meshlet(pass1);

    gl_set_uniform_mat4("projMat", &projMat[0][0]);
    gl_set_uniform_mat4("viewMat", &viewMat[0][0]);
    gl_set_uniform_mat4("modelMat", &modelMat[0][0]);

    static auto meshlet = gl_create_meshlet_sphere(2, 64, 32);
    gl_bind_buffer(meshlet.vertex_vbo, {.binding = 0, .target = GL_SHADER_STORAGE_BUFFER,});
    gl_bind_buffer(meshlet.normal_vbo, {.binding = 1, .target = GL_SHADER_STORAGE_BUFFER,});
    gl_bind_buffer(meshlet.uv_vbo, {.binding = 2, .target = GL_SHADER_STORAGE_BUFFER,});
    gl_bind_buffer(meshlet.index_vbo, {.binding = 3, .target = GL_SHADER_STORAGE_BUFFER,});

    static auto texture0 = gl_load_texture("../../Earth.png");
    gl_bind_texture(texture0, {.binding = 0,});

    gl_set_viewport(0, 0, width, height);
    gl_draw_mesh_task(meshlet.index_count / 3, 1, 1);

    gl_end_meshlet(pass1);

    gl_draw_screen(width, height, pass1_color);
}