#ifndef EDGE_PULSE_EFFECT_H
#define EDGE_PULSE_EFFECT_H

#include "Effect.h"

class EdgePulseEffect : public Effect {
public:
    EdgePulseEffect();
    ~EdgePulseEffect() override;

    bool init(int fbWidth, int fbHeight) override;
    void update(float deltaTime) override;
    void render() override;
    void onResize(int fbWidth, int fbHeight) override;

    void triggerNotification();
    void setCornerRadius(float radius);
    void setCoreWidth(float width);
    void setGlowWidth(float width);
    void setPosition(float x, float y);
    void setSize(float width, float height);

private:
    int m_width;
    int m_height;
    float m_cornerRadius;
    float m_perimeterW;
    float m_perimeterH;
    float m_offsetX;
    float m_offsetY;

    struct Impl;
    Impl* m_impl;
};

#endif
