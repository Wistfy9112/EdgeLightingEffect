#ifndef EFFECT_H
#define EFFECT_H

class Effect {
public:
    virtual ~Effect() = default;

    virtual bool init(int fbWidth, int fbHeight) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    virtual void onResize(int fbWidth, int fbHeight) {}
};

#endif