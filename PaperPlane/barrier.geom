#version 330 core

const mat4 projection = mat4(
    2, 0, 0, 0,
    0, -2, 0, 0,
    0, 0, -1, 0,
    -1, 1, 0, 1);

const float HOLE_WIDTH = 0.25;

layout (points) in;
layout (triangle_strip, max_vertices = 10) out;

in uint type[];

float y = gl_in[0].gl_Position.y;
float height = gl_in[0].gl_Position.z;

void emit(float x, float y) {
    gl_Position = projection * vec4(x, y, 0, 1);
    EmitVertex();
}

void emitLeftBarrier(float x) {
    emit(0, y);
    emit(0, y + height);
    emit(x, y + height);
    emit(0, y);
    emit(x, y);

    EndPrimitive();
}

void emitRightBarrier(float x) {
    emit(x, y);
    emit(x, y + height);
    emit(1, y + height);
    emit(x, y);
    emit(1, y);

    EndPrimitive();
}

void main() {
    float x = gl_in[0].gl_Position.x;
    switch (type[0]) {
        case 0: // Left
            emitLeftBarrier(x);
            return;
        case 1: // Right
            emitRightBarrier(x);
            return;
        case 2: // Slit
            emitLeftBarrier(x);
            emitRightBarrier(x + HOLE_WIDTH);
            return;
    }
}
