
#include "common.incl"

#ifdef ATTRIB_COLOR0
varying vec4 vColor0;
#endif
#ifdef ATTRIB_COORDS0
varying vec2 vTex0;
#endif

varying vec4 vPosH;


#ifdef VS

void main()
{
    vec4 posH = uModelViewProj * aPosition;
    
    vPosH = posH;
    VSOutput = posH;
    
#ifdef ATTRIB_COLOR0
    vColor0 = aColor0;
#endif
#ifdef ATTRIB_COORDS0
    vTex0 = aCoords0;
#endif
}

#else // FS

uniform sampler2D uDiffuseAcc;
uniform vec3 uAmbientColor;

void main()
{
    vec2 screenUV = clip_to_screenUV(vPosH);
    vec4 lightDiffuse = texture2D(uDiffuseAcc, screenUV.st);
    
    vec3 diffuse = vec3(1,1,1);
#ifdef TEXTURE0
    diffuse *= texture2D(uTex0, vTex0.st).rgb;
#endif
    diffuse *= max(lightDiffuse.rgb, uAmbientColor);
    
    vec3 specular = normalize(diffuse)*lightDiffuse.a;
    
    //FSOutput(0) = vec4(posV, 1);
    FSOutput(0) = vec4(diffuse + specular, 1);
}

#endif