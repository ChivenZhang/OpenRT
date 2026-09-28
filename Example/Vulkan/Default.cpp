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
#include "../../Vulkan.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdio>
#include <cstring>
#include <vector>

[[maybe_unused]] static constexpr auto VS = R"(
    #version 450
    layout(location = 0) in vec3 in_vertex;
    layout(location = 1) in vec3 in_normal;
    layout(location = 2) in vec2 in_uv;
    layout(location = 0) out vec3 color;

    void main()
    {
        vec3 colors[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));
        color = colors[gl_VertexIndex];
        gl_Position = vec4(in_vertex, 1.0);
    }
)";

[[maybe_unused]] static constexpr auto FS = R"(
    #version 450
    layout(location = 0) in vec3 color;
    layout(location = 0) out vec4 final;

    void main()
    {
        final = vec4(color, 1.0);
    }
)";

static constexpr uint32_t kVS[] = {
    0x07230203, 0x00010000, 0x000d000b, 0x00000030, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x000b000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x00000014, 0x00000017, 0x00000021,
    0x00000024, 0x0000002c, 0x0000002f, 0x00030003, 0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47,
    0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365, 0x00006576,
    0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572, 0x00657669,
    0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00040005, 0x0000000c, 0x6f6c6f63, 0x00007372,
    0x00040005, 0x00000014, 0x6f6c6f63, 0x00000072, 0x00060005, 0x00000017, 0x565f6c67, 0x65747265,
    0x646e4978, 0x00007865, 0x00060005, 0x0000001f, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000,
    0x00060006, 0x0000001f, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006, 0x0000001f,
    0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x0000001f, 0x00000002,
    0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e, 0x00070006, 0x0000001f, 0x00000003, 0x435f6c67,
    0x446c6c75, 0x61747369, 0x0065636e, 0x00030005, 0x00000021, 0x00000000, 0x00050005, 0x00000024,
    0x765f6e69, 0x65747265, 0x00000078, 0x00050005, 0x0000002c, 0x6e5f6e69, 0x616d726f, 0x0000006c,
    0x00040005, 0x0000002f, 0x755f6e69, 0x00000076, 0x00040047, 0x00000014, 0x0000001e, 0x00000000,
    0x00040047, 0x00000017, 0x0000000b, 0x0000002a, 0x00030047, 0x0000001f, 0x00000002, 0x00050048,
    0x0000001f, 0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000001f, 0x00000001, 0x0000000b,
    0x00000001, 0x00050048, 0x0000001f, 0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x0000001f,
    0x00000003, 0x0000000b, 0x00000004, 0x00040047, 0x00000024, 0x0000001e, 0x00000000, 0x00040047,
    0x0000002c, 0x0000001e, 0x00000001, 0x00040047, 0x0000002f, 0x0000001e, 0x00000002, 0x00020013,
    0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017,
    0x00000007, 0x00000006, 0x00000003, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002b,
    0x00000008, 0x00000009, 0x00000003, 0x0004001c, 0x0000000a, 0x00000007, 0x00000009, 0x00040020,
    0x0000000b, 0x00000007, 0x0000000a, 0x0004002b, 0x00000006, 0x0000000d, 0x3f800000, 0x0004002b,
    0x00000006, 0x0000000e, 0x00000000, 0x0006002c, 0x00000007, 0x0000000f, 0x0000000d, 0x0000000e,
    0x0000000e, 0x0006002c, 0x00000007, 0x00000010, 0x0000000e, 0x0000000d, 0x0000000e, 0x0006002c,
    0x00000007, 0x00000011, 0x0000000e, 0x0000000e, 0x0000000d, 0x0006002c, 0x0000000a, 0x00000012,
    0x0000000f, 0x00000010, 0x00000011, 0x00040020, 0x00000013, 0x00000003, 0x00000007, 0x0004003b,
    0x00000013, 0x00000014, 0x00000003, 0x00040015, 0x00000015, 0x00000020, 0x00000001, 0x00040020,
    0x00000016, 0x00000001, 0x00000015, 0x0004003b, 0x00000016, 0x00000017, 0x00000001, 0x00040020,
    0x00000019, 0x00000007, 0x00000007, 0x00040017, 0x0000001c, 0x00000006, 0x00000004, 0x0004002b,
    0x00000008, 0x0000001d, 0x00000001, 0x0004001c, 0x0000001e, 0x00000006, 0x0000001d, 0x0006001e,
    0x0000001f, 0x0000001c, 0x00000006, 0x0000001e, 0x0000001e, 0x00040020, 0x00000020, 0x00000003,
    0x0000001f, 0x0004003b, 0x00000020, 0x00000021, 0x00000003, 0x0004002b, 0x00000015, 0x00000022,
    0x00000000, 0x00040020, 0x00000023, 0x00000001, 0x00000007, 0x0004003b, 0x00000023, 0x00000024,
    0x00000001, 0x00040020, 0x0000002a, 0x00000003, 0x0000001c, 0x0004003b, 0x00000023, 0x0000002c,
    0x00000001, 0x00040017, 0x0000002d, 0x00000006, 0x00000002, 0x00040020, 0x0000002e, 0x00000001,
    0x0000002d, 0x0004003b, 0x0000002e, 0x0000002f, 0x00000001, 0x00050036, 0x00000002, 0x00000004,
    0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003b, 0x0000000b, 0x0000000c, 0x00000007,
    0x0003003e, 0x0000000c, 0x00000012, 0x0004003d, 0x00000015, 0x00000018, 0x00000017, 0x00050041,
    0x00000019, 0x0000001a, 0x0000000c, 0x00000018, 0x0004003d, 0x00000007, 0x0000001b, 0x0000001a,
    0x0003003e, 0x00000014, 0x0000001b, 0x0004003d, 0x00000007, 0x00000025, 0x00000024, 0x00050051,
    0x00000006, 0x00000026, 0x00000025, 0x00000000, 0x00050051, 0x00000006, 0x00000027, 0x00000025,
    0x00000001, 0x00050051, 0x00000006, 0x00000028, 0x00000025, 0x00000002, 0x00070050, 0x0000001c,
    0x00000029, 0x00000026, 0x00000027, 0x00000028, 0x0000000d, 0x00050041, 0x0000002a, 0x0000002b,
    0x00000021, 0x00000022, 0x0003003e, 0x0000002b, 0x00000029, 0x000100fd, 0x00010038,
};

static constexpr uint32_t kFS[] = {
    0x07230203, 0x00010000, 0x000d000b, 0x00000013, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000c, 0x00030010,
    0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f,
    0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365, 0x00006576, 0x00080004,
    0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572, 0x00657669, 0x00040005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00040005, 0x00000009, 0x616e6966, 0x0000006c, 0x00040005,
    0x0000000c, 0x6f6c6f63, 0x00000072, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047,
    0x0000000c, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
    0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020,
    0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017,
    0x0000000a, 0x00000006, 0x00000003, 0x00040020, 0x0000000b, 0x00000001, 0x0000000a, 0x0004003b,
    0x0000000b, 0x0000000c, 0x00000001, 0x0004002b, 0x00000006, 0x0000000e, 0x3f800000, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000a,
    0x0000000d, 0x0000000c, 0x00050051, 0x00000006, 0x0000000f, 0x0000000d, 0x00000000, 0x00050051,
    0x00000006, 0x00000010, 0x0000000d, 0x00000001, 0x00050051, 0x00000006, 0x00000011, 0x0000000d,
    0x00000002, 0x00070050, 0x00000007, 0x00000012, 0x0000000f, 0x00000010, 0x00000011, 0x0000000e,
    0x0003003e, 0x00000009, 0x00000012, 0x000100fd, 0x00010038,
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
{
    const char* level = "INFO";
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) level = "ERROR";
    else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) level = "WARNING";
    fprintf(stderr, "[Vulkan][%s] %s\n", level, data && data->pMessage ? data->pMessage : "");
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

void frame(int width, int height)
{
    static auto module = rt_create_module_render({
        .vshader = {.code = (const char*)kVS, .size = sizeof(kVS)},
        .fshader = {.code = (const char*)kFS, .size = sizeof(kFS)},
        .colors = {{.format = RT_TEXTURE_RGBA8UNORM,}},
        .vertex = {rt_vertex_vertex, rt_vertex_normal, rt_vertex_uv,},
        .cull_mode = RT_CULL_NONE,
    });
    static auto pass_color = rt_create_texture_color(width, height, nullptr);
    {
        rt_pass_render_t pass = {.module = module, .colors = {{.texture = pass_color, .clear = true,}},};
        rt_begin_render(pass);

        static auto mesh = rt_create_mesh_triangle(1);
        rt_draw_mesh(mesh);

        rt_end_render(pass);
    }

    rt_submit();

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan: failed to begin command buffer\n");
        abort();
    }

    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
    VkImage src = *(VkImage*)pass_color.native;

    VkImageMemoryBarrier srcToTransfer = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT, .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = src, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkImageMemoryBarrier dstToTransfer = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = 0, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = swapImages[imageIndex], .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &srcToTransfer);
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &dstToTransfer);
    VkImageBlit blit = {.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, .srcOffsets = {{0, 0, 0}, {(int32_t)pass_color.width, (int32_t)pass_color.height, 1}}, .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, .dstOffsets = {{0, 0, 0}, {(int32_t)extent.width, (int32_t)extent.height, 1}}};
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
    int w = 600, h = 600;
    SDL_Init(SDL_INIT_VIDEO);
    auto window = SDL_CreateWindow("Vulkan", w, h, SDL_WINDOW_VULKAN);

    uint32_t extCount = 0;
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&extCount);
    std::vector instanceExts(sdlExts, sdlExts + extCount);
    instanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    if (layerCount) vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    bool hasValidation = false;
    for (auto& layer : layers)
        if (strcmp(layer.layerName, validationLayer) == 0) { hasValidation = true; break; }
    if (!hasValidation)
        fprintf(stderr, "[Vulkan][ERROR] VK_LAYER_KHRONOS_validation is not available\n");

    VkDebugUtilsMessengerCreateInfoEXT dbgInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
    };
    VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "Vulkan", .apiVersion = VK_API_VERSION_1_4};
    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pNext = hasValidation ? &dbgInfo : nullptr, .pApplicationInfo = &appInfo,
        .enabledLayerCount = hasValidation ? 1u : 0u, .ppEnabledLayerNames = hasValidation ? &validationLayer : nullptr,
        .enabledExtensionCount = (uint32_t)instanceExts.size(), .ppEnabledExtensionNames = instanceExts.data(),
    };
    VkInstance instance = nullptr;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        fprintf(stderr, "[Vulkan][ERROR] vkCreateInstance failed\n");
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
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceVulkan14Features features14 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = &features13,
        .dynamicRenderingLocalRead = VK_TRUE,
    };
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features14,
    };
    const char* deviceExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,};
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
        fprintf(stderr, "[Vulkan][ERROR] vkCreateDevice failed\n");
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

    vk_load_library(instance, physical, device, queue, cmdBuffer, family);

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
