#include "common.incl"

varying vec2 vTex0;

#ifdef VS

void main()
{
    vTex0 = aCoords0;
    VSOutput = aPosition;
}

#else // FS

void main()
{
    const int uBlurSize = 4;
    
    vec2 texelSize = uInvTex0Size;
    
    float result = 0.0;
    vec2 hlim = vec2(float(-uBlurSize) * 0.5 + 0.5);
    for (int i = 0; i < uBlurSize; ++i)
    {
        for (int j = 0; j < uBlurSize; ++j)
        {
            vec2 offset = (hlim + vec2(float(i), float(j))) * texelSize;
            result += texture2D(uTex0, vTex0 + offset).r;
        }
    }
    
    result = result / float(uBlurSize * uBlurSize);
    
    FSOutput(0) = vec4(vec3(result*result), 1);
}

#endif