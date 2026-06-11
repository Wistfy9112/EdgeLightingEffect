cmake_minimum_required(VERSION 3.10)
project(EdgeLightingDemo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/EdgeLightingEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perimeter/Perimeter.cpp
)

# Add external include paths
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external)

# Main executable
add_executable(${PROJECT_NAME} ${SRC_FILES}
    ${EXTERNAL_DIR}/src/glad.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EXTERNAL_DIR}/include
)

# Build GLFW from source
add_subdirectory(${EXTERNAL_DIR}/src/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

# Copy shaders
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../assets DESTINATION ${CMAKE_BINARY_DIR})
