#ifndef SHADER_H
#define SHADER_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader {
public:
    Shader();
    ~Shader();

    bool load(const char* vertexPath, const char* fragmentPath);
    void use() const;

    void setMat4(const char* name, const glm::mat4& matrix) const;
    void setVec4(const char* name, const glm::vec4& value) const;
    void setVec3(const char* name, const glm::vec3& value) const;
    void setFloat(const char* name, float value) const;
    void setInt(const char* name, int value) const;

    unsigned int getId() const { return m_id; }

private:
    unsigned int m_id;

    std::string readFile(const char* path) const;
    bool checkCompileErrors(unsigned int shader, const char* type) const;
};

#endif
