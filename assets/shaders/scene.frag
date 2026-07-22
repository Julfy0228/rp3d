#version 330 core

out vec4 FragColor;

uniform vec4 uColor;
uniform vec3 uLightDirection;

in vec3 vNormal;

void main()
{
    vec3 normal = normalize(vNormal);
    if (!gl_FrontFacing) {
        normal = -normal;
    }
    vec3 light_dir = normalize(uLightDirection);
    float diffuse = max(dot(normal, light_dir), 0.0);
    float lighting = 0.25 + diffuse * 0.75;
    FragColor = vec4(uColor.rgb * lighting, uColor.a);
}
