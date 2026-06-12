#version 330 core
in vec2 vLocal;
in vec4 vColor;

out vec4 FragColor;

uniform float uAlphaScale;

void main() {
    float dist = length(vLocal);
    float alpha = 1.0 - smoothstep(0.0, 1.0, dist);
    FragColor = vColor * alpha * uAlphaScale;
}
