//
//  Texture.h
//  glt
//
//  Created by Mario Hros on 4. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef __glt__Texture__
#define __glt__Texture__

#include <string>

enum ETextureFormat
{
    TF_NONE=0,
    TF_RGB8,
    TF_RGBA8,
    TF_RGBA16F,  // SRenderCaps.floatTextures must be true
    TF_RGBA32F, // SRenderCaps.floatTextures must be true
    TF_LA8,
    TF_DEPTH32,
    TF_DEPTH16, //TODO: caps?
    TF_DEPTH24, //TODO: caps?
    TF_DEPTH24S8, //24 depth, 8 stencil SRenderCaps.packedDepthStencil must be true
};

struct SGLTextureFormatInfo
{
    int internalFormat; // like RGBA8
    unsigned format; // like GL_RGBA
    unsigned type; // like GL_UNSIGNED_BYTE
};

class CTexture
{
    struct SGLTexture
    {
        unsigned glTexture;
        unsigned width;
        unsigned height;
    };
    
public:
    static const CTexture& None()
    {
        static CTexture tex;
        return tex;
    }
    
    static CTexture* FromFile(const char* path)
    {
        CTexture* tex = new CTexture();
        return tex->LoadFromFile(path)?tex:NULL;
    }
    static const SGLTextureFormatInfo& GLFormat(ETextureFormat fmt);
    
    CTexture()
    :_name("undefined"),_gltex(0),_width(0),_height(0),_doNotDeleteTexture(false){};
    
    CTexture(const char* name, unsigned gltexture, unsigned width, unsigned height, bool doNotDeleteTexture)
    :_name(name),_gltex(gltexture),_width(width),_height(height),_doNotDeleteTexture(doNotDeleteTexture){};
    
    ~CTexture();
    void Release(){}; //TODO: refcount
    
    int GetGLTexture()const{ return _gltex; }; // used by CShaderProgram
    bool IsValid()const{ return _gltex != 0; };
    void Use(unsigned unit = 0)const;
    const char* GetName()const{ return _name.c_str(); };
    bool LoadFromFile(const char* path);
    
private:
    bool CreateTexture(unsigned wid, unsigned hei, ETextureFormat pixFormat, const char* textureData);
    bool LoadAsPNG(const char* path);
    bool LoadAsTGA(const char* path);
    
    std::string _name;
    unsigned _gltex;
    unsigned _width, _height;
    bool _doNotDeleteTexture;
};

#endif /* defined(__glt__Texture__) */
