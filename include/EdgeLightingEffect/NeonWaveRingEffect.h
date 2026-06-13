#ifndef NEON_WAVE_RING_EFFECT_H
#define NEON_WAVE_RING_EFFECT_H

#include "Effect.h"

class NeonWaveRingEffect : public Effect {
public:
    NeonWaveRingEffect();
    ~NeonWaveRingEffect() override;

    bool init(int fbWidth, int fbHeight) override;
    void update(float deltaTime) override;
    void render() override;
    void onResize(int fbWidth, int fbHeight) override;

    void setRadius(float radius);
    void setAmplitude(float amp);
    void setSpeed(float speed);
    void setWaveCount(float count);
    void setLineWidth(float width);
    void setGlowWidth(float width);
    void setGlowIntensity(float intensity);

private:
    struct Impl;
    Impl* m_impl;
};

#endif
