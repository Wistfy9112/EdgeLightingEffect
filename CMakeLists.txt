cmake_minimum_required(VERSION 3.10)
project(EdgeLightingEffect)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC OFF)

# Collect all source files
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

# Exclude any build output directories that may have been placed in src/
list(FILTER SRC_FILES EXCLUDE REGEX ".*/output/.*")

# GLFW (build from source)
add_subdirectory(external/src/glfw)

# GLAD
add_library(glad OBJECT external/src/glad.c)
target_include_directories(glad PUBLIC external/include)

# GLM (header-only, just need include path)

# Main executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

# Copy assets to the executable output directory (works for all configurations)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
