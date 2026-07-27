#version 330 core

in vec3 vNearPoint;
in vec3 vFarPoint;
in mat4 vView;
in mat4 vProjection;

out vec4 FragColor;

uniform vec4  uGridColorMinor;
uniform vec4  uGridColorMajor;
uniform float uCellSize;
uniform float uFadeDistance;

float gridLine(vec2 uv, float cell)
{
    vec2 coord = uv / cell;
    vec2 deriv = fwidth(coord);
    vec2 grid  = abs(fract(coord - 0.5) - 0.5) / deriv;
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

void main()
{
    float t = -vNearPoint.z / (vFarPoint.z - vNearPoint.z);

    if (t < 0.0)
        discard;

    vec3 hit = vNearPoint + t * (vFarPoint - vNearPoint);

    float major_cell = uCellSize * 8.0;
    float alpha_minor = gridLine(hit.xy, uCellSize);
    float alpha_major = gridLine(hit.xy, major_cell);

    vec3 cam_pos = -vec3(vView[3]) * mat3(vView);
    cam_pos = vec3(inverse(vView)[3]);
    float dist = length(hit.xy - cam_pos.xy);
    float fade = 1.0 - clamp(dist / uFadeDistance, 0.0, 1.0);
    fade = fade * fade;

    if (fade < 0.001)
        discard;

    vec4 color = mix(
        vec4(uGridColorMinor.rgb, uGridColorMinor.a * alpha_minor),
        vec4(uGridColorMajor.rgb, uGridColorMajor.a * alpha_major),
        alpha_major
    );

    color.a *= fade;

    if (color.a < 0.004)
        discard;

    FragColor = color;

    vec4 clip_pos  = vProjection * vView * vec4(hit, 1.0);
    float ndc_depth = clip_pos.z / clip_pos.w;
    gl_FragDepth   = (ndc_depth + 1.0) * 0.5;
}
