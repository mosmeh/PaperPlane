#version 330 core

layout (location = 0) in uint in_type;
layout (location = 1) in float leftToHole;
layout (location = 2) in float height;
layout (location = 3) in float yPos;

out uint type;

void main() {
    type = in_type;
    gl_Position = vec4(leftToHole, yPos, height, 1);
}
