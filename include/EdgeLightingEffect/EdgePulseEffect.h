#ifndef EDGE_PULSE_EFFECT_API_H
#define EDGE_PULSE_EFFECT_API_H

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

private:
    int m_width;
    int m_height;
    float m_cornerRadius;

    struct Impl;
    Impl* m_impl;
};

#endif
