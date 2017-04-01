//
//  RenderTarget.cpp
//  glt
//
//  Created by Mario Hros on 3. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

// Useful resources http://www.opengl.org/wiki/Framebuffer_Object
// http://www.opengl.org/wiki/Framebuffer_Object_Examples

#include "RenderTarget.h"

#include "glstuff.h"
#include "Shared.h"
#include "Engine.h"

#include <stdio.h>

#ifdef WIN32
# define isnan _isnan
#endif

CRenderTarget::CRenderTarget()
: _frameBuffer(0), _valid(true/*empty buffer is screen - so valid*/), _viewportX(0), _viewportY(0), _viewportWid(0), _viewportHei(0)
{
}

CRenderTarget::~CRenderTarget()
{
    STD_CONST_FOREACH(SRTDataArray, _colorAttachments, it)
    {
        if (it->glTex) glDeleteTextures(1, &it->glTex);
        if (it->glRenderbuffer) glDeleteRenderbuffers(1, &it->glRenderbuffer);
        if (it->texture) delete it->texture;
    }
    
    if (_depthAttachment.glTex) glDeleteTextures(1, &_depthAttachment.glTex);
    if (_depthAttachment.glRenderbuffer) glDeleteRenderbuffers(1, &_depthAttachment.glRenderbuffer);
    if (_depthAttachment.texture) delete _depthAttachment.texture;
    
    if (_frameBuffer) glDeleteFramebuffers(1, &_frameBuffer);
}

void CRenderTarget::SetColorMask(bool red, bool green, bool blue, bool alpha)const
{
    glColorMask(red?GL_TRUE:GL_FALSE, green?GL_TRUE:GL_FALSE, blue?GL_TRUE:GL_FALSE, alpha?GL_TRUE:GL_FALSE);
}
void CRenderTarget::SetDepthMask(bool enable)const
{
    glDepthMask(enable?GL_TRUE:GL_FALSE);
}
void CRenderTarget::SetStencilMask(bool enable)const
{
    glStencilMask(enable?GL_TRUE:GL_FALSE);
}
bool CRenderTarget::SetClearColor(unsigned colorAttachmentIndex, const glm::vec4& color)
{
    if (colorAttachmentIndex >= _colorAttachments.size())
    {
        printf("No color RT renderbuffer for color attachment index %u\n", colorAttachmentIndex);
        return false;
    }
    
    _colorAttachments[colorAttachmentIndex].clearColor = color;
    return true;
}

void CRenderTarget::CreateFramebufferInNotExists()
{
    if (!_frameBuffer)
    {
        glGenFramebuffers(1, &_frameBuffer);
        PrintGLError("generating framebuffer");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    PrintGLError("binding framebuffer");
}

const char* framebufferStatusString(GLenum status)
{
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return "OK";
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            return "unsupported framebuffer format";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            return "missing attachment";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            return "duplicate attachment";
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            return "attached images must have same dimensions";
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            return "attached images must have same format";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            return "missing draw buffer";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            return "missing read buffer";
        default:
        {
            static char str[64];
            sprintf(str, "unknown status %u", status);
            return str;
        }
    }
}

bool CRenderTarget::CheckFramebufferValidity()
{
    CreateFramebufferInNotExists();
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    _valid = true;
    if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Warning: Incomplete FBO: %s. You can safely ignore the warning if you are fixing it by adding more attachments.\n", framebufferStatusString(status));
        _valid = false;
    }
    
    // set to onscreen renderbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return _valid;
}

bool CRenderTarget::AddColorTexture(int width, int height, ETextureFormat format, bool genMips)
{
    if (_colorAttachments.size() >= CEngine::Inst()->GetRendererCapabilities().maxColorAttachments)
    {
        printf("Maximum RT color attachments exceeded!\n");
        return false;
    }
    
    if (!_viewportWid || !_viewportHei)
    {
        _viewportWid = width;
        _viewportHei = height;
    }
    
    SRTData rtData;
    rtData.wid = width;
    rtData.hei = height;
    rtData.format = format;
    
    CreateFramebufferInNotExists();
    
    // texture
    glGenTextures(1, &rtData.glTex);
    PrintGLError("generating RT color texture");
    glBindTexture(GL_TEXTURE_2D, rtData.glTex);
    PrintGLError("binding RT color texture");
    
    // set filtering - needed
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    PrintGLError("setting RT color min/mag filter");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    PrintGLError("setting RT color wrapping mode");
    
    const int level = 0;
    
    glGetError();
    glTexImage2D(GL_TEXTURE_2D, level, CTexture::GLFormat(format).internalFormat, width, height, 0, CTexture::GLFormat(format).format, CTexture::GLFormat(format).type, 0);
    if (glGetError() != GL_NO_ERROR)
    {
        printf("Error creating RT color texture. Size: [%d,%d], Format: %d\n", width, height, format);
        glDeleteTextures(1, &rtData.glTex);
        rtData.glTex = 0;
        
        // set to onscreen renderbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    if (genMips)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        PrintGLError("setting RT color mipmap generation");
    }
    
    // attach texture to renderbuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GLenum(GL_COLOR_ATTACHMENT0 + _colorAttachments.size()), GL_TEXTURE_2D, rtData.glTex, level);
    PrintGLError("attaching RT color texture to framebuffer");
    
    if (!CheckFramebufferValidity())
    {
        glDeleteTextures(1, &rtData.glTex);
        rtData.glTex = 0;
        
        // set to onscreen renderbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    rtData.texture = new CTexture("rtcolor", rtData.glTex, rtData.wid, rtData.hei, true);
    
    _colorAttachments.push_back(rtData);
    
    // set to onscreen renderbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return true;
}

bool CRenderTarget::AddDepthStencilTexture(int width, int height, ETextureFormat format)
{
    if (_depthAttachment.glTex || _depthAttachment.glRenderbuffer)
    {
        printf("RT depth texture or renderbuffer already set\n");
        return false;
    }
    
    if (!_viewportWid || !_viewportHei)
    {
        _viewportWid = width;
        _viewportHei = height;
    }
    
    _depthAttachment.wid = width;
    _depthAttachment.hei = height;
    _depthAttachment.format = format;
    
    CreateFramebufferInNotExists();
    
    // texture
    glGenTextures(1, &_depthAttachment.glTex);
    PrintGLError("generating RT color texture");
    glBindTexture(GL_TEXTURE_2D, _depthAttachment.glTex);
    PrintGLError("binding RT color texture");
    
    // set filtering
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    PrintGLError("setting RT depth min/mag filter");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    PrintGLError("setting RT depth wrapping mode");
    
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
    PrintGLError("setting RT depth texture mode");
    // This texture's GL_TEXTURE_COMPARE_MODE parameter is set to GL_COMPARE_R_TO_TEXTURE to allow it to be referenced by a GLSL sampler2DShadow.
    // This sampler not only performs the texture look-up, but also handles the depth comparison as defined by the OpenGL shadow map extension
    // (http://oss.sgi.com/projects/ogl-sample/registry/ARB/shadow.txt).
    // It returns not a texture value, but a floating point value indicating whether the fragment is or is not in shadow.
    // More here: http://csc.lsu.edu/~kooima/misc/cs594/final/part3.html
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    PrintGLError("setting RT depth compare mode");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    PrintGLError("setting RT depth compare func");
    
    const int level = 0;
    
    glGetError();
    glTexImage2D(GL_TEXTURE_2D, level, CTexture::GLFormat(format).internalFormat, width, height, 0, CTexture::GLFormat(format).format, CTexture::GLFormat(format).type, 0);
    if (glGetError() != GL_NO_ERROR)
    {
        printf("Error creating RT depth texture. Size: [%d,%d], Format: %d", width, height, format);
        glDeleteTextures(1, &_depthAttachment.glTex);
        _depthAttachment.glTex = 0;
        
        // set to onscreen renderbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    // attach texture to renderbuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthAttachment.glTex, level);
    PrintGLError("attaching RT depth texture to framebuffer");
    
    if (!CheckFramebufferValidity())
    {
        glDeleteTextures(1, &_depthAttachment.glTex);
        _depthAttachment.glTex = 0;
        
        // set to onscreen renderbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    _depthAttachment.texture = new CTexture("rtdepth", _depthAttachment.glTex, _depthAttachment.wid, _depthAttachment.hei, true);
    
    return true;
}

bool CRenderTarget::AddRenderbuffer(unsigned& outRenderbuffer, unsigned attachment/*GLenum*/, unsigned internalformat/*GLenum*/, int width, int height)
{
    glGenRenderbuffers(1, &outRenderbuffer);
    PrintGLError("generating renderbuffer");
    glBindRenderbuffer(GL_RENDERBUFFER, outRenderbuffer);
    PrintGLError("binding renderbuffer");
    
    glGetError();
    glRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
    if (glGetError() != GL_NO_ERROR)
    {
        printf("Error setting renderbuffer storage format. Size: [%d,%d], GLFormat: 0x%x\n", width, height, internalformat);
        glDeleteRenderbuffers(1, &outRenderbuffer);
        outRenderbuffer = 0;
        return false;
    }
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, outRenderbuffer);
    if (glGetError() != GL_NO_ERROR)
    {
        printf("Error setting renderbuffer to framebuffer. Renderbuffer size: [%d,%d], GLFormat: 0x%x\n", width, height, internalformat);
        glDeleteRenderbuffers(1, &outRenderbuffer);
        outRenderbuffer = 0;
        return false;
    }
    
    if (!CheckFramebufferValidity())
    {
        glDeleteRenderbuffers(1, &outRenderbuffer);
        outRenderbuffer = 0;
        
        // set to onscreen renderbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    return true;
}

bool CRenderTarget::AddColorRenderbuffer(int width, int height, ETextureFormat format)
{
    if (_colorAttachments.size() >= CEngine::Inst()->GetRendererCapabilities().maxColorAttachments)
    {
        printf("Maximum RT color attachments (%d) reached!\n", (int)_colorAttachments.size());
        return false;
    }
    
    CreateFramebufferInNotExists();
    
    SRTData rtData;
    rtData.wid = width;
    rtData.hei = height;
    rtData.format = format;
    
    if (!AddRenderbuffer(rtData.glRenderbuffer, GLenum(GL_COLOR_ATTACHMENT0 + _colorAttachments.size()), CTexture::GLFormat(format).internalFormat, width, height))
        return false;
    
    _colorAttachments.push_back(rtData);
    
    // set to onscreen renderbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return true;
}

bool CRenderTarget::AddDepthRenderbuffer(int width, int height, ETextureFormat format)
{
    if (_depthAttachment.glTex || _depthAttachment.glRenderbuffer)
    {
        printf("RT depth texture or renderbuffer already set\n");
        return false;
    }
    
    CreateFramebufferInNotExists();
    
    bool ret = AddRenderbuffer(_depthAttachment.glRenderbuffer, GL_DEPTH_ATTACHMENT, CTexture::GLFormat(format).internalFormat, width, height);
    
    // set to onscreen renderbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return ret;
}

bool CRenderTarget::AddStencilRenderbuffer(int width, int height, ETextureFormat format)
{
    if (_stencilAttachment.glTex || _stencilAttachment.glRenderbuffer)
    {
        printf("RT stencil texture or renderbuffer already set\n");
        return false;
    }
    
    CreateFramebufferInNotExists();
    
    bool ret = AddRenderbuffer(_depthAttachment.glRenderbuffer, GL_STENCIL_ATTACHMENT, CTexture::GLFormat(format).internalFormat, width, height);
    
    // set to onscreen renderbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return ret;
}

static bool ReadPixels(unsigned framebuffer, ETextureFormat fmt, int x, int y, int width, int height, GLenum readbufferMode, char* inoutData)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    PrintGLError("binding RT framebuffer");
    glReadBuffer(readbufferMode);
    PrintGLError("trying to read RT color renderbuffer attachment");
    glReadPixels(x, y, width, height, CTexture::GLFormat(fmt).internalFormat, CTexture::GLFormat(fmt).type, inoutData);
    PrintGLError("trying to read pixels from RT color renderbuffer");
    
    return true;
}

bool CRenderTarget::ReadColorPixels(unsigned colorAttachmentIndex, int x, int y, int width, int height, ETextureFormat format, char* inoutData)
{
    if (colorAttachmentIndex >= _colorAttachments.size())
    {
        printf("No color RT renderbuffer for color attachment index %u\n", colorAttachmentIndex);
        return false;
    }
    
    if (!_colorAttachments[colorAttachmentIndex].glRenderbuffer)
    {
        printf("No color RT renderbuffer for color attachment index %u\n", colorAttachmentIndex);
        return false;
    }
    
    return ReadPixels(_frameBuffer, _colorAttachments[colorAttachmentIndex].format, x, y, width, height, GLenum(GL_COLOR_ATTACHMENT0 + colorAttachmentIndex), inoutData);
}

bool CRenderTarget::ReadDepthPixels(int x, int y, int width, int height, ETextureFormat format, char* inoutData)
{
    if (!_depthAttachment.glRenderbuffer)
    {
        printf("No depth RT renderbuffer for depth attachment\n");
        return false;
    }
    
    return ReadPixels(_frameBuffer, _depthAttachment.format, x, y, width, height, GL_DEPTH_ATTACHMENT, inoutData);
}

bool CRenderTarget::ReadStencilPixels(int x, int y, int width, int height, ETextureFormat format, char* inoutData)
{
    if (!_stencilAttachment.glRenderbuffer)
    {
        printf("No stencil RT renderbuffer for stencil attachment\n");
        return false;
    }
    
    return ReadPixels(_frameBuffer, _stencilAttachment.format, x, y, width, height, GL_STENCIL_ATTACHMENT, inoutData);
}

void CRenderTarget::SetViewport(int x, int y, int wid, int hei)
{
    _viewportX = x; _viewportY = y; _viewportWid = wid; _viewportHei = hei;
}

void CRenderTarget::Use(unsigned enabledAttachments, unsigned clear)const
{
    if (!_frameBuffer)
    {
        // set to onscreen renderbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, CEngine::Inst()->GetScreenSize().width, CEngine::Inst()->GetScreenSize().height);
    }
    else
    {
        // bind framebuffer and set the viewport
        glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
        PrintGLError("binding RT framebuffer");
        glViewport(0, 0, _viewportWid, _viewportHei);
        PrintGLError("setting RT viewport");
        
        // construct the list of enabled attachments
        GLenum buffs[32]; unsigned idx=0;
        if (enabledAttachments & ATT_COLOR0)
            buffs[idx++] = GL_COLOR_ATTACHMENT0;
        if (enabledAttachments & ATT_COLOR1)
            buffs[idx++] = GL_COLOR_ATTACHMENT1;
        if (enabledAttachments & ATT_COLOR2)
            buffs[idx++] = GL_COLOR_ATTACHMENT2;
        if (enabledAttachments & ATT_COLOR3)
            buffs[idx++] = GL_COLOR_ATTACHMENT3;
        if (enabledAttachments & ATT_COLOR4)
            buffs[idx++] = GL_COLOR_ATTACHMENT4;
        if (enabledAttachments & ATT_COLOR5)
            buffs[idx++] = GL_COLOR_ATTACHMENT5;
        if (enabledAttachments & ATT_COLOR6)
            buffs[idx++] = GL_COLOR_ATTACHMENT6;
        if (enabledAttachments & ATT_COLOR7)
            buffs[idx++] = GL_COLOR_ATTACHMENT7;
        if (enabledAttachments & ATT_DEPTH)
            buffs[idx++] = GL_DEPTH_ATTACHMENT;
        
        glDrawBuffers(idx, buffs);
    }
    
    // clear buffers
    GLenum glclear = 0;
    if (clear & CLEAR_COLOR)
        glclear |= GL_COLOR_BUFFER_BIT;
    if (clear & CLEAR_DEPTH)
        glclear |= GL_DEPTH_BUFFER_BIT;
    if (clear & CLEAR_STENCIL)
        glclear |= GL_STENCIL_BUFFER_BIT;
    
    if (glclear)
    {
#define RT_COLOR_HELPER(idx) if (enabledAttachments & ATT_COLOR##idx && _colorAttachments.size()>idx) \
                            color = &_colorAttachments[idx].clearColor;
        
        // Set clear color to the first enabled one
        // TODO: what about MRT case?
        const glm::vec4* color = NULL;
        RT_COLOR_HELPER(0)
        else RT_COLOR_HELPER(1)
        else RT_COLOR_HELPER(2)
        else RT_COLOR_HELPER(3)
        else RT_COLOR_HELPER(4)
        else RT_COLOR_HELPER(5)
        else RT_COLOR_HELPER(6)
        else RT_COLOR_HELPER(7)
            
        if (color && !isnan(color->r))
        {
            glClearColor(color->r, color->g, color->b, color->a);
        }
        
        glClear(glclear);
    }
}

const CTexture& CRenderTarget::GetColorTexture(unsigned colorAttachmentIndex)const
{
    if (colorAttachmentIndex >= _colorAttachments.size())
    {
        printf("No color RT renderbuffer for color attachment index %u\n", colorAttachmentIndex);
        return CTexture::None();
    }
    
    if (_colorAttachments[colorAttachmentIndex].texture)
        return *_colorAttachments[colorAttachmentIndex].texture;
    else
        return CTexture::None();
}

const CTexture& CRenderTarget::GetDepthStencilTexture()const
{
    if (_depthAttachment.texture)
        return *_depthAttachment.texture;
    else
        return CTexture::None();
}

// TODO: discarding attachments & what about pc?
/*
void CRenderTarget::Discard(bool color, bool depth)
{
	if (!_bColorTexture) color = false;
	if (!_bDepthTexture) depth = false;
	
	glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
	
    GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT};
     
	if (color && depth)
		glDiscardFramebufferEXT(GL_FRAMEBUFFER, 2, attachments);
	else if (color)
		glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, attachments);
	else if (depth)
		glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, &attachments[1]);
 
}*/

