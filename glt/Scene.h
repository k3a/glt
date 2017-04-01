//
//  Scene.h
//  glt
//
//  Created by Mario Hros on 3. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef glt_Scene_h
#define glt_Scene_h

#include "vec3.hpp"
#include "mat4x4.hpp"

class CRenderTarget;
class CTexture;
class CShaderProgram;

class IScene
{
public:
    enum ERT
    {
        RT_DEPTH=0,
        RT_NORMAL,
        RT_DIFFUSE_ACC,
        RT_FINAL,
        RT_POSTPROCESS,
        RT_POSTPROCESS2,
        _RT_NUM
    };
    
    virtual ~IScene();
    
    virtual bool Init()=0;
    virtual void Update(float delta)=0;
    virtual void Draw()=0;
    
    const CRenderTarget& GetRT()const;
    const CTexture& GetRTTexture(ERT type)const;
    void DrawLight(glm::vec3 lightPosW, glm::vec3 color, float range)const;
    void SetCommonUniforms(CShaderProgram* prog, const glm::mat4& modelTransform)const;
    
    void SetAmbientColor(const glm::vec3 color){_ambientColor = color;};
    const glm::vec3 GetAmbientColor()const{ return _ambientColor; };
    
private:
    CRenderTarget* _rt;
    glm::vec3    _ambientColor;
};


#endif
