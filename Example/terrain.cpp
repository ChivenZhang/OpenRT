#define OPENGL_IMPLEMENTATION
#define OPENGLX_IMPLEMENTATION
#include "../OpenGLX.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void frame(int width, int height);

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    auto window = SDL_CreateWindow( "Terrain Demo", 1000, 600, SDL_WINDOW_OPENGL);
    auto context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

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

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void frame(int width, int height)
{
    constexpr auto MS = R"(
        #version 460
        #extension GL_NV_mesh_shader : require

        layout(local_size_x = 1) in;
        layout(max_vertices=256, max_primitives=126) out;
        layout(triangles) out;

        out PerVertexData
        {
            vec3 vertex;
            vec3 normal;
            vec2 uv;
        }  ms_out[];

        layout(binding = 1) uniform sampler2D texture1;

        uniform float height;
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

        void tessellate(
            vec3 in_vertex[3], vec2 in_uv[3],
            out vec3 out_vertex[3], out vec2 out_uv[3])
        {
            for(uint i = 0; i < 3; ++i)
            {
                out_vertex[i] = 0.5 * (in_vertex[i] + in_vertex[(i+1) % 3]);
                out_uv[i] = 0.5 * (in_uv[i] + in_uv[(i+1) % 3]);
            }
        }

        // LOD 0：直接输出
        void assembleLOD0(vec3 in_vertex[3], vec2 in_uv[3], inout uint vid, inout uint iid)
        {
            for(uint i = 0; i < 3; ++i)
            {
                ms_out[vid + i].vertex = in_vertex[i];
                ms_out[vid + i].normal = normalize(cross(in_vertex[1] - in_vertex[0], in_vertex[2] - in_vertex[0]));
                ms_out[vid + i].uv = in_uv[i];
                gl_MeshVerticesNV[vid + i].gl_Position = projMat * viewMat * vec4(in_vertex[i], 1);
                gl_PrimitiveIndicesNV[iid + i] = vid + i;
            }
            vid += 3;
            iid += 3;
        }

        // LOD 1：细分一次
        void assembleLOD1(vec3 in_vertex[3], vec2 in_uv[3], inout uint vid, inout uint iid)
        {
            vec3 out_vertex[3];
            vec2 out_uv[3];
            tessellate(in_vertex, in_uv, out_vertex, out_uv);

            for(uint i = 0; i < 3; ++i)
                out_vertex[i].y = texture(texture1, out_uv[i]).r * height;

            // 4个子三角形
            vec3 v0[3] = vec3[3](in_vertex[0], out_vertex[0], out_vertex[2]);
            vec2 uv0[3] = vec2[3](in_uv[0], out_uv[0], out_uv[2]);
            assembleLOD0(v0, uv0, vid, iid);

            vec3 v1[3] = vec3[3](in_vertex[1], out_vertex[1], out_vertex[0]);
            vec2 uv1[3] = vec2[3](in_uv[1], out_uv[1], out_uv[0]);
            assembleLOD0(v1, uv1, vid, iid);

            vec3 v2[3] = vec3[3](in_vertex[2], out_vertex[2], out_vertex[1]);
            vec2 uv2[3] = vec2[3](in_uv[2], out_uv[2], out_uv[1]);
            assembleLOD0(v2, uv2, vid, iid);

            vec3 v3[3] = vec3[3](out_vertex[0], out_vertex[1], out_vertex[2]);
            vec2 uv3[3] = vec2[3](out_uv[0], out_uv[1], out_uv[2]);
            assembleLOD0(v3, uv3, vid, iid);
        }

        // LOD 2：细分两次
        void assembleLOD2(vec3 in_vertex[3], vec2 in_uv[3], inout uint vid, inout uint iid)
        {
            vec3 out_vertex[3];
            vec2 out_uv[3];
            tessellate(in_vertex, in_uv, out_vertex, out_uv);

            for(uint i = 0; i < 3; ++i)
                out_vertex[i].y = texture(texture1, out_uv[i]).r * height;

            vec3 v0[3] = vec3[3](in_vertex[0], out_vertex[0], out_vertex[2]);
            vec2 uv0[3] = vec2[3](in_uv[0], out_uv[0], out_uv[2]);
            assembleLOD1(v0, uv0, vid, iid);

            vec3 v1[3] = vec3[3](in_vertex[1], out_vertex[1], out_vertex[0]);
            vec2 uv1[3] = vec2[3](in_uv[1], out_uv[1], out_uv[0]);
            assembleLOD1(v1, uv1, vid, iid);

            vec3 v2[3] = vec3[3](in_vertex[2], out_vertex[2], out_vertex[1]);
            vec2 uv2[3] = vec2[3](in_uv[2], out_uv[2], out_uv[1]);
            assembleLOD1(v2, uv2, vid, iid);

            vec3 v3[3] = vec3[3](out_vertex[0], out_vertex[1], out_vertex[2]);
            vec2 uv3[3] = vec2[3](out_uv[0], out_uv[1], out_uv[2]);
            assembleLOD1(v3, uv3, vid, iid);
        }

        // LOD 3：细分三次
        void assembleLOD3(vec3 in_vertex[3], vec2 in_uv[3], inout uint vid, inout uint iid)
        {
            vec3 out_vertex[3];
            vec2 out_uv[3];
            tessellate(in_vertex, in_uv, out_vertex, out_uv);

            for(uint i = 0; i < 3; ++i)
                out_vertex[i].y = texture(texture1, out_uv[i]).r * height;

            vec3 v0[3] = vec3[3](in_vertex[0], out_vertex[0], out_vertex[2]);
            vec2 uv0[3] = vec2[3](in_uv[0], out_uv[0], out_uv[2]);
            assembleLOD2(v0, uv0, vid, iid);

            vec3 v1[3] = vec3[3](in_vertex[1], out_vertex[1], out_vertex[0]);
            vec2 uv1[3] = vec2[3](in_uv[1], out_uv[1], out_uv[0]);
            assembleLOD2(v1, uv1, vid, iid);

            vec3 v2[3] = vec3[3](in_vertex[2], out_vertex[2], out_vertex[1]);
            vec2 uv2[3] = vec2[3](in_uv[2], out_uv[2], out_uv[1]);
            assembleLOD2(v2, uv2, vid, iid);

            vec3 v3[3] = vec3[3](out_vertex[0], out_vertex[1], out_vertex[2]);
            vec2 uv3[3] = vec2[3](out_uv[0], out_uv[1], out_uv[2]);
            assembleLOD2(v3, uv3, vid, iid);
        }

        void main()
        {
            uint meshlet_id = gl_WorkGroupID.x;

            ivec2 isize = textureSize(texture1, 0);
            vec2 size = vec2(float(isize.x), float(isize.y));

            vec3 in_vertex[3];
            vec2 in_uv[3];

            uint vid = 0, iid = 0;
            for(uint i=0; i<3; ++i)
            {
                uint index = indices[meshlet_id * 3 + i];
                in_vertex[i] = vec3(modelMat * vec4(vertices[index], 1));
                in_uv[i] = uvs[index];
                in_vertex[i].y = texture(texture1, uvs[index]).r * height;
            }
            assembleLOD3(in_vertex, in_uv, vid, iid);

            gl_PrimitiveCountNV = iid / 3;
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
        layout(binding = 1) uniform sampler2D texture1;

        // 光源属性
        const vec3 LIGHT_POSITION = vec3(5.0, 5.0, 5.0);
        const vec3 LIGHT_COLOR = vec3(1.0, 0.98, 0.94);       // 暖白色太阳光
        const float LIGHT_INTENSITY = 1.5;                     // 太阳光强度

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
    static auto pass_color = gl_create_texture_color(width, height, nullptr);
    static auto pass_depth = gl_create_texture_depth(width, height, nullptr);
    {
        gl_pass_t pass = {.module = module, .colors = {{.texture = pass_color, .clear = true,}}, .depth = {.texture = pass_depth, .clear = true, .write = true, .func = GL_LEQUAL,}, .fill_mode = GL_FILL,};
        gl_begin_meshlet(pass);

        gl_push_const_float("height", 2.5f);
        gl_push_const_mat4("projMat", &projMat[0][0]);
        gl_push_const_mat4("viewMat", &viewMat[0][0]);
        gl_push_const_mat4("modelMat", &modelMat[0][0]);

        static auto meshlet = gl_create_meshlet_plane(5, 100);
        gl_bind_buffer(meshlet.vertex_vbo, {.binding = 0, .target = GL_SHADER_STORAGE_BUFFER,});
        gl_bind_buffer(meshlet.normal_vbo, {.binding = 1, .target = GL_SHADER_STORAGE_BUFFER,});
        gl_bind_buffer(meshlet.uv_vbo, {.binding = 2, .target = GL_SHADER_STORAGE_BUFFER,});
        gl_bind_buffer(meshlet.index_vbo, {.binding = 3, .target = GL_SHADER_STORAGE_BUFFER,});

        static auto texture0 = gl_load_texture("../../DiffuseTerrain.png");
        static auto texture1 = gl_load_texture("../../HeightTerrain.png");
        gl_bind_texture(texture0, {.binding = 0,});
        gl_bind_texture(texture1, {.binding = 1,});

        gl_set_viewport(0, 0, width, height);
        gl_draw_meshlet(meshlet.index_count / 3);

        gl_end_meshlet(pass);
    }

    gl_draw_screen(width, height, pass_color);
}