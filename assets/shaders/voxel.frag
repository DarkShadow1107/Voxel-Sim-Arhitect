#version 330 core

out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vColor;
in vec2 vTexCoord;
in float vDistance;

uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform sampler2D uTexture;
uniform float uTime;
uniform vec2 uResolution;
uniform vec3 uColorTint;

void main() {
    vec3 n = normalize(vNormal);
    vec3 l = normalize(-uLightDir);

    vec2 uv = vTexCoord;
    vec2 atlasSize = vec2(16.0, 16.0);
    vec2 tileBase = floor(uv * atlasSize);
    vec2 tileFract = fract(uv * atlasSize);

    bool isWater = (tileBase.x == 4.0 && tileBase.y == 0.0);
    bool isLava  = (tileBase.x == 5.0 && tileBase.y == 0.0);
    bool isIce   = (tileBase.x == 12.0 && tileBase.y == 1.0);
    bool isSnow  = (tileBase.x == 9.0 && tileBase.y == 0.0);
    bool isCherryLog = (tileBase.x == 6.0 && tileBase.y == 1.0);
    bool isCherryLeaf = (tileBase.x == 7.0 && tileBase.y == 1.0);
    bool isGlass = (tileBase.x == 14.0 && tileBase.y == 0.0);
    bool isLeaves = (tileBase.x == 7.0 && tileBase.y == 0.0) || isCherryLeaf || (tileBase.x == 5.0 && tileBase.y == 1.0);

    // Water: gentle scrolling animation (Minecraft-style)
    if (isWater) {
        float t = uTime * 0.4;
        tileFract += vec2(t * 0.03, t * 0.02);
    }

    // Lava: slow flow
    if (isLava) {
        float speed = uTime * 0.08;
        tileFract += vec2(speed * 0.05, speed * 0.03);
    }

    // Wrap UVs within the tile
    tileFract = fract(tileFract);

    vec2 finalUV = (tileBase + tileFract) / atlasSize;
    vec4 texColor = texture(uTexture, finalUV);
    if (texColor.a < 0.05) discard;

    texColor.rgb *= vColor * uColorTint;

    // Cherry Tint
    if (isCherryLog) {
        texColor.rgb *= vec3(1.15, 0.85, 0.92);
    }
    if (isCherryLeaf) {
        texColor.rgb *= vec3(1.2, 0.75, 0.95);
    }

    vec3 viewDir = normalize(uViewPos - vWorldPos);
    float ndotl = max(dot(n, l), 0.0);

    // Day/night factor from sun Y position
    float nightFactor = clamp(uLightDir.y * 2.0 + 0.8, 0.35, 1.0);

    // Minecraft-style directional face shading
    // Top (Y+) = brightest, sides = medium, bottom = darkest
    float faceBrightness = 1.0;
    if (abs(n.y) > 0.5) {
        faceBrightness = n.y > 0.0 ? 1.0 : 0.5;
    } else if (abs(n.z) > 0.5) {
        faceBrightness = 0.8;
    } else {
        faceBrightness = 0.6;
    }

    // Ambient + diffuse (clean Minecraft-like lighting)
    float ambient = 0.4;
    float diffuse = 0.6 * ndotl;
    float lighting = (ambient + diffuse) * faceBrightness * nightFactor;

    // Height-based AO (subtle, for underground depth cue only)
    if (vWorldPos.y < 30.0) {
        lighting *= mix(0.85, 1.0, clamp(vWorldPos.y / 30.0, 0.0, 1.0));
    }

    // Water rendering
    float alphaOut = texColor.a;
    vec3 emissive = vec3(0.0);

    if (isWater) {
        // Minecraft water: uniform blue tint, semi-transparent
        texColor.rgb = mix(texColor.rgb, vec3(0.15, 0.35, 0.75), 0.6);
        alphaOut = 0.65;

        // Subtle specular on water surface (top face only)
        if (n.y > 0.5) {
            vec3 halfDir = normalize(l + viewDir);
            float spec = pow(max(dot(n, halfDir), 0.0), 128.0);
            emissive += vec3(1.0, 0.98, 0.95) * spec * 0.6 * nightFactor;
        }
    }

    // Ice
    if (isIce) {
        alphaOut = 0.75;
        vec3 halfDir = normalize(l + viewDir);
        float spec = pow(max(dot(n, halfDir), 0.0), 128.0);
        emissive += vec3(0.5, 0.7, 1.0) * spec * 0.4;
    }

    // Glass
    if (isGlass) {
        float fresnel = pow(1.0 - max(dot(viewDir, n), 0.0), 3.0);
        alphaOut = mix(0.12, 0.4, fresnel);
        vec3 halfDir = normalize(l + viewDir);
        float spec = pow(max(dot(n, halfDir), 0.0), 96.0);
        emissive += vec3(0.6) * spec * 0.3;
    }

    // Lava: emissive glow
    if (isLava) {
        float pulse = 0.85 + 0.15 * sin(uTime * 2.0);
        emissive += vec3(1.0, 0.4, 0.05) * 1.5 * pulse;
        emissive += vec3(1.0, 0.7, 0.0) * 0.3;
        alphaOut = 1.0;
        lighting = 1.0; // Lava is self-lit
    }

    // Snow sparkle (very subtle)
    if (isSnow && n.y > 0.5) {
        float sparkle = fract(sin(dot(floor(vWorldPos.xz * 8.0), vec2(12.9898, 78.233))) * 43758.5453);
        if (sparkle > 0.97) {
            emissive += vec3(0.3) * nightFactor;
        }
    }

    // Final color
    vec3 color = texColor.rgb * lighting + emissive;

    // Snow: ensure bright white after lighting (min brightness 0.85)
    if (isSnow) {
        float snowBright = max(lighting, 0.85);
        color = vec3(snowBright) + emissive;
    }

    // Sun glow (subtle)
    float sunGlow = pow(max(dot(viewDir, l), 0.0), 256.0);
    color += vec3(1.0, 0.95, 0.9) * sunGlow * nightFactor * 0.4;

    // Fog (Minecraft-style: linear, starts far, matches sky)
    // Use render distance ~128 blocks (4 chunks * 32). Fog starts at 60% of that.
    float fogStart = 80.0;
    float fogEnd = 160.0;
    float fogFactor = clamp((vDistance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    fogFactor = fogFactor * fogFactor; // Smooth curve

    // Fog color matches sky
    vec3 fogColor = vec3(0.53, 0.65, 0.9) * nightFactor;
    if (uLightDir.y < 0.0) {
        fogColor = vec3(0.02, 0.02, 0.05);
    }
    color = mix(color, fogColor, fogFactor);

    // Simple contrast boost (no tone mapping - keep colors vivid)
    color = pow(color, vec3(1.05));

    // Clamp to prevent overbright
    color = clamp(color, 0.0, 1.0);

    FragColor = vec4(color, alphaOut);
}
