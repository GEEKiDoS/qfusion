#include "include/common.glsl"
#include "include/uniforms.glsl"

qf_varying vec2 v_TexCoord;

uniform sampler2D u_BaseTexture;
uniform sampler2D u_MotionVectorTexture;
uniform sampler2D u_DepthTexture;

#define MOTION_BLUR_SAMPLES 4

void main(void)
{
    vec2 screenPos = v_TexCoord * u_Viewport.zw;
    float ign = fract(fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))) * 52.9829189) * 2.0 - 1.0;
    
    vec2 velocity = texture2D(u_MotionVectorTexture, v_TexCoord).rg * 0.3;
    vec2 startUV = v_TexCoord - velocity * 0.5;
    vec2 stepLen = velocity / float(MOTION_BLUR_SAMPLES);
    
    startUV += stepLen * ign;
    
    vec3 sum = vec3(0.0);
    for(int i = 0; i < MOTION_BLUR_SAMPLES; i++) {
        vec2 sampleUV = startUV + stepLen * float(i);
        sum += texture2D(u_BaseTexture, sampleUV).rgb;
    }
    
    qf_FragColor = vec4(sum / MOTION_BLUR_SAMPLES, 1.0);
}
