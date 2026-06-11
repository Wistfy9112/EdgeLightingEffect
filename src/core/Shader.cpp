#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>

Shader::Shader() : m_id(0) {}

Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

std::string Shader::readFile(const char* path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool Shader::checkCompileErrors(unsigned int shader, const char* type) const {
    int success;
    char infoLog[1024];
    if (std::string(type) == "PROGRAM") {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "Program link error: " << infoLog << std::endl;
            return false;
        }
    } else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "Shader compile error (" << type << "): " << infoLog << std::endl;
            return false;
        }
    }
    return true;
}

bool Shader::load(const char* vertexPath, const char* fragmentPath) {
    std::string vertCode = readFile(vertexPath);
    std::string fragCode = readFile(fragmentPath);
    if (vertCode.empty() || fragCode.empty()) return false;

    const char* vCode = vertCode.c_str();
    const char* fCode = fragCode.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vCode, nullptr);
    glCompileShader(vertex);
    if (!checkCompileErrors(vertex, "VERTEX")) return false;

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fCode, nullptr);
    glCompileShader(fragment);
    if (!checkCompileErrors(fragment, "FRAGMENT")) return false;

    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    glLinkProgram(m_id);
    if (!checkCompileErrors(m_id, "PROGRAM")) return false;

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return true;
}

void Shader::use() const {
    glUseProgram(m_id);
}

void Shader::setMat4(const char* name, const glm::mat4& matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::setVec4(const char* name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(m_id, name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(m_id, name), 1, glm::value_ptr(value));
}

void Shader::setFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(m_id, name), value);
}

void Shader::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(m_id, name), value);
}
