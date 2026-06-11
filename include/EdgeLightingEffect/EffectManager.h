#ifndef EFFECT_MANAGER_H
#define EFFECT_MANAGER_H

#include <vector>
#include "Effect.h"

class EffectManager {
public:
    EffectManager();
    ~EffectManager();

    void addEffect(Effect* effect);
    void removeEffect(Effect* effect);
    void clearEffects();

    bool init(int fbWidth, int fbHeight);
    void update(float deltaTime);
    void render();
    void onResize(int fbWidth, int fbHeight);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    std::vector<Effect*> m_effects;
    int m_width = 0, m_height = 0;
};

#endif