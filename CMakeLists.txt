cmake_minimum_required(VERSION 3.0...4.0)
project(OpenRT)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    link_directories(${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib)
else()
    link_directories(${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib)
endif()

# add_definitions(-DOPENRT_STATIC)

if(APPLE)
add_definitions(-DMETAL_IMPLEMENTATION)
elseif(WIN32)
add_definitions(-DDIRECTX_IMPLEMENTATION)
elseif(EMSCRIPTEN)
add_definitions(-DWEBGPU_IMPLEMENTATION)
else()
add_definitions(-DOPENGL_IMPLEMENTATION)
add_definitions(-DVULKAN_IMPLEMENTATION)
endif()

add_library(OpenRT STATIC
        OpenRT.cpp
        OpenGL.cpp
        Vulkan.cpp
        DirectX.cpp
        Metal.cpp
        WebGPU.cpp
)
target_compile_definitions(OpenRT PUBLIC OPENRT_EXPORTS)
find_package(Stb REQUIRED)
target_include_directories(OpenRT PRIVATE ${Stb_INCLUDE_DIR})
if(EMSCRIPTEN)
target_compile_options(OpenRT PRIVATE "SHELL:-sUSE_WEBGPU=1")
else()
find_package(GLEW CONFIG REQUIRED)
target_link_libraries(OpenRT PRIVATE GLEW::GLEW)
find_package(Vulkan REQUIRED)
target_link_libraries(OpenRT PRIVATE Vulkan::Vulkan)
if(WIN32)
target_link_libraries(OpenRT PRIVATE d3d12 dxgi)
endif()
if(APPLE)
target_include_directories(OpenRT PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp)
target_link_libraries(OpenRT PRIVATE "-framework Metal" "-framework MetalKit" "-framework AppKit" "-framework Foundation" "-framework QuartzCore")
endif()
endif()

if(NOT EMSCRIPTEN)
#=======================OpenGL==========================

add_executable(OpenGL-Default Example/OpenGL/Default.cpp)
add_dependencies(OpenGL-Default OpenRT)
target_link_libraries(OpenGL-Default PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(OpenGL-Default PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(OpenGL-Default PRIVATE glm::glm)

add_executable(OpenGL-Meshlet Example/OpenGL/Meshlet.cpp)
add_dependencies(OpenGL-Meshlet OpenRT)
target_link_libraries(OpenGL-Meshlet PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(OpenGL-Meshlet PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(OpenGL-Meshlet PRIVATE glm::glm)

add_executable(OpenGL-Render Example/OpenGL/Render.cpp)
add_dependencies(OpenGL-Render OpenRT)
target_link_libraries(OpenGL-Render PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(OpenGL-Render PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(OpenGL-Render PRIVATE glm::glm)

add_executable(OpenGL-Transfer Example/OpenGL/Transfer.cpp)
add_dependencies(OpenGL-Transfer OpenRT)
target_link_libraries(OpenGL-Transfer PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(OpenGL-Transfer PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(OpenGL-Transfer PRIVATE glm::glm)

#=======================Vulkan==========================

add_executable(Vulkan-Default Example/Vulkan/Default.cpp)
add_dependencies(Vulkan-Default OpenRT)
target_link_libraries(Vulkan-Default PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Vulkan-Default PRIVATE SDL3::SDL3)
target_link_libraries(Vulkan-Default PRIVATE Vulkan::Vulkan)

add_executable(Vulkan-Render Example/Vulkan/Render.cpp)
add_dependencies(Vulkan-Render OpenRT)
target_link_libraries(Vulkan-Render PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Vulkan-Render PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Vulkan-Render PRIVATE glm::glm)
target_link_libraries(Vulkan-Render PRIVATE Vulkan::Vulkan)

#=======================DirectX==========================

if(WIN32)
add_executable(DirectX-Default Example/DirectX/Default.cpp)
add_dependencies(DirectX-Default OpenRT)
target_link_libraries(DirectX-Default PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(DirectX-Default PRIVATE SDL3::SDL3)
target_link_libraries(DirectX-Default PRIVATE d3d12 dxgi d3dcompiler)

add_executable(DirectX-Render Example/DirectX/Render.cpp)
add_dependencies(DirectX-Render OpenRT)
target_link_libraries(DirectX-Render PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(DirectX-Render PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(DirectX-Render PRIVATE glm::glm)
target_link_libraries(DirectX-Render PRIVATE d3d12 dxgi d3dcompiler)
endif()

#=======================Metal==========================

if(APPLE)
target_compile_definitions(OpenRT PRIVATE METAL_IMPLEMENTATION)
add_executable(Metal-Default Example/Metal/Default.cpp)
add_dependencies(Metal-Default OpenRT)
target_compile_definitions(Metal-Default PRIVATE METAL_IMPLEMENTATION)
target_include_directories(Metal-Default PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp)
target_link_libraries(Metal-Default PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Metal-Default PRIVATE SDL3::SDL3)
target_link_libraries(Metal-Default PRIVATE "-framework Metal" "-framework QuartzCore" "-framework Foundation")

add_executable(Metal-Render Example/Metal/Render.cpp)
add_dependencies(Metal-Render OpenRT)
target_compile_definitions(Metal-Render PRIVATE METAL_IMPLEMENTATION)
target_include_directories(Metal-Render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp)
target_link_libraries(Metal-Render PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Metal-Render PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Metal-Render PRIVATE glm::glm)
target_link_libraries(Metal-Render PRIVATE "-framework Metal" "-framework QuartzCore" "-framework Foundation")
endif()
endif()

#=======================WebGPU==========================

if(EMSCRIPTEN)
add_executable(WebGPU-Default Example/WebGPU/Default.cpp)
add_dependencies(WebGPU-Default OpenRT)
target_compile_definitions(WebGPU-Default PRIVATE WEBGPU_IMPLEMENTATION)
target_include_directories(WebGPU-Default PRIVATE ${Stb_INCLUDE_DIR})
target_compile_options(WebGPU-Default PRIVATE "SHELL:-sUSE_SDL=3" "SHELL:-sUSE_WEBGPU=1")
target_link_libraries(WebGPU-Default PRIVATE OpenRT)
target_link_options(WebGPU-Default PRIVATE
    "SHELL:-sUSE_SDL=3"
    "SHELL:-sUSE_WEBGPU=1"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
)
set_target_properties(WebGPU-Default PROPERTIES SUFFIX ".html")
endif()
