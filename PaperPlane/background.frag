#version 330 core

in vec2 uv;
out vec4 color;

uniform float x;

void main() {
    if (abs(uv.x - x) < 0.01 && abs(uv.y - 0.8) < 0.01) {
        color = vec4(1, 0, 0, 1);
        return;
    }
    if (0.1 < uv.x && uv.x < 0.9) {
        color = vec4(1, 1, 1, 1);
    } else {
        color = vec4(0, 0, 0, 1);
    }
}
