#ifndef AUDIO_REACTIVE_EDGE_LIGHTING_EFFECT_H
#define AUDIO_REACTIVE_EDGE_LIGHTING_EFFECT_H

#include "Effect.h"
#include <vector>

class AudioReactiveEdgeLightingEffect : public Effect {
public:
    AudioReactiveEdgeLightingEffect();
    ~AudioReactiveEdgeLightingEffect() override;

    bool init(int fbWidth, int fbHeight) override;
    void update(float deltaTime) override;
    void render() override;
    void onResize(int fbWidth, int fbHeight) override;

    // Audio input (call each frame before update)
    void setSpectrum(const float* data, int count);
    void setBass(float value);
    void setMid(float value);
    void setTreble(float value);
    void setKick(bool active);
    void setVolume(float value);

    // Runtime config
    void setCornerRadius(float r);
    void setLineWidth(float w);
    void setGlowIntensity(float i);
    void setSensitivity(float s);
    void setPosition(float x, float y);
    void setSize(float width, float height);

private:
    struct Impl;
    Impl* m_impl;
};

#endif
