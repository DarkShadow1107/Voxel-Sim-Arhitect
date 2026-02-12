#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;

out vec3 vNormal;
out vec3 vWorldPos;
out vec3 vColor;
out vec2 vTexCoord;
out float vDistance;

void main() {
    vec3 pos = aPos;

    // Wave effect for water
    vec2 atlasSize = vec2(16.0, 16.0);
    vec2 tileBase = floor(aTexCoord * atlasSize);
    bool isWater = (tileBase.x == 4.0 && tileBase.y == 0.0);

    if (isWater && aNormal.y > 0.5) {
        pos.y -= 0.12;
        // Multi-octave wave for more natural water surface
        float wave1 = sin(uTime * 1.8 + aPos.x * 0.7 + aPos.z * 0.5) * 0.04;
        float wave2 = sin(uTime * 2.5 + aPos.x * 1.3 - aPos.z * 0.9) * 0.02;
        float wave3 = cos(uTime * 1.2 + aPos.z * 0.4 + aPos.x * 0.3) * 0.03;
        pos.y += wave1 + wave2 + wave3;
    }

    vec4 worldPos = uModel * vec4(pos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
    vTexCoord = aTexCoord;

    vec4 viewPos = uView * worldPos;
    vDistance = length(viewPos.xyz);

    gl_Position = uProjection * viewPos;
}
