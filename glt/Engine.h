//
//  Engine.h
//  glt
//
//  Created by Mario Hros on 3. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef __glt__Engine__
#define __glt__Engine__

#include "CVar.h"
#include "Scene.h"
#include "FlyCamera.h"

#include "glv.h"

class CTestScene;

class CEngine : public ICVarListener
{
public:
    struct SRendererCaps
    {
        SRendererCaps():MRT(false),floatTextures(false),packedDepthStencil(false),
        maxColorAttachments(1),maxDrawBuffers(1), maxTextureAnisotropy(0){ api[0]=0; renderer[0]=0; glsl[0]=0;};
        
        char api[64];
        char renderer[64];
        char glsl[64];
        bool MRT;
        bool floatTextures;
        bool packedDepthStencil;
        int maxColorAttachments; // in a MRT
        int maxDrawBuffers; // mostly for MRT https://www.opengl.org/sdk/docs/man4/xhtml/glDrawBuffers.xml
        int maxTextureAnisotropy; // 0-anisotropic filtering unavailable, maximum amount of anisotropy otherwise
    };
    
    struct SRendererConfig
    {
        SRendererConfig():textureAnisotropy(0){};
        
        int textureAnisotropy;
    };
    
    struct SScreenSize
    {
        unsigned width;
        unsigned height;
    };
    
public:
    static CEngine* Inst()
    {
        static CEngine* inst = NULL;
        if (!inst) inst = new CEngine();
        return inst;
    }
    
    CEngine() : _scene(0), _deltaT(0.00001f), _status(NULL) {};
    
    void InKeyboard(unsigned char key, bool down, bool special);
    void InMouseClick(int btn, int ax, int ay, bool down);
    void InMouseMove(int ax, int ay, bool down);
    void InSizeChange(int wid, int hei);
    bool InInit();
    void InUpdate(float deltaTime);
    void InRender();
    
    bool ProcessConsoleLine(const char* line);
    void Shutdown();
    const SRendererCaps& GetRendererCapabilities()const{ return _rcaps; };
    const SRendererConfig& GetRendererConfig()const{ return _config; };
    
    IScene* GetScene()const{ return _scene; };
    void SetScene(IScene* scene){ _scene = scene; };
    const SScreenSize& GetScreenSize()const{ return _screenSize; };
    const CFlyCamera& GetCamera()const{ return _cam; };
    
    float GetDeltaTime(){ return _deltaT; };
    bool IsWireframeMode()const;
    
private:
    bool CVarCalled(CVar* cv, unsigned argc, const char* argv[], bool& outResult);
    void InitGLV();
    
    CFlyCamera _cam;
    IScene* _scene;
    float _deltaT;
    
    glv::GLV _glv;
    glv::Label* _status;
    float _downX, _downY;
    SScreenSize _screenSize;
    
    SRendererCaps _rcaps;
    SRendererConfig _config;
};

#endif /* defined(__glt__Engine__) */
