
#include "common.incl"

#ifdef ATTRIB_COLOR0
varying vec4 vColor0;
#endif
#ifdef ATTRIB_COORDS0
varying vec2 vTex0;
#endif


#ifdef VS

void main()
{
#ifdef FULLSCREEN_QUAD
    VSOutput = aPosition;
#else
    VSOutput = uModelViewProj * aPosition;
#endif
    
#ifdef ATTRIB_COLOR0
    vColor0 = aColor0;
#endif
#ifdef ATTRIB_COORDS0
    vTex0 = aCoords0;
#endif
    
}

#else // FS

struct test
{
    float a;
    float b;
};

uniform float uTestFloat;

void main()
{
    vec4 diffuse = vec4(1,1,1,1);
    
#ifdef ATTRIB_COLOR0
    // TODO: glsl compiler is too dumb -  when ATTRIB_COLOR0 is defined, it will still say
    // that uTestFloat uniform is active even though we are replacing diffuse with vColor0,
    // not using previous diffuse value and thus not using uTestFloat uniform.
    // That can be a major problem as we will be updating value of a known uniform unnecessarily!!
    // Solutions:
    // 1. use a custom pre-parser which will keep track of used and unused uniforms. That would be
    //    cool as it could pre-process #ifdefs so that for example CVertexShader would get the list
    //    of possible defines only for VS part
    // 2. create a simple shader effect format where we would state which uniforms are in use.
    //    That is simpler than making a pre-parser, but we have to manually keep track of used
    //    uniforms which is a bit inconvenient
    diffuse = vColor0;
#endif
    
#ifdef TEXTURE0
    vec4 tex0 = texture2D(uTex0, vTex0.st);
# ifdef DEBUG_VISUALIZE_ALPHA
    diffuse *= vec4(tex0.rgb + vec3(0,0.5,0.5)*tex0.a, 1);
# else
    diffuse *= tex0;
# endif
#endif // TEXTURE0
    
#ifdef TEXTURE1
    diffuse *= texture2D(uTex1, vTex0.st);
#endif
    
    FSOutput(0) = diffuse;
}

#endif