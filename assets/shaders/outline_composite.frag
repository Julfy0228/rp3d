#version 330 core

uniform sampler2D uMaskTexture;
uniform vec2 uTexelSize;
uniform vec4 uOutlineColor;
uniform float uOutlineWidth;

in vec2 vUv;
out vec4 FragColor;

void main()
{
    float mask = texture(uMaskTexture, vUv).r;
    
    float maxNeighbor = 0.0;
    int radius = int(uOutlineWidth);
    
    for (int x = -radius; x <= radius; x++)
    {
        for (int y = -radius; y <= radius; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * uTexelSize;
            maxNeighbor = max(maxNeighbor, texture(uMaskTexture, vUv + offset).r);
        }
    }
    
    float outline = maxNeighbor * (1.0 - mask);
    
    FragColor = vec4(uOutlineColor.rgb, outline * uOutlineColor.a);
}
