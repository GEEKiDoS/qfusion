#include "include/common.glsl"
#include "include/uniforms.glsl"
#include "include/attributes.glsl"
#include "include/rgbgen.glsl"

qf_varying vec2 v_TexCoord;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;

	QF_TransformVerts(Position, Normal, TexCoord);

	qf_FrontColor = a_Color;
	v_TexCoord = TexCoord;
	gl_Position = u_ModelViewProjectionMatrix * Position;
}
