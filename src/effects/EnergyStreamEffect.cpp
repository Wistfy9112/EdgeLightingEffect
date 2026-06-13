#include "EdgeLightingEffect/EnergyStreamEffect.h"
#include "core/Shader.h"
#include "perimeter/Perimeter.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static const float PI = 3.14159265f;

struct LineVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct EnergyParticle {
    float t;
    float speed;
    int direction;
    float life;
    float maxLife;
    float size;
    float brightness;
    float radialOffset;
    glm::vec3 color;
    float phase;

    static const int MAX_TRAIL = 14;

private:
    glm::vec2 m_trail[MAX_TRAIL];
    int m_trailCount = 0;
    int m_trailHead = 0;

public:
    EnergyParticle() : t(0), speed(0), direction(1), life(0), maxLife(1),
                       size(4), brightness(1), radialOffset(0), phase(0) {}

    void pushTrail(glm::vec2 p) {
        m_trail[m_trailHead] = p;
        m_trailHead = (m_trailHead + 1) % MAX_TRAIL;
        if (m_trailCount < MAX_TRAIL) m_trailCount++;
    }

    int trailCount() const { return m_trailCount; }
    int trailCapacity() const { return MAX_TRAIL; }

    glm::vec2 getTrail(int i) const {
        int idx = (m_trailHead - m_trailCount + i + MAX_TRAIL) % MAX_TRAIL;
        return m_trail[idx];
    }
};

struct Comet {
    float t;
    int direction;
    glm::vec3 color;
    float brightness;
    float phase;

    static const int MAX_TRAIL = 120;

private:
    glm::vec2 m_trail[MAX_TRAIL];
    int m_trailCount = 0;
    int m_trailHead = 0;

public:
    Comet() : t(0), direction(1), color(1.0f), brightness(1.0f), phase(0) {}

    void pushTrail(glm::vec2 p) {
        m_trail[m_trailHead] = p;
        m_trailHead = (m_trailHead + 1) % MAX_TRAIL;
        if (m_trailCount < MAX_TRAIL) m_trailCount++;
    }

    int trailCount() const { return m_trailCount; }
    int trailCapacity() const { return MAX_TRAIL; }

    glm::vec2 getTrail(int i) const {
        int idx = (m_trailHead - m_trailCount + i + MAX_TRAIL) % MAX_TRAIL;
        return m_trail[idx];
    }
};

static const glm::vec3 COMET_PALETTE[6] = {
    glm::vec3(0.3f, 0.8f, 1.0f),
    glm::vec3(1.0f, 0.7f, 0.2f),
    glm::vec3(1.0f, 0.3f, 0.8f),
    glm::vec3(0.5f, 1.0f, 0.3f),
    glm::vec3(1.0f, 0.5f, 0.1f),
    glm::vec3(0.7f, 0.3f, 1.0f)
};

struct EnergyStreamEffect::Impl {
    Shader lineShader;
    Shader particleShader;
    glm::mat4 projection;

    int fbWidth = 0, fbHeight = 0;
    float elapsedTime = 0.0f;

    float speed = 1.0f;
    float intensity = 1.0f;
    int targetCount = 400;
    float glowIntensity = 1.0f;
    float cornerRadius = 40.0f;
    float lineWidth = 4.0f;
    float perimeterW = 0.0f, perimeterH = 0.0f;
    float offsetX = 0.0f, offsetY = 0.0f;
    glm::vec3 baseColor = glm::vec3(0.3f, 0.8f, 1.0f);

    Perimeter* perimeter = nullptr;

    struct Stream {
        int direction;
        std::vector<EnergyParticle> particles;
        float emitAccum = 0.0f;
    };
    Stream streamCW;
    Stream streamCCW;

    std::vector<Comet> comets;

    unsigned int particleVAO = 0, particleVBO = 0;
    int particleVertexCount = 0;
    unsigned int glowVAO = 0, glowVBO = 0;
    int glowVertexCount = 0;
    unsigned int trailVAO = 0, trailVBO = 0;
    int trailVertexCount = 0;

    float randFloat() const {
        return static_cast<float>(std::rand()) / RAND_MAX;
    }

    ~Impl() {
        delete perimeter;
        if (particleVAO) glDeleteVertexArrays(1, &particleVAO);
        if (particleVBO) glDeleteBuffers(1, &particleVBO);
        if (glowVAO) glDeleteVertexArrays(1, &glowVAO);
        if (glowVBO) glDeleteBuffers(1, &glowVBO);
        if (trailVAO) glDeleteVertexArrays(1, &trailVAO);
        if (trailVBO) glDeleteBuffers(1, &trailVBO);
    }

    void updateProjection() {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        projection = glm::ortho(-w * 0.5f + offsetX, w * 0.5f + offsetX,
                                -h * 0.5f + offsetY, h * 0.5f + offsetY);
    }

    void rebuildPerimeter() {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        float pw = perimeterW > 0.0f ? perimeterW : w * 0.9f;
        float ph = perimeterH > 0.0f ? perimeterH : h * 0.9f;
        delete perimeter;
        perimeter = new Perimeter(pw, ph, cornerRadius);
    }

    glm::vec2 getPosition(float t) const {
        return perimeter->getPointAtProgress(t).position;
    }

    glm::vec3 randomColor() const {
        float r = randFloat();
        if (r < 0.4f) {
            return glm::mix(baseColor, glm::vec3(0.6f, 1.0f, 1.0f), randFloat());
        } else if (r < 0.6f) {
            return glm::mix(baseColor, glm::vec3(1.0f), 0.7f + randFloat() * 0.3f);
        }
        return baseColor;
    }

    void emitParticle(Stream& stream) {
        EnergyParticle p;
        p.t = randFloat();
        p.direction = stream.direction;

        float baseSpeed = (0.08f + randFloat() * 0.15f) * speed;
        p.speed = baseSpeed;

        p.maxLife = 2.0f + randFloat() * 3.0f;
        p.life = p.maxLife;

        p.size = 1.5f + randFloat() * 6.0f;
        p.brightness = 0.4f + randFloat() * 0.6f;
        p.radialOffset = (randFloat() - 0.5f) * 12.0f;
        p.phase = randFloat() * PI * 2.0f;
        p.color = randomColor();

        glm::vec2 pos = getPosition(p.t) + glm::normalize(getPosition(p.t)) * p.radialOffset;
        p.pushTrail(pos);
        stream.particles.push_back(p);
    }

    void updateStream(Stream& stream, float dt) {
        float emitRate = targetCount * 0.3f;
        stream.emitAccum += emitRate * dt * intensity;
        while (stream.emitAccum >= 1.0f && stream.particles.size() < static_cast<size_t>(targetCount * 0.5f)) {
            emitParticle(stream);
            stream.emitAccum -= 1.0f;
        }

        for (int i = 0; i < static_cast<int>(stream.particles.size()); i++) {
            EnergyParticle& p = stream.particles[i];
            p.life -= dt;
            if (p.life <= 0.0f) {
                stream.particles[i] = stream.particles.back();
                stream.particles.pop_back();
                i--;
                continue;
            }

            float lifeRatio = p.life / p.maxLife;

            float speedPulse = 0.8f + 0.2f * sin(elapsedTime * 2.0f + p.phase);
            float tStep = p.speed * speedPulse * dt * static_cast<float>(p.direction);
            p.t += tStep;
            if (p.t > 1.0f) p.t -= 1.0f;
            if (p.t < 0.0f) p.t += 1.0f;

            p.radialOffset += sin(elapsedTime * (1.5f + p.phase * 0.5f) + p.phase) * 8.0f * dt;
            p.radialOffset *= 0.98f;

            glm::vec2 base = getPosition(p.t);
            glm::vec2 normal = glm::normalize(base);
            glm::vec2 pos = base + normal * p.radialOffset;

            if (randFloat() < 0.015f) {
                p.radialOffset += (randFloat() - 0.5f) * 20.0f;
            }

            p.pushTrail(pos);

            float pulse = 0.6f + 0.4f * sin(elapsedTime * (1.5f + p.phase * 0.5f) + p.phase);
            float fadeIn = std::min(lifeRatio * 4.0f, 1.0f);
            p.brightness = (0.4f + 0.6f * lifeRatio) * pulse * fadeIn;
        }
    }

    void updateComets(float dt) {
        for (size_t i = 0; i < comets.size(); i++) {
            Comet& c = comets[i];
            float tStep = speed * 0.25f * dt * static_cast<float>(c.direction);
            c.t += tStep;
            if (c.t > 1.0f) c.t -= 1.0f;
            if (c.t < 0.0f) c.t += 1.0f;
            glm::vec2 pos = getPosition(c.t);
            c.pushTrail(pos);
            float pulse = 0.6f + 0.4f * sin(elapsedTime * 2.0f + c.phase);
            c.brightness = (0.9f + 0.1f * pulse) * intensity;
        }
    }

    void buildStreamTrailGeometry(std::vector<LineVertex>& vertices, bool clear) {
        if (clear) vertices.clear();

        auto addTrail = [&](const EnergyParticle& p) {
            int count = p.trailCount();
            if (count < 2) return;

            for (int i = 0; i < count - 1; i++) {
                float t = static_cast<float>(i) / (count - 1);
                float t1 = static_cast<float>(i + 1) / (count - 1);

                float alpha0 = (1.0f - t) * p.brightness * 0.5f;
                float alpha1 = (1.0f - t1) * p.brightness * 0.5f;

                if (alpha0 < 0.01f && alpha1 < 0.01f) continue;

                glm::vec2 p0 = p.getTrail(i);
                glm::vec2 p1 = p.getTrail(i + 1);
                glm::vec2 diff = p1 - p0;
                if (glm::length(diff) < 0.001f) continue;
                glm::vec2 dir = diff / glm::length(diff);
                glm::vec2 perp(-dir.y, dir.x);

                float trailW = lineWidth * 0.5f + p.size * 0.2f;
                glm::vec3 col = p.color * p.brightness;

                LineVertex v[6];
                v[0] = { p0.x - perp.x * trailW, p0.y - perp.y * trailW, 0, -1, col.r, col.g, col.b, alpha0 };
                v[1] = { p0.x + perp.x * trailW, p0.y + perp.y * trailW, 0,  1, col.r, col.g, col.b, alpha0 };
                v[2] = { p1.x - perp.x * trailW, p1.y - perp.y * trailW, 0, -1, col.r, col.g, col.b, alpha1 };
                v[3] = { p1.x - perp.x * trailW, p1.y - perp.y * trailW, 0, -1, col.r, col.g, col.b, alpha1 };
                v[4] = { p0.x + perp.x * trailW, p0.y + perp.y * trailW, 0,  1, col.r, col.g, col.b, alpha0 };
                v[5] = { p1.x + perp.x * trailW, p1.y + perp.y * trailW, 0,  1, col.r, col.g, col.b, alpha1 };

                vertices.insert(vertices.end(), v, v + 6);
            }
        };

        for (const auto& p : streamCW.particles)
            addTrail(p);
        for (const auto& p : streamCCW.particles)
            addTrail(p);
    }

    void buildStreamParticleQuads(std::vector<LineVertex>& vertices, bool clear, float sizeScale, float alphaMul, float brightnessMul) {
        if (clear) vertices.clear();

        auto addQuad = [&](const EnergyParticle& p) {
            float alpha = std::min(p.life / p.maxLife * 2.5f, 1.0f) * alphaMul;
            if (alpha < 0.01f) return;

            glm::vec3 col = p.color * (p.brightness * brightnessMul);
            float halfSize = p.size * sizeScale * 0.5f;

            glm::vec2 pos = getPosition(p.t);
            glm::vec2 normal = glm::normalize(pos);
            pos = pos + normal * p.radialOffset;

            float x = pos.x, y = pos.y;

            LineVertex v[6];
            v[0] = { x - halfSize, y - halfSize, -1, -1, col.r, col.g, col.b, alpha };
            v[1] = { x + halfSize, y - halfSize,  1, -1, col.r, col.g, col.b, alpha };
            v[2] = { x - halfSize, y + halfSize, -1,  1, col.r, col.g, col.b, alpha };
            v[3] = { x + halfSize, y - halfSize,  1, -1, col.r, col.g, col.b, alpha };
            v[4] = { x - halfSize, y + halfSize, -1,  1, col.r, col.g, col.b, alpha };
            v[5] = { x + halfSize, y + halfSize,  1,  1, col.r, col.g, col.b, alpha };

            vertices.insert(vertices.end(), v, v + 6);
        };

        for (const auto& p : streamCW.particles)
            addQuad(p);
        for (const auto& p : streamCCW.particles)
            addQuad(p);
    }

    void buildCometTrailGeometry(std::vector<LineVertex>& vertices, bool clear) {
        if (clear) vertices.clear();

        for (size_t ci = 0; ci < comets.size(); ci++) {
            const Comet& c = comets[ci];
            int count = c.trailCount();
            if (count < 2) continue;

            for (int i = 0; i < count - 1; i++) {
                float headness0 = static_cast<float>(i) / (count - 1);
                float headness1 = static_cast<float>(i + 1) / (count - 1);

                float alpha0 = headness0 * headness0 * c.brightness * 1.2f;
                float alpha1 = headness1 * headness1 * c.brightness * 1.2f;

                if (alpha0 < 0.005f && alpha1 < 0.005f) continue;

                glm::vec2 p0 = c.getTrail(i);
                glm::vec2 p1 = c.getTrail(i + 1);

                glm::vec2 diff = p1 - p0;
                float segLen = glm::length(diff);
                if (segLen < 0.001f) continue;
                glm::vec2 dir = diff / segLen;
                glm::vec2 perp(-dir.y, dir.x);

                float spread0 = 1.0f + (1.0f - headness0) * 8.0f;
                float spread1 = 1.0f + (1.0f - headness1) * 8.0f;
                float w0 = lineWidth * spread0;
                float w1 = lineWidth * spread1;

                glm::vec3 col = c.color * c.brightness;

                LineVertex v[6];
                v[0] = { p0.x - perp.x * w0, p0.y - perp.y * w0, 0, -1, col.r, col.g, col.b, alpha0 };
                v[1] = { p0.x + perp.x * w0, p0.y + perp.y * w0, 0,  1, col.r, col.g, col.b, alpha0 };
                v[2] = { p1.x - perp.x * w1, p1.y - perp.y * w1, 0, -1, col.r, col.g, col.b, alpha1 };
                v[3] = { p1.x - perp.x * w1, p1.y - perp.y * w1, 0, -1, col.r, col.g, col.b, alpha1 };
                v[4] = { p0.x + perp.x * w0, p0.y + perp.y * w0, 0,  1, col.r, col.g, col.b, alpha0 };
                v[5] = { p1.x + perp.x * w1, p1.y + perp.y * w1, 0,  1, col.r, col.g, col.b, alpha1 };

                vertices.insert(vertices.end(), v, v + 6);
            }
        }
    }

    void buildCometCoreQuads(std::vector<LineVertex>& vertices, bool clear, float sizeMul, float alphaMul) {
        if (clear) vertices.clear();

        for (size_t ci = 0; ci < comets.size(); ci++) {
            const Comet& c = comets[ci];
            float alpha = c.brightness * alphaMul;
            if (alpha < 0.01f) continue;

            float halfSize = 10.0f * sizeMul;
            glm::vec2 pos = getPosition(c.t);
            glm::vec3 col = c.color * (c.brightness * 2.0f);

            float x = pos.x, y = pos.y;

            LineVertex v[6];
            v[0] = { x - halfSize, y - halfSize, -1, -1, col.r, col.g, col.b, alpha };
            v[1] = { x + halfSize, y - halfSize,  1, -1, col.r, col.g, col.b, alpha };
            v[2] = { x - halfSize, y + halfSize, -1,  1, col.r, col.g, col.b, alpha };
            v[3] = { x + halfSize, y - halfSize,  1, -1, col.r, col.g, col.b, alpha };
            v[4] = { x - halfSize, y + halfSize, -1,  1, col.r, col.g, col.b, alpha };
            v[5] = { x + halfSize, y + halfSize,  1,  1, col.r, col.g, col.b, alpha };

            vertices.insert(vertices.end(), v, v + 6);
        }
    }

    void setupVAO(unsigned int vao, unsigned int vbo) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
};

EnergyStreamEffect::EnergyStreamEffect()
    : m_impl(new Impl()) {
    m_impl->streamCW.direction = 1;
    m_impl->streamCCW.direction = -1;
}

EnergyStreamEffect::~EnergyStreamEffect() { delete m_impl; }

bool EnergyStreamEffect::init(int fbWidth, int fbHeight) {
    auto& p = *m_impl;
    p.fbWidth = fbWidth;
    p.fbHeight = fbHeight;

    if (!p.lineShader.load("assets/shaders/line.vert", "assets/shaders/line.frag")) {
        std::cerr << "Failed to load line shaders" << std::endl;
        return false;
    }
    if (!p.particleShader.load("assets/shaders/particle.vert", "assets/shaders/particle.frag")) {
        std::cerr << "Failed to load particle shaders" << std::endl;
        return false;
    }

    p.updateProjection();
    p.rebuildPerimeter();

    for (size_t i = 0; i < p.comets.size(); i++) {
        glm::vec2 pos = p.getPosition(p.comets[i].t);
        for (int j = 0; j < p.comets[i].MAX_TRAIL; j++) {
            p.comets[i].pushTrail(pos);
        }
    }

    glGenVertexArrays(1, &p.particleVAO);
    glGenBuffers(1, &p.particleVBO);
    p.setupVAO(p.particleVAO, p.particleVBO);

    glGenVertexArrays(1, &p.glowVAO);
    glGenBuffers(1, &p.glowVBO);
    p.setupVAO(p.glowVAO, p.glowVBO);

    glGenVertexArrays(1, &p.trailVAO);
    glGenBuffers(1, &p.trailVBO);
    p.setupVAO(p.trailVAO, p.trailVBO);

    return true;
}

void EnergyStreamEffect::update(float deltaTime) {
    auto& p = *m_impl;
    p.elapsedTime += deltaTime;

    p.updateStream(p.streamCW, deltaTime);
    p.updateStream(p.streamCCW, deltaTime);
    p.updateComets(deltaTime);

    std::vector<LineVertex> vertices;

    p.buildStreamTrailGeometry(vertices, true);
    p.buildCometTrailGeometry(vertices, false);
    p.trailVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.trailVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);

    p.buildStreamParticleQuads(vertices, true, 4.0f, 0.15f * p.glowIntensity, 0.6f);
    p.buildCometCoreQuads(vertices, false, 4.0f, 0.15f * p.glowIntensity);
    p.glowVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.glowVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);

    p.buildStreamParticleQuads(vertices, true, 1.0f, 1.0f, 1.0f);
    p.buildCometCoreQuads(vertices, false, 1.0f, 1.0f);
    p.particleVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.particleVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);
}

void EnergyStreamEffect::render() {
    auto& p = *m_impl;

    glEnable(GL_BLEND);

    if (p.trailVertexCount > 0) {
        p.lineShader.use();
        p.lineShader.setMat4("uProjection", p.projection);
        p.lineShader.setFloat("uAlphaScale", 0.6f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindVertexArray(p.trailVAO);
        glDrawArrays(GL_TRIANGLES, 0, p.trailVertexCount);
    }

    if (p.glowVertexCount > 0) {
        p.particleShader.use();
        p.particleShader.setMat4("uProjection", p.projection);
        p.particleShader.setFloat("uAlphaScale", 1.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindVertexArray(p.glowVAO);
        glDrawArrays(GL_TRIANGLES, 0, p.glowVertexCount);
    }

    if (p.particleVertexCount > 0) {
        p.particleShader.setFloat("uAlphaScale", 1.2f);
        glBindVertexArray(p.particleVAO);
        glDrawArrays(GL_TRIANGLES, 0, p.particleVertexCount);
    }
}

void EnergyStreamEffect::onResize(int fbWidth, int fbHeight) {
    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;
    m_impl->updateProjection();
    m_impl->rebuildPerimeter();
}

void EnergyStreamEffect::setSpeed(float speed) { m_impl->speed = speed; }
void EnergyStreamEffect::setIntensity(float intensity) { m_impl->intensity = intensity; }
void EnergyStreamEffect::setParticleCount(int count) { m_impl->targetCount = std::max(10, count); }
void EnergyStreamEffect::setGlowIntensity(float intensity) { m_impl->glowIntensity = intensity; }
void EnergyStreamEffect::setCornerRadius(float radius) {
    m_impl->cornerRadius = radius;
    m_impl->rebuildPerimeter();
}
void EnergyStreamEffect::setLineWidth(float width) { m_impl->lineWidth = width; }
void EnergyStreamEffect::setPosition(float x, float y) {
    m_impl->offsetX = x;
    m_impl->offsetY = y;
    m_impl->updateProjection();
}
void EnergyStreamEffect::setSize(float width, float height) {
    m_impl->perimeterW = width;
    m_impl->perimeterH = height;
    m_impl->rebuildPerimeter();
}

void EnergyStreamEffect::setCometCount(int count) {
    if (count < 0) count = 0;
    if (count > 6) count = 6;
    auto& p = *m_impl;
    p.comets.resize(count);
    for (int i = 0; i < count; i++) {
        p.comets[i] = Comet();
        p.comets[i].t = static_cast<float>(i) / count;
        p.comets[i].direction = 1;
        p.comets[i].phase = 0.0f;
        p.comets[i].color = COMET_PALETTE[i % 6];
        if (p.perimeter) {
            glm::vec2 pos = p.getPosition(p.comets[i].t);
            for (int j = 0; j < p.comets[i].MAX_TRAIL; j++) {
                p.comets[i].pushTrail(pos);
            }
        }
    }
}

void EnergyStreamEffect::setCometColor(int index, float r, float g, float b) {
    auto& p = *m_impl;
    if (index >= 0 && index < static_cast<int>(p.comets.size())) {
        p.comets[index].color = glm::vec3(r, g, b);
    }
}

int EnergyStreamEffect::getCometCount() const {
    return static_cast<int>(m_impl->comets.size());
}
