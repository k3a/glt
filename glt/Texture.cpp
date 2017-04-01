//
//  Texture.cpp
//  glt
//
//  Created by Mario Hros on 4. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#include "Texture.h"
#include "Engine.h"

#include <stdio.h>
#include <stdlib.h>
#include "glstuff.h"
#include "png.h"
#include "tgalib.h"
    
// array from ETextureFormat to gl texture format
static SGLTextureFormatInfo s_texFormat[] = {
    {0, 0, 0}, //TF_NONE
    {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE}, // TF_RGB8
    {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}, // TF_RGBA8
    {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT}, // TF_RGBA16F
    {GL_RGBA32F, GL_RGBA, GL_FLOAT}, // TF_RGBA32F
    {GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE}, // TF_LA8
    {GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT}, // TF_DEPTH32
    {GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT}, // TF_DEPTH16
    {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT}, // TF_DEPTH24
    {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8}, // TF_DEPTH24S8
};

const SGLTextureFormatInfo& CTexture::GLFormat(ETextureFormat fmt)
{
    if (fmt >= sizeof(s_texFormat)/sizeof(SGLTextureFormatInfo))
        return s_texFormat[0];
        
    return s_texFormat[fmt];
}

CTexture::~CTexture()
{
    if (_gltex && !_doNotDeleteTexture) glDeleteTextures(1, &_gltex);
}

struct SDataMem
{
    SDataMem():mem(NULL), len(0), pos(0){};
    
    char* mem;
    unsigned len;
    unsigned pos;
};
static SDataMem loadWholeFile(const char* path)
{
    SDataMem r;
    r.mem = NULL;
    
    FILE* fp = fopen(path, "rb");
    if (!fp) return r;
    
    fseek(fp, 0, SEEK_END);
    unsigned size = (unsigned)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* mem = (char*)malloc(size);
    if (!mem) return r;
    
    if (fread(mem, size, 1, fp) != 1)
    {
        fclose(fp);
        return r;
    }
    
    r.mem = mem;
    r.len = size;
    return r;
}

static void readPngFromMem(png_structp png_ptr, png_bytep data, png_size_t length)
{
    SDataMem* dm = (SDataMem*)png_get_io_ptr(png_ptr);
    
    if (dm->pos + length > dm->len)
        return;
    
    memcpy(data, dm->mem + dm->pos, length);
    dm->pos += length;
}

static void pngError(png_structp png, png_const_charp err)
{
    printf("PNG ERROR/WARNING: %s\n", err);
}

bool CTexture::CreateTexture(unsigned wid, unsigned hei, ETextureFormat pixFormat, const char* textureData)
{
    _width = wid;
    _height = hei;
    
    glGenTextures(1, &_gltex);
    PrintGLError("generating texture");
	glBindTexture(GL_TEXTURE_2D, _gltex);
    PrintGLError("binding texture");
    
    // set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR/*for mipmaps, otherwise would use GL_LINEAR*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    PrintGLError("setting texture filtering and wrapping");
    
    int aniso = Min(CEngine::Inst()->GetRendererConfig().textureAnisotropy, CEngine::Inst()->GetRendererCapabilities().maxTextureAnisotropy);
    if (aniso>0)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        PrintGLError("setting texture anisotropy");
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    //if (comprformat)
    //    glCompressedTexImage2D(GL_TEXTURE_2D, 0, comprformat, wid, hei, 0, (bitsPerPixel*width*height)/8, textureData);
	//else
    glTexImage2D(GL_TEXTURE_2D, 0/*level*/, GLFormat(pixFormat).internalFormat, wid, hei, 0, GLFormat(pixFormat).format, GLFormat(pixFormat).type, textureData);
    PrintGLError("uploading texture data");
    
    glGenerateMipmap(GL_TEXTURE_2D);
    PrintGLError("generating mipmaps");
    
    return true;
}

bool CTexture::LoadAsTGA(const char* path)
{
    tgaInfo* tga = tgaLoad(path);
    if (!tga) return false;
    
    if (tga->type != TGA_OK)
    {
        tgaDestroy(tga);
        return false;
    }
    
    unsigned components = tga->pixelDepth/8;
    
    ETextureFormat pixFormat = TF_NONE;
    if (components==4)
        pixFormat = TF_RGBA8;
    else if (components==3)
        pixFormat = TF_RGB8;
    else if (components==2)
        pixFormat = TF_LA8;
    else if (components==1)
        printf("%s: 1-component texture is not supported\n", GetName());
    else
        printf("%s: Unknown format with %u components\n", GetName(), components);
    
    if (!CreateTexture(tga->width, tga->height, pixFormat, (const char*)tga->imageData))
    {
        tgaDestroy(tga);
        return false;
    }
    
    tgaDestroy(tga);
    printf("%s: TGA Texture loaded\n", GetName());
    return true;
}

bool CTexture::LoadAsPNG(const char* path)
{
    SDataMem dm = loadWholeFile(path);
    
    if (!dm.mem)
    {
        //printf("%s: Failed to open file %s\n", GetName(), path);
        return false;
    }
    
    if(!png_check_sig((const png_bytep)dm.mem, 8))
    {
        //printf("%s: Failed to load the texture. File %s is not a valid PNG file!\n", GetName(), path);
        return false;
    }
    
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    
    
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, pngError, pngError);
    if(!png_ptr)
    {
        printf("%s: Failed to create PNG struct\n", GetName());
        return false;
    }
    if(setjmp(png_jmpbuf(png_ptr)))
    {
        printf("%s: Failed to set PNG jmp. Apple compression? Not supported! https://discussions.apple.com/thread/1751896?start=0&tstart=0\n", GetName());
        return false;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr)
    {
        printf("%s: Failed to create png info struct\n", GetName());
        return false;
    }
    
    //png_set_expand(png_ptr); expand indexed or <8bpp images to RGB8
    
    dm.pos = 0;
    png_set_read_fn(png_ptr, &dm, readPngFromMem);
    png_read_info(png_ptr, info_ptr);
    
    int bitDepth = png_get_bit_depth(png_ptr, info_ptr);
    
    // convert palette or grayscale to RGB
    int colorType = png_get_color_type(png_ptr, info_ptr);
    if(colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
        png_set_gray_to_rgb(png_ptr);
    
    // convert to usable format
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if(bitDepth == 16)
        png_set_strip_16(png_ptr);
    else if(bitDepth < 8)
        png_set_packing(png_ptr);
    
    // get PNG info
    unsigned wid, hei;
    png_read_update_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &wid, &hei, &bitDepth, &colorType, NULL, NULL, NULL);;
    
    // get component num.
    unsigned components;
    switch(colorType)
    {
        case PNG_COLOR_TYPE_GRAY: components = 1; break;
        case PNG_COLOR_TYPE_GRAY_ALPHA: components = 2; break;
        case PNG_COLOR_TYPE_RGB: components = 3; break;
        case PNG_COLOR_TYPE_RGB_ALPHA: components = 4; break;
        default: {
            if(png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            printf("%s: Unsupported color type\n", GetName());
            return false;
        }
    }
    
    // copute pixel data len
    unsigned textureDataLen = wid * hei * components;
    char* textureData = (char*)malloc(textureDataLen);
    
    // make row pointers
    png_bytep* row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * hei);
    for(unsigned i = 0; i < hei; ++i)
        row_pointers[i] = (png_bytep)(textureData + (i * wid * components));
    
    // read all image rows
    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, NULL);
    
    // close
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    
    // get the right GL format
    ETextureFormat pixFormat = TF_NONE;
    if (components==4)
        pixFormat = TF_RGBA8;
    else if (components==3)
        pixFormat = TF_RGB8;
    else if (components==2)
        pixFormat = TF_LA8;
    else if (components==1)
        printf("%s: 1-component texture is not supported\n", GetName());
    else
        printf("%s: Unknown format with %u components\n", GetName(), components);
    
    if (!CreateTexture(wid, hei, pixFormat, textureData))
    {
        free(row_pointers);
        free(textureData);
        return false;
    }
    
    free(textureData);
    free(row_pointers); // oh do I need to do it manually??!!!
    
    printf("%s: PNG Texture loaded\n", GetName());
    return true;
}

bool CTexture::LoadFromFile(const char* path)
{
    if (!path || !*path) return false;
    
    _name = basename(const_cast<char*>(path));
    
    if (LoadAsPNG(path))
        return true;
    
    if (LoadAsTGA(path))
        return true;
        
    return false;
}

void CTexture::Use(unsigned unit)const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    PrintGLError("setting active texture unit");
    glBindTexture(GL_TEXTURE_2D, _gltex);
    PrintGLError("binding texture");
}




