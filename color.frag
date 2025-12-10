#version 330 core
in vec4 chCol;

out vec4 outCol;

uniform float uR;
uniform float uG;
uniform float uB;

void main()
{
    outCol = vec4(chCol.r + uR, chCol.g + uG, chCol.b + uB, chCol.a);
} 