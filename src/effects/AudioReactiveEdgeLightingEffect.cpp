#include "EdgeLightingEffect/AudioReactiveEdgeLightingEffect.h"
#include "core/Shader.h"
#include "perimeter/Perimeter.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static const float PI = 3.14159265f;

struct LineVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct Particle {
    glm::vec2 pos;
    glm::vec2 vel;
    float life;
    float maxLife;
    glm::vec3 color;
    float size;
};

// One-pole low-pass filter
struct SmoothFloat {
    float value = 0.0f;
    float target = 0.0f;
    float coeff = 0.2f;

    void setTarget(float t, float c = -1.0f) {
        target = t;
        if (c >= 0.0f) coeff = c;
    }

    float update() {
        value += (target - value) * coeff;
        return value;
    }
};

struct AudioReactiveEdgeLightingEffect::Impl {
    // Core
    Shader lineShader;
    Shader particleShader;
    glm::mat4 projection;
    int fbWidth = 0, fbHeight = 0;
    float elapsedTime = 0.0f;

    // Perimeter
    Perimeter* perimeter = nullptr;
    float cornerRadius = 40.0f;
    float perimeterW = 0.0f, perimeterH = 0.0f;

    // Raw audio input
    float bass = 0.0f;
    float mid = 0.0f;
    float treble = 0.0f;
    float volume = 0.0f;
    bool kick = false;
    float kickDecay = 0.0f;

    // Smoothed audio
    SmoothFloat bassSmooth;
    SmoothFloat midSmooth;
    SmoothFloat trebleSmooth;
    SmoothFloat volumeSmooth;
    SmoothFloat bassEnergy;

    // Spectrum
    std::vector<float> spectrum;
    std::vector<float> specSmoothed;

    // Visual parameters
    float lineWidth = 6.0f;
    float glowIntensity = 1.0f;
    float sensitivity = 1.0f;

    // Smooth visual state
    SmoothFloat displacement;
    SmoothFloat glowWidth;
    SmoothFloat glowAlphaMul;

    // VAOs / VBOs
    unsigned int coreVAO = 0, coreVBO = 0;
    int coreVertexCount = 0;
    unsigned int glowVAO = 0, glowVBO = 0;
    int glowVertexCount = 0;
    unsigned int midGlowVAO = 0, midGlowVBO = 0;
    int midGlowVertexCount = 0;
    unsigned int specVAO = 0, specVBO = 0;
    int specVertexCount = 0;
    unsigned int particleVAO = 0, particleVBO = 0;
    int particleVertexCount = 0;

    // Trail particles (fall from top edge inside rectangle)
    std::vector<Particle> trailParticles;
    float trailEmitAccum = 0.0f;

    // Spectrum particles (spark up from bottom edge bars)
    std::vector<Particle> spectrumParticles;

    int maxParticles = 3000;

    // Flow
    float flowPhase = 0.0f;

    // Energy bursts traveling along perimeter
    struct EnergyBurst {
        float pos;
        float speed;
        float intensity;
        float life;
    };
    std::vector<EnergyBurst> bursts;

    ~Impl() {
        delete perimeter;
        if (coreVAO) glDeleteVertexArrays(1, &coreVAO);
        if (coreVBO) glDeleteBuffers(1, &coreVBO);
        if (glowVAO) glDeleteVertexArrays(1, &glowVAO);
        if (glowVBO) glDeleteBuffers(1, &glowVBO);
        if (midGlowVAO) glDeleteVertexArrays(1, &midGlowVAO);
        if (midGlowVBO) glDeleteBuffers(1, &midGlowVBO);
        if (specVAO) glDeleteVertexArrays(1, &specVAO);
        if (specVBO) glDeleteBuffers(1, &specVBO);
        if (particleVAO) glDeleteVertexArrays(1, &particleVAO);
        if (particleVBO) glDeleteBuffers(1, &particleVBO);
    }

    void updateProjection() {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        projection = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);
    }

    void rebuildPerimeter() {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        float pw = perimeterW > 0.0f ? perimeterW : w * 0.9f;
        float ph = perimeterH > 0.0f ? perimeterH : h * 0.9f;
        delete perimeter;
        perimeter = new Perimeter(pw, ph, cornerRadius);
        perimeterW = pw;
        perimeterH = ph;
    }

    // Gradient for perimeter (unchanged)
    glm::vec3 getGradientColor(float t) const {
        float shiftedT = fmod(t + elapsedTime * 0.04f, 1.0f);
        if (shiftedT < 0.0f) shiftedT += 1.0f;

        struct Stop { float pos; glm::vec3 color; };
        static const Stop stops[] = {
            {0.00f, glm::vec3(0.0f, 0.8f, 1.0f)},
            {0.33f, glm::vec3(0.0f, 0.3f, 1.0f)},
            {0.66f, glm::vec3(0.4f, 0.0f, 0.8f)},
            {1.00f, glm::vec3(0.0f, 0.8f, 1.0f)},
        };

        if (shiftedT <= stops[0].pos) return stops[0].color;
        if (shiftedT >= stops[3].pos) return stops[3].color;
        for (int i = 0; i < 3; i++) {
            if (shiftedT >= stops[i].pos && shiftedT < stops[i + 1].pos) {
                float s = (shiftedT - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
                return glm::mix(stops[i].color, stops[i + 1].color, s);
            }
        }
        return stops[0].color;
    }

    // Gradient for spectrum: Purple → Blue → Pink → Purple (slow cycle ~6s)
    glm::vec3 getSpectrumColor(float t) const {
        float shift = fmod(t + elapsedTime * 0.003f, 1.0f); // ~333s per cycle
        if (shift < 0.0f) shift += 1.0f;

        struct Stop { float pos; glm::vec3 color; };
        static const Stop stops[] = {
            {0.00f, glm::vec3(0.6f, 0.0f, 1.0f)},  // Purple
            {0.50f, glm::vec3(0.0f, 0.3f, 1.0f)},  // Blue
            {1.00f, glm::vec3(1.0f, 0.0f, 0.5f)},  // Pink
        };

        float s = shift * 2.0f; // map [0,1] → 2 passes
        if (s > 1.0f) s = 2.0f - s; // triangle wave: 0→1→0

        if (s < 0.5f) {
            return glm::mix(stops[0].color, stops[1].color, s * 2.0f);
        } else {
            return glm::mix(stops[1].color, stops[2].color, (s - 0.5f) * 2.0f);
        }
    }

    float getEnergyAt(float t) const {
        float e = 0.0f;
        for (int i = 0; i < bursts.size(); i++) {
            float dist = t - bursts[i].pos;
            if (dist < -0.5f) dist += 1.0f;
            if (dist > 0.5f) dist -= 1.0f;
            float w = bursts[i].intensity * std::max(0.0f, 1.0f - std::abs(dist) * 8.0f);
            e += w;
        }
        return e;
    }

    void buildCoreGeometry(std::vector<LineVertex>& vertices) {
        const int SEG = 300;
        float total = perimeter->getTotalLength();
        float disp = displacement.value;
        float halfW = lineWidth * 0.5f * (1.0f + bassEnergy.value * 0.3f);

        vertices.clear();
        vertices.reserve((SEG + 1) * 2);

        for (int i = 0; i <= SEG; i++) {
            float t = static_cast<float>(i) / SEG;
            float dist = t * total;
            PerimeterPoint pp = perimeter->getPointAtDistance(dist);

            glm::vec2 pos = pp.position + pp.normal * disp;

            glm::vec3 col = getGradientColor(t);
            float alpha = 0.75f + 0.25f * volumeSmooth.value;

            float e = getEnergyAt(t);
            if (e > 0.01f) {
                col = glm::mix(col, glm::vec3(1.0f), e * 0.5f);
                alpha = std::min(alpha + e * 0.3f, 1.0f);
            }

            if (kickDecay > 0.01f) {
                col = glm::mix(col, glm::vec3(1.0f), kickDecay * 0.4f);
                alpha = std::min(alpha + kickDecay * 0.2f, 1.0f);
            }

            glm::vec2 offset = pp.normal * halfW;

            LineVertex v1, v2;
            v1.x = pos.x + offset.x; v1.y = pos.y + offset.y;
            v1.u = t; v1.v = 1.0f;
            v1.r = col.r; v1.g = col.g; v1.b = col.b; v1.a = alpha;

            v2.x = pos.x - offset.x; v2.y = pos.y - offset.y;
            v2.u = t; v2.v = -1.0f;
            v2.r = col.r; v2.g = col.g; v2.b = col.b; v2.a = alpha;

            vertices.push_back(v1);
            vertices.push_back(v2);
        }
        vertices.push_back(vertices[0]);
        vertices.push_back(vertices[1]);
    }

    void buildGlowLayer(std::vector<LineVertex>& vertices, float width, float baseAlpha) {
        const int SEG = 200;
        float total = perimeter->getTotalLength();
        float halfW = width * 0.5f;
        float gi = glowIntensity * (0.3f + 0.7f * volumeSmooth.value);

        vertices.clear();
        vertices.reserve((SEG + 1) * 2);

        for (int i = 0; i <= SEG; i++) {
            float t = static_cast<float>(i) / SEG;
            float dist = t * total;
            PerimeterPoint pp = perimeter->getPointAtDistance(dist);

            glm::vec2 pos = pp.position + pp.normal * displacement.value;

            glm::vec3 col = getGradientColor(t);
            float alpha = baseAlpha * gi * glowAlphaMul.value;

            float e = getEnergyAt(t);
            if (e > 0.01f) {
                col = glm::mix(col, glm::vec3(1.0f), e * 0.4f);
                alpha += e * 0.2f;
            }

            if (kickDecay > 0.01f) {
                col = glm::mix(col, glm::vec3(1.0f), kickDecay * 0.5f);
                alpha += kickDecay * 0.3f;
            }

            glm::vec2 offset = pp.normal * halfW;

            LineVertex v1, v2;
            v1.x = pos.x + offset.x; v1.y = pos.y + offset.y;
            v1.u = t; v1.v = 1.0f;
            v1.r = col.r; v1.g = col.g; v1.b = col.b; v1.a = alpha;

            v2.x = pos.x - offset.x; v2.y = pos.y - offset.y;
            v2.u = t; v2.v = -1.0f;
            v2.r = col.r; v2.g = col.g; v2.b = col.b; v2.a = alpha;

            vertices.push_back(v1);
            vertices.push_back(v2);
        }
        vertices.push_back(vertices[0]);
        vertices.push_back(vertices[1]);
    }

    // Spectrum bars — number of bars = number of input bands (set by user)
    void buildSpectrumGeometry(std::vector<LineVertex>& vertices) {
        if (spectrum.empty()) return;

        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        float halfW = w * 0.5f;
        float rectH = perimeterH > 0.0f ? perimeterH : h * 0.9f;
        float baseY = -rectH * 0.5f;
        float maxH = h * 0.2f;
        float availW = w * 0.85f;

        int numBars = static_cast<int>(spectrum.size());
        float barW = availW / numBars;

        vertices.clear();
        vertices.reserve(numBars * 6);

        for (int i = 0; i < numBars; i++) {
            float val = specSmoothed[i] * sensitivity;
            float height = std::min(val * val * maxH, maxH);
            if (height < 2.0f) height = 2.0f;

            float x0 = -halfW + 0.075f * w + i * barW;
            float x1 = x0 + barW * 0.8f;
            float y0 = baseY;
            float y1 = baseY + height;

            float t = static_cast<float>(i) / numBars;
            glm::vec3 col = getSpectrumColor(t);

            float alpha = 0.4f + 0.6f * val;
            float baseAlpha = alpha * 0.2f;

            LineVertex v[6];
            v[0] = { x0, y0, 0, 0, col.r, col.g, col.b, baseAlpha };
            v[1] = { x1, y0, 0, 0, col.r, col.g, col.b, baseAlpha };
            v[2] = { x0, y1, 0, 0, col.r, col.g, col.b, alpha };
            v[3] = { x1, y0, 0, 0, col.r, col.g, col.b, baseAlpha };
            v[4] = { x0, y1, 0, 0, col.r, col.g, col.b, alpha };
            v[5] = { x1, y1, 0, 0, col.r, col.g, col.b, alpha };

            vertices.insert(vertices.end(), v, v + 6);
        }
    }

    // Emit trail particles falling from top edge of rectangle
    void emitTrailParticles(int count) {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        float rectW = perimeterW > 0.0f ? perimeterW : w * 0.9f;
        float rectH = perimeterH > 0.0f ? perimeterH : h * 0.9f;
        float topY = rectH * 0.5f;
        float leftX = -rectW * 0.5f;

        for (int i = 0; i < count && trailParticles.size() < maxParticles; i++) {
            Particle p;

            float t = static_cast<float>(rand()) / RAND_MAX;
            float cornerR = std::min(cornerRadius, rectW * 0.5f);
            float straightW = rectW - 2.0f * cornerR;
            p.pos.x = leftX + cornerR + t * straightW;
            p.pos.y = topY + 1.0f;

            float bounce = 1.0f + bassEnergy.value * 2.0f + kickDecay * 4.0f;
            p.vel.x = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 25.0f * bounce;
            p.vel.y = -(15.0f + static_cast<float>(rand()) / RAND_MAX * 60.0f) * bounce;

            p.life = 0.6f + static_cast<float>(rand()) / RAND_MAX * 1.0f;
            p.maxLife = p.life;

            p.color = getSpectrumColor(t + elapsedTime * 0.002f);
            p.size = 1.5f + static_cast<float>(rand()) / RAND_MAX * 3.0f;

            trailParticles.push_back(p);
        }
    }

    // Emit spark particles from the top of every spectrum bar
    void emitSpectrumParticles() {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        float halfW = w * 0.5f;
        float rectH = perimeterH > 0.0f ? perimeterH : h * 0.9f;
        float baseY = -rectH * 0.5f;
        float maxH = h * 0.2f;
        float availW = w * 0.85f;
        int numBars = static_cast<int>(spectrum.size());
        if (numBars == 0) return;
        float barW = availW / numBars;

        float boost = 1.0f + bassEnergy.value * 2.0f + kickDecay * 4.0f;

        for (int b = 0; b < numBars; b++) {
            float val = (!specSmoothed.empty()) ? specSmoothed[b] * sensitivity : 0.0f;
            if (val <= 0.9f) continue;
            if ((trailParticles.size() + spectrumParticles.size()) >= maxParticles) break;

            float height = std::min(val * val * maxH, maxH);
            if (height < 2.0f) height = 2.0f;

            float barCenterX = -halfW + 0.075f * w + (b + 0.5f) * barW;
            float barT = static_cast<float>(b) / numBars;

            int count = 1;
            for (int k = 0; k < count; k++) {
                if ((trailParticles.size() + spectrumParticles.size()) >= maxParticles) break;

                Particle p;
                p.pos.x = barCenterX + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * barW;
                p.pos.y = baseY + height;

                float speed = (20.0f + val * 50.0f + static_cast<float>(rand()) / RAND_MAX * 30.0f) * boost;
                float a = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.4f;
                p.vel.x = sin(a) * speed * 0.6f;
                p.vel.y = cos(a) * speed;

                p.life = 0.15f + static_cast<float>(rand()) / RAND_MAX * 0.25f;
                p.maxLife = p.life;

                p.color = getSpectrumColor(barT + elapsedTime * 0.002f);
                p.size = 2.0f + val * 4.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f;

                spectrumParticles.push_back(p);
            }
        }
    }

    void updateParticles(float dt) {
        // Prune trail particles
        int tw = 0;
        for (int i = 0; i < trailParticles.size(); i++) {
            trailParticles[i].life -= dt;
            if (trailParticles[i].life > 0.0f)
                trailParticles[tw++] = trailParticles[i];
        }
        trailParticles.resize(tw);

        // Prune spectrum particles
        int sw = 0;
        for (int i = 0; i < spectrumParticles.size(); i++) {
            spectrumParticles[i].life -= dt;
            if (spectrumParticles[i].life > 0.0f)
                spectrumParticles[sw++] = spectrumParticles[i];
        }
        spectrumParticles.resize(sw);

        // Physics for trail particles
        for (int i = 0; i < trailParticles.size(); i++) {
            Particle& p = trailParticles[i];
            p.pos += p.vel * dt;
            p.vel.y -= 12.0f * dt;
            p.vel.x += sin(p.life * 15.0f + p.pos.x * 0.02f) * 12.0f * dt;
            float jitter = bassEnergy.value * 20.0f + kickDecay * 40.0f;
            p.vel.x += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * jitter * dt;
        }

        // Physics for spectrum particles (float upward, slow down)
        for (int i = 0; i < spectrumParticles.size(); i++) {
            Particle& p = spectrumParticles[i];
            p.pos += p.vel * dt;
            p.vel *= 1.0f - 1.5f * dt;
            p.vel.x += sin(p.life * 20.0f + p.pos.y * 0.01f) * 8.0f * dt;
        }
    }

    void buildParticleGeometry(std::vector<LineVertex>& vertices) {
        vertices.clear();

        auto addParticle = [&](const Particle& p) {
            float lifeRatio = p.life / p.maxLife;
            float alpha = lifeRatio * lifeRatio;
            float s = p.size * (0.3f + 0.7f * lifeRatio);
            glm::vec3 col = p.color;

            // Glow quad -- aUV = local coords (-1..1) relative to particle center
            float gs = s * 2.5f;

            LineVertex v[6];
            v[0] = { p.pos.x - gs, p.pos.y - gs, -1, -1, col.r, col.g, col.b, alpha * 0.5f };
            v[1] = { p.pos.x + gs, p.pos.y - gs,  1, -1, col.r, col.g, col.b, alpha * 0.5f };
            v[2] = { p.pos.x - gs, p.pos.y + gs, -1,  1, col.r, col.g, col.b, alpha * 0.5f };
            v[3] = { p.pos.x + gs, p.pos.y - gs,  1, -1, col.r, col.g, col.b, alpha * 0.5f };
            v[4] = { p.pos.x - gs, p.pos.y + gs, -1,  1, col.r, col.g, col.b, alpha * 0.5f };
            v[5] = { p.pos.x + gs, p.pos.y + gs,  1,  1, col.r, col.g, col.b, alpha * 0.5f };

            vertices.insert(vertices.end(), v, v + 6);

            // Core dot (smaller, brighter)
            float cs = s * 0.6f;

            LineVertex cv[6];
            cv[0] = { p.pos.x - cs, p.pos.y - cs, -1, -1, col.r, col.g, col.b, alpha };
            cv[1] = { p.pos.x + cs, p.pos.y - cs,  1, -1, col.r, col.g, col.b, alpha };
            cv[2] = { p.pos.x - cs, p.pos.y + cs, -1,  1, col.r, col.g, col.b, alpha };
            cv[3] = { p.pos.x + cs, p.pos.y - cs,  1, -1, col.r, col.g, col.b, alpha };
            cv[4] = { p.pos.x - cs, p.pos.y + cs, -1,  1, col.r, col.g, col.b, alpha };
            cv[5] = { p.pos.x + cs, p.pos.y + cs,  1,  1, col.r, col.g, col.b, alpha };

            vertices.insert(vertices.end(), cv, cv + 6);
        };

        for (int i = 0; i < trailParticles.size(); i++)
            addParticle(trailParticles[i]);
        for (int i = 0; i < spectrumParticles.size(); i++)
            addParticle(spectrumParticles[i]);
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

AudioReactiveEdgeLightingEffect::AudioReactiveEdgeLightingEffect()
    : m_impl(new Impl()) {}

AudioReactiveEdgeLightingEffect::~AudioReactiveEdgeLightingEffect() { delete m_impl; }

bool AudioReactiveEdgeLightingEffect::init(int fbWidth, int fbHeight) {
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

    p.bassSmooth.coeff = 0.15f;
    p.midSmooth.coeff = 0.12f;
    p.trebleSmooth.coeff = 0.10f;
    p.volumeSmooth.coeff = 0.08f;
    p.bassEnergy.coeff = 0.25f;
    p.displacement.coeff = 0.18f;
    p.glowWidth.coeff = 0.12f;
    p.glowAlphaMul.coeff = 0.15f;

    glGenVertexArrays(1, &p.coreVAO);
    glGenBuffers(1, &p.coreVBO);
    p.setupVAO(p.coreVAO, p.coreVBO);

    glGenVertexArrays(1, &p.midGlowVAO);
    glGenBuffers(1, &p.midGlowVBO);
    p.setupVAO(p.midGlowVAO, p.midGlowVBO);

    glGenVertexArrays(1, &p.glowVAO);
    glGenBuffers(1, &p.glowVBO);
    p.setupVAO(p.glowVAO, p.glowVBO);

    glGenVertexArrays(1, &p.specVAO);
    glGenBuffers(1, &p.specVBO);
    p.setupVAO(p.specVAO, p.specVBO);

    glGenVertexArrays(1, &p.particleVAO);
    glGenBuffers(1, &p.particleVBO);
    p.setupVAO(p.particleVAO, p.particleVBO);

    return true;
}

void AudioReactiveEdgeLightingEffect::update(float deltaTime) {
    auto& p = *m_impl;
    p.elapsedTime += deltaTime;

    // Audio smoothing
    p.bassSmooth.setTarget(p.bass);
    p.midSmooth.setTarget(p.mid);
    p.trebleSmooth.setTarget(p.treble);
    p.volumeSmooth.setTarget(p.volume);

    p.bassSmooth.update();
    p.midSmooth.update();
    p.trebleSmooth.update();
    p.volumeSmooth.update();

    float bassEnergyTarget = p.bass * (0.5f + 0.5f * p.bassSmooth.value);
    p.bassEnergy.setTarget(bassEnergyTarget, 0.22f);
    p.bassEnergy.update();

    // Kick
    if (p.kick) {
        p.kickDecay = 1.0f;
        p.kick = false;
    }
    p.kickDecay *= 1.0f - 5.0f * deltaTime;
    if (p.kickDecay < 0.0f) p.kickDecay = 0.0f;

    // Spectrum smoothing
    int bandCount = static_cast<int>(p.spectrum.size());
    if (bandCount > 0) {
        if (p.specSmoothed.size() != bandCount)
            p.specSmoothed.assign(bandCount, 0.0f);
        float specCoeff = 0.25f;
        for (int i = 0; i < bandCount; i++) {
            p.specSmoothed[i] += (p.spectrum[i] - p.specSmoothed[i]) * specCoeff;
        }
    }

    // Smooth visual state
    float dispTarget = p.bassSmooth.value * 10.0f * p.sensitivity;
    p.displacement.setTarget(dispTarget);
    p.displacement.update();

    float gwTarget = p.lineWidth * 3.0f + p.bassEnergy.value * 30.0f * p.sensitivity;
    p.glowWidth.setTarget(gwTarget);
    p.glowWidth.update();

    float gaTarget = 0.5f + 0.5f * p.volumeSmooth.value;
    p.glowAlphaMul.setTarget(gaTarget);
    p.glowAlphaMul.update();

    // Flow phase
    p.flowPhase += deltaTime * (0.4f + p.midSmooth.value * 1.5f);

    // Energy bursts
    if (p.midSmooth.value > 0.3f && (rand() % 100) < static_cast<int>(p.midSmooth.value * 15.0f)) {
        Impl::EnergyBurst burst;
        burst.pos = static_cast<float>(rand()) / RAND_MAX;
        burst.speed = 0.1f + p.bassSmooth.value * 0.3f;
        burst.intensity = 0.3f + p.midSmooth.value * 0.5f;
        burst.life = 0.5f;
        p.bursts.push_back(burst);
    }
    for (int i = 0; i < p.bursts.size(); i++) {
        p.bursts[i].pos += p.bursts[i].speed * deltaTime;
        if (p.bursts[i].pos >= 1.0f) p.bursts[i].pos -= 1.0f;
        p.bursts[i].life -= deltaTime;
    }
    int bw = 0;
    for (int i = 0; i < p.bursts.size(); i++) {
        if (p.bursts[i].life > 0.0f) p.bursts[bw++] = p.bursts[i];
    }
    p.bursts.resize(bw);

    // Trail particles (fall from top edge inside rectangle)
    float trailRate = 100.0f + p.volumeSmooth.value * 120.0f + p.trebleSmooth.value * 100.0f + p.bassEnergy.value * 80.0f;
    p.trailEmitAccum += trailRate * deltaTime;
    int trailCount = static_cast<int>(p.trailEmitAccum);
    if (trailCount > 0) {
        p.emitTrailParticles(trailCount);
        p.trailEmitAccum -= trailCount;
    }

    // Spectrum spark particles (continuous, bars > 70%)
    p.emitSpectrumParticles();

    p.updateParticles(deltaTime);

    // Rebuild geometries
    std::vector<LineVertex> vertices;

    p.buildGlowLayer(vertices, p.glowWidth.value * 0.5f, 0.15f);
    p.midGlowVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.midGlowVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);

    p.buildGlowLayer(vertices, p.glowWidth.value, 0.08f);
    p.glowVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.glowVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);

    p.buildCoreGeometry(vertices);
    p.coreVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.coreVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);

    if (!p.spectrum.empty()) {
        p.buildSpectrumGeometry(vertices);
        p.specVertexCount = static_cast<int>(vertices.size());
        glBindBuffer(GL_ARRAY_BUFFER, p.specVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);
    } else {
        p.specVertexCount = 0;
    }

    p.buildParticleGeometry(vertices);
    p.particleVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.particleVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);
}

void AudioReactiveEdgeLightingEffect::render() {
    auto& p = *m_impl;

    glEnable(GL_BLEND);
    p.lineShader.use();

    glm::mat4 proj = p.projection;
    if (p.kickDecay > 0.01f) {
        float shake = p.kickDecay * 2.0f;
        proj = glm::translate(proj, glm::vec3(
            sin(p.elapsedTime * 137.0f) * shake,
            sin(p.elapsedTime * 251.0f) * shake, 0.0f
        ));
    }
    p.lineShader.setMat4("uProjection", proj);

    // 1. Outer glow
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    p.lineShader.setFloat("uAlphaScale", 0.3f + p.volumeSmooth.value * 0.3f);
    glBindVertexArray(p.glowVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.glowVertexCount);

    // 2. Mid glow
    p.lineShader.setFloat("uAlphaScale", 0.5f + p.volumeSmooth.value * 0.3f);
    glBindVertexArray(p.midGlowVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.midGlowVertexCount);

    // 3. Spectrum bars on top edge
    if (p.specVertexCount > 0) {
        p.lineShader.setFloat("uAlphaScale", 0.9f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindVertexArray(p.specVAO);
        glDrawArrays(GL_TRIANGLES, 0, p.specVertexCount);
    }

    // 4. Core line
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    p.lineShader.setFloat("uAlphaScale", 1.0f);
    glBindVertexArray(p.coreVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.coreVertexCount);

    // 5. Particles (rendered with dedicated particle shader for round falloff)
    if (p.particleVertexCount > 0) {
        p.particleShader.use();
        p.particleShader.setMat4("uProjection", proj);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        p.particleShader.setFloat("uAlphaScale", 1.0f);
        glBindVertexArray(p.particleVAO);
        glDrawArrays(GL_TRIANGLES, 0, p.particleVertexCount);
        p.lineShader.use();
        p.lineShader.setMat4("uProjection", proj);
    }
}

void AudioReactiveEdgeLightingEffect::onResize(int fbWidth, int fbHeight) {
    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;
    m_impl->updateProjection();
    m_impl->rebuildPerimeter();
}

void AudioReactiveEdgeLightingEffect::setSpectrum(const float* data, int count) {
    auto& p = *m_impl;
    p.spectrum.assign(data, data + count);
    if (p.specSmoothed.size() != static_cast<size_t>(count))
        p.specSmoothed.assign(count, 0.0f);
}

void AudioReactiveEdgeLightingEffect::setBass(float value) { m_impl->bass = value; }
void AudioReactiveEdgeLightingEffect::setMid(float value) { m_impl->mid = value; }
void AudioReactiveEdgeLightingEffect::setTreble(float value) { m_impl->treble = value; }
void AudioReactiveEdgeLightingEffect::setKick(bool active) { m_impl->kick = active; }
void AudioReactiveEdgeLightingEffect::setVolume(float value) { m_impl->volume = value; }

void AudioReactiveEdgeLightingEffect::setCornerRadius(float r) {
    m_impl->cornerRadius = r;
    m_impl->rebuildPerimeter();
}
void AudioReactiveEdgeLightingEffect::setLineWidth(float w) { m_impl->lineWidth = w; }
void AudioReactiveEdgeLightingEffect::setGlowIntensity(float i) { m_impl->glowIntensity = i; }
void AudioReactiveEdgeLightingEffect::setSensitivity(float s) { m_impl->sensitivity = s; }
