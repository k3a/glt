//
//  Shared.cpp
//  glt
//
//  Created by Mario Hros on 2. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#include "Shared.h"
#include "glstuff.h"
#include "main.h"

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

const char* basename(const char* str)
{
    const char* ret = str;
    for (int i=0; i<strlen(str); i++)
    {
        if (str[i] == '\\' || str[i] == '/')
            ret = &str[i+1];
    }
    return ret;
}

const char* LocateFile(const char* filename)
{
    static char temp[2048];
    
    strcpy(temp, GetExecutableDir());
    
    strcat(temp, "/");
    strcat(temp, filename);
    
    struct stat stFileInfo;
	if(stat(temp, &stFileInfo) != 0)
    {
        // doesn't exist, try basename
        strcpy(temp, GetExecutableDir());
        
        strcat(temp, "/");
        char* tmp = strdup(filename);
        strcat(temp, basename(tmp));
        free(tmp);
        
        if(stat(temp, &stFileInfo) != 0)
            return NULL; // hmm, givig up
    }
        
    return temp;
}

void PrintGLError(const char* where)
{
#ifndef DEBUG
    return;
#endif
    
    GLenum error = glGetError();
    if (!error) return;
    
	if(error == GL_NO_ERROR)
		printf("GL error during %s: GL_NO_ERROR: No error has been recorded.\n", where);
	else if(error == GL_INVALID_ENUM)
		printf("GL error during %s: GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument.\n", where);
	else if(error == GL_INVALID_VALUE)
		printf("GL error during %s: GL_INVALID_VALUE: A numeric argument is out of range.\n", where);
	else if(error == GL_INVALID_OPERATION)
		printf("GL error during %s: GL_INVALID_OPERATION: The specified operation is not allowed in the current state.\n", where);
	else if(error == GL_STACK_OVERFLOW)
		printf("GL error during %s: GL_STACK_OVERFLOW: This command would cause a stack overflow.\n", where);
	else if(error == GL_STACK_UNDERFLOW)
		printf("GL error during %s: GL_STACK_UNDERFLOW: This command would cause a stack underflow.\n", where);
	else if(error == GL_OUT_OF_MEMORY)
		printf("GL error during %s: GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.\n", where);
	//GL_TABLE_TOO_LARGE was introduced in GL version 1.2
	else if(error == GL_TABLE_TOO_LARGE)
		printf("GL error during %s: GL_TABLE_TOO_LARGE: The specified table exceeds the implementation's maximum supported table size.\n", where);
	else
		printf("GL error during %s: Unknown OpenGL Error Code!\n", where);
}









