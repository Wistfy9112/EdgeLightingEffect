#ifndef EDGE_LIGHTING_EFFECT_API_H
#define EDGE_LIGHTING_EFFECT_API_H

#include "Effect.h"

class EdgeLightingEffect : public Effect {
public:
    EdgeLightingEffect();
    ~EdgeLightingEffect() override;

    bool init(int fbWidth, int fbHeight) override;
    void update(float deltaTime) override;
    void render() override;
    void onResize(int fbWidth, int fbHeight) override;

    void triggerNotification();
    void setCornerRadius(float radius);
    void setGlowWidth(float width);
    void setCoreWidth(float width);

private:
    int m_width;
    int m_height;
    float m_cornerRadius;

    struct Impl;
    Impl* m_impl;
};

#endif