//
//  TestScene.h
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef __glt__TestScene__
#define __glt__TestScene__

#include "Shaders.h"
#include "Mesh.h"
#include "Scene.h"
#include "Texture.h"

#define SSAO_KERNEL_SIZE 8

class CTestScene : public IScene
{
public:
    bool Init();
    void Update(float delta);
    void Draw();
    
private:
    
    CShaderProgram* _colorProg;
    CShaderProgram* _fullscreenQuadProg;
    CShaderProgram* _fullscreenQuadProgSSAO;
    CShaderProgram* _fullscreenQuadProgDebug;
    CShaderProgram* _ssaoProg;
    CShaderProgram* _ssaoBlurProg;
    
    CMesh* _mesh;
    
    CTexture* _ssaoRandom;
    glm::vec3 _ssaoKernel[SSAO_KERNEL_SIZE];
};

#endif /* defined(__glt__TestScene__) */
