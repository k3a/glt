//
//  Shared.h
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef glt_Shared_h
#define glt_Shared_h

#include <stdlib.h> // for rand()

#define STD_FOREACH(collectionType, collectionInst, iteratorName) \
for( collectionType::iterator iteratorName = (collectionInst).begin() ; iteratorName != (collectionInst).end() ; iteratorName++)

#define STD_FOREACH_R(collectionType, collectionInst, iteratorName) \
for( collectionType::reverse_iterator iteratorName = (collectionInst).rbegin() ; iteratorName != (collectionInst).rend() ; iteratorName++)

#define STD_CONST_FOREACH(collectionType, collectionInst, iteratorName) \
for( collectionType::const_iterator iteratorName = (collectionInst).begin() ; iteratorName != (collectionInst).end() ; iteratorName++)

#define STD_CONST_FOREACH_R(collectionType, collectionInst, iteratorName) \
for( collectionType::const_reverse_iterator iteratorName = (collectionInst).rbegin() ; iteratorName != (collectionInst).rend() ; iteratorName++)

#define STD_FOREACH_NOINC(collectionType, collectionInst, iteratorName) \
for( collectionType::iterator iteratorName = (collectionInst).begin() ; iteratorName != (collectionInst).end() ; )

#define STD_FOREACH_R_NOINC(collectionType, collectionInst, iteratorName) \
for( collectionType::reverse_iterator iteratorName = (collectionInst).rbegin() ; iteratorName != (collectionInst).rend() ; )

#define STD_CONST_FOREACH_NOINC(collectionType, collectionInst, iteratorName) \
for( collectionType::const_iterator iteratorName = (collectionInst).begin() ; iteratorName != (collectionInst).end() ; )

#define STD_CONST_FOREACH_R_NOINC(collectionType, collectionInst, iteratorName) \
for( collectionType::const_reverse_iterator iteratorName = (collectionInst).rbegin() ; iteratorName != (collectionInst).rend() ; )

#define INVALID_GL_OBJECT   0xfffffff0

/// Returns the lower of or equal to specified numbers
template <typename T>
inline T Min(T a, T b)
{
    return (b<a)?b:a;
}

/// Returns the higher of or equal to specified numbers
template <typename T>
inline T Max(T a, T b)
{
    return (b>a)?b:a;
}

/// Returns the number restricted within specified min max boundaries
template <typename T>
inline T MinMax(T val, T min, T max)
{
    return Max(Min(val, max), min);
}

/// Finds nearest power-of-two dimensions
inline float NearestPOTSize(float input, float maxSize=0)
{
	if (maxSize && input>maxSize) input=maxSize;
	
	float output = 2;
	
	while (input > 2)
	{
		input *= 0.5f;
		output *= 2;
	}
	
	return output;
}

/** Returns the random floating-point value between the min and max incl.
 @param min Minimal value
 @param max Maximal value
 */
inline float FloatRand(float min, float max)
{
    return min + (max-min)/100000*(rand()%100000);
}


const char* LocateFile(const char* filename);
void PrintGLError(const char* where);

const char* basename(const char* str);

#endif
