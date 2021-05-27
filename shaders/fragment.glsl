#version 460 core
out vec4 fragColor;

void main()
{
    fragColor = vec4(vec2(gl_FragCoord.x / 800.0f, gl_FragCoord.y / 600.0f), gl_FragCoord.z, 1.0f);
}