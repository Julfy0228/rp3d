#version 330 core

out vec4 FragColor;

uniform vec4  uColor;

uniform vec3  uAmbientColor;
uniform vec3  uLightDirection;
uniform vec3  uLightColor;
uniform vec3  uCameraPos;

in vec3 vNormal;
in vec3 vWorldPos;

const float MAT_ROUGHNESS   = 0.55;
const float MAT_METALLIC    = 0.0;
const float MAT_SPECULAR    = 0.04;

vec3 FresnelSchlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float DistributionGGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (3.14159265 * d * d);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float r  = roughness + 1.0;
    float k  = (r * r) / 8.0;
    float gv = NdotV / (NdotV * (1.0 - k) + k);
    float gl = NdotL / (NdotL * (1.0 - k) + k);
    return gv * gl;
}

void main()
{
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing)
        N = -N;

    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(uLightDirection);
    vec3 H = normalize(V + L);

    vec3 albedo = uColor.rgb;

    vec3 F0 = mix(vec3(MAT_SPECULAR), albedo, MAT_METALLIC);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float D = DistributionGGX(NdotH, MAT_ROUGHNESS);
    float G = GeometrySmith(NdotV, NdotL, MAT_ROUGHNESS);
    vec3  F = FresnelSchlick(HdotV, F0);

    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);

    vec3 kD = (1.0 - F) * (1.0 - MAT_METALLIC);
    vec3 diffuse = kD * albedo / 3.14159265;

    vec3 Lo = (diffuse + specular) * uLightColor * NdotL;

    vec3 ambient = uAmbientColor * albedo;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, uColor.a);
}
