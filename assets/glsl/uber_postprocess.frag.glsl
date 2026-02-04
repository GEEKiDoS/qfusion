#include "include/common.glsl"
#include "include/uniforms.glsl"

qf_varying vec2 v_TexCoord;
uniform sampler2D u_BaseTexture;

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main(void)
{
    float centerDist = length(vec2(0.5, 0.5) - v_TexCoord);
    
    vec2 redOffset = vec2(0.002, 0.0) * centerDist;
    vec2 blueOffset = vec2(-0.002, 0.0) * centerDist;

    vec3 color;
    // chromatic aberration
    color.r = texture2D(u_BaseTexture, v_TexCoord + redOffset).r;
    color.g = texture2D(u_BaseTexture, v_TexCoord).g;
    color.b = texture2D(u_BaseTexture, v_TexCoord + blueOffset).b;
    
    // srgb to linear
    color = pow(color, vec3(2.2));

    color = ACESFilm(color);

    // vignette
    color *= min(1.0, pow(1.0 - centerDist, 2.2));

    // linear to srgb
    color = pow(color, vec3(0.4545));
    qf_FragColor = vec4(color, 1.0);
}
