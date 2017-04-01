
#include "common.incl"

varying vec3 vNormalV;
#ifdef NORMAL_SPECULAR_MAP
varying vec2 vTex0;
varying vec3 vTangentV;
varying vec3 vBitangentV;
#endif

#ifdef VS

void main()
{
    VSOutput = uModelViewProj * aPosition;
    vNormalV = normalize((uModelView * vec4(aNormal, 0)).xyz); // normalize in VS is enough but can be theoretically removed as well
#ifdef NORMAL_SPECULAR_MAP
    vTex0 = aCoords0;
    vTangentV = normalize((uModelView * vec4(aTangent, 0)).xyz);
    vBitangentV = normalize((uModelView * vec4(aBitangent, 0)).xyz);
#endif
}

#else // FS

void main()
{
#ifdef NORMAL_SPECULAR_MAP // normal map texture
    
    vec4 normalTex = texture2D(uTexNormalSpecular, vTex0.st);
    vec3 normalT = normalTex.rgb * vec3(2) - vec3(1);
# ifdef NORMAL_SPECULAR_MAP_INVERSEY
    normalT.y = -normalT.y;
# endif
    vec3 normalV = vec3(dot(normalT,vec3(vTangentV.x, vBitangentV.x, vNormalV.x)),
                        dot(normalT,vec3(vTangentV.y, vBitangentV.y, vNormalV.y)),
                        dot(normalT,vec3(vTangentV.z, vBitangentV.z, vNormalV.z)));
    
    FSOutput(0) = vec4(PackNormal(normalV), normalTex.a);

#else // no normal map texture

    FSOutput(0) = vec4(PackNormal(vNormalV), 0.0); // default specular is 0

#endif
}

#endif