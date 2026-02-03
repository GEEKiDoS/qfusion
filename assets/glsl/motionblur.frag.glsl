#include "include/common.glsl"
#include "include/uniforms.glsl"

qf_varying vec2 v_TexCoord;

uniform sampler2D u_BaseTexture;
uniform sampler2D u_MotionVectorTexture;
uniform sampler2D u_DepthTexture;

#define MOTION_BLUR_SAMPLES 4

void main(void)
{
    float random = fract(sin(dot(v_TexCoord, vec2(12.9898, 78.233))) * 43758.5453);
    
    vec2 velocity = -texture2D(u_MotionVectorTexture, v_TexCoord).rg;
    vec2 startUV = v_TexCoord - velocity * 0.5;
    vec2 stepLen = velocity / float(MOTION_BLUR_SAMPLES);
    
    startUV += stepLen * random;
    
    // TODO: Depth-aware motion blur
    vec3 sum = vec3(0.0);
    for(int i = 0; i < MOTION_BLUR_SAMPLES; i++) {
        vec2 sampleUV = startUV + stepLen * float(i);
        sum += texture2D(u_BaseTexture, sampleUV).rgb;
    }
    
    qf_FragColor = vec4(sum / float(MOTION_BLUR_SAMPLES), 1.0);
}
