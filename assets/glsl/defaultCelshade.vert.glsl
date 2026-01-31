#include "include/common.glsl"
#include "include/uniforms.glsl"
#include "include/attributes.glsl"
#include "include/rgbgen.glsl"
#include_if(APPLY_FOG) "include/fog.glsl"

qf_varying vec2 v_TexCoord;
qf_varying vec3 v_TexCoordCube;

#if defined(APPLY_FOG) && !defined(APPLY_FOG_COLOR)
qf_varying vec2 v_FogCoord;
#endif

uniform mat3 u_ReflectionTexMatrix;

void main(void)
{
	vec4 Position = a_Position;
	vec3 Normal = a_Normal.xyz;
	vec2 TexCoord = a_TexCoord;
	vec4 inColor = vec4(a_Color);

	QF_TransformVerts(Position, Normal, TexCoord);

	vec4 outColor = VertexRGBGen(Position, Normal, inColor);

#ifdef APPLY_FOG
#ifdef APPLY_FOG_COLOR
	FogGenColor(Position, outColor, u_BlendMix);
#else
	FogGenCoord(Position, v_FogCoord);
#endif
#endif

	qf_FrontColor = vec4(outColor);

#if defined(APPLY_TC_MOD)
	v_TexCoord = TextureMatrix2x3Mul(u_TextureMatrix, TexCoord);
#else
	v_TexCoord = TexCoord;
#endif
	v_TexCoordCube = u_ReflectionTexMatrix * reflect(normalize(Position.xyz - u_EntityDist), Normal.xyz);

	gl_Position = u_ModelViewProjectionMatrix * Position;

#ifdef APPLY_MOTION_VECTORS
	// 计算上一帧位置
	vec4 PrevPosition = u_LastModelViewProjectionMatrix * Position;

	// 应用上一帧的模型视图投影变换
	// 对于静态模型，使用上一帧矩阵计算位置（包含相机和物体运动）
	// 对于骨骼动画，使用上一帧矩阵和上一帧骨骼位置计算位置
	v_MotionVector = (gl_Position.xy / gl_Position.w) - (PrevPosition.xy / PrevPosition.w);
#endif
}
