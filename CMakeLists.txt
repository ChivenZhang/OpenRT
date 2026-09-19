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

add_library(OpenRT STATIC
        OpenGL.cpp
        OpenRT.cpp
)
target_compile_definitions(OpenRT PRIVATE OPENRT_EXPORTS)
find_package(GLEW CONFIG REQUIRED)
target_link_libraries(OpenRT PRIVATE GLEW::GLEW)
find_package(Stb REQUIRED)
target_include_directories(OpenRT PRIVATE ${Stb_INCLUDE_DIR})

add_executable(Default Example/default.cpp)
add_dependencies(Default OpenRT)
target_link_libraries(Default PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Default PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Default PRIVATE glm::glm)

add_executable(Terrain Example/terrain.cpp)
add_dependencies(Terrain OpenRT)
target_link_libraries(Terrain PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Terrain PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Terrain PRIVATE glm::glm)

add_executable(Render Example/Render.cpp)
add_dependencies(Render OpenRT)
target_link_libraries(Render PRIVATE OpenRT)
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(Render PRIVATE SDL3::SDL3)
find_package(glm CONFIG REQUIRED)
target_link_libraries(Render PRIVATE glm::glm)
