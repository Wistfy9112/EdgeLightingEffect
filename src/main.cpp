#include <iostream>
#include "core/Application.h"
#include "effects/EdgeLightingEffect.h"

int main() {
    Application app(1280, 720, "Edge Lighting Effect");
    if (!app.init()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return -1;
    }

    EdgeLightingEffect effect;
    if (!effect.init(&app)) {
        std::cerr << "Failed to initialize effect" << std::endl;
        return -1;
    }

    double lastTime = glfwGetTime();
    while (!app.shouldClose()) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        if (dt > 0.05f) dt = 0.05f;

        effect.update(dt);
        effect.render(dt);

        glfwPollEvents();
    }

    return 0;
}
