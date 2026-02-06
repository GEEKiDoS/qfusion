#ifdef LIGHTMAP_ARRAYS
#define LightmapSampler sampler2DArray
#define SampleLightmap(t, c, l) qf_textureArray(t, vec3(c, l))
#else
#define LightmapSampler sampler2D
#define SampleLightmap(t, c, l) qf_texture(t, c)
#endif

#define BICUBIC_LIGHTMAP 1

#if BICUBIC_LIGHTMAP
uniform vec2 u_LightmapTexelSize;

// MIT
// https://github.com/godotengine/godot/pull/89919
float w0(float a) {
	return (1.0 / 6.0) * (a * (a * (-a + 3.0) - 3.0) + 1.0);
}

float w1(float a) {
	return (1.0 / 6.0) * (a * a * (3.0 * a - 6.0) + 4.0);
}

float w2(float a) {
	return (1.0 / 6.0) * (a * (a * (-3.0 * a + 3.0) + 3.0) + 1.0);
}

float w3(float a) {
	return (1.0 / 6.0) * (a * a * a);
}

// g0 and g1 are the two amplitude functions
float g0(float a) {
	return w0(a) + w1(a);
}

float g1(float a) {
	return w2(a) + w3(a);
}

// h0 and h1 are the two offset functions
float h0(float a) {
	return -1.0 + w1(a) / (w0(a) + w1(a));
}

float h1(float a) {
	return 1.0 + w3(a) / (w2(a) + w3(a));
}

vec4 Lightmap( LightmapSampler lightmap, vec2 texCoord, float lightmapLayer ) {
    texCoord = texCoord * (1.0 / u_LightmapTexelSize) + vec2(0.5, 0.5);

    vec2 iuv = floor( texCoord.xy );
    vec2 fuv = fract( texCoord.xy );

    float g0x = g0( fuv.x );
    float g1x = g1( fuv.x );
    float h0x = h0( fuv.x );
    float h1x = h1( fuv.x );
    float h0y = h0( fuv.y );
    float h1y = h1( fuv.y );

    vec2 p0 = ( vec2( iuv.x + h0x, iuv.y + h0y ) - vec2( 0.5, 0.5 ) ) * u_LightmapTexelSize;
    vec2 p1 = ( vec2( iuv.x + h1x, iuv.y + h0y ) - vec2( 0.5, 0.5 ) ) * u_LightmapTexelSize;
    vec2 p2 = ( vec2( iuv.x + h0x, iuv.y + h1y ) - vec2( 0.5, 0.5 ) ) * u_LightmapTexelSize;
    vec2 p3 = ( vec2( iuv.x + h1x, iuv.y + h1y ) - vec2( 0.5, 0.5 ) ) * u_LightmapTexelSize;

    vec4 result = ( g0( fuv.y ) * ( g0x * SampleLightmap( lightmap, p0, lightmapLayer ) + g1x * SampleLightmap( lightmap, p1, lightmapLayer ) ) ) +
                  ( g1( fuv.y ) * ( g0x * SampleLightmap( lightmap, p2, lightmapLayer ) + g1x * SampleLightmap( lightmap, p3, lightmapLayer ) ) );

    return result;
}
#else
#define Lightmap(t,c,l) SampleLightmap(t,c,l)
#endif
