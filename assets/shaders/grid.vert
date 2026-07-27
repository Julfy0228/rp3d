#version 330 core

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNearPoint;
out vec3 vFarPoint;
out mat4 vView;
out mat4 vProjection;

vec3 unproject(float x, float y, float z, mat4 inv_view, mat4 inv_proj)
{
    vec4 clip     = vec4(x, y, z, 1.0);
    vec4 view_pos = inv_proj * clip;
    view_pos /= view_pos.w;
    vec4 world_pos = inv_view * view_pos;
    return world_pos.xyz;
}

void main()
{
    vec2 positions[3];
    positions[0] = vec2(-1.0, -1.0);
    positions[1] = vec2( 3.0, -1.0);
    positions[2] = vec2(-1.0,  3.0);

    vec2 p = positions[gl_VertexID];

    mat4 inv_view = inverse(uView);
    mat4 inv_proj = inverse(uProjection);

    vNearPoint   = unproject(p.x, p.y, -1.0, inv_view, inv_proj);
    vFarPoint    = unproject(p.x, p.y,  1.0, inv_view, inv_proj);
    vView        = uView;
    vProjection  = uProjection;

    gl_Position = vec4(p, 0.0, 1.0);
}
