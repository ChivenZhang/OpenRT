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

add_definitions(
#        -DOPENRT_STATIC
        -DOPENGL_IMPLEMENTATION
        -DVULKAN_IMPLEMENTATION
        -DDIRECTX_IMPLEMENTATION
#        -DMETAL_IMPLEMENTATION
#        -DWEBGPU_IMPLEMENTATION
)

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
find_package(GLEW CONFIG REQUIRED)
target_link_libraries(OpenRT PRIVATE GLEW::GLEW)
find_package(Vulkan REQUIRED)
target_link_libraries(OpenRT PRIVATE Vulkan::Vulkan)
if(WIN32)
target_link_libraries(OpenRT PRIVATE d3d12 dxgi)
endif()
if(APPLE)
target_include_directories(OpenRT PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp ${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp-extensions)
target_link_libraries(OpenRT PRIVATE "-framework Metal" "-framework MetalKit" "-framework AppKit" "-framework Foundation" "-framework QuartzCore")
endif()

add_executable(Default Example/Default.cpp)
add_dependencies(Default OpenRT)
target_link_libraries(Default PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Default PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Default PRIVATE glm::glm)

add_executable(Meshlet Example/Meshlet.cpp)
add_dependencies(Meshlet OpenRT)
target_link_libraries(Meshlet PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Meshlet PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Meshlet PRIVATE glm::glm)

add_executable(Render Example/Render.cpp)
add_dependencies(Render OpenRT)
target_link_libraries(Render PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Render PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Render PRIVATE glm::glm)

add_executable(Transfer Example/Transfer.cpp)
add_dependencies(Transfer OpenRT)
target_link_libraries(Transfer PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Transfer PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Transfer PRIVATE glm::glm)
