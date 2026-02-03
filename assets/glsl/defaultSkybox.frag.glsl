#include "include/common.glsl"
#include "include/uniforms.glsl"

qf_varying vec2 v_TexCoord;
uniform sampler2D u_BaseTexture;

void main(void)
{
	vec4 color = vec4(qf_FrontColor);
	vec4 diffuse = vec4(qf_texture(u_BaseTexture, v_TexCoord));

	qf_FragColor = color * diffuse;
}
