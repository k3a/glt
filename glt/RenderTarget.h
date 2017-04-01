//
//  RenderTarget.h
//  glt
//
//  Created by Mario Hros on 3. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef __glt__RenderTarget__
#define __glt__RenderTarget__

#include <vector>
#include <math.h>

#include "Texture.h"
#include "vec4.hpp"

#ifdef WIN32
    #ifndef NAN
        static const unsigned long __nan[2] = {0xffffffff, 0x7fffffff};
        #define NAN (*(const float *) __nan)
    #endif
#endif

class CRenderTarget
{
    struct SRTData
    {
        SRTData():glTex(0), clearColor(NAN), texture(NULL), glRenderbuffer(0), wid(0), hei(0){};
        unsigned glTex; // renderable texture (or 0 if not assigned = is a renderbuffer)
        unsigned glRenderbuffer; // renderbuffer handle (or 0 if not assigned = is a framebuffer texture)
        unsigned wid, hei;
        ETextureFormat format;
        CTexture* texture;
        glm::vec4 clearColor;
    };
    typedef std::vector<SRTData> SRTDataArray;
    
public:
    enum EAttachment
    {
        ATT_NONE,
        ATT_COLOR0  = 1,
        ATT_COLOR1  = 1<<1,
        ATT_COLOR2  = 1<<2,
        ATT_COLOR3  = 1<<3,
        ATT_COLOR4  = 1<<4,
        ATT_COLOR5  = 1<<5,
        ATT_COLOR6  = 1<<6,
        ATT_COLOR7  = 1<<7,
        ATT_DEPTH   = 1<<8,
        ATT_ALL = 0xffffffff
    };
    
    enum EClearFlags
    {
        CLEAR_NONE,
        CLEAR_COLOR  = 1,
        CLEAR_DEPTH  = 1<<1,
        CLEAR_STENCIL= 1<<2,
        CLEAR_BOTH = CLEAR_COLOR | CLEAR_DEPTH,
        CLEAR_ALL = 0xffffffff
    };
    
    static const CRenderTarget& Screen()
    {
        static CRenderTarget rt;
        return rt;
    }
    
	CRenderTarget();
	~CRenderTarget();
	
    // - common tasks
    /// \param enabledAttachments bitmask made of EAttachment
	void Use(unsigned enabledAttachments = ATT_COLOR0, unsigned clear = CLEAR_NONE)const;
    bool IsValid()const{ return _valid; };
    void SetViewport(int x, int y, int wid, int hei);
    void SetColorMask(bool red, bool green, bool blue, bool alpha)const;
    void SetDepthMask(bool enable)const;
    void SetStencilMask(bool enable)const;
    bool SetClearColor(unsigned colorAttachmentIndex, const glm::vec4& color);
    
    // -- adding attachments
    /// Color texture can be then used as a normal texture; You can get the textury by calling GetColorTexture
    bool AddColorTexture(int width, int height, ETextureFormat format, bool genMips=false);
    bool AddDepthStencilTexture(int width, int height, ETextureFormat format);
    bool AddStencilTexture(int width, int height, ETextureFormat format);
    /// Renderbuffer is used for offscreen rendering - it won't produce a GPU texture; you can read the data by calling ReadPixels
    bool AddColorRenderbuffer(int width, int height, ETextureFormat format);
    bool AddDepthRenderbuffer(int width, int height, ETextureFormat format);
    bool AddStencilRenderbuffer(int width, int height, ETextureFormat format);
    
    // - reading pixels.
    /// \note Blocks until pixels are copied! Better alternative for reading may be GL PBO (see http://stackoverflow.com/a/12159293/314437)
    bool ReadColorPixels(unsigned colorAttachmentIndex, int x, int y, int width, int height, ETextureFormat format, char* inoutData);
	/// \note Blocks until pixels are copied! Better alternative for reading may be GL PBO (see http://stackoverflow.com/a/12159293/314437)
    bool ReadDepthPixels(int x, int y, int width, int height, ETextureFormat format, char* inoutData);
    /// \note Blocks until pixels are copied! Better alternative for reading may be GL PBO (see http://stackoverflow.com/a/12159293/314437)
    bool ReadStencilPixels(int x, int y, int width, int height, ETextureFormat format, char* inoutData);
    
    // - getting textures
    /// Returns 0 if not available
	const CTexture& GetColorTexture(unsigned colorAttachmentIndex)const;
	/// Returns 0 if not available
    const CTexture& GetDepthStencilTexture()const;
	
private:
    void CreateFramebufferInNotExists();
    bool CheckFramebufferValidity();
    bool AddRenderbuffer(unsigned& outRenderbuffer, unsigned attachment, unsigned internalformat, int width, int height);
    
    bool _valid;
    unsigned _frameBuffer;
    
    SRTDataArray _colorAttachments;
    SRTData _depthAttachment;
    SRTData _stencilAttachment;
    
    int _viewportX, _viewportY, _viewportWid, _viewportHei;
};

#endif /* defined(__glt__RenderTarget__) */
