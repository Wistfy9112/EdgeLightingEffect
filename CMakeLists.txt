cmake_minimum_required(VERSION 3.10)
project(EdgeLightingEffect)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)

# GLFW (build from source)
add_subdirectory(external/src/glfw)

# GLAD
add_library(glad OBJECT external/src/glad.c)
target_include_directories(glad PUBLIC external/include)

# Library sources (exclude main.cpp)
file(GLOB LIB_SRC_FILES
    src/EffectManager.cpp
    src/effects/*.cpp
    src/core/*.cpp
    src/perimeter/*.cpp
)

add_library(EdgeLightingEffectLib STATIC ${LIB_SRC_FILES})
target_include_directories(EdgeLightingEffectLib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_include_directories(EdgeLightingEffectLib PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/include
)
target_link_libraries(EdgeLightingEffectLib PUBLIC glfw glad)
if(WIN32)
    target_link_libraries(EdgeLightingEffectLib PUBLIC opengl32)
endif()

# Demo executable
add_executable(EdgeLightingEffectDemo src/main.cpp)
target_include_directories(EdgeLightingEffectDemo PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/include
)
target_link_libraries(EdgeLightingEffectDemo PRIVATE EdgeLightingEffectLib)

# Copy assets to the demo executable output directory
add_custom_command(TARGET EdgeLightingEffectDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:EdgeLightingEffectDemo>/assets
)