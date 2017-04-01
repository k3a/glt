//
//  Engine.cpp
//  glt
//
//  Created by Mario Hros on 3. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

// Sponza test file Copyright 2002 Marko Dabrovic Morgan McGuire painted bump maps. Kenzie Lamar assigned UVs and converted to OBJ.
// http://graphics.cs.williams.edu/data/meshes/dabrovic-sponza-copyright.html

#include "Engine.h"

#include "TestScene.h"

#include "CVar.h"
#include "glstuff.h"
#include "glv.h"
#include "glm.hpp"
#include "../assimp/include/assimp/cimport.h"
#include <time.h>

static CVar cvQuit("quit");
static CVar cvExtensios("r_printExtensions");

static CVar cvWireframe("r_wireframe", false, CVar::FLAG_GUI_TWEAKABLE);
static CVar cvFov("r_fov", 1, CVar::FLAG_GUI_TWEAKABLE|CVar::FLAG_GUI_PRINT, 0.2, 1.5);
static CVar cvMouseSens("r_mouseSensitivity", 9.0);
static CVar cvKeySens("r_keyboardSensitivity", 6.0);

static glv::TextView* s_console = NULL;

#ifdef WIN32
# define strncasecmp strnicmp
# define strcasecmp stricmp
# define snprintf _snprintf
#endif

////////////////////////////////////////////////////////////////////////////////////////////////
/// HELPERS

struct SLabelCVarTuple
{
    SLabelCVarTuple(glv::Label* lbl, CVar* cv) : label(lbl), cvar(cv){}
    glv::Label* label;
    CVar* cvar;
    
    bool operator <(const SLabelCVarTuple& b)const{ return strcmp(cvar->GetName(), b.cvar->GetName())<0; }
};
typedef std::set<SLabelCVarTuple> PrintableCVarSet;
static PrintableCVarSet s_printableCvars;

// this must be called whenever a GLUT input event for a keyboard or mouse
// callback is generated.
static void modToGLV(glv::GLV * g)
{
	if(g){
		int mod = glutGetModifiers();
		const_cast<glv::Keyboard *>(&g->keyboard())->alt  (mod & GLUT_ACTIVE_ALT);
		const_cast<glv::Keyboard *>(&g->keyboard())->ctrl (mod & GLUT_ACTIVE_CTRL);
		const_cast<glv::Keyboard *>(&g->keyboard())->shift(mod & GLUT_ACTIVE_SHIFT);
		//printf("a:%d c:%d s:%d\n", g->keyboard.alt(), g->keyboard.ctrl(), g->keyboard.shift());
	}
}

static void keyToGLV(glv::GLV * g, unsigned int key, bool down, bool special){
	//printf("GLUT: keyboard event k:%d d:%d s:%d\n", key, down, special);
    
	if(g){
		if(special){
            
#define CS(glut, glvK) case GLUT_KEY_##glut: key = glv::Key::glvK; break;
			switch(key){
                    CS(LEFT, Left) CS(UP, Up) CS(RIGHT, Right) CS(DOWN, Down)
                    CS(PAGE_UP, PageUp) CS(PAGE_DOWN, PageDown)
                    CS(HOME, Home) CS(END, End) CS(INSERT, Insert)
                    
                    CS(F1, F1) CS(F2, F2) CS(F3, F3) CS(F4, F4)
                    CS(F5, F5) CS(F6, F6) CS(F7, F7) CS(F8, F8)
                    CS(F9, F9) CS(F10, F10)	CS(F11, F11) CS(F12, F12)
			}
#undef CS
		}
		else{
            
			//printf("GLUT i: %3d %c\n", key, key);
            
#define MAP(i,o) case i: key=o; break
            
            //			bool shft = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
            //			if(shft && (key>32 && key<127)){
            //				const char * QWERTYunshifted =
            //					" 1\'3457\'908=,-./0123456789;;,=./"
            //					"2abcdefghijklmnopqrstuvwxyz[\\]6-"
            //					"`abcdefghijklmnopqrstuvwxyz[\\]`"
            //				;
            //				key = QWERTYunshifted[key-32];
            //			}
            
			// Reassign keycodes when CTRL is down
#ifdef GLV_PLATFORM_OSX
            
			bool ctrl = glutGetModifiers() & GLUT_ACTIVE_CTRL;
            
			if(ctrl){
				// All alphabetical keys get dropped to lower ASCII range.
				// Some will conflict with standard non-printable characters.
				// There is no way to detect this, since the control modified
				// keycode gets sent to the GLUT callback. We will assume that
				// ctrl-key events are the printable character keys.
                
				//Enter		=3
				//BackSpace	=8
				//Tab		=9
				//Return	=13
				//Escape	=27
				
				if(key <= 26){ key += 96; }
				
				// only some non-alphabetic keys are wrong...
				else{
					
					switch(key){
                            MAP(27, '[');
                            MAP(28, '\\');
                            MAP(29, ']');
                            MAP(31, '-');
					};
					
				}
			}
			
#endif
			
#undef MAP
            
			//printf("GLUT o: %3d %c\n", key, key);
		}
		
		down ? g->setKeyDown(key) : g->setKeyUp(key);
		modToGLV(g);
		g->propagateEvent();
	}
}

static void motionToGLV(glv::GLV * g, int ax, int ay, glv::Event::t e)
{
	if(g){
        glv::space_t x = (glv::space_t)ax;
		glv::space_t y = (glv::space_t)ay;
		glv::space_t relx = x;
		glv::space_t rely = y;
        
		g->setMouseMotion(relx, rely, e);
		g->setMousePos((int)x, (int)y, relx, rely);
		//modToGLV();	// GLUT complains about calling glutGetModifiers()
		g->propagateEvent();
	}
}


static void cvarValueChanged(const glv::Notification& n)
{
    glv::Widget* w = (glv::Widget*)n.sender();
    CVar* cv = n.receiver<CVar>();
    
    switch(cv->GetType())
    {
        T_BOOL:
            cv->Set(w->getValue<bool>());
            break;
        T_FLOAT:
            cv->Set(w->getValue<float>());
            break;
        T_INT:
            cv->Set(w->getValue<int>());
            break;
        default:
            cv->Set(w->getValue<std::string>().c_str());
            break;
    }
}

static bool glvKeyDown(glv::View * v, glv::GLV& glv)
{
    static std::vector<std::string> s_consoleHistory;
    static unsigned s_curHistory=0;
    
	switch(glv.keyboard().key())
    {
		case '`':
        {
            glv.toggle(glv::Visible);
            glv.setFocus(s_console);
            return false;
        }
        case 13: // enter
        {
            if (s_console->enabled(glv::Focused))
            {
                const char* line = s_console->getValue().c_str();
                printf("] %s\n", line);
                if (!CEngine::Inst()->ProcessConsoleLine(line))
                    printf("Unknown variable or command in the line '%s'!\n", line);
                
                s_curHistory = 0;
                s_consoleHistory.push_back(std::string(line));
                
                s_console->setValue("");
                s_console->setPos(0);
                return false;
            }
            break;
        }
        case 9: // tab
        {
            // try to auto-complete
            const char* prefix = s_console->getValue().c_str();
            CVar* cv = CVar::FirstCVar();
            while(cv)
            {
                if (!strncasecmp(prefix, cv->GetName(), strlen(prefix)))
                {
                    char buf[1023];
                    sprintf(buf, "%s ", cv->GetName());
                    std::string str(buf);
                    
                    s_console->setValue(str);
                    s_console->setPos((int)str.size());
                    break;
                }
                cv = cv->NextCVar();
            }
            break;
        }
        case 270: // up arrow
        {
            if (s_curHistory < s_consoleHistory.size())
                s_curHistory++;
            
            if (s_curHistory>0)
            {
                std::string str = s_consoleHistory[s_consoleHistory.size()-s_curHistory];
                s_console->setValue(str);
                s_console->setPos((int)str.size());
            }
            
            return false;
        }
        case 272: // dn arrow
        {
            if (s_curHistory > 1 && s_consoleHistory.size() > 0)
            {
                s_curHistory--;
                std::string str = s_consoleHistory[s_consoleHistory.size()-s_curHistory];
                s_console->setValue(str);
                s_console->setPos((int)str.size());
            }
            else if (s_curHistory==1)
            {
                s_curHistory = 0;
                s_console->setValue("");
                s_console->setPos(0);
            }
            
            return false;
        }
	}
	return true;
}

static bool CheckExtension(const char *extName)
{
    char *p = (char *) glGetString(GL_EXTENSIONS);
    char *end;
    int extNameLen;

    extNameLen = (int)strlen(extName);
    end = p + strlen(p);

    while (p < end)
    {
     int n = (int)strcspn(p, " ");
     if ((extNameLen == n) && (strncmp(extName, p, n) == 0))
         return true;
     p += (n + 1);
    }
    
    return false;
 }

void CEngine::InitGLV()
{
    _glv.colors().set(glv::StyleColor::WhiteOnBlack);
    _glv.colors().back.set(1,0.1);
    _glv.colors().fore.set(0.6,0.6,0.6,0.8);
    _glv(glv::Event::KeyDown, glvKeyDown);
    
    
    /// RIGHT COLUMN /////////
    glv::Placer* rightColumn = new glv::Placer(_glv, glv::Direction::S, glv::Place::TL, -100, 10, 4);
    
    // STATUS LINE
    _status = new glv::Label("Status", 0, 0);
    _status->anchor(glv::Place::TR);
    *rightColumn << _status;
    
    // PRINTABLE CVARS
    CVar* cv = CVar::FirstCVar();
    while(cv)
    {
        // skip non-gui-editable CVars
        if ( !(cv->GetFlags() & CVar::FLAG_GUI_PRINT) )
        {
            cv = cv->NextCVar();
            continue;
        }
        
        glv::Label* lbl = new glv::Label();
        lbl->pos(glv::Place::TR).anchor(glv::Place::TR);
        lbl->setValue(cv->GetName());
        *rightColumn << lbl;
        
        s_printableCvars.insert(SLabelCVarTuple(lbl, cv));
        
        // next
        cv = cv->NextCVar();
    }
    
    /// LEFT COLUMN //////////
    glv::Placer* leftColumn = new glv::Placer(_glv, glv::Direction::S, glv::Place::TL, 10, 10, 4);
    
    // CONSOLE
    s_console = new glv::TextView(glv::Rect(300, 20), 8);
    (*s_console)(glv::Event::KeyDown, glvKeyDown);
    *leftColumn << s_console;
    
    // CVARS
    cv = CVar::FirstCVar();
    while(cv)
    {
        // skip non-gui-editable CVars
        if ( !(cv->GetFlags() & CVar::FLAG_GUI_TWEAKABLE) )
        {
            cv = cv->NextCVar();
            continue;
        }
        
        if (cv->GetType() == T_BOOL)
        {
            glv::Button* btn = new glv::Button(glv::Rect(20, 20), false);
            btn->setValue(cv->GetBool());
            btn->attach(cvarValueChanged, glv::Update::Value, cv);
            *btn << (new glv::Label(cv->GetName()))->anchor(glv::Place::CR).pos(glv::Place::CL, 4,0);
            *leftColumn << btn;
        }
        else if (cv->GetType() == T_INT || cv->GetType() == T_FLOAT)
        {
            glv::Slider* sld = new glv::Slider(glv::Rect(100, 20));
            sld->interval(cv->GetMaxVal(), cv->GetMinVal());
            sld->setValue(cv->GetFloat());
            sld->attach(cvarValueChanged, glv::Update::Value, cv);
            *sld << (new glv::Label(cv->GetName()))->anchor(glv::Place::BC).pos(glv::Place::BC, 0, -4);
            *leftColumn << sld;
        }
        
        // next
        cv = cv->NextCVar();
    }

}

bool CEngine::InInit()
{
    // JS Stick Letters (http://patorjk.com/software/taag/#p=testall&f=Ogre&t=GLT%20Engine)
    printf("  __       ___     ___       __          ___\n");
    printf(" / _` |     |     |__  |\\ | / _` | |\\ | |__\n");
    printf(" \\__> |___  |     |___ | \\| \\__> | | \\| |___ %s\n", "v1.0");
    printf("          %s\n", __TIMESTAMP__);
    printf("===================================================================\n\n");
    
    srand( time(NULL) );
    
    CVar::AddListener(this);
    
    struct aiLogStream stream;
     stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
     aiAttachLogStream(&stream);
    
    
    glGetError();
    
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
    PrintGLError("initialization");
    
    // get capabilities
    snprintf(_rcaps.api, 64, "OpenGL %s", (char*)glGetString(GL_VERSION));
    snprintf(_rcaps.renderer, 64, "%s",  (char*)glGetString(GL_RENDERER));
    snprintf(_rcaps.glsl, 64, "%s",  (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    _rcaps.MRT = CheckExtension("GL_ARB_draw_buffers");
    _rcaps.floatTextures = CheckExtension("GL_ARB_texture_float");
    _rcaps.packedDepthStencil = CheckExtension("GL_EXT_packed_depth_stencil");
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &_rcaps.maxColorAttachments);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &_rcaps.maxDrawBuffers);
    if (CheckExtension("GL_EXT_texture_filter_anisotropic"))
    {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &_rcaps.maxTextureAnisotropy);
    }
    
    // print capabilities
    printf(" %18s : %s\n", "API", _rcaps.api);
    printf(" %18s : %s\n", "Renderer", _rcaps.renderer);
    printf(" %18s : %s\n", "GLSL Version", _rcaps.glsl);
    printf(" %18s : %s (%d attachments)\n", "MRT", _rcaps.MRT?"yes":"no", _rcaps.maxColorAttachments);
    printf(" %18s : %d\n", "Max. Draw Buffers", _rcaps.maxColorAttachments);
    printf(" %18s : %s\n", "Float Textures", _rcaps.floatTextures?"yes":"no");
    printf(" %18s : %s\n", "PackedDepthStencil", _rcaps.packedDepthStencil?"yes":"no");
    printf(" %18s : %d\n", "Max. Anisotropy", _rcaps.maxTextureAnisotropy);
    printf(" %18s : %s\n", "Extensions", glGetString(GL_EXTENSIONS));
    printf("\n");
    
    // set config
    _config.textureAnisotropy = 8;
    
    // init GLV
    InitGLV();
    
    // initialize the scene
    _scene = new CTestScene();
    _scene->Init();
    
    // setup camera
    _cam.SetPosition(glm::vec3(0,0,0));
    _cam.SetNearAndFarPlanes(1, 100);
    
    return true;
}

void CEngine::InKeyboard(unsigned char key, bool down, bool special)
{
    switch (key)
    {
        case 27:        // ESC
            CEngine::Inst()->Shutdown();
            return;
    }
    
    keyToGLV(&_glv, key, down, special);
}

void CEngine::InMouseClick(int btn, int ax, int ay, bool down)
{
    _downX = ax;
    _downY = ay;
    
	glv::GLV * g = &_glv;

    glv::space_t x = (glv::space_t)ax;
    glv::space_t y = (glv::space_t)ay;
    glv::space_t relx = x;
    glv::space_t rely = y;
    
    switch(btn){
        case 0:		btn = glv::Mouse::Left; break;
        case 1:     btn = glv::Mouse::Middle; break;
        case 2:		btn = glv::Mouse::Right; break;
        default:	btn = glv::Mouse::Extra;		// unrecognized button
    }
    
    if(down)
        g->setMouseDown(relx, rely, btn, 0);
    else
        g->setMouseUp  (relx, rely, btn, 0);
    
    g->setMousePos((int)x, (int)y, relx, rely);
    modToGLV(&_glv);
    g->propagateEvent();
	
}

void CEngine::InMouseMove(int ax, int ay, bool down)
{
    if (down)
    {
        float fx = ax, fy = ay;
        if (_glv.focusedView()->findTarget(fx, fy) == &_glv)
        {
            // scene movement
            float delta = CEngine::Inst()->GetDeltaTime() * cvMouseSens.GetFloat();
            _cam.OffsetOrientation((ax-_downX)*delta, (ay-_downY)*delta);
            _downX = ax; _downY = ay;
            return;
        }
        motionToGLV(&_glv, ax, ay, glv::Event::MouseDrag);
    }
    else
        motionToGLV(&_glv, ax, ay, glv::Event::MouseMove);
}

void CEngine::InSizeChange(int wid, int hei)
{
    _screenSize.width = wid; _screenSize.height = hei;
    
    // use the whole window
    glViewport(0, 0, _screenSize.width, _screenSize.height);
    
    // update camera
    _cam.SetViewportAspectRatio(float(_screenSize.width)/_screenSize.height);
    
    // set active camera projection
    _cam.SetFieldOfView( glm::degrees(cvFov.GetFloat()) );
    
    // set glv size
    _glv.width(wid);
    _glv.height(hei);
}

void CEngine::InUpdate(float deltaTime)
{
    _deltaT = deltaTime;
    
    // update camera
    float fx = _glv.mouse().x(), fy = _glv.mouse().y();
    if (_glv.focusedView()->findTarget(fx, fy) == &_glv)
    {
        const float posDelta = deltaTime * cvKeySens.GetFloat();
        if (_glv.keyboard().key('w') && _glv.keyboard().isDown())
            _cam.OffsetPosition(posDelta * _cam.GetForward());
        else if (_glv.keyboard().key('s') && _glv.keyboard().isDown())
            _cam.OffsetPosition(posDelta * -_cam.GetForward());
        else if (_glv.keyboard().key('a') && _glv.keyboard().isDown())
            _cam.OffsetPosition(posDelta * -_cam.GetRight());
        else if (_glv.keyboard().key('d') && _glv.keyboard().isDown())
            _cam.OffsetPosition(posDelta * _cam.GetRight());
    }
    
    // scene update
    if (_scene) _scene->Update(deltaTime);

    // update printable cvars in GLV
    STD_CONST_FOREACH(PrintableCVarSet, s_printableCvars, pcv)
    {
        const SLabelCVarTuple& tuple = *pcv;
        
        char val[1024];
        sprintf(val, "%s: %s", tuple.cvar->GetName(), tuple.cvar->GetString());
        tuple.label->setValue(std::string(val));
        tuple.label->fit();
    }
    
    // status line
    char status[512];
    sprintf(status, "FPS: %.0f", 1.0f/deltaTime);
    _status->setValue(std::string(status));
}

void CEngine::InRender()
{
    // set MVP matrix each frame...
    InSizeChange(_screenSize.width, _screenSize.height);
    
    glGetError();
    
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    PrintGLError("clearing backbuffer");
    
    // SCENE
    glClearColor(0.0, 1.0, 1.0, 0.0);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    if (_scene) _scene->Draw();
    
    // GLV
    CShaderProgram::None().Use();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_TEXTURE_2D);
    glMatrixMode (GL_TEXTURE);
    glLoadIdentity();
    
    _glv.drawGLV(_screenSize.width, _screenSize.height, GLUT_ELAPSED_TIME);
    
    // SWAP!
    glutSwapBuffers();
    PrintGLError("swapping buffers");
}

bool CEngine::IsWireframeMode()const
{
    return cvWireframe;
}

/// Processes a console command (it can either be a variable assignment "x_var value" or command call "doSomehing")
/// \param outResult Success result (ie when command is handled but failed, processConsoleCommand returns true but outResult is set to false). Can be NULL.
/// \return Returns true if the call has been handled, false when no such variable or command
static bool processConsoleCommand(unsigned argc, const char* argv[], bool *outResult)
{
    if (argc==0)
        return false;
    
    // try to find cvar and call
    CVar* cv = CVar::FirstCVar();
    while(cv)
    {
        if (!strcasecmp(argv[0], cv->GetName()))
        {
            return cv->Call(argc-1, &argv[1], outResult);
        }
        cv = cv->NextCVar();
    }
    
    return false;
}

bool CEngine::ProcessConsoleLine(const char* line)
{
    const char* argv[128];
    unsigned argc = 0;
    
    char* str = strdup(line);
    char* pch = strtok(str, " \t\n");
    while (pch != NULL)
    {
        argv[argc++] = pch;
        pch = strtok (NULL, " \t\n");
    }
    
    bool ret = processConsoleCommand(argc, argv, NULL);
    
    free(str);
    
    return ret;
}

void CEngine::Shutdown()
{
    exit(0);
}

bool CEngine::CVarCalled(CVar *cv, unsigned int argc, const char **argv, bool& outResult)
{
    if (cv == &cvQuit)
    {
        Shutdown();
        return true;
    }
    else if (cv == &cvExtensios)
    {
        printf("OpenGL Extensions: %s\n", glGetString(GL_EXTENSIONS));
        return true;
    }
    
    return false;
}

