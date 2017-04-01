//
//  TestScene.cpp
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#include "TestScene.h"
#include "Shared.h"
#include "CVar.h"
#include "Engine.h"
#include "RenderTarget.h"

#include "glstuff.h"
#include "func_matrix.hpp"
#include "transform.hpp"

//#define CRYTEK

static CVar cvRT("r_rt", 0, CVar::FLAG_GUI_TWEAKABLE, 0, 6 +0.9f);

static glm::vec3 s_ambient(0.07,0.05,0.05);

bool CTestScene::Init()
{
    // load resources
#ifdef CRYTEK
    _mesh = CMesh::FromFile(LocateFile("crytek-sponza.obj"), true, true);
    _mesh->SetScale(glm::vec3(0.0131,0.0131,0.0131));
    _mesh->SetPosition(glm::vec3(0,-1,0.6));
#else
    _mesh = CMesh::FromFile(LocateFile("sponza.obj"), true, true);
    _mesh->SetPosition(glm::vec3(0,-1,0));
#endif
    
    // load a programs
    CShaderDefines defines("");
    
    // for fullscreen quad
    defines.UndefineAll().Define("TEXTURE0,ATTRIB_COORDS0,FULLSCREEN_QUAD");
    _fullscreenQuadProg = CShaderManager::Inst()->GetProgram("basic.glsl", &defines);
    // TEXTURE1 for SSAO multiply
    _fullscreenQuadProgSSAO = CShaderManager::Inst()->GetProgram("basic.glsl", &CShaderDefines(defines).Define("TEXTURE1"));
    // DEBUG_VISUALIZE_ALPHA to visualize alpha channel
    _fullscreenQuadProgDebug = CShaderManager::Inst()->GetProgram("basic.glsl", &CShaderDefines(defines).Define("DEBUG_VISUALIZE_ALPHA"));
    
    // ssao
    defines.UndefineAll();
    _ssaoProg = CShaderManager::Inst()->GetProgram("ssao.glsl", &defines);
    _ssaoBlurProg = CShaderManager::Inst()->GetProgram("ssao_blur.glsl", &defines);
    _ssaoRandom = CTexture::FromFile(LocateFile("ssao_random.png"));
    
    // for axis lines
    defines.UndefineAll().Define("ATTRIB_COLOR0");
    _colorProg = CShaderManager::Inst()->GetProgram("basic.glsl", &defines);
    
    // after having all programs linked, we can delete all shaders
    CShaderManager::Inst()->PurgeShaderCaches();
    
    SetAmbientColor(s_ambient);
    
    for (unsigned i=0; i<SSAO_KERNEL_SIZE; i++)
    {
        float the = 2*3.14f/SSAO_KERNEL_SIZE*i;
        _ssaoKernel[i] = glm::vec3(
             sinf(the),
             cosf(the),
             0.2f+sinf(the*8)*0.8f
        );
        _ssaoKernel[i] = glm::normalize(_ssaoKernel[i]) * (0.5f+sinf(the*8)/0.5f);
        _ssaoKernel[i] *= 0.3+(1.0-0.3)/SSAO_KERNEL_SIZE*(i%2);
    }
    
    return IScene::Init();
}

void CTestScene::Update(float delta)
{
    // axis lines - model is identity
    SetCommonUniforms(_colorProg, glm::mat4());
    // fullscreen quad - no model matrix used
    SetCommonUniforms(_fullscreenQuadProg, glm::mat4());
}

static void beginWireframe()
{
    if (CEngine::Inst()->IsWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1);
    }
}
static void endWireframe()
{
    if (CEngine::Inst()->IsWireframeMode())
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void CTestScene::Draw()
{
    glDepthMask(GL_TRUE); // enable z write
    glEnable(GL_DEPTH_TEST); // enable z test
    glLineWidth(1);
    glDisable(GL_BLEND);
    
    // LINEAR Z
    // TODO: simplify Use() on render target here so we don't need to remember attachment number and use symbolic names like IScene::RT_DEPTH
    GetRT().Use(CRenderTarget::ATT_COLOR0, CRenderTarget::CLEAR_BOTH); // can theoretically not clear the color buffer (depending on the scene)
    _mesh->Draw(CMesh::DRAW_Z);
    
    // NORMAL
    GetRT().Use(CRenderTarget::ATT_COLOR1, CRenderTarget::CLEAR_COLOR); // can theoretically not clear the color buffer (depending on the scene)
    glDepthMask(GL_FALSE); // disable z write (we already have z renderbuffer created)
    _mesh->Draw(CMesh::DRAW_NORMAL);
    
    // ACCUMULATION
    GetRT().Use(CRenderTarget::ATT_COLOR2, CRenderTarget::CLEAR_COLOR);
    glDepthMask(GL_FALSE); // disable z write (so that two intersecting light spheres will work)
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    static float s_pos = 0.1f;
    //s_pos += CEngine::Inst()->GetDeltaTime() * 0.4;
    DrawLight(glm::vec3(5*sinf(s_pos)+6,0.5,0), glm::vec3(1,0.5,0), 12);
    DrawLight(glm::vec3(-5*sinf(s_pos)-6,0.5,0), glm::vec3(1.0,0.9,0.7), 12);
    
    // middle top
    DrawLight(glm::vec3(0,7,-5), glm::vec3(1.0,0.9,0.7), 5);
    DrawLight(glm::vec3(0,7,5), glm::vec3(1.0,0.9,0.7), 5);
    
    DrawLight(glm::vec3(7,7,-5), glm::vec3(1.0,0.9,0.7), 5);
    DrawLight(glm::vec3(7,7,5), glm::vec3(1.0,0.9,0.7), 5);
    DrawLight(glm::vec3(-7,7,-5), glm::vec3(1.0,0.9,0.7), 5);
    DrawLight(glm::vec3(-7,7,5), glm::vec3(1.0,0.9,0.7), 5);
    
    glDisable(GL_BLEND);
    
    // FINAL SCENE
    GetRT().Use(CRenderTarget::ATT_COLOR3, CRenderTarget::CLEAR_COLOR);
    //glEnable(GL_MULTISAMPLE_ARB);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    beginWireframe();
    glEnable(GL_DEPTH_TEST); // enable depth test (if not enabled; we want to reuse z for the final pass)
    glDepthMask(GL_TRUE); // disable z-write (we have the wbuffer already)
    _mesh->Draw(CMesh::DRAW_MATERIAL);
    endWireframe();
    //glDisable(GL_MULTISAMPLE_ARB);
    
    glDisable(GL_DEPTH_TEST); // disable z test (no more scene drawing; now postprocess only)
    
    // SSAO
    GetRT().Use(CRenderTarget::ATT_COLOR4, CRenderTarget::CLEAR_COLOR);
    _ssaoProg->Use();
    SetCommonUniforms(_ssaoProg, glm::mat4());
    _ssaoProg->SetUniform("uDepthTex", GetRTTexture(IScene::RT_DEPTH), 0);
    _ssaoProg->SetUniform("uNormalTex", GetRTTexture(IScene::RT_NORMAL), 1);
    _ssaoProg->SetUniform("uRandomTex", *_ssaoRandom, 2);
    _ssaoProg->SetUniform("uKernel", SSAO_KERNEL_SIZE, _ssaoKernel[0]);
    _ssaoProg->SetUniform("uKernelSize", SSAO_KERNEL_SIZE);
    CMesh::FullscreenQuad().Draw();
    
    // SSAO BLUR
    GetRT().Use(CRenderTarget::ATT_COLOR5, CRenderTarget::CLEAR_COLOR);
    _ssaoBlurProg->Use();
    SetCommonUniforms(_ssaoBlurProg, glm::mat4());
    _ssaoBlurProg->SetUniform("uInvTex0Size", glm::vec2(1.0f/800, 1.0f/600)); // TODO: for now hardcodded
    _ssaoBlurProg->SetUniform("uTex0", GetRTTexture(IScene::RT_POSTPROCESS), 0);
    CMesh::FullscreenQuad().Draw();
    
    // SCREEN AGAIN
    CRenderTarget::Screen().Use(0);
    glClearColor(0, 1, 1, 1);
    glEnable(GL_CULL_FACE); // wa don't want to render backfaces again...
    // (no need to clear anything - we are rendering fullscreen quad (unless wireframe))
    
    // draw final scene to the screen
    if (cvRT.GetInt()>0)
    {
        // draw fullscreen quad with RT (for debug)
        glDisable(GL_DEPTH_TEST); // disable z test (no need to z-test for rendering fullscreen quads). Z-write is already disabled (no point in doing it for fullscreen quads)
        _fullscreenQuadProgDebug->Use();
        _fullscreenQuadProgDebug->SetUniform("uTex0", GetRT().GetColorTexture(cvRT.GetInt()-1));
        CMesh::FullscreenQuad().Draw();
    }
    else
    {
        // normal scene
        _fullscreenQuadProgSSAO->Use();
        _fullscreenQuadProgSSAO->SetUniform("uTex0", GetRTTexture(RT_FINAL), 0);
        _fullscreenQuadProgSSAO->SetUniform("uTex1", GetRTTexture(RT_POSTPROCESS2), 1);
    }
    CMesh::FullscreenQuad().Draw();
    
    // AXIS always last :P
    glDisable(GL_DEPTH_TEST); // disable z test (always render a visible axis)
    glLineWidth(2);
    _colorProg->Use();
    CMesh::AxisLines().Draw();
}



