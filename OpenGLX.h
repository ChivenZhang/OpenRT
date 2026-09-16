#pragma once
#include "OpenRHI.h"

rhi_texture_t rhi_load_texture_file(const char* filename, bool flip = false);
rhi_mesh_t rhi_create_mesh_cube(float width, float height, float length);
rhi_mesh_t rhi_create_mesh_plane(float size, int N = 1);
rhi_mesh_t rhi_create_mesh_sphere(float radius, int rings, int slices);
rhi_mesh_t rhi_create_mesh_capsule(float radius, float height, int rings, int slices);
rhi_mesh_t rhi_create_mesh_footprint(float length, float width, float thickness, bool left_foot);
rhi_meshlet_t rhi_create_meshlet_plane(float size, int N = 1);
rhi_meshlet_t rhi_create_meshlet_sphere(float radius, int rings, int slices);
rhi_meshlet_t rhi_create_meshlet_capsule(float radius, float height, int rings, int slices);

#ifdef OPENGLX_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static rhi_texture_t rhi_load_texture_file(const char* filename, bool flip)
{
    // stb 默认以左上角为原点，flip 为 true 时翻转为 OpenGL 的左下角原点
    stbi_set_flip_vertically_on_load(flip ? 1 : 0);

    int width = 0, height = 0, channels = 0;
    void* data = nullptr;
    GLenum type = GL_UNSIGNED_BYTE;

    // 按文件实际位深选择加载方式：HDR -> float，16 位 PNG/PSD -> ushort，其余 -> byte
    if (stbi_is_hdr(filename)) {
        data = stbi_loadf(filename, &width, &height, &channels, 0);
        type = GL_FLOAT;
    }
    else if (stbi_is_16_bit(filename)) {
        data = stbi_load_16(filename, &width, &height, &channels, 0);
        type = GL_UNSIGNED_SHORT;
    }
    else {
        data = stbi_load(filename, &width, &height, &channels, 0);
        type = GL_UNSIGNED_BYTE;
    }

    if (!data) {
        fprintf(stderr, "Failed to load image: %s (%s)\n", filename, stbi_failure_reason());
        return {};
    }

    rhi_texture_desc_t info = {};
    info.width = (uint32_t)width;
    info.height = (uint32_t)height;
    info.depth = 1;
    info.target = GL_TEXTURE_2D;
    info.type = type;
    info.data = data;

    // stb 输出始终为 RGB/RGBA 顺序，无需通道转换；根据通道数和数据类型选择格式
    switch (channels)
    {
        case 1:
            info.format = GL_RED;
            switch (type)
            {
                case GL_UNSIGNED_BYTE:  info.internal_format = GL_R8;   break;
                case GL_UNSIGNED_SHORT: info.internal_format = GL_R16;  break;
                case GL_FLOAT:          info.internal_format = GL_R32F; break;
            }
            break;

        case 2:
            info.format = GL_RG;
            switch (type)
            {
                case GL_UNSIGNED_BYTE:  info.internal_format = GL_RG8;   break;
                case GL_UNSIGNED_SHORT: info.internal_format = GL_RG16;  break;
                case GL_FLOAT:          info.internal_format = GL_RG32F; break;
            }
            break;

        case 3:
            info.format = GL_RGB;
            switch (type)
            {
                case GL_UNSIGNED_BYTE:  info.internal_format = GL_RGB8;   break;
                case GL_UNSIGNED_SHORT: info.internal_format = GL_RGB16;  break;
                case GL_FLOAT:          info.internal_format = GL_RGB32F; break;
            }
            break;

        case 4:
            info.format = GL_RGBA;
            switch (type)
            {
                case GL_UNSIGNED_BYTE:  info.internal_format = GL_RGBA8;   break;
                case GL_UNSIGNED_SHORT: info.internal_format = GL_RGBA16;  break;
                case GL_FLOAT:          info.internal_format = GL_RGBA32F; break;
            }
            break;

        default:
            fprintf(stderr, "Unsupported channel count: %d\n", channels);
            stbi_image_free(data);
            return {};
    }

    rhi_texture_t texture = rhi_create_texture(info);
    stbi_image_free(data);
    return texture;
}

#include <vector>

static rhi_mesh_t rhi_create_mesh_plane(float size, int N)
{
    int vertsX = N + 1;
    int vertsY = N + 1;
    size_t vertex_count = vertsX * vertsY;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);
    std::vector<float> uvs(vertex_count * 2);

    // ---- 生成顶点 ----
    for (int z = 0; z <= N; z++)
    {
        for (int x = 0; x <= N; x++)
        {
            size_t i = z * vertsX + x;

            float fx = (float)x / N;
            float fz = (float)z / N;

            positions[i * 3 + 0] = (fx - 0.5f) * size;
            positions[i * 3 + 1] = 0.0f; // Y = 0
            positions[i * 3 + 2] = (fz - 0.5f) * size;

            normals[i * 3 + 0] = 0.0f;
            normals[i * 3 + 1] = 1.0f; // 法线朝上 (+Y)
            normals[i * 3 + 2] = 0.0f;

            uvs[i * 2 + 0] = fx;
            uvs[i * 2 + 1] = fz;
        }
    }

    // ---- 生成索引 ----
    size_t index_count = N * N * 6;
    std::vector<unsigned int> indices(index_count);

    size_t idx = 0;
    for (int z = 0; z < N; z++)
    {
        for (int x = 0; x < N; x++)
        {
            unsigned int tl = z * vertsX + x;
            unsigned int tr = tl + 1;
            unsigned int bl = (z + 1) * vertsX + x;
            unsigned int br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;

            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }

    return rhi_create_mesh(
        positions.data(),
        normals.data(),
        uvs.data(),
        vertex_count,
        indices.data(),
        index_count
    );
}

static rhi_mesh_t rhi_create_mesh_cube(float width, float height, float length)
{
    static const float v[24 * 3] = {
        // 前
        -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1,
        // 后
        1, -1, -1, -1, -1, -1, -1, 1, -1, 1, 1, -1,
        // 上
        -1, 1, 1, 1, 1, 1, 1, 1, -1, -1, 1, -1,
        // 下
        -1, -1, -1, 1, -1, -1, 1, -1, 1, -1, -1, 1,
        // 右
        1, -1, 1, 1, -1, -1, 1, 1, -1, 1, 1, 1,
        // 左
        -1, -1, -1, -1, -1, 1, -1, 1, 1, -1, 1, -1,
    };

    static const float n[24 * 3] = {
        // 前
        0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
        // 后
        0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
        // 上
        0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
        // 下
        0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,
        // 右
        1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
        // 左
        -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
    };

    static const float uv[24 * 2] = {
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
        0, 0, 1, 0, 1, 1, 0, 1,
    };

    static const unsigned int idx[6 * 6] = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    std::vector<float> positions(24 * 3);
    std::vector<float> normals(24 * 3);
    std::vector<float> uvs(24 * 2);

    for (int i = 0; i < 24; i++)
    {
        positions[i * 3 + 0] = v[i * 3 + 0] * width * 0.5f;
        positions[i * 3 + 1] = v[i * 3 + 1] * height * 0.5f;
        positions[i * 3 + 2] = v[i * 3 + 2] * length * 0.5f;

        normals[i * 3 + 0] = n[i * 3 + 0];
        normals[i * 3 + 1] = n[i * 3 + 1];
        normals[i * 3 + 2] = n[i * 3 + 2];

        uvs[i * 2 + 0] = uv[i * 2 + 0];
        uvs[i * 2 + 1] = uv[i * 2 + 1];
    }

    return rhi_create_mesh(
        positions.data(), normals.data(), uvs.data(),
        24, idx, 36
    );
}

static rhi_mesh_t rhi_create_mesh_footprint(
    float length,    // 鞋印长度（Z方向）
    float width,     // 鞋印宽度（X方向）
    float thickness, // 厚度（Y方向）
    bool left_foot)  // true=左脚, false=右脚
{
    // ========== 静态轮廓点（XZ平面，脚尖 +Z） ==========
    // 12个点，逆时针绕一圈
    static const float sole_profile[50][2] = {
        {+0.0152f, +1.0000f},  // 点 01 - 脚尖顶端
        {-0.0753f, +0.9730f},  // 点 02
        {-0.1562f, +0.9227f},  // 点 03
        {-0.2297f, +0.8546f},  // 点 04
        {-0.2859f, +0.7800f},  // 点 05
        {-0.3329f, +0.6978f},  // 点 06
        {-0.3685f, +0.6108f},  // 点 07
        {-0.3936f, +0.5195f},  // 点 08
        {-0.4102f, +0.4247f},  // 点 09
        {-0.4185f, +0.3264f},  // 点 10 - 外侧最宽
        {-0.4185f, +0.2270f},  // 点 11
        {-0.4075f, +0.1298f},  // 点 12
        {-0.3936f, +0.0338f},  // 点 13
        {-0.3771f, -0.0587f},  // 点 14
        {-0.3581f, -0.1523f},  // 点 15
        {-0.3437f, -0.2460f},  // 点 16
        {-0.3301f, -0.3421f},  // 点 17
        {-0.3249f, -0.4417f},  // 点 18
        {-0.3273f, -0.5421f},  // 点 19
        {-0.3269f, -0.6413f},  // 点 20
        {-0.3085f, -0.7315f},  // 点 21
        {-0.2749f, -0.8193f},  // 点 22
        {-0.2216f, -0.8958f},  // 点 23
        {-0.1464f, -0.9558f},  // 点 24
        {-0.0624f, -0.9890f},  // 点 25
        {+0.0348f, -0.9945f},  // 点 26 - 脚跟底端
        {+0.1277f, -0.9732f},  // 点 27
        {+0.2070f, -0.9270f},  // 点 28
        {+0.2750f, -0.8534f},  // 点 29
        {+0.3163f, -0.7711f},  // 点 30
        {+0.3329f, -0.6785f},  // 点 31
        {+0.3255f, -0.5838f},  // 点 32
        {+0.3001f, -0.4949f},  // 点 33
        {+0.2721f, -0.4048f},  // 点 34
        {+0.2555f, -0.3099f},  // 点 35 - 足弓内侧收腰最凹处
        {+0.2638f, -0.2116f},  // 点 36
        {+0.2887f, -0.1225f},  // 点 37
        {+0.3246f, -0.0357f},  // 点 38
        {+0.3605f, +0.0512f},  // 点 39
        {+0.3877f, +0.1377f},  // 点 40
        {+0.4072f, +0.2290f},  // 点 41
        {+0.4157f, +0.3272f},  // 点 42 - 内侧最宽
        {+0.4157f, +0.4266f},  // 点 43
        {+0.4074f, +0.5249f},  // 点 44
        {+0.3881f, +0.6186f},  // 点 45
        {+0.3593f, +0.7084f},  // 点 46
        {+0.3186f, +0.7933f},  // 点 47
        {+0.2662f, +0.8733f},  // 点 48
        {+0.1954f, +0.9441f},  // 点 49
        {+0.1123f, +0.9890f}   // 点 50 - 闭合回脚尖
    };

    const int N = 50;

    // 顶点布局：
    // 0        = 顶面中心
    // 1        = 底面中心
    // 2~13     = 顶面轮廓
    // 14~25    = 底面轮廓
    const int top_center = 0;
    const int bot_center = 1;
    const int top_start  = 2;
    const int bot_start  = 2 + N;
    const int vert_count = 2 + N * 2;

    std::vector<float> positions(vert_count * 3);
    std::vector<float> normals(vert_count * 3);
    std::vector<float> uvs(vert_count * 2);
    std::vector<unsigned int> indices;

    float y_top =  thickness;
    float y_bot = 0;

    // 辅助函数：设置顶点
    auto set_vertex = [&](int idx, float x, float y, float z,
                          float nx, float ny, float nz,
                          float u, float v)
    {
        positions[idx * 3 + 0] = x;
        positions[idx * 3 + 1] = y;
        positions[idx * 3 + 2] = z;
        normals[idx * 3 + 0] = nx;
        normals[idx * 3 + 1] = ny;
        normals[idx * 3 + 2] = nz;
        uvs[idx * 2 + 0] = u;
        uvs[idx * 2 + 1] = v;
    };

    // ========== 中心点 ==========
    set_vertex(top_center,
        0.0f, y_top, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.5f, 0.5f);

    set_vertex(bot_center,
        0.0f, y_bot, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.5f, 0.5f);

    // ========== 轮廓点 ==========
    for (int i = 0; i < N; i++)
    {
        float px = sole_profile[i][0];
        float pz = sole_profile[i][1];

        // 左右脚镜像
        if (!left_foot) px = -px;

        px *= width;
        pz *= length;

        float u = (sole_profile[i][0] + 0.6f) / 1.2f;
        float v = (sole_profile[i][1] + 0.8f) / 1.6f;

        set_vertex(top_start + i,
            px, y_top, pz,
            0.0f, 1.0f, 0.0f, u, v);

        set_vertex(bot_start + i,
            px, y_bot, pz,
            0.0f, -1.0f, 0.0f, u, v);
    }

    // ========== 顶面 Triangle Fan ==========
    for (int i = 0; i < N; i++)
    {
        int a = top_start + i;
        int b = top_start + ((i + 1) % N);
        indices.push_back(top_center);
        indices.push_back(b);
        indices.push_back(a);
    }

    // ========== 底面 Triangle Fan（反向绕序） ==========
    for (int i = 0; i < N; i++)
    {
        int a = bot_start + i;
        int b = bot_start + ((i + 1) % N);
        indices.push_back(bot_center);
        indices.push_back(a);
        indices.push_back(b);
    }

    // ========== 侧面 ==========
    for (int i = 0; i < N; i++)
    {
        int cur  = top_start + i;
        int nxt  = top_start + ((i + 1) % N);
        int curB = bot_start + i;
        int nxtB = bot_start + ((i + 1) % N);

        // 侧面法线（XZ平面内）
        float dx = positions[nxt * 3 + 0] - positions[cur * 3 + 0];
        float dz = positions[nxt * 3 + 2] - positions[cur * 3 + 2];
        float len = sqrtf(dx * dx + dz * dz);
        float nx = -dz / len;
        float nz =  dx / len;

        // 两个三角形 = 一个侧面四边形
        indices.push_back(cur);
        indices.push_back(nxt);
        indices.push_back(curB);

        indices.push_back(nxt);
        indices.push_back(nxtB);
        indices.push_back(curB);

        // 覆盖外圈顶点法线
        for (int vtx : {cur, nxt, curB, nxtB})
        {
            normals[vtx * 3 + 0] = nx;
            normals[vtx * 3 + 1] = 0.0f;
            normals[vtx * 3 + 2] = nz;
        }
    }

    return rhi_create_mesh(
        positions.data(),
        normals.data(),
        uvs.data(),
        vert_count,
        indices.data(),
        (unsigned int)indices.size()
    );
}

static rhi_mesh_t rhi_create_mesh_sphere(float radius, int rings, int slices)
{
    int vertex_count = (rings + 1) * (slices + 1);
    int index_count = rings * slices * 6;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);
    std::vector<float> uvs(vertex_count * 2);
    std::vector<unsigned int> indices(index_count);

    int v = 0;
    for (int r = 0; r <= rings; r++)
    {
        float theta = r * 3.14159265f / rings;
        for (int s = 0; s <= slices; s++)
        {
            float phi = -s * 2.0f * 3.14159265f / slices;

            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = sinf(theta) * sinf(phi);

            positions[v * 3 + 0] = x * radius;
            positions[v * 3 + 1] = y * radius;
            positions[v * 3 + 2] = z * radius;

            normals[v * 3 + 0] = x;
            normals[v * 3 + 1] = y;
            normals[v * 3 + 2] = z;

            uvs[v * 2 + 0] = (float)s / slices;
            uvs[v * 2 + 1] = (float)r / rings;

            v++;
        }
    }

    int idx = 0;
    for (int r = 0; r < rings; r++)
    {
        for (int s = 0; s < slices; s++)
        {
            int tl = r * (slices + 1) + s;
            int tr = tl + 1;
            int bl = (r + 1) * (slices + 1) + s;
            int br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;

            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }

    return rhi_create_mesh(
        positions.data(), normals.data(), uvs.data(),
        vertex_count, indices.data(), index_count
    );
}

static rhi_mesh_t rhi_create_mesh_capsule(float radius, float height, int rings, int slices)
{
    // 半球纬度只用到 90°，所以 rings 参数复用
    // 上半球：从顶(0)到底(rings)
    // 下半球：从底(0)到顶(rings)
    // 圆柱：中间额外一层（或复用半球接缝）

    int halfRings = rings;  // 每个半球的纬度段数
    int cylinderRings = 2;  // 圆柱段数（可改）

    // 顶点布局：
    // [0 .. halfRings]           下半球（底→赤道）
    // [halfRings .. halfRings+cylinderRings]  圆柱
    // [halfRings+cylinderRings .. ]  上半球（赤道→顶）

    int totalRings = halfRings * 2 + cylinderRings;
    int vertexCount = (totalRings + 1) * (slices + 1);
    int indexCount = totalRings * slices * 6;

    std::vector<float> positions(vertexCount * 3);
    std::vector<float> normals(vertexCount * 3);
    std::vector<float> uvs(vertexCount * 2);
    std::vector<unsigned int> indices(indexCount);

    float halfHeight = height * 0.5f;
    const float PI = 3.14159265f;

    int v = 0;

    // ---- 生成顶点 ----
    for (int r = 0; r <= totalRings; r++)
    {
        float y, ny, theta;

        if (r <= halfRings)
        {
            // 下半球：从底(-PI/2)到赤道(0)
            theta = -PI * 0.5f + (float)r / halfRings * (PI * 0.5f);
            y = -halfHeight + radius * sinf(theta);
            ny = sinf(theta);
        }
        else if (r <= halfRings + cylinderRings)
        {
            // 圆柱段：线性插值
            float t = (float)(r - halfRings) / cylinderRings;
            y = -halfHeight + t * height;
            ny = 0.0f;
        }
        else
        {
            // 上半球：从赤道(0)到顶(PI/2)
            int localR = r - (halfRings + cylinderRings);
            theta = (float)localR / halfRings * (PI * 0.5f);
            y = halfHeight + radius * sinf(theta);
            ny = sinf(theta);
        }

        for (int s = 0; s <= slices; s++)
        {
            float phi = - (float)s / slices * 2.0f * PI;

            float nx, nz;
            if (r <= halfRings || r > halfRings + cylinderRings)
            {
                // 球面法线
                nx = cosf(theta) * cosf(phi);
                nz = cosf(theta) * sinf(phi);
            }
            else
            {
                // 圆柱法线
                nx = cosf(phi);
                nz = sinf(phi);
            }

            positions[v * 3 + 0] = nx * radius;
            positions[v * 3 + 1] = y;
            positions[v * 3 + 2] = nz * radius;

            normals[v * 3 + 0] = nx;
            normals[v * 3 + 1] = ny;
            normals[v * 3 + 2] = nz;

            uvs[v * 2 + 0] = (float)s / slices;
            uvs[v * 2 + 1] = (float)r / totalRings;

            v++;
        }
    }

    // ---- 生成索引（Triangle List） ----
    int idx = 0;
    for (int r = 0; r < totalRings; r++)
    {
        for (int s = 0; s < slices; s++)
        {
            int tl = r * (slices + 1) + s;
            int tr = tl + 1;
            int bl = (r + 1) * (slices + 1) + s;
            int br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;

            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }

    return rhi_create_mesh(
        positions.data(), normals.data(), uvs.data(),
        vertexCount, indices.data(), indexCount
    );
}

// ====================================================================

static rhi_mesh_t rhi_create_mesh_quad(float width, float height)
{
    const size_t vertex_count = 4;
    const size_t index_count  = 6;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);
    std::vector<float> uvs(vertex_count * 2);
    std::vector<unsigned int> indices(index_count);

    float w = width * 0.5f;
    float h = height * 0.5f;

    // 四个顶点（XY 平面）
    float verts[4][2] = {
        {-w, -h},
        { w, -h},
        { w,  h},
        {-w,  h}
    };

    float uv[4][2] = {
        {0, 0},
        {1, 0},
        {1, 1},
        {0, 1}
    };

    for (int i = 0; i < 4; i++)
    {
        positions[i * 3 + 0] = verts[i][0];
        positions[i * 3 + 1] = verts[i][1];
        positions[i * 3 + 2] = 0.0f;

        normals[i * 3 + 0] = 0.0f;
        normals[i * 3 + 1] = 0.0f;
        normals[i * 3 + 2] = 1.0f;

        uvs[i * 2 + 0] = uv[i][0];
        uvs[i * 2 + 1] = uv[i][1];
    }

    // 两个三角形
    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    indices[3] = 0; indices[4] = 2; indices[5] = 3;

    return rhi_create_mesh(
        positions.data(),
        normals.data(),
        uvs.data(),
        vertex_count,
        indices.data(),
        index_count
    );
}

static rhi_mesh_t rhi_create_mesh_circle(float radius, int segments)
{
    size_t vertex_count = segments + 1; // 中心点 + 边缘点
    size_t index_count  = segments * 3;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);
    std::vector<float> uvs(vertex_count * 2);
    std::vector<unsigned int> indices(index_count);

    // 中心点
    positions[0] = 0.0f; positions[1] = 0.0f; positions[2] = 0.0f;
    normals[0] = 0; normals[1] = 0; normals[2] = 1;
    uvs[0] = 0.5f; uvs[1] = 0.5f;

    float angle_step = 2.0f * GL_PI / segments;

    for (int i = 0; i < segments; i++)
    {
        float a = i * angle_step;
        float x = cosf(a) * radius;
        float y = sinf(a) * radius;

        size_t vi = i + 1;

        positions[vi * 3 + 0] = x;
        positions[vi * 3 + 1] = y;
        positions[vi * 3 + 2] = 0.0f;

        normals[vi * 3 + 0] = 0;
        normals[vi * 3 + 1] = 0;
        normals[vi * 3 + 2] = 1;

        uvs[vi * 2 + 0] = (x / radius) * 0.5f + 0.5f;
        uvs[vi * 2 + 1] = (y / radius) * 0.5f + 0.5f;
    }

    for (int i = 0; i < segments; i++)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2 > segments ? 1 : i + 2;
    }

    return rhi_create_mesh(
        positions.data(),
        normals.data(),
        uvs.data(),
        vertex_count,
        indices.data(),
        index_count
    );
}

static rhi_mesh_t rhi_create_mesh_ring(
    float inner_radius,
    float outer_radius,
    int segments)
{
    size_t vertex_count = segments * 2;
    size_t index_count  = segments * 6;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);
    std::vector<float> uvs(vertex_count * 2);
    std::vector<unsigned int> indices(index_count);

    float angle_step = 2.0f * GL_PI / segments;

    for (int i = 0; i < segments; i++)
    {
        float a = i * angle_step;
        float cosA = cosf(a);
        float sinA = sinf(a);

        size_t i0 = i * 2;
        size_t i1 = i * 2 + 1;

        // 内圈
        positions[i0 * 3 + 0] = cosA * inner_radius;
        positions[i0 * 3 + 1] = sinA * inner_radius;
        positions[i0 * 3 + 2] = 0.0f;

        // 外圈
        positions[i1 * 3 + 0] = cosA * outer_radius;
        positions[i1 * 3 + 1] = sinA * outer_radius;
        positions[i1 * 3 + 2] = 0.0f;

        normals[i0 * 3 + 2] = 1;
        normals[i1 * 3 + 2] = 1;

        uvs[i0 * 2 + 0] = (cosA + 1) * 0.5f;
        uvs[i0 * 2 + 1] = (sinA + 1) * 0.5f;
        uvs[i1 * 2 + 0] = (cosA + 1) * 0.5f;
        uvs[i1 * 2 + 1] = (sinA + 1) * 0.5f;
    }

    for (int i = 0; i < segments; i++)
    {
        unsigned int i0 = i * 2;
        unsigned int i1 = i * 2 + 1;
        unsigned int i2 = ((i + 1) % segments) * 2;
        unsigned int i3 = ((i + 1) % segments) * 2 + 1;

        size_t idx = i * 6;
        indices[idx++] = i0;
        indices[idx++] = i2;
        indices[idx++] = i1;

        indices[idx++] = i2;
        indices[idx++] = i3;
        indices[idx++] = i1;
    }

    return rhi_create_mesh(
        positions.data(),
        normals.data(),
        uvs.data(),
        vertex_count,
        indices.data(),
        index_count
    );
}

// ====================================================================

static rhi_meshlet_t rhi_create_meshlet_plane(float size, int N)
{
    int vertsX = N + 1;
    int vertsY = N + 1;
    size_t vertex_count = vertsX * vertsY;

    std::vector<float> positions(vertex_count * 4);
    std::vector<float> normals(vertex_count * 4);
    std::vector<float> uvs(vertex_count * 2);

    // ---- 生成顶点 ----
    for (int z = 0; z <= N; z++)
    {
        for (int x = 0; x <= N; x++)
        {
            size_t i = z * vertsX + x;

            float fx = (float)x / N;
            float fz = (float)z / N;

            positions[i * 4 + 0] = (fx - 0.5f) * size;
            positions[i * 4 + 1] = 0.0f; // Y = 0
            positions[i * 4 + 2] = (fz - 0.5f) * size;

            normals[i * 4 + 0] = 0.0f;
            normals[i * 4 + 1] = 1.0f; // 法线朝上 (+Y)
            normals[i * 4 + 2] = 0.0f;

            uvs[i * 2 + 0] = fx;
            uvs[i * 2 + 1] = fz;
        }
    }

    // ---- 生成索引 ----
    size_t index_count = N * N * 6;
    std::vector<unsigned int> indices(index_count);

    size_t idx = 0;
    for (int z = 0; z < N; z++)
    {
        for (int x = 0; x < N; x++)
        {
            unsigned int tl = z * vertsX + x;
            unsigned int tr = tl + 1;
            unsigned int bl = (z + 1) * vertsX + x;
            unsigned int br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;

            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }

    return rhi_create_meshlet(
        positions.data(),
        normals.data(),
        uvs.data(),
        vertex_count,
        indices.data(),
        index_count
    );
}

static rhi_meshlet_t rhi_create_meshlet_sphere(float radius, int rings, int slices)
{
    int vertex_count = (rings + 1) * (slices + 1);
    int index_count = rings * slices * 6;

    std::vector<float> positions(vertex_count * 4);
    std::vector<float> normals(vertex_count * 4);
    std::vector<float> uvs(vertex_count * 2);
    std::vector<unsigned int> indices(index_count);

    int v = 0;
    for (int r = 0; r <= rings; r++)
    {
        float theta = r * 3.14159265f / rings;
        for (int s = 0; s <= slices; s++)
        {
            float phi = -s * 2.0f * 3.14159265f / slices;

            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = sinf(theta) * sinf(phi);

            positions[v * 4 + 0] = x * radius;
            positions[v * 4 + 1] = y * radius;
            positions[v * 4 + 2] = z * radius;

            normals[v * 4 + 0] = x;
            normals[v * 4 + 1] = y;
            normals[v * 4 + 2] = z;

            uvs[v * 2 + 0] = (float)s / slices;
            uvs[v * 2 + 1] = (float)r / rings;

            v++;
        }
    }

    int idx = 0;
    for (int r = 0; r < rings; r++)
    {
        for (int s = 0; s < slices; s++)
        {
            int tl = r * (slices + 1) + s;
            int tr = tl + 1;
            int bl = (r + 1) * (slices + 1) + s;
            int br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;

            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }

    return rhi_create_meshlet(
        positions.data(), normals.data(), uvs.data(),
        vertex_count, indices.data(), index_count
    );
}

static rhi_meshlet_t rhi_create_meshlet_capsule(float radius, float height, int rings, int slices)
{
    // 半球纬度只用到 90°，所以 rings 参数复用
    // 上半球：从顶(0)到底(rings)
    // 下半球：从底(0)到顶(rings)
    // 圆柱：中间额外一层（或复用半球接缝）

    int halfRings = rings;  // 每个半球的纬度段数
    int cylinderRings = 2;  // 圆柱段数（可改）

    // 顶点布局：
    // [0 .. halfRings]           下半球（底→赤道）
    // [halfRings .. halfRings+cylinderRings]  圆柱
    // [halfRings+cylinderRings .. ]  上半球（赤道→顶）

    int totalRings = halfRings * 2 + cylinderRings;
    int vertexCount = (totalRings + 1) * (slices + 1);
    int indexCount = totalRings * slices * 6;

    std::vector<float> positions(vertexCount * 4);
    std::vector<float> normals(vertexCount * 4);
    std::vector<float> uvs(vertexCount * 2);
    std::vector<unsigned int> indices(indexCount);

    float halfHeight = height * 0.5f;
    const float PI = 3.14159265f;

    int v = 0;

    // ---- 生成顶点 ----
    for (int r = 0; r <= totalRings; r++)
    {
        float y, ny, theta;

        if (r <= halfRings)
        {
            // 下半球：从底(-PI/2)到赤道(0)
            theta = -PI * 0.5f + (float)r / halfRings * (PI * 0.5f);
            y = -halfHeight + radius * sinf(theta);
            ny = sinf(theta);
        }
        else if (r <= halfRings + cylinderRings)
        {
            // 圆柱段：线性插值
            float t = (float)(r - halfRings) / cylinderRings;
            y = -halfHeight + t * height;
            ny = 0.0f;
        }
        else
        {
            // 上半球：从赤道(0)到顶(PI/2)
            int localR = r - (halfRings + cylinderRings);
            theta = (float)localR / halfRings * (PI * 0.5f);
            y = halfHeight + radius * sinf(theta);
            ny = sinf(theta);
        }

        for (int s = 0; s <= slices; s++)
        {
            float phi = - (float)s / slices * 2.0f * PI;

            float nx, nz;
            if (r <= halfRings || r > halfRings + cylinderRings)
            {
                // 球面法线
                nx = cosf(theta) * cosf(phi);
                nz = cosf(theta) * sinf(phi);
            }
            else
            {
                // 圆柱法线
                nx = cosf(phi);
                nz = sinf(phi);
            }

            positions[v * 4 + 0] = nx * radius;
            positions[v * 4 + 1] = y;
            positions[v * 4 + 2] = nz * radius;

            normals[v * 4 + 0] = nx;
            normals[v * 4 + 1] = ny;
            normals[v * 4 + 2] = nz;

            uvs[v * 2 + 0] = (float)s / slices;
            uvs[v * 2 + 1] = (float)r / totalRings;

            v++;
        }
    }

    // ---- 生成索引（Triangle List） ----
    int idx = 0;
    for (int r = 0; r < totalRings; r++)
    {
        for (int s = 0; s < slices; s++)
        {
            int tl = r * (slices + 1) + s;
            int tr = tl + 1;
            int bl = (r + 1) * (slices + 1) + s;
            int br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;

            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }

    return rhi_create_meshlet(
        positions.data(), normals.data(), uvs.data(),
        vertexCount, indices.data(), indexCount
    );
}

#endif