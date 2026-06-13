#ifndef ENERGY_STREAM_EFFECT_H
#define ENERGY_STREAM_EFFECT_H

#include "Effect.h"

class EnergyStreamEffect : public Effect {
public:
    EnergyStreamEffect();
    ~EnergyStreamEffect() override;

    bool init(int fbWidth, int fbHeight) override;
    void update(float deltaTime) override;
    void render() override;
    void onResize(int fbWidth, int fbHeight) override;

    void setSpeed(float speed);
    void setIntensity(float intensity);
    void setParticleCount(int count);
    void setGlowIntensity(float intensity);
    void setCornerRadius(float radius);
    void setLineWidth(float width);
    void setPosition(float x, float y);
    void setSize(float width, float height);

private:
    struct Impl;
    Impl* m_impl;
};

#endif
