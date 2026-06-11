#ifndef EDGE_LIGHTING_EFFECT_H
#define EDGE_LIGHTING_EFFECT_H

#include <vector>
#include <glm/glm.hpp>
#include "core/Shader.h"
#include "core/Application.h"
#include "perimeter/Perimeter.h"

struct LineVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

class EdgeLightingEffect {
public:
    EdgeLightingEffect();
    ~EdgeLightingEffect();

    bool init(Application* app);
    void update(float deltaTime);
    void render(float deltaTime);

private:
    Application* m_app;
    Perimeter* m_perimeter;

    Shader m_lineShader;

    glm::mat4 m_projection;

    float m_elapsedTime;

    // Notification system
    float m_notifTimer;
    float m_shakeIntensity;
    float m_pulseIntensity;

    // Pulsing animation
    float m_baseGlowWidth;
    float m_baseCoreWidth;
    float m_glowAlphaScale;

    // Line geometry
    unsigned int m_coreVAO, m_coreVBO;
    int m_coreVertexCount;
    unsigned int m_glowVAO, m_glowVBO;
    int m_glowVertexCount;

    glm::vec4 getGradientColor(float t, float time, float pulse) const;
    void buildLineGeometry(float lineWidth, std::vector<LineVertex>& vertices);
    void updateProjection();
};

#endif