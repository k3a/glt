//
//  main.cpp
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#include <iostream>
#include <set>

#include "glstuff.h"
#include "main.h"
#include "shared.h"
#include "Engine.h"
#include "Shaders.h"


////////////////////////////////////////////////////////////////////////////////////////////////
/// GLUT CALLBACKS

static void keyboard(unsigned char key, int x, int y){ CEngine::Inst()->InKeyboard(key, true, false); }
static void keyboardUp(unsigned char key, int x, int y){ CEngine::Inst()->InKeyboard(key, false, false); }
static void keyboardSpecial(int key, int x, int y){ CEngine::Inst()->InKeyboard(key, true, true); }
static void keyboardSpecialUp(int key, int x, int y){ CEngine::Inst()->InKeyboard(key, false, false); }
static void mouse(int btn, int state, int ax, int ay)
{
    if (GLUT_DOWN != state && GLUT_UP != state)
        return;
    
    CEngine::Inst()->InMouseClick(btn, ax, ay, GLUT_DOWN == state);
}
static void motion(int ax, int ay){ CEngine::Inst()->InMouseMove(ax, ay, true); }
static void passiveMotion(int ax, int ay){ CEngine::Inst()->InMouseMove(ax, ay, false); }\
static void update(int value)
{
    static int s_oldTime = 0;
    
    int tim = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (tim - s_oldTime)*0.001f;
    s_oldTime = tim;
    
    // update the engine
    CEngine::Inst()->InUpdate(deltaTime);
    
    // request next frame
    glutPostRedisplay();
#ifdef DEBUG
    glutTimerFunc(25, update, 0);
#endif
}
#ifndef DEBUG
static void idle()
{
    // refresh as often as possible in release builds
    update(0);
}
#endif
static void reshape(int w, int h){ CEngine::Inst()->InSizeChange(w, h); }
static void display(void){ CEngine::Inst()->InRender(); }


////////////////////////////////////////////////////////////////////////////////////////////////
/// MAIN
static char s_executableDir[2048];
const char* GetExecutableDir()
{
    return s_executableDir;
}

int main(int argc, char * argv[])
{
    strcpy(s_executableDir, argv[0]);
    for (int i=(int)strlen(s_executableDir); i>=0; i--)
    {
        if (s_executableDir[i] == '\\' || s_executableDir[i] == '/')
        {
            s_executableDir[i]=0;
            break;
        }
    }
    
    glutInit(&argc, argv);
    
    int wid = 800, hei = 600;
    
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH /*| GLUT_MULTISAMPLE*/); // double buffers RGBA, depth
    glutInitWindowSize(wid, hei);
    glutCreateWindow("glt");
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(keyboardSpecial);
	glutSpecialUpFunc(keyboardSpecialUp);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
    glutPassiveMotionFunc(passiveMotion);
#ifndef DEBUG
    glutIdleFunc(idle);
#endif

#ifdef WIN32
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		printf("GLEW Error: %s\n", glewGetErrorString(err));
		return 1;
	}
#endif
    
    CEngine::Inst()->InInit();
    CEngine::Inst()->InSizeChange(wid, hei);
    
    /* // uncomment to disable sync with refresh
    CGLContextObj context = CGLGetCurrentContext();
    const GLint SYNC_TO_REFRESH = 0;
    CGLSetParameter(context, kCGLCPSwapInterval, &SYNC_TO_REFRESH);
    */
     
    glutTimerFunc(25, update, 0);
    glutMainLoop();

    return 0;
}



