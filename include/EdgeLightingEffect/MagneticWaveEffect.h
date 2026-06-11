#ifndef MAGNETIC_WAVE_EFFECT_H
#define MAGNETIC_WAVE_EFFECT_H

#include "Effect.h"

class MagneticWaveEffect : public Effect {
public:
    MagneticWaveEffect();
    ~MagneticWaveEffect() override;

    bool init(int fbWidth, int fbHeight) override;
    void update(float deltaTime) override;
    void render() override;
    void onResize(int fbWidth, int fbHeight) override;

    void setSpeed(float speed);
    void setAmplitude(float amp);
    void setWaveCount(int count);
    void setLineWidth(float width);
    void setColorShift(float shift);

private:
    struct Impl;
    Impl* m_impl;
};

#endif