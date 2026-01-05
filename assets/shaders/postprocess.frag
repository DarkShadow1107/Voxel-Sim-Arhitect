#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main() {
    vec3 col = texture(screenTexture, TexCoords).rgb;
    // Simple grayscale post-processing effect
    float average = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
    FragColor = vec4(vec3(average), 1.0);
}
