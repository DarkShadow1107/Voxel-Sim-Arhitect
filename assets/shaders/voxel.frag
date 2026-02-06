#version 330 core

out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPos;
in vec3 vColor;
in vec2 vTexCoord;

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

    // Water: subtle wave distortion + scrolling (clamped to tile)
    if (isWater) {
        float wave = sin(uTime * 1.5 + vWorldPos.x * 0.8 + vWorldPos.z * 0.8) * 0.04;
        float wave2 = cos(uTime * 1.2 + vWorldPos.x * 0.5 - vWorldPos.z * 0.6) * 0.04;
        
        // Scroll texture with two layers for "depth"
        tileFract += vec2(uTime * 0.12 + wave, uTime * 0.08 + wave2);
    }

    // Lava: more aggressive distortion + scrolling
    if (isLava) {
        float speed = uTime * 0.15;
        float noise = sin(vWorldPos.x * 0.5 + speed) * cos(vWorldPos.z * 0.5 + speed);
        tileFract += vec2(speed * 0.1 + noise * 0.05, speed * 0.05);
    }

    // Wrap UVs within the tile
    tileFract = fract(tileFract);
    
    vec2 finalUV = (tileBase + tileFract) / atlasSize;
    vec4 texColor = texture(uTexture, finalUV);
    if (texColor.a < 0.05) discard;

    texColor.rgb *= vColor * uColorTint;

    // Snow: make it whiter
    if (isSnow) {
        texColor.rgb = mix(texColor.rgb, vec3(1.0), 0.7); // Even whiter
    }

    // Cherry Tint
    if (isCherryLog) {
        texColor.rgb *= vec3(1.2, 0.8, 0.9); // Pinker wood
    }
    if (isCherryLeaf) {
        texColor.rgb *= vec3(1.3, 0.7, 1.0); // Very pink leaves
    }

    vec3 viewDir = normalize(uViewPos - vWorldPos);

    float ndotl = max(dot(n, l), 0.0);
    
    // Night factor (dim ambient/diffuse when sun is low)
    // Brighter night: min 0.45
    float nightFactor = clamp(uLightDir.y * 1.5 + 0.7, 0.45, 1.0);
    
    // Improved lighting: Ambient Occlusion (Fake)
    float ao = 1.0;
    if (vWorldPos.y < 40.0) ao *= 0.96;
    if (vWorldPos.y < 20.0) ao *= 0.92;

    vec3 ambient = vec3(0.55) * nightFactor * ao;
    vec3 diffuse = vec3(0.85) * ndotl * nightFactor * ao;

    // Snow Sparkle & Shades
    if (isSnow) {
        // Reduce AO impact on snow to keep it whiter
        ambient = mix(ambient, vec3(0.65 * nightFactor), 0.4);
        
        float sparkle = fract(sin(dot(vWorldPos.xz * 10.0, vec2(12.9898, 78.233))) * 43758.5453);
        if (sparkle > 0.98) {
            float s = pow(max(dot(reflect(-l, n), viewDir), 0.0), 32.0);
            diffuse += vec3(1.0, 1.0, 1.0) * s * 1.2; // Pure white sparkle
        }
        // Shades of white (subtle noise-based variation, strictly grayscale)
        float shades = fract(sin(dot(floor(vWorldPos.xyz * 3.0), vec3(12.989, 78.233, 45.164))) * 43758.5453);
        ambient *= (0.98 + shades * 0.05); 
    }

    // Water: translucent + fresnel + spec
    float alphaOut = texColor.a;
    vec3 emissive = vec3(0.0);
    if (isWater) {
        float fresnel = pow(1.0 - max(dot(viewDir, n), 0.0), 3.0);
        float spec = pow(max(dot(reflect(-l, n), viewDir), 0.0), 128.0);
        ambient += vec3(0.1, 0.2, 0.4) * nightFactor;
        
        // Caustics-like effect
        float caustics = sin(vWorldPos.x * 2.0 + uTime) * cos(vWorldPos.z * 2.0 + uTime);
        caustics = pow(max(caustics, 0.0), 4.0);
        emissive += vec3(0.2, 0.5, 0.9) * fresnel + vec3(0.8) * spec + vec3(0.4, 0.6, 1.0) * caustics;
        
        alphaOut = 0.6; 
    }

    // Ice: translucent
    if (isIce) {
        alphaOut = 0.75;
        float spec = pow(max(dot(reflect(-l, n), viewDir), 0.0), 128.0);
        emissive += vec3(0.5, 0.7, 1.0) * spec;
    }

    // Lava: emissive animated glow
    if (isLava) {
        float pulse = 0.8 + 0.2 * sin(uTime * 2.5);
        float flow = sin(vWorldPos.x * 0.4 + uTime) * cos(vWorldPos.z * 0.4 + uTime);
        emissive += vec3(1.0, 0.3, 0.0) * 1.8 * pulse; // Deep orange
        emissive += vec3(1.0, 0.8, 0.0) * max(flow, 0.0) * 0.5; // Bright yellow flow highlights
        alphaOut = 1.0;
    }

    vec3 color = (ambient + diffuse) * vColor * texColor.rgb + emissive;
    
    // Lava Light Bleed (Fake nearby lighting)
    if (!isLava && !isWater) {
        // If the block is "warm" colored (like stone near lava), boost it
        if (vColor.r > 0.7 && vColor.g > 0.5 && vColor.b < 0.6) { 
             color += vec3(0.15, 0.05, 0.0) * nightFactor;
        }
    }
    
    // Sun Halo / Glow (Whiter)
    float sunGlow = pow(max(dot(viewDir, l), 0.0), 128.0);
    color += vec3(1.0, 0.95, 0.9) * sunGlow * nightFactor;

    // Fog
    float dist = length(uViewPos - vWorldPos);
    float fogFactor = clamp((dist - 100.0) / 150.0, 0.0, 1.0);
    vec3 fogColor = vec3(0.4, 0.6, 0.9) * nightFactor;
    if (uLightDir.y < 0.0) fogColor = vec3(0.05, 0.05, 0.1);
    color = mix(color, fogColor, fogFactor);

    // Contrast & Saturation boost
    color = pow(color, vec3(1.1)); // Contrast
    
    // Vignette
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    float vignette = 1.0 - length(screenUV - 0.5) * 0.6;
    color *= clamp(vignette, 0.0, 1.0);

    FragColor = vec4(color, alphaOut);
}
