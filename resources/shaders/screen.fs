#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool grayScaleInd;

void main()
{
    vec3 col = texture(screenTexture, TexCoords).rgb;
    if(grayScaleInd){
    float grayscale = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
    FragColor = vec4(vec3(grayscale), 1.0);
    }else{
    FragColor = vec4(col, 1.0);
    }

}