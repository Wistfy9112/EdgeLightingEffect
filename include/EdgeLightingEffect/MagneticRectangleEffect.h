#ifndef MAGNETIC_RECTANGLE_EFFECT_H
#define MAGNETIC_RECTANGLE_EFFECT_H

#include "Effect.h"

class MagneticRectangleEffect : public Effect {
public:
    MagneticRectangleEffect();
    ~MagneticRectangleEffect() override;

    bool init(int fbWidth, int fbHeight) override;
    void update(float deltaTime) override;
    void render() override;
    void onResize(int fbWidth, int fbHeight) override;

    void setSpeed(float speed);
    void setAmplitude(float amp);
    void setLineWidth(float width);
    void setCornerRadius(float radius);
    void setColorShift(float shift);
    void setScale(float scale);

private:
    struct Impl;
    Impl* m_impl;
};

#endif