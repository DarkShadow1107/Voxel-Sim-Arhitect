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
    bool isFire  = (tileBase.x == 8.0 && tileBase.y == 4.0);

    // Water: two-layer scroll (primary + perpendicular interference for natural surface movement)
    if (isWater) {
        float t = uTime * 0.4;
        tileFract += vec2(t * 0.03, t * 0.02);
        // Secondary perpendicular ripple layer creates interference pattern
        float ripple = sin(uTime * 0.9 + tileFract.y * 6.0) * 0.008
                     + cos(uTime * 0.6 + tileFract.x * 5.0) * 0.006;
        tileFract += vec2(ripple, -ripple * 0.8);
    }

    // Lava: multi-layer flowing animation (Minecraft-like viscous lava)
    if (isLava) {
        float t = uTime * 0.07; // Very slow base flow speed (viscous lava)
        // Layer 1: large slow flow
        vec2 flow1 = vec2(sin(t * 0.8 + tileFract.y * 2.0) * 0.04, t * 0.025);
        // Layer 2: smaller faster swirl
        vec2 flow2 = vec2(cos(t * 1.3 + tileFract.x * 3.0) * 0.02, t * 0.018 + sin(t*0.5)*0.01);
        // Layer 3: bubble/cracking movement
        vec2 flow3 = vec2(sin(t * 2.1 + vWorldPos.x * 0.7) * 0.015, cos(t * 1.7 + vWorldPos.z * 0.5) * 0.015);
        tileFract += flow1 + flow2 * 0.6 + flow3 * 0.4;
    }

    // Fire: multi-speed upward scroll with turbulent sway
    if (isFire) {
        float ft = uTime * 2.2;
        // Primary upward scroll
        float scrollY = -ft * 0.10;
        // Turbulent horizontal sway (multiple frequencies)
        float swayX = sin(ft * 0.7 + vWorldPos.y * 3.0 + vWorldPos.x) * 0.05
                    + sin(ft * 1.3 + vWorldPos.z * 2.5) * 0.03;
        // Secondary bulge effect
        float bulge = cos(ft * 0.5 + tileFract.y * 6.28) * 0.02;
        tileFract += vec2(swayX + bulge, scrollY);
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

    // --- Fake SSAO / Edge Darkening using derivatives ---
    // This gives a nice cel-shaded/voxel pop effect
    vec3 dpdx = dFdx(vWorldPos);
    vec3 dpdy = dFdy(vWorldPos);
    vec3 crossDeriv = cross(dpdx, dpdy);
    float edgeFactor = length(crossDeriv);
    // Darken edges slightly
    lighting *= mix(1.0, 0.75, clamp(edgeFactor * 0.5, 0.0, 1.0));

    // Water rendering
    float alphaOut = texColor.a;
    vec3 emissive = vec3(0.0);

    if (isWater) {
        // Minecraft water: uniform blue tint, semi-transparent
        texColor.rgb = mix(texColor.rgb, vec3(0.1, 0.3, 0.8), 0.8);
        alphaOut = 0.8;

        // Enhanced specular on water surface (top face only)
        if (n.y > 0.5) {
            vec3 halfDir = normalize(l + viewDir);
            float spec = pow(max(dot(n, halfDir), 0.0), 128.0);
            emissive += vec3(1.0, 0.98, 0.95) * spec * 0.8 * nightFactor;

            // Add subtle reflection from the sky
            float fresnel = pow(1.0 - max(dot(viewDir, n), 0.0), 4.0);
            vec3 skyReflectColor = vec3(0.4, 0.6, 0.9) * nightFactor;
            emissive += skyReflectColor * fresnel * 0.5;

            // Caustic shimmer: animated bright patches on water surface
            vec2 causticUV = vWorldPos.xz * 0.4;
            float caustic = 0.5 + 0.5 * sin(uTime * 1.2 + causticUV.x * 2.3 + causticUV.y * 1.7)
                          * sin(uTime * 0.9 + causticUV.x * 1.5 - causticUV.y * 2.1);
            caustic = pow(max(caustic, 0.0), 3.0);
            emissive += vec3(0.6, 0.75, 1.0) * caustic * 0.15 * nightFactor;
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

    // Lava: full-bright emissive with multi-layer realistic look
    if (isLava) {
        lighting = 1.0; // No shadows on lava

        // Use float time for animated color blending
        float lt = uTime;

        // Per-position hash for unique variation per block
        float hash = fract(sin(dot(floor(vWorldPos.xz), vec2(127.1, 311.7))) * 43758.5453);

        // Layer 1: bright molten core (hot yellow-orange)
        float coreNoise  = fract(sin(dot(tileFract * 4.0, vec2(12.9898, 78.233))) * 43758.5) * 0.5 + 0.5;
        // Layer 2: crust darkening (cooler patches simulating solidifying crust)
        float crustNoise = fract(sin(dot(tileFract * 8.0 + vec2(lt*0.03), vec2(93.9898, 67.345))) * 53231.1) * 0.5 + 0.5;
        // Layer 3: slow large-scale brightness waves
        float waveNoise  = 0.75 + 0.25 * sin(lt * 1.2 + vWorldPos.x * 0.3 + vWorldPos.z * 0.4 + hash * 6.28);

        // Lava color layers:
        // Bright molten zones = yellow-white
        // Normal zones = orange
        // Crust zones = deep red-brown
        vec3 lavaHot    = vec3(1.00, 0.85, 0.20); // Yellow-white molten
        vec3 lavaMid    = vec3(1.00, 0.40, 0.02); // Bright orange
        vec3 lavaCool   = vec3(0.55, 0.10, 0.00); // Dark red crust
        vec3 lavaBlack  = vec3(0.20, 0.06, 0.01); // Very dark solidifying

        // Texture grayscale used to pick color from gradient
        float gray = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));

        // Build crust pattern: use crustNoise to determine how "cool" a spot is
        float crustFactor = smoothstep(0.55, 0.80, crustNoise);
        // Core factor: bright spots
        float coreFactor  = smoothstep(0.50, 0.85, gray * coreNoise * waveNoise);

        vec3 lavaColor = mix(lavaCool, lavaMid, gray * 1.5); // Base warm gradient
        lavaColor = mix(lavaColor, lavaHot, coreFactor);    // Add bright hotspots
        lavaColor = mix(lavaColor, lavaBlack, crustFactor * 0.6); // Add crust darkening

        texColor.rgb = lavaColor;

        // Pulsating glow: fast small flicker + slow large wave
        float fastFlicker = 0.92 + 0.08 * sin(lt * 6.0 + hash * 12.0);
        float slowPulse   = 0.88 + 0.12 * sin(lt * 1.5 + vWorldPos.x * 0.2 + vWorldPos.z * 0.3);
        float emissivePow = fastFlicker * slowPulse;

        // Brighter emissive on hot parts
        emissive += lavaColor * 1.6 * emissivePow;
        // Extra glow on the hottest zones
        emissive += vec3(1.0, 0.7, 0.1) * coreFactor * 1.2 * emissivePow;

        // Surface vent highlights on the top face: bright rising-bubble sparks
        if (n.y > 0.5) {
            float ventHash = fract(sin(dot(floor(vWorldPos.xz * 2.0), vec2(127.1, 311.7))) * 43758.5453);
            float ventPulse = ventHash > 0.82
                ? max(0.0, sin(lt * (3.0 + ventHash * 5.0) + ventHash * 6.28)) * 0.35
                : 0.0;
            emissive += vec3(1.0, 0.6, 0.1) * ventPulse;
        }

        alphaOut = 1.0;
    }

    // Fire: realistic flickering flames with color gradient
    if (isFire) {
        float ft = uTime;
        // Multiple flicker frequencies for organic feel
        float flicker1 = 0.70 + 0.30 * sin(ft * 9.0  + vWorldPos.x * 6.1 + vWorldPos.z * 4.7);
        float flicker2 = 0.80 + 0.20 * sin(ft * 14.5 + vWorldPos.z * 8.3 + vWorldPos.y * 2.1);
        float flicker3 = 0.85 + 0.15 * sin(ft * 5.5  + vWorldPos.x * 3.2);
        float flicker  = flicker1 * flicker2 * flicker3;

        // Vertical gradient: yellow at base, orange in middle, red-orange at top
        // tileFract.y = 0 is bottom of fire, 1 is top
        float heightGrad = clamp(tileFract.y, 0.0, 1.0);

        vec3 fireBottom = vec3(1.00, 0.95, 0.30); // Bright yellow - hottest core
        vec3 fireMid    = vec3(1.00, 0.55, 0.05); // Orange
        vec3 fireTop    = vec3(0.85, 0.20, 0.00); // Red-orange - tips

        vec3 fireColor = mix(fireBottom, fireMid, smoothstep(0.0, 0.45, heightGrad));
        fireColor = mix(fireColor, fireTop, smoothstep(0.4, 0.85, heightGrad));

        // Apply texture brightness variation
        float texBright = dot(texColor.rgb, vec3(0.5, 0.4, 0.1));
        fireColor *= (0.7 + texBright * 0.8);

        texColor.rgb = fireColor;

        // Emissive: very bright, warm glow
        emissive += vec3(1.00, 0.60, 0.08) * 2.8 * flicker;         // Main warm glow
        emissive += vec3(1.00, 0.90, 0.15) * 1.2 * flicker1;        // Bright yellow core
        emissive += fireTop * 0.6 * flicker3;                         // Red edge contribution

        // Alpha: quadratic fade — dense base, wispy tips for realistic flame volume
        float baseAlpha = 1.0 - heightGrad * heightGrad * 0.72;
        alphaOut = baseAlpha * flicker1 * 0.95;
        lighting = 1.0;
    }

    // Snow sparkle (subtle Minecraft-like)
    if (isSnow && n.y > 0.5) {
        vec2 floorPos  = floor(vWorldPos.xz * 16.0);
        float sparkle = fract(sin(dot(floorPos, vec2(12.9898, 78.233))) * 43758.5453);
        if (sparkle > 0.98) {
            emissive += vec3(0.15, 0.15, 0.15) * nightFactor;
        }
        // Make snow slightly brighter and cooler
        texColor.rgb = mix(texColor.rgb, vec3(0.95, 0.98, 1.0), 0.3);
    }

    // Final color
    vec3 color = texColor.rgb * lighting + emissive;

    // Snow: bright white
    if (isSnow) {
        color = texColor.rgb * lighting + emissive;
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

    // Color grading & tone mapping (vivid, high contrast)
    // ACES-like curve approximation for better highlights
    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);

    FragColor = vec4(color, alphaOut);
}

