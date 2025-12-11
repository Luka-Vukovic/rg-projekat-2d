#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform float uX;
uniform float uY;
uniform vec2 uDoorPos;
uniform vec2 uSeatPos;

void main()
{
    vec2 currentPos = uDoorPos + vec2(uX, uY) * (uSeatPos - uDoorPos);
    vec2 finalPos = currentPos + aPos;
    gl_Position = vec4(finalPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}