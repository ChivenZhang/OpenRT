#pragma once
#include "OpenGL.h"

gl_texture_t gl_load_texture(const char* filename);
gl_mesh_t gl_create_mesh_cube(float width, float height, float length);
gl_mesh_t gl_create_mesh_plane(float size, int N = 1);
gl_mesh_t gl_create_mesh_sphere(float radius, int rings, int slices);
gl_mesh_t gl_create_mesh_capsule(float radius, float height, int rings, int slices);
gl_meshlet_t gl_create_meshlet_plane(float size, int N = 1);
gl_meshlet_t gl_create_meshlet_sphere(float radius, int rings, int slices);
gl_meshlet_t gl_create_meshlet_capsule(float radius, float height, int rings, int slices);

#ifdef OPENGLX_IMPLEMENTATION

#include <opencv2/opencv.hpp>
static gl_texture_t gl_load_texture(const char* filename)
{
    cv::Mat image = cv::imread(filename, cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        fprintf(stderr, "Failed to load image: %s\n", filename);
        return {};
    }
    if (image.channels() == 1) cv::cvtColor(image, image, cv::COLOR_GRAY2RGBA);
    else if (image.channels() == 3) cv::cvtColor(image, image, cv::COLOR_BGR2RGBA);
    else if (image.channels() == 4) cv::cvtColor(image, image, cv::COLOR_BGRA2RGBA);
    else return {};
    return gl_create_texture_color(image.cols, image.rows, image.data);
}

#include <vector>
static gl_mesh_t gl_create_mesh_cube(float width, float height, float length)
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

    return gl_create_mesh(
        positions.data(), normals.data(), uvs.data(),
        24, idx, 36
    );
}

static gl_mesh_t gl_create_mesh_plane(float size, int N)
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

    return gl_create_mesh(
        positions.data(),
        normals.data(),
        uvs.data(),
        vertex_count,
        indices.data(),
        index_count
    );
}

static gl_mesh_t gl_create_mesh_sphere(float radius, int rings, int slices)
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

    return gl_create_mesh(
        positions.data(), normals.data(), uvs.data(),
        vertex_count, indices.data(), index_count
    );
}

static gl_mesh_t gl_create_mesh_capsule(float radius, float height, int rings, int slices)
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

    return gl_create_mesh(
        positions.data(), normals.data(), uvs.data(),
        vertexCount, indices.data(), indexCount
    );
}

// ====================================================================

static gl_meshlet_t gl_create_meshlet_plane(float size, int N)
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

    return gl_create_meshlet(
        positions.data(),
        normals.data(),
        uvs.data(),
        vertex_count,
        indices.data(),
        index_count
    );
}

static gl_meshlet_t gl_create_meshlet_sphere(float radius, int rings, int slices)
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

    return gl_create_meshlet(
        positions.data(), normals.data(), uvs.data(),
        vertex_count, indices.data(), index_count
    );
}

static gl_meshlet_t gl_create_meshlet_capsule(float radius, float height, int rings, int slices)
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

    return gl_create_meshlet(
        positions.data(), normals.data(), uvs.data(),
        vertexCount, indices.data(), indexCount
    );
}

#endif