#include "EdgeLightingEffect/EffectManager.h"

EffectManager::EffectManager() = default;
EffectManager::~EffectManager() = default;

void EffectManager::addEffect(Effect* effect) {
    m_effects.push_back(effect);
}

void EffectManager::removeEffect(Effect* effect) {
    for (auto it = m_effects.begin(); it != m_effects.end(); ++it) {
        if (*it == effect) {
            m_effects.erase(it);
            return;
        }
    }
}

void EffectManager::clearEffects() {
    m_effects.clear();
}

bool EffectManager::init(int fbWidth, int fbHeight) {
    m_width = fbWidth;
    m_height = fbHeight;
    for (auto* e : m_effects) {
        if (!e->init(fbWidth, fbHeight))
            return false;
    }
    return true;
}

void EffectManager::update(float deltaTime) {
    for (auto* e : m_effects)
        e->update(deltaTime);
}

void EffectManager::render() {
    for (auto* e : m_effects)
        e->render();
}

void EffectManager::onResize(int fbWidth, int fbHeight) {
    m_width = fbWidth;
    m_height = fbHeight;
    for (auto* e : m_effects)
        e->onResize(fbWidth, fbHeight);
}