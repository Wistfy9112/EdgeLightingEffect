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

add_library(EdgePulseEffectLib STATIC ${LIB_SRC_FILES})
target_include_directories(EdgePulseEffectLib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_include_directories(EdgePulseEffectLib PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/include
)
target_link_libraries(EdgePulseEffectLib PUBLIC glfw glad)
if(WIN32)
    target_link_libraries(EdgePulseEffectLib PUBLIC opengl32)
endif()

# Demo executable
add_executable(EdgePulseEffectDemo src/main.cpp)
target_include_directories(EdgePulseEffectDemo PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/include
)
target_link_libraries(EdgePulseEffectDemo PRIVATE EdgePulseEffectLib)

# Copy assets to the demo executable output directory
add_custom_command(TARGET EdgePulseEffectDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:EdgePulseEffectDemo>/assets
)
