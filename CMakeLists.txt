cmake_minimum_required(VERSION 3.0...4.0)
project(OpenGL)

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
        -DOPENGL_IMPLEMENTATION
        -DVULKAN_IMPLEMENTATION
)

add_executable(OpenGL Example/default.cpp
        OpenGL.cpp
        OpenRT.cpp
        Vulkan.cpp
        Vulkan.h
)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(OpenGL PRIVATE SDL3::SDL3)
find_package(GLEW CONFIG REQUIRED)
target_link_libraries(OpenGL PRIVATE GLEW::GLEW)
find_package(Vulkan REQUIRED)
target_link_libraries(OpenGL PRIVATE Vulkan::Vulkan)
find_package(glm CONFIG REQUIRED)
target_link_libraries(OpenGL PRIVATE glm::glm)
find_package(Stb REQUIRED)
target_include_directories(OpenGL PRIVATE ${Stb_INCLUDE_DIR})

add_executable(Terrain Example/terrain.cpp
        OpenGL.cpp
        OpenRT.cpp
        Vulkan.cpp
        Vulkan.h
)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Terrain PRIVATE SDL3::SDL3)
find_package(GLEW CONFIG REQUIRED)
target_link_libraries(Terrain PRIVATE GLEW::GLEW)
find_package(Vulkan REQUIRED)
target_link_libraries(Terrain PRIVATE Vulkan::Vulkan)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Terrain PRIVATE glm::glm)
find_package(Stb REQUIRED)
target_include_directories(Terrain PRIVATE ${Stb_INCLUDE_DIR})