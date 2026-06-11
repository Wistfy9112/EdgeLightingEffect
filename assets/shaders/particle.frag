#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    if (dist > 0.5) discard;
    float glow = 1.0 - smoothstep(0.0, 0.5, dist);
    float core = 1.0 - smoothstep(0.0, 0.15, dist);
    float alpha = mix(glow * 0.3, core, 0.7);
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
