//
//  Scene.cpp
//  glt
//
//  Created by Mario Hros on 3. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#include "Scene.h"
#include "RenderTarget.h"
#include "Shaders.h"
#include "Engine.h"
#include "Mesh.h"
#include "CVar.h"

#include "func_matrix.hpp"
#include "transform.hpp"
#include "glstuff.h"

static CVar cvLightDebug("r_lightDebug", false, CVar::FLAG_GUI_TWEAKABLE);

IScene::~IScene()
{
    if (_rt) delete _rt;
}

bool IScene::Init()
{
    _rt = new CRenderTarget();
    
    _rt->AddDepthRenderbuffer(800, 600, TF_DEPTH32);
//#ifdef WIN32
//	_rt->AddColorTexture(800, 600, TF_RGB8); // 0: linear z
//#else
    _rt->AddColorTexture(800, 600, TF_RGBA16F); // 0: linear z
//#endif
	_rt->SetClearColor(RT_DEPTH, glm::vec4(0,0,0,1));
    
    _rt->AddColorTexture(800, 600, TF_RGBA8); // 1: normal; specular texture
    _rt->SetClearColor(RT_NORMAL, glm::vec4(0,0,0,0));
    
    _rt->AddColorTexture(800, 600, TF_RGBA8); // 2: diffuse light accum; specular accum
    _rt->SetClearColor(RT_DIFFUSE_ACC, glm::vec4(0,0,0,0));
    
    _rt->AddColorTexture(800, 600, TF_RGB8); // 3: final scene
    _rt->SetClearColor(RT_FINAL, glm::vec4(0,1,1,0));
    
    _rt->AddColorTexture(800, 600, TF_RGB8); // 4: post-process
    _rt->SetClearColor(RT_POSTPROCESS, glm::vec4(0,0,0,0));
    
    _rt->AddColorTexture(800, 600, TF_RGB8); // 5: post-process2
    _rt->SetClearColor(RT_POSTPROCESS2, glm::vec4(0,0,0,0));
    
    // pre-load standard shaders
    CShaderDefines defines;
    CShaderManager::Inst()->GetProgram("point_light.glsl", &defines);
    
    return true;
}

const CRenderTarget& IScene::GetRT()const
{
    if (_rt)
        return *_rt;
    else
        return CRenderTarget::Screen();
}

const CTexture& IScene::GetRTTexture(IScene::ERT type)const
{
    if (type >= IScene::_RT_NUM)
        return CTexture::None();
    
    return _rt->GetColorTexture(type);
}

void IScene::SetCommonUniforms(CShaderProgram *prog, const glm::mat4& modelTransform)const
{
    const CFlyCamera& cam = CEngine::Inst()->GetCamera();
    
    glm::mat4 viewProj = cam.GetProjection() * cam.GetView();
    
    prog->SetUniform("uModelViewProj", viewProj * modelTransform);
    prog->SetUniform("uProj", cam.GetProjection());
    prog->SetUniform("uIProj", glm::inverse(cam.GetProjection()));
    prog->SetUniform("uModelView", cam.GetView() * modelTransform);
    prog->SetUniform("uNearFar", glm::vec2(cam.GetNearPlane(), cam.GetFarPlane()));
    const CEngine::SScreenSize& screenSize = CEngine::Inst()->GetScreenSize();
    prog->SetUniform("uScreenSize", glm::vec2(screenSize.width, screenSize.height));
    float tanHalfFov = tanf(glm::radians(cam.GetFieldOfView()/2.0f));
    prog->SetUniform("uTanFovAspect", glm::vec2(cam.GetViewportAspectRatio()*tanHalfFov, tanHalfFov));
}

void IScene::DrawLight(glm::vec3 lightPosW, glm::vec3 color, float range)const
{
    const CFlyCamera& cam = CEngine::Inst()->GetCamera();
 
    CShaderDefines defines;
    if (cvLightDebug) defines.Define("LIGHT_DEBUG");
    CShaderProgram* prog = CShaderManager::Inst()->GetProgram("point_light.glsl", &defines);
    
    prog->Use();
    
    SetCommonUniforms(prog, glm::scale(glm::translate(lightPosW), glm::vec3(range)));
    prog->SetUniform("uLightPosV", glm::vec3(cam.GetView() * glm::vec4(lightPosW,1)) );
    prog->SetUniform("uDepthTex", GetRTTexture(IScene::RT_DEPTH), 0);
    prog->SetUniform("uNormalTex", GetRTTexture(IScene::RT_NORMAL), 1);
    prog->SetUniform("uLightColorInvRange", glm::vec4(color.r, color.g, color.b, 1.0f/range));
    
    glEnable(GL_CULL_FACE);
    if (glm::length(lightPosW-cam.GetPosition()) < range+1+cam.GetNearPlane())
    {
        // cam inside
        glDisable(GL_DEPTH_TEST); // disable depth so we light everything
        glCullFace(GL_FRONT); // when inside, draw backfaces only
    }
    else
    {
        glEnable(GL_DEPTH_TEST); // enable z-test when OUTSIDE light volume (so we don't compute light on objects between light sphere and camera)
        glCullFace(GL_BACK); // we can see only faces with normals pointing to the direction of camera
    }
    
    CMesh::UnitIcosphere().Draw();
}

