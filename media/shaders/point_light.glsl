
#include "common.incl"

varying vec4 vPosH;

#ifdef VS

void main()
{
    vec4 posH = uModelViewProj * aPosition;
    
    vPosH = posH;
    VSOutput = posH;
}

#else // FS

uniform vec3 uLightPosV; // view-space light position
uniform vec4 uLightColorInvRange;

void main()
{
    vec2 screenUV = clip_to_screenUV(vPosH);
    
    vec4 normalRT = texture2D(uNormalTex, screenUV.st);
    vec3 normalV = normalize(UnpackNormal(normalRT.rgb)); // normalize here is useful as we are reading from an RGB texture
    float depthV = texture2D(uDepthTex, screenUV.st).a * uNearFar.y; // length from eye to the '3D pixel' in view-space
    //if (depthV >= uNearFar.y)
    //    discard;
    
    // view-space position reconstruction
    vec3 posV = ReconstructViewspacePos(screenUV, depthV);
    
    vec3 toLightV = uLightPosV-posV; // vector to light
    float lightDist = length(toLightV); // distance to light
    
    vec3 lightV = toLightV/lightDist; // unit view-space light vector
    float NL = dot(normalV, lightV);
    
    // attenuation
    float att = lightDist * uLightColorInvRange.a;
    att = saturate(0.9-att*att);
    
    // diffuse
#ifdef HALF_LAMBERT // https://developer.valvesoftware.com/wiki/Half_Lambert
    float hl = (NL*att) * 0.5 + 0.5;
    vec3 diffuse = saturate( uLightColorInvRange.rgb * vec3(hl * hl) );
#else 
    vec3 diffuse = saturate( uLightColorInvRange.rgb * vec3(NL*att, NL*att, NL*att) );
#endif
    
    // specular
    vec3 viewV = normalize(posV);
    float specularInp = dot(reflect(lightV, normalV), viewV);
    float specular = att * BlinnSpecular(lightV, normalV, viewV, 20.0);
    //float specular = att * PhongSpecular(lightV, normalV, viewV, 20.0);
    
    FSOutput(0) = vec4(diffuse, specular * normalRT.a);
    //FSOutput(0) = vec4(abs(posV - texture2D(uDepthTex, screenUV.st).rgb)*10.0, 0);      // view-space position diff
#ifdef LIGHT_DEBUG
    FSOutput(0) = vec4(0.2,0,0,1);    // good for measuring light overdraw
#endif
}

#endif