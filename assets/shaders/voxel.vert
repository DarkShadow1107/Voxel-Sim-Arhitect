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

void main() {
    vec3 pos = aPos;
    
    // Wave effect for water
    vec2 atlasSize = vec2(16.0, 16.0);
    vec2 tileBase = floor(aTexCoord * atlasSize);
    bool isWater = (tileBase.x == 4.0 && tileBase.y == 0.0);
    
    if (isWater && aNormal.y > 0.5) {
        pos.y -= 0.15; // Slightly lower water level
        pos.y += sin(uTime * 2.0 + aPos.x * 0.5 + aPos.z * 0.5) * 0.05;
    }

    vec4 worldPos = uModel * vec4(pos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
    vTexCoord = aTexCoord;

    gl_Position = uProjection * uView * worldPos;
}
