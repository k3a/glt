
#include "common.incl"

varying vec2 vCoord;
varying vec2 vKernelCoord;

#ifdef VS

void main()
{
    VSOutput = aPosition;
    vCoord = aPosition.xy * vec2(0.5) + vec2(0.5);
    vKernelCoord = vCoord * (uScreenSize / vec2(4.0));
}

#else // FS

vec3 randomSpherePoint(vec2 co)
{
    float s = rand(co) * 3.1415926 * 2.0;
    float t = rand(co*s) * 2.0 - 1.0;
    return vec3(sin(s), cos(s), t) / sqrt(1.0 + t * t);
}

uniform sampler2D uRandomTex;
uniform int uKernelSize;
uniform vec3 uKernel[8];

void main()
{
    vec3 rvec = texture2D(uRandomTex, vKernelCoord).xyz * 2.0 - 1.0;
    
    vec4 normalRT = texture2D(uNormalTex, vCoord.st);
    vec3 normalV = normalize(UnpackNormal(normalRT.rgb)); // normalize here is useful as we are reading from an RGB texture
    float depthV = texture2D(uDepthTex, vCoord.st).a * uNearFar.y;
    vec3 posV = ReconstructViewspacePos(vCoord, depthV);
    if (posV.z <= -uNearFar.y) {
        FSOutput(0) = vec4(1, 1, 1, 1);
        return;
    }
    
    vec3 tangentV = normalize(rvec - normalV * dot(rvec, normalV));
    vec3 bitangentV = cross(normalV, tangentV);
    mat3 tbn = mat3(tangentV, bitangentV, normalV);
    
    const float radius = 0.4;
    
    float occlusion = 0.0;
    for (int i = 0; i < uKernelSize; i++)
    {
        // get sample position:
        vec3 sampleV = tbn * uKernel[i];
        sampleV = posV + sampleV * radius;
        //sampleV = posV + /*sampleV*/randomSpherePoint(vCoord) * radius;
        
        // project sample position:
        vec4 offset = vec4(sampleV, 0.0);
        offset = uProj * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        // get sample depth:
        float sampleDepth = texture2D(uDepthTex, offset.xy).a * uNearFar.y;
        //FSOutput(0) = vec4(offset.xy,0, 1);
        //return;
        
        // range check & accumulate:
        float rangeCheck = smoothstep(1.0, 0.0, abs(depthV - sampleDepth) - 0.3*radius); // maximum depth difference
        occlusion += (sampleDepth < -sampleV.z ? 1.0 : 0.0) * rangeCheck; // if sampled point is farther, increase occlusion
    }
    occlusion = 1.0 - (occlusion / float(uKernelSize));
    
    FSOutput(0) = vec4(vec3(occlusion), 1);
}

/*
vec3 randomSpherePoint(vec2 co)
{
    float s = rand(co) * 3.1415926 * 2.0;
    float t = rand(co*s) * 2.0 - 1.0;
    return vec3(sin(s), cos(s), t) / sqrt(1.0 + t * t);
}

void main ()
{
    float occlusion = 0.0;
        
        //get depth of current pixel
        float depth = texture2D(uDepthTex, vCoord.st).r * uNearFar.y;
        
        
        //reconstruct view-space pixel position
        vec3 origin = ReconstructViewspacePos(vCoord, depth);
        
        
        int kernelSize = 8;
        float radius = 1.0;
        
        for (int i=0; i<kernelSize; ++i)
        {
            vec3 samplePos = vec3(1,0,0) * radius + origin;
            
            vec4 offset = vec4(samplePos, 0.0);
            offset = uProj * offset;
            offset.xy /= offset.w;
            offset.xy = offset.xy * 0.5 + 0.5;
            
            float sampleDepth = texture2D(uDepthTex, offset.xy).r;
            //sampleDepth = linearizeDepth(sampleDepth, ProjectionMatrix);
            
            FSOutput(0) = vec4(offset.xy,0, 1);
            return;
            
            occlusion += step(sampleDepth, -samplePos.z);
        }
    occlusion = 1.0 - (occlusion / float(uKernelSize));
    
    FSOutput(0) = vec4(vec3(occlusion), 1);
}
 */

/* //funkcni s randomSpherePoint
void main()
{
    vec3 rvec = texture2D(uRandomTex, vKernelCoord).xyz * 2.0 - 1.0;
    
    vec4 normalRT = texture2D(uNormalTex, vCoord.st);
    vec3 normalV = normalize(UnpackNormal(normalRT.rgb)); // normalize here is useful as we are reading from an RGB texture
    float depthV = texture2D(uDepthTex, vCoord.st).r * uNearFar.y;
    vec3 posV = ReconstructViewspacePos(vCoord, depthV);
    
    vec3 tangentV = normalize(rvec - normalV * dot(rvec, normalV));
    vec3 bitangentV = cross(normalV, tangentV);
    mat3 tbn = mat3(tangentV, bitangentV, normalV);
    
    const float radius = 1.0;
    
    float occlusion = 0.0;
    for (int i = 0; i < uKernelSize; i++)
    {
        // get sample position:
        vec3 sampleV = tbn * uKernel[i];
        sampleV = posV + randomSpherePoint(vCoord*vec2(i,i)) * radius;
        
        // project sample position:
        vec4 offset = vec4(sampleV, 0.0);
        offset = uProj * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        // get sample depth:
        float sampleDepth = texture2D(uDepthTex, offset.xy).r * uNearFar.y;
        
        // range check & accumulate:
        float rangeCheck = abs(depthV - sampleDepth) < radius ? 1.0 : 0.0; // maximum depth difference
        occlusion += (sampleDepth < -sampleV.z ? 1.0 : 0.0) * rangeCheck; // if sampled point is farther, increase occlusion
    }
    occlusion = 1.0 - (occlusion / float(uKernelSize));
    
    FSOutput(0) = vec4(vec3(occlusion), 1);
}*/

#endif



