#version 330 core
in vec2 vUV;
in vec4 vColor;

out vec4 FragColor;

uniform float uAlphaScale;

void main() {
    float dist = abs(vUV.y);
    float alpha = 1.0 - smoothstep(0.0, 1.0, dist);
    FragColor = vColor * alpha * uAlphaScale;
}
