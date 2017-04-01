//
//  glstuff.h
//  glt
//
//  Created by Mario Hros on 2. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef glt_glstuff_h
#define glt_glstuff_h

#if defined(__APPLE__)
# include <OpenGL/OpenGL.h>
# include <GLUT/glut.h>
#else
# include <GL/glew.h>
# include <GL/glut.h>
#endif

#ifdef __APPLE__
# define GL_RGBA16F GL_RGBA16F_ARB
# define GL_RGBA32F GL_RGBA32F_ARB
#endif

void PrintGLError(const char* where);

#endif
