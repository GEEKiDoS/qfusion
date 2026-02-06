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
    vec3 color;

    vec2 vigUv = v_TexCoord.xy * (1.0 - v_TexCoord.yx);
    float vig = vigUv.x * vigUv.y * 15.0;
    vig = min(1.0, pow(vig, 0.3) + 0.15); 

    // chromatic aberration
    // use vignette factor for chromatic aberration amount
    vec2 redOffset = vec2(0.002, 0.0) * (1-vig);
    vec2 blueOffset = vec2(-0.002, 0.0) * (1-vig);
    
    color.r = texture2D(u_BaseTexture, v_TexCoord + redOffset).r;
    color.g = texture2D(u_BaseTexture, v_TexCoord).g;
    color.b = texture2D(u_BaseTexture, v_TexCoord + blueOffset).b;

    // srgb to linear
    color = pow(color, vec3(2.2));

    color = ACESFilm(color);

    // linear to srgb
    color = pow(color, vec3(0.4545));

    // moving ign for debanding
    float ign = fract(fract(dot(gl_FragCoord.xy + u_ShaderTime, vec2(0.06711056, 0.00583715))) * 52.9829189);
    color.rgb += (ign * 2.0 - 1.0) * 0.01;

    // vignette
    color *= vig;

    qf_FragColor = vec4(min(vec3(1.0), color * 1.1), 1.0);
}
