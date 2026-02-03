qf_varying vec4 v_PositionClip;
qf_varying vec4 v_LastPositionClip;

#ifdef VERTEX_SHADER
void ApplyMotionVector() {
    v_PositionClip = gl_Position;

	vec4 Position = a_Position;
#ifdef QF_NUM_BONE_INFLUENCES
	QF_VertexDualQuatsTransformPosition_Last(Position);
#endif

	v_LastPositionClip = u_LastModelViewProjectionMatrix * Position;
}
#endif
#ifdef FRAGMENT_SHADER
void ApplyMotionVector() {
    vec2 motionVector = (v_PositionClip.xy / v_PositionClip.w) - (v_LastPositionClip.xy / v_LastPositionClip.w);
	qf_FragMotionVector.rg = motionVector;
	qf_FragMotionVector.b = length(motionVector);
}
#endif
