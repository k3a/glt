
#include "common.incl"

#ifdef VS

void main()
{
    VSOutput = uModelViewProj * aPosition;
}

#else // FS

void main()
{
    FSOutput(0) = vec4( (1.0/gl_FragCoord.w)/uNearFar.y );
}

#endif