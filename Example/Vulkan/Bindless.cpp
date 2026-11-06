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
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cstring>
#include <vector>

[[maybe_unused]] static constexpr auto VS = R"(
    #version 450
    layout(location = 0) in vec3 in_vertex;
    layout(location = 1) in vec3 in_normal;
    layout(location = 2) in vec2 in_uv;
    layout(location = 0) out vec3 vertex;
    layout(location = 1) out vec3 normal;
    layout(location = 2) out vec2 uv;

    layout(push_constant) uniform Push
    {
        mat4 projView;
        vec4 c0;
        vec4 c1;
        vec4 c2;
        vec3 translation;
        uint albedo;
    } push;

    void main()
    {
        mat4 meshMat = mat4(push.c0, push.c1, push.c2, vec4(push.translation, 1.0));
        vertex = vec3(meshMat * vec4(in_vertex, 1.0));
        normal = vec3(meshMat * vec4(in_normal, 0.0));
        uv = in_uv;
        gl_Position = push.projView * vec4(vertex, 1.0);
    }
)";

[[maybe_unused]] static constexpr auto FS = R"(
    #version 450
    #extension GL_EXT_nonuniform_qualifier : require
    layout(location = 0) in vec3 vertex;
    layout(location = 1) in vec3 normal;
    layout(location = 2) in vec2 uv;
    layout(location = 0) out vec4 final;

    layout(push_constant) uniform Push
    {
        mat4 projView;
        vec4 c0;
        vec4 c1;
        vec4 c2;
        vec3 translation;
        uint albedo;
    } push;

    layout(std430, set = 0, binding = 0) readonly buffer BindlessTextures
    {
        uint handles[];
    };
    layout(set = 0, binding = 1) uniform sampler samp;
    layout(set = 1, binding = 0) uniform texture2D textures[];

    const vec3 LIGHT_POSITION = vec3(5.0, 5.0, 5.0);
    const vec3 LIGHT_COLOR = vec3(1.0, 0.98, 0.94);
    const float LIGHT_INTENSITY = 1.5;
    const vec3 AMBIENT_COLOR = vec3(0.1, 0.1, 0.1);
    const vec3 DIFFUSE_COLOR = vec3(1.0, 1.0, 1.0);
    const vec3 SPECULAR_COLOR = vec3(0.5, 0.5, 0.5);
    const float SHININESS = 32.0;
    const vec3 CAMERA_POSITION = vec3(0.0, 2.0, 8.0);

    void main()
    {
        vec3 N = normalize(normal);
        vec3 L = normalize(LIGHT_POSITION - vertex);
        vec3 V = normalize(CAMERA_POSITION - vertex);
        vec3 H = normalize(L + V);
        vec3 ambient = AMBIENT_COLOR;
        float diff = max(dot(N, L), 0.0);
        uint index = handles[push.albedo];
        vec3 sampled = texture(sampler2D(textures[nonuniformEXT(index)], samp), uv).rgb;
        vec3 diffuse = diff * DIFFUSE_COLOR * sampled;
        float spec = pow(max(dot(N, H), 0.0), SHININESS);
        vec3 specular = spec * SPECULAR_COLOR;
        final = vec4(ambient + LIGHT_INTENSITY * LIGHT_COLOR * (diffuse + specular), 1.0);
    }
)";

#include "bindless.spv"

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
{
    const char* level = "INFO";
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) level = "ERROR";
    else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) level = "WARNING";
    fprintf(stderr, "[Vulkan][%s] %s", level, data && data->pMessage ? data->pMessage : "");
    fflush(stderr);
    return VK_FALSE;
}

static VkDevice device = nullptr;
static VkQueue queue = nullptr;
static VkSwapchainKHR swapchain = nullptr;
static VkExtent2D extent = {};
static std::vector<VkImage> swapImages;
static VkCommandBuffer cmdBuffer = nullptr;
static VkSemaphore imageAvailable = nullptr;
static VkSemaphore renderFinished = nullptr;
static VkFence fence = nullptr;

struct Push
{
    glm::mat4 projView;
    glm::vec4 c0;
    glm::vec4 c1;
    glm::vec4 c2;
    glm::vec3 translation;
    uint32_t albedo;
};
static_assert(sizeof(Push) == 128, "bindless push constant is 128 bytes");

static Push make_push(glm::mat4 const& projView, glm::mat4 const& mesh, uint32_t albedo)
{
    Push push = {};
    push.projView = projView;
    push.c0 = mesh[0];
    push.c1 = mesh[1];
    push.c2 = mesh[2];
    push.translation = glm::vec3(mesh[3]);
    push.albedo = albedo;
    return push;
}

static rt_texture_t load_bindless(const char* filename)
{
    rt_image_t image = rt_load_image_file(filename, true);
    rt_texture_t texture = rt_create_texture({
        .width = image.width,
        .height = image.height,
        .depth = 1,
        .mipmaps = 1,
        .target = RT_TEXTURE_2D,
        .format = image.format,
        .usages = RT_TEXTURE_USAGE_COPY_SRC | RT_TEXTURE_USAGE_COPY_DST | RT_TEXTURE_USAGE_TEXTURE_BINDING | RT_TEXTURE_USAGE_STORAGE_BINDING | RT_TEXTURE_USAGE_RENDER_ATTACHMENT | RT_TEXTURE_USAGE_TEXTURE_BINDLESS,
        .min_filter = RT_LINEAR,
        .mag_filter = RT_LINEAR,
        .address_u = RT_REPEAT,
        .address_v = RT_REPEAT,
        .address_w = RT_REPEAT,
        .data = image.data,
    });
    rt_destroy_image(image);
    return texture;
}

static void write_handles(void* memory, uint32_t stride, rt_texture_t const* textures, int count)
{
    if (!memory || stride == 0) return;
    if (stride == sizeof(uint32_t))
    {
        auto* handles = (uint32_t*)memory;
        for (int i = 0; i < count; ++i)
            handles[i] = (uint32_t)textures[i].default_view.bindless;
    }
    else if (stride == sizeof(uint64_t))
    {
        auto* handles = (uint64_t*)memory;
        for (int i = 0; i < count; ++i)
            handles[i] = textures[i].default_view.bindless;
    }
}

void frame(int width, int height)
{
    static auto layout = rt_create_layout({
        .entry = {{.binding = 0, .type = RT_BINDING_STORAGE_BUFFER}, {.binding = 1, .type = RT_BINDING_SAMPLER}},
    });
    static auto module = rt_create_module_render({
        .layout = layout,
        .vshader = {.code = (const char*)kVS, .size = sizeof(kVS)},
        .fshader = {.code = (const char*)kFS, .size = sizeof(kFS)},
        .colors = {{.format = RT_TEXTURE_RGBA8UNORM,}},
        .depth = {.write = true, .func = RT_LEQUAL,},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
    });
    static auto pass_color = rt_create_texture_color(width, height);
    static auto pass_depth = rt_create_texture_depth(width, height);
    static auto cube = rt_create_mesh_cube(1.4f, 1.4f, 1.4f);
    static const char* files[] = {
        "../../Example/OpenGL/Default.png",
        "../../Example/OpenGL/Example.png",
        "../../Example/OpenGL/Meshlet.png",
        "../../Example/OpenGL/Render.png",
    };
    static rt_texture_t textures[4] = {
        load_bindless(files[0]),
        load_bindless(files[1]),
        load_bindless(files[2]),
        load_bindless(files[3]),
    };
    static const glm::vec3 places[4] = {
        {-3.3f, 0.0f, 0.0f},
        {-1.1f, 0.0f, 0.0f},
        { 1.1f, 0.0f, 0.0f},
        { 3.3f, 0.0f, 0.0f},
    };
    static auto limits = rt_get_limits();
    static auto table = rt_create_buffer({
        .size = (size_t)limits.max_bindless_handle_size * 4,
        .usages = RT_BUFFER_USAGE_STORAGE | RT_BUFFER_USAGE_MAP_WRITE,
    });
    static auto samp = rt_create_sampler({.min_filter = RT_LINEAR, .mag_filter = RT_LINEAR, .address_u = RT_REPEAT, .address_v = RT_REPEAT,});
    static auto binding = rt_create_binding({
        .layout = layout,
        .entry = {
            {.binding = 0, .buffer = table},
            {.binding = 1, .sampler = samp},
        },
    });
    if (auto* memory = rt_map_buffer(table, RT_WRITE_ONLY, 0, table.size))
    {
        write_handles(memory, limits.max_bindless_handle_size, textures, 4);
        rt_unmap_buffer(table);
    }
    {
        rt_pass_render_t pass = {.colors = {{.texture = pass_color.default_view, .clear = true,}}, .depth = {.texture = pass_depth.default_view, .clear = true,},};
        rt_begin_render(pass);
        rt_bind_module_render(module);
        rt_bind_binding(binding);

        auto projView = glm::perspectiveRH_ZO(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f) * glm::lookAt(glm::vec3(0, 2, 8), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        float spin = (float)SDL_GetTicks() / 2000.0f;
        for (int i = 0; i < 4; ++i)
        {
            auto meshMat = glm::translate(glm::mat4(1), places[i]);
            meshMat = glm::rotate(meshMat, spin + i * 0.4f, glm::vec3(0, 1, 0));
            meshMat = glm::rotate(meshMat, glm::radians(-18.0f), glm::vec3(1, 0, 0));
            Push push = make_push(projView, meshMat, (uint32_t)i);
            rt_push_constant((const uint8_t*)&push, sizeof(push));
            rt_buffer_view_t vertex[] = {{.buffer = cube.vertex[0]}, {.buffer = cube.vertex[1]}, {.buffer = cube.vertex[2]}};
            rt_buffer_view_t index = {.buffer = cube.index};
            rt_draw_index(vertex, 3, index, cube.index.size / sizeof(uint32_t), 1, 0, 0, 0);
        }

        rt_end_render(pass);
    }

    rt_submit();

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to begin command buffer");
        abort();
    }

    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
    VkImage src = *(VkImage*)pass_color.native;

    VkImageMemoryBarrier srcToTransfer = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT, .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = src, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkImageMemoryBarrier dstToTransfer = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = 0, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = swapImages[imageIndex], .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &srcToTransfer);
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &dstToTransfer);
    VkImageBlit blit = {.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, .srcOffsets = {{0, 0, 0}, {width, height, 1}}, .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, .dstOffsets = {{0, 0, 0}, {(int32_t)extent.width, (int32_t)extent.height, 1}}};
    vkCmdBlitImage(cmdBuffer, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    VkImageMemoryBarrier dstToPresent = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .dstAccessMask = 0, .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = swapImages[imageIndex], .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkImageMemoryBarrier srcToColor = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT, .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = src, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &dstToPresent);
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &srcToColor);

    vkEndCommandBuffer(cmdBuffer);
    vkResetFences(device, 1, &fence);
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .waitSemaphoreCount = 1, .pWaitSemaphores = &imageAvailable, .pWaitDstStageMask = &waitStage, .commandBufferCount = 1, .pCommandBuffers = &cmdBuffer, .signalSemaphoreCount = 1, .pSignalSemaphores = &renderFinished};
    vkQueueSubmit(queue, 1, &submit, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, .waitSemaphoreCount = 1, .pWaitSemaphores = &renderFinished, .swapchainCount = 1, .pSwapchains = &swapchain, .pImageIndices = &imageIndex};
    vkQueuePresentKHR(queue, &presentInfo);

    vkResetCommandBuffer(cmdBuffer, 0);
}

int main()
{
    int w = 1000, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    auto window = SDL_CreateWindow("Vulkan-Bindless", w, h, SDL_WINDOW_VULKAN);

    uint32_t extCount = 0;
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&extCount);
    std::vector instanceExts(sdlExts, sdlExts + extCount);
    instanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef __APPLE__
    instanceExts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    if (layerCount) vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    bool hasValidation = false;
    for (auto& layer : layers)
        if (strcmp(layer.layerName, validationLayer) == 0) { hasValidation = true; break; }
    if (!hasValidation)
        fprintf(stderr, "[Vulkan][ERROR] VK_LAYER_KHRONOS_validation is not available");

    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
    };
    VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "Bindless", .apiVersion = VK_API_VERSION_1_4};
    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pNext = hasValidation ? &dbgInfo : nullptr,
#ifdef __APPLE__
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = hasValidation ? 1u : 0u, .ppEnabledLayerNames = hasValidation ? &validationLayer : nullptr,
        .enabledExtensionCount = (uint32_t)instanceExts.size(), .ppEnabledExtensionNames = instanceExts.data(),
    };
    VkInstance instance = nullptr;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        fprintf(stderr, "[Vulkan][ERROR] vkCreateInstance failed");
        return 1;
    }
    VkDebugUtilsMessengerEXT messenger = nullptr;
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (hasValidation && vkCreateDebugUtilsMessengerEXT)
        vkCreateDebugUtilsMessengerEXT(instance, &dbgInfo, nullptr, &messenger);
    VkSurfaceKHR surface = nullptr;
    SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);

    uint32_t physCount = 0;
    vkEnumeratePhysicalDevices(instance, &physCount, nullptr);
    std::vector<VkPhysicalDevice> devices(physCount);
    vkEnumeratePhysicalDevices(instance, &physCount, devices.data());
    VkPhysicalDevice physical = nullptr;
    uint32_t family = 0;
    for (auto phys : devices)
    {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(phys, &props);
        if (props.apiVersion < VK_API_VERSION_1_4) continue;
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &familyCount, families.data());
        for (uint32_t i = 0; i < familyCount; ++i)
        {
            VkBool32 support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys, i, surface, &support);
            constexpr auto universalBits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
            if ((families[i].queueFlags & universalBits) == universalBits && support)
            {
                physical = phys;
                family = i;
                break;
            }
        }
        if (physical) break;
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = family,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    VkPhysicalDeviceVulkan11Features features11 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .shaderDrawParameters = VK_TRUE,
    };
    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &features11,
        .descriptorIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &features12,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceVulkan14Features features14 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = &features13,
        .dynamicRenderingLocalRead = VK_TRUE,
    };
    VkPhysicalDeviceMeshShaderFeaturesNV meshNV = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV,
        .pNext = &features14,
        .taskShader = VK_TRUE,
        .meshShader = VK_TRUE,
    };
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &meshNV,
    };
    const char* deviceExts[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME, VK_NV_MESH_SHADER_EXTENSION_NAME,
#ifdef __APPLE__
        "VK_KHR_portability_subset",
#endif
    };
    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = std::size(deviceExts),
        .ppEnabledExtensionNames = deviceExts
    };
    if (vkCreateDevice(physical, &deviceInfo, nullptr, &device) != VK_SUCCESS)
    {
        fprintf(stderr, "[Vulkan][ERROR] vkCreateDevice failed");
        return 1;
    }
    vkGetDeviceQueue(device, family, 0, &queue);

    VkCommandPoolCreateInfo poolInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = family};
    VkCommandPool cmdPool = nullptr;
    vkCreateCommandPool(device, &poolInfo, nullptr, &cmdPool);
    VkCommandBufferAllocateInfo allocInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = cmdPool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1};
    vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);

    VkSurfaceCapabilitiesKHR caps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps);
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, formats.data());
    VkSurfaceFormatKHR chosen = formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR} : formats[0];
    for (auto& format : formats)
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM || format.format == VK_FORMAT_R8G8B8A8_UNORM) { chosen = format; break; }
    extent = caps.currentExtent.width != 0xFFFFFFFFu ? caps.currentExtent : VkExtent2D{(uint32_t)w, (uint32_t)h};
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;
    VkSwapchainCreateInfoKHR swapInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, .surface = surface, .minImageCount = imageCount,
        .imageFormat = chosen.format, .imageColorSpace = chosen.colorSpace, .imageExtent = extent, .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = caps.currentTransform, .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, .presentMode = VK_PRESENT_MODE_FIFO_KHR, .clipped = VK_TRUE,
    };
    vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain);
    uint32_t swapCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapCount, nullptr);
    swapImages.resize(swapCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapCount, swapImages.data());

    VkSemaphoreCreateInfo semInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailable);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderFinished);
    VkFenceCreateInfo fenceInfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    vkCreateFence(device, &fenceInfo, nullptr, &fence);

    rt_load_library({.backend = RT_VULKAN, .vulkan = {.instance = instance, .physical = physical, .device = device, .queue = queue, .cmdbuf = cmdBuffer, .family = family}});

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) running = false;
        // ====================================================================

        frame(w, h);

        // ====================================================================
    }

    vkDeviceWaitIdle(device);
    rt_unload_library();

    vkDestroyFence(device, fence, nullptr);
    vkDestroySemaphore(device, renderFinished, nullptr);
    vkDestroySemaphore(device, imageAvailable, nullptr);
    vkDestroyCommandPool(device, cmdPool, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (messenger && vkDestroyDebugUtilsMessengerEXT)
        vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}