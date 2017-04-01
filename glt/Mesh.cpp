//
//  Mesh.cpp
//  glt
//
//  Created by Mario Hros on 2. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#include "Mesh.h"

#include <stdlib.h>
#include <assert.h>

#include "Shared.h"
#include "Texture.h"
#include "Shaders.h"
#include "Engine.h"

#include "glstuff.h"
#include "../assimp/include/assimp/cimport.h"
#include "../assimp/include/assimp/scene.h"
#include "../assimp/include/assimp/postprocess.h"
#include "func_matrix.hpp"
#include "matrix_transform.hpp"

///////////////////////////////////////////////////////////////////////////////////////////
//// HELPERS

#ifdef __APPLE__
# define glDeleteVertexArrays glDeleteVertexArraysAPPLE
# define glGenVertexArrays glGenVertexArraysAPPLE
# define glBindVertexArray glBindVertexArrayAPPLE
#endif

// must be defined for each EPrimitiveType
static GLenum s_primTypes[] = {
    0,//PRIM_NONE=0,
    GL_POINTS,//PRIM_POINTS,
    GL_LINE_STRIP,//PRIM_LINE_STRIP,
    GL_LINES,//PRIM_LINES,
    GL_TRIANGLE_STRIP,//PRIM_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,//PRIM_TRIANGLE_FAN,
    GL_TRIANGLES,//PRIM_TRIANGLES,
    GL_QUADS,//PRIM_QUADS
    GL_POLYGON//PRIM_POLYGON
};

// must be defined for each EType
static GLenum s_types[] = {
    0,//T_UNKNOWN=0,
    GL_BOOL,//T_BOOL,
    GL_BYTE,//T_BYTE, // 1B
    GL_UNSIGNED_BYTE,//T_UNSIGNED_BYTE,
    GL_SHORT,//T_SHORT, // 2B
    GL_UNSIGNED_SHORT,//T_UNSIGNED_SHORT,
    GL_INT,//T_INT, // 3B
    GL_UNSIGNED_INT,//T_UNSIGNED_INT,
    GL_FLOAT,//T_FLOAT, // sizeof(float)
    0//T_STRING // const char* or std::string
};
    
unsigned Attrib2Index(EVertexAttrib a) // 0 for ATTRIB_POSITION, 1 for ATTRIB_COLOR0, ...
{
	for (unsigned i=0; i<32; i++)
	{
		if ( (1<<i) & a )
			return i;
	}
	return 0;
}

unsigned ComputeVertDataLen(unsigned attribs)
{
	unsigned len=0;
	
	if (attribs & ATTRIB_POSITION)
		len += sizeof(float)*3;
	if (attribs & ATTRIB_COLOR0)
		len += 4;
	if (attribs & ATTRIB_COORDS0)
		len += sizeof(float)*2;
	if (attribs & ATTRIB_NORMAL)
		len += sizeof(float)*3;
	if (attribs & ATTRIB_TANGENT)
		len += sizeof(float)*3;
	if (attribs & ATTRIB_BITANGENT)
		len += sizeof(float)*3;
    if (attribs & ATTRIB_PROJVEC)
		len += sizeof(float)*3;
	
	return len;
}

const char* GetAttribString(unsigned attribs)
{
	static char buf[64];
	buf[0] = 0;
	
	if (attribs & ATTRIB_POSITION)
		strcat(buf,"Pos");
	if (attribs & ATTRIB_COLOR0)
		strcat(buf,"Cl0");
	if (attribs & ATTRIB_COORDS0)
		strcat(buf,"Tx0");
	if (attribs & ATTRIB_NORMAL)
		strcat(buf,"Nor");
	if (attribs & ATTRIB_TANGENT)
		strcat(buf,"Tan");
	if (attribs & ATTRIB_BITANGENT)
		strcat(buf,"Btn");
    if (attribs & ATTRIB_PROJVEC)
		strcat(buf,"Prv");
	
	return buf;
}

unsigned ComputeAttribOffset(unsigned attrib, unsigned allAttribs)
{
	unsigned mask = 0;
	unsigned maxShift = Attrib2Index((EVertexAttrib)attrib);
	for (unsigned i=0; i<maxShift; i++)
		mask |= (1<<i) & allAttribs;
	
	return ComputeVertDataLen(mask);
}

////////////////////////////////////////////////////////////////////////////////////
//// MESH CLASS HELPERS

CMeshStreamData::CMeshStreamData(const void* data, unsigned byteSize, bool makeCopy)
: _shouldFreeData(makeCopy), _byteSize(byteSize)
{
    if (!makeCopy)
        _data = const_cast<void*>(data);
    else
    {
        _data = malloc(_byteSize);
        assert(_data);
        memcpy(_data, data, byteSize);
    }
}

CMeshStreamData::~CMeshStreamData()
{
    if (_shouldFreeData && _data)
        free(_data);
}

////////////////////////////////////////////////////////////////////////////////////
//// MESH CLASS

CMesh::CMesh(const char* name)
: _scene(NULL), _attrs(NULL), _name(name), _scale(glm::vec3(1,1,1))
{
    
}

CMesh::~CMesh()
{
    STD_CONST_FOREACH(GLBufferArray, _glbuff, it)
    {
        glDeleteBuffers(1, &it->vertBuffer);
        glDeleteBuffers(1, &it->indBuffer);
        glDeleteVertexArrays(1, &it->vertArrayObj);
        if (it->diffuseTex) it->diffuseTex->Release();
        if (it->normalSpecularTex) it->normalSpecularTex->Release();
        if (it->normalProg) it->normalProg->Release();
        if (it->zProg) it->zProg->Release();
        if (it->materialProg) it->materialProg->Release();
    }
}

const CMesh& CMesh::FullscreenQuad()
{
    static CMesh* mesh = NULL;
 
    if (!mesh)
    {
        mesh = new CMesh("FullscreenQuad");
        
        float pos[4][3] = {
            {-1, 1,0},
            {-1,-1,0},
            { 1,-1,0},
            { 1, 1,0}
        };
        float coords[4][2] = {
            {0,1},
            {0,0},
            {1,0},
            {1,1}
        };
        unsigned short inds[] = { 0, 1, 2, 3 };
        
        CMeshPart mp(PRIM_QUADS, CVertexStream(ATTRIB_POSITION, pos, sizeof(pos), false), CIndexStream(T_UNSIGNED_SHORT, inds, sizeof(inds), false));
        mp.AddVertexStream(CVertexStream(ATTRIB_COORDS0, coords, sizeof(coords), false));
        //mp.AddVertexStream(CVertexStream(ATTRIB_PROJVEC, pos, sizeof(pos), false));
        // TODO: prepare fullscreen plane
        
        mesh->AddMeshPart(mp);
    }
    
    return *mesh;
}
const CMesh& CMesh::UnitPlane()
{
    static CMesh* mesh = NULL;
    
    if (!mesh)
    {
        mesh = new CMesh("UnitPlane");
    
        float pos[4][3] = {
            {1,0,0},
            {0,0,0},
            {0,1,0},
            {1,1,0}
        };
        float coords[4][2] = {
            {1,1},
            {0,1},
            {0,0},
            {1,0}
        };
        unsigned short inds[] = { 0, 1, 2, 2, 3, 0 };
        
        CMeshPart mp(PRIM_TRIANGLES, CVertexStream(ATTRIB_POSITION, pos, sizeof(pos), false), CIndexStream(T_UNSIGNED_SHORT, inds, sizeof(inds), false));
        mp.AddVertexStream(CVertexStream(ATTRIB_COORDS0, coords, sizeof(coords), false));
        
        mesh->AddMeshPart(mp);
    }
    
    return *mesh;
}
/* cube
 float pos[] = {
 -1.0f, -1.0f, 1.0f,
 1.0f, -1.0f, 1.0f,
 1.0f, 1.0f, 1.0f,
 -1.0f, 1.0f, 1.0f,
 -1.0f, -1.0f, -1.0f,
 1.0f, -1.0f, -1.0f,
 1.0f, 1.0f, -1.0f,
 -1.0f, 1.0f, -1.0f
 };
 
 unsigned short inds[] = {  0, 1, 2, 2, 3, 0,
 3, 2, 6, 6, 7, 3,
 7, 6, 5, 5, 4, 7,
 4, 0, 3, 3, 7, 4,
 0, 1, 5, 5, 4, 0,
 1, 5, 6, 6, 2, 1  };
 */
const CMesh& CMesh::UnitIcosphere()
{
    static CMesh* mesh = NULL;
    
    if (!mesh)
    {
        mesh = new CMesh("UnitIcosphere");
        
        float pos[] = {0.000000,-1.000000,0.000000,0.723607,-0.447220,0.525725,-0.276388,-0.447220,0.850649,-0.894426,-0.447216,0.000000,-0.276388,-0.447220,-0.850649,0.723607,-0.447220,-0.525725,0.276388,0.447220,0.850649,-0.723607,0.447220,0.525725,-0.723607,0.447220,-0.525725,0.276388,0.447220,-0.850649,0.894426,0.447216,0.000000,0.000000,1.000000,0.000000,0.425323,-0.850654,0.309011,0.262869,-0.525738,0.809012,-0.162456,-0.850654,0.499995,0.425323,-0.850654,-0.309011,0.850648,-0.525736,0.000000,-0.688189,-0.525736,0.499997,-0.525730,-0.850652,0.000000,-0.688189,-0.525736,-0.499997,-0.162456,-0.850654,-0.499995,0.262869,-0.525738,-0.809012,0.951058,0.000000,-0.309013,0.951058,0.000000,0.309013,0.587786,0.000000,0.809017,0.000000,0.000000,1.000000,-0.587786,0.000000,0.809017,-0.951058,0.000000,0.309013,-0.951058,0.000000,-0.309013,-0.587786,0.000000,-0.809017,0.000000,0.000000,-1.000000,0.587786,0.000000,-0.809017,0.688189,0.525736,0.499997,-0.262869,0.525738,0.809012,-0.850648,0.525736,0.000000,-0.262869,0.525738,-0.809012,0.688189,0.525736,-0.499997,0.525730,0.850652,0.000000,0.162456,0.850654,0.499995,-0.425323,0.850654,0.309011,-0.425323,0.850654,-0.309011,0.162456,0.850654,-0.499995,};
        
        unsigned short inds[] = {
            0,12,14,1,12,16,0,14,18,0,18,20,0,20,15,1,16,23,2,13,25,3,17,27,4,19,29,5,21,31,1,23,24,2,25,26,3,27,28,4,29,30,5,31,22,6,32,38,7,33,39,8,34,40,9,35,41,10,36,37,14,13,2,14,12,13,12,1,13,16,15,5,16,12,15,12,0,15,18,17,3,18,14,17,14,2,17,20,19,4,20,18,19,18,3,19,15,21,5,15,20,21,20,4,21,23,22,10,23,16,22,16,5,22,25,24,6,25,13,24,13,1,24,27,26,7,27,17,26,17,2,26,29,28,8,29,19,28,19,3,28,31,30,9,31,21,30,21,4,30,24,32,6,24,23,32,23,10,32,26,33,7,26,25,33,25,6,33,28,34,8,28,27,34,27,7,34,30,35,9,30,29,35,29,8,35,22,36,10,22,31,36,31,9,36,38,37,11,38,32,37,32,10,37,39,38,11,39,33,38,33,6,38,40,39,11,40,34,39,34,7,39,41,40,11,41,35,40,35,8,40,37,41,11,37,36,41,36,9,41,
        };
        
        CMeshPart mp(PRIM_TRIANGLES, CVertexStream(ATTRIB_POSITION, pos, sizeof(pos), false), CIndexStream(T_UNSIGNED_SHORT, inds, sizeof(inds), false));
        //mp.AddVertexStream(CVertexStream(ATTRIB_NORMAL, pos, sizeof(pos), false));
        
        mesh->AddMeshPart(mp);
    }
    
    return *mesh;
}
const CMesh& CMesh::UnitBox()
{
    static CMesh* mesh = NULL;
    
    if (!mesh)
    {
        mesh = new CMesh("UnitBox");
        
        float pos[] = {-1.0f,-1.0f,1.0f,1.0f,-1.0f,1.0f,1.0f,1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,-1.0f,-1.0f,1.0f,-1.0f,-1.0f,1.0f,1.0f,-1.0f,-1.0f,1.0f,-1.0f};
        float uv[] = {-1.0f,-1.0f,1.0f,1.0f,-1.0f,1.0f,1.0f,1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,-1.0f,-1.0f,1.0f,-1.0f,-1.0f,1.0f,1.0f,-1.0f,-1.0f,1.0f,-1.0f};
        
        unsigned short inds[] = {
            0,1,2,2,3,0,3,2,6,6,7,3,7,6,5,5,4,7,4,0,3,3,7,4,0,1,5,5,4,0,1,5,6,6,2,1
        };
        
        CMeshPart mp(PRIM_TRIANGLES, CVertexStream(ATTRIB_POSITION, pos, sizeof(pos), false), CIndexStream(T_UNSIGNED_SHORT, inds, sizeof(inds), false));
        mp.AddVertexStream(CVertexStream(ATTRIB_COORDS0, uv, sizeof(uv), false));
        
        mesh->AddMeshPart(mp);
    }
    
    return *mesh;
}
const CMesh& CMesh::AxisLines()
{
    static CMesh* mesh = NULL;
    
    if (!mesh)
    {
        mesh = new CMesh("AxisLines");
        
        float pos[4][3] = {
            {0,0,0},
            {1,0,0},
            {0,1,0},
            {0,0,1}
        };
        unsigned char colors[4][4] = {
            {1,1,1,1},
            {1,0,0,1},
            {0,1,0,1},
            {0,0,1,1}
        };
        unsigned short inds[] = { 0, 1, 0, 2, 0, 3 };
        
        CMeshPart mp(PRIM_LINES, CVertexStream(ATTRIB_POSITION, pos, sizeof(pos), false), CIndexStream(T_UNSIGNED_SHORT, inds, sizeof(inds), false));
        mp.AddVertexStream(CVertexStream(ATTRIB_COLOR0, colors, sizeof(colors), false));
        
        mesh->AddMeshPart(mp);
    }
    
    return *mesh;
}

bool CMesh::LoadFromFile(const char* path, bool generateNormals, bool calcTangentSpace)
{
    if (!path) return false;
    
    _name = basename(const_cast<char*>(path));
    
    unsigned optionalFlags = /*aiProcess_GenUVCoords*/0;
    if (generateNormals) optionalFlags |= aiProcess_GenNormals;
    if (calcTangentSpace) optionalFlags |= aiProcess_CalcTangentSpace;
    
    _scene = aiImportFile(path, aiProcess_PreTransformVertices | aiProcess_Triangulate | aiProcess_SortByPType | optionalFlags );
    if (!_scene)
    {
        printf("Failed to load mesh file %s - unknown file type?\n", path);
        return false;
    }
    
    bool success = _scene != 0;
    if (success)
    {
        // create GL objects
        CreateGLMeshesFromAssimp();
        
        // print stats
        unsigned verts = 0, inds = 0;
        CalcStatsInNode(_scene->mRootNode, verts, inds);
        printf("%s: Mesh loaded; Attributes: %s (%d bytes), Meshes: %u, Vertices: %u, Indices: %u\n",
               GetName(), GetAttribString(_attrs), ComputeVertDataLen(_attrs), _scene->mNumMeshes, verts, inds);
    }
    
    return success;
}

void CMesh::Draw(EDrawPass pass)const
{
    if (_scene)
        DrawNode(pass, _scene->mRootNode);
    else
        DrawBufferArray(pass);
}

std::string CMesh::LocateTexture(const char* path) const
{
    if (!path || !*path)
        return std::string();
    
    
    char tmpPath[1024];
    const char* finalPath = NULL;
    
    unsigned pathLen = (unsigned)strlen(path);
    
    // try to remove subdirectories
    finalPath = path;
    for(unsigned i= 0; i<pathLen; i++)
        if (path[i] == '/' || path[i] == '\\')
            finalPath = &path[i+1];
    path = finalPath;
    pathLen = (unsigned)strlen(path);
    
    // try as-is
    finalPath = LocateFile(path);
    
    // change extension to png
    if (!finalPath)
    {
        strcpy(tmpPath, path);
        if (pathLen>4 && tmpPath[pathLen-4] == '.')
            strcpy(&tmpPath[pathLen-4], ".png");
        
        finalPath = LocateFile(tmpPath);
    }
    
    // change extension to tga
    if (!finalPath)
    {
        strcpy(tmpPath, path);
        if (pathLen>4 && tmpPath[pathLen-4] == '.')
            strcpy(&tmpPath[pathLen-4], ".tga");
        
        finalPath = LocateFile(tmpPath);
    }
    
    if (!finalPath)
        return std::string();
    else
        return std::string(finalPath);
}

void CMesh::CreateGLMeshesFromAssimp()
{
    assert(_scene);
    
    glGetError();
    
    for (int m=0; m<_scene->mNumMeshes; m++)
    {
        const struct aiMesh* mesh = _scene->mMeshes[m];
        const struct aiMaterial* mat = _scene->mMaterials[mesh->mMaterialIndex];
        
        GLBuffer glbuff;
        
        // material
        if (mat->GetTextureCount(aiTextureType_DIFFUSE)>0)
        {
            bool inverseNormalY = false;
            
            // get path to diffuse texture
            aiString path;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
            
            char tmpPath[1024];
            
            // load diffuse
            glbuff.diffuseTex = CTexture::FromFile(LocateTexture(path.C_Str()).c_str());
            
            // load normal texture
            std::string normalPath;
            if (mat->GetTextureCount(aiTextureType_NORMALS)>0)
            {
                mat->GetTexture(aiTextureType_NORMALS, 0, &path);
                normalPath = LocateTexture(normalPath.c_str());
            }
            
            // try to find normal based on diffusemap path
            if (!normalPath.length())
            {
                strcpy(tmpPath, path.C_Str());
                if (path.length>4 && tmpPath[path.length-4] == '.')
                {
                    tmpPath[path.length-4] = 0;
                    strcat(tmpPath, "_ns.png");
                }
                normalPath = LocateTexture(tmpPath);
            }
            if (!normalPath.length())
            {
                strcpy(tmpPath, path.C_Str());
                if (path.length>4 && tmpPath[path.length-4] == '.')
                {
                    tmpPath[path.length-4] = 0;
                    strcat(tmpPath, "_ddn.tga");
                }
                normalPath = LocateTexture(tmpPath);
                if (normalPath.length()) inverseNormalY = true;
            }
            if (!normalPath.length())
            {
                strcpy(tmpPath, path.C_Str());
                if (path.length>8 && tmpPath[path.length-4] == '.')
                {
                    tmpPath[path.length-8] = 0;
                    strcat(tmpPath, "ddn.tga");
                }
                normalPath = LocateTexture(tmpPath);
                if (normalPath.length()) inverseNormalY = true;
            }
            if (normalPath.length())
                glbuff.normalSpecularTex = CTexture::FromFile(normalPath.c_str());
            
            CShaderDefines defines;
            if (glbuff.diffuseTex)
            {
                defines.Define("TEXTURE0,ATTRIB_COORDS0");
                if (glbuff.normalSpecularTex)
                    defines.Define("NORMAL_SPECULAR_MAP");
            }
            
            // load shaders
            CShaderDefines normalDefines(defines);
            if (inverseNormalY) normalDefines.Define("NORMAL_SPECULAR_MAP_INVERSEY");
            glbuff.normalProg = CShaderManager::Inst()->GetProgram("normal.glsl", &normalDefines);
            glbuff.zProg = CShaderManager::Inst()->GetProgram("z.glsl", &defines);
            glbuff.materialProg = CShaderManager::Inst()->GetProgram("material.glsl", &defines);
        }
        
        // VAO
        glGenVertexArrays(1, &glbuff.vertArrayObj);
        PrintGLError("generating VAO");
        glBindVertexArray(glbuff.vertArrayObj);
        PrintGLError("binding VAO");
        
        
        // VERTEX BUFFER
        glGenBuffers(1, &glbuff.vertBuffer);
        PrintGLError("generating vertex buffer");
        glBindBuffer(GL_ARRAY_BUFFER, glbuff.vertBuffer);
        PrintGLError("binding vertex buffer");
        
        _attrs = ATTRIB_POSITION;
        
        if (mesh->mVertices)
            _attrs |= ATTRIB_POSITION;
        if (mesh->mColors[0])
            _attrs |= ATTRIB_COLOR0;
        if (mesh->mTextureCoords[0])
            _attrs |= ATTRIB_COORDS0;
        if (mesh->mNormals)
            _attrs |= ATTRIB_NORMAL;
        if (mesh->mTangents)
            _attrs |= ATTRIB_TANGENT;
        if (mesh->mBitangents)
            _attrs |= ATTRIB_BITANGENT;
        
        unsigned vertlen = ComputeVertDataLen(_attrs);
        unsigned vbuflen = mesh->mNumVertices * vertlen;
        char* vbufdata = (char*)malloc(vbuflen);
        EVertexAttrib atr = ATTRIB_NONE;
        
        for (unsigned v=0; v<mesh->mNumVertices; v++)
        {
            char* const vertStart = vbufdata + v*vertlen;
            
            atr = ATTRIB_POSITION;
            if (mesh->mVertices)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)NULL);
                    PrintGLError("setting vertex attribute pointer");
                }
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), &mesh->mVertices[v].x, 3*sizeof(float));
            }
            
            atr = ATTRIB_COLOR0;
            if (mesh->mColors[0])
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 4, GL_UNSIGNED_BYTE, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                unsigned char* col = (unsigned char*)(vertStart + ComputeAttribOffset(atr, _attrs));
                col[0] = mesh->mColors[0][v].r*255.f; col[1] = mesh->mColors[1][v].g*255; col[0] = mesh->mColors[2][v].b*255; col[0] = mesh->mColors[3][v].a*255;
            }
            
            atr = ATTRIB_COORDS0;
            if (mesh->mTextureCoords[0])
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 2, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), &mesh->mTextureCoords[0][v].x, 2*sizeof(float));
            }
            
            atr = ATTRIB_NORMAL;
            if (mesh->mNormals)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), &mesh->mNormals[v].x, 3*sizeof(float));
            }
            
            atr = ATTRIB_TANGENT;
            if (mesh->mTangents)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), &mesh->mTangents[v].x, 3*sizeof(float));
            }
            
            atr = ATTRIB_BITANGENT;
            if (mesh->mBitangents)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), &mesh->mBitangents[v].x, 3*sizeof(float));
            }
        }
        
        // VERTEX BUFFER DATA
        glBufferData(GL_ARRAY_BUFFER, vbuflen, vbufdata, GL_STATIC_DRAW);
        PrintGLError("uploading vertex buffer data");
        free(vbufdata);
        
        // INDEX BUFFER
        glGenBuffers(1, &glbuff.indBuffer);
        PrintGLError("generating index buffer");
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glbuff.indBuffer);
        PrintGLError("binding index buffer");
        
        glbuff.numInds = 0;
        for (unsigned f=0; f<mesh->mNumFaces; f++)
            glbuff.numInds += mesh->mFaces[f].mNumIndices;
        
        glbuff.primType = 0;
        if (mesh->mNumFaces>0)
        {
            switch(mesh->mFaces[0].mNumIndices)
            {
                case 1: glbuff.primType = GL_POINTS; break;
                case 2: glbuff.primType = GL_LINES; break;
                case 3: glbuff.primType = GL_TRIANGLES; break;
                default: glbuff.primType = GL_POLYGON; break;
            }
        }
        
        unsigned short* ibufdata = (unsigned short*)malloc(glbuff.numInds*sizeof(unsigned short));
        unsigned short* iptr = ibufdata;
        for (unsigned f=0; f<mesh->mNumFaces; f++)
        {
            for (unsigned i=0; i<mesh->mFaces[f].mNumIndices; i++)
                *(iptr++) = mesh->mFaces[f].mIndices[i];
        }
        
        // INDEX BUFFER DATA
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*glbuff.numInds, ibufdata, GL_STATIC_DRAW);
        PrintGLError("sending index buffer data");
        
        glbuff.indBufferType = GL_UNSIGNED_SHORT;
        
        free(ibufdata);
        
        // ADD GLMESH TO LIST
        _glbuff.push_back(glbuff);
    }
    
    // CLEAN.. just in case..
    glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glGetError();
}

bool CMesh::AddMeshPart(const CMeshPart &part)
{
    // get total usage attributes & do safe checks
    unsigned numVerts = 0;
    bool hasPositions = false;
    _attrs = 0;
    
    STD_CONST_FOREACH(CMeshPart::VertexStreamArray, part.GetVertexStreams(), it)
    {
        const CVertexStream* stream = *it;
        if (!numVerts)
            numVerts = stream->GetByteLength()/ComputeVertDataLen(stream->GetUsage());
        else if (stream->GetByteLength()/ComputeVertDataLen(stream->GetUsage()) != numVerts)
        {
            printf("Vertex streams contain different number of elements (vertices) or unit length is not standard according to usage specified!\n");
            return false;
        }
        
        if (stream->GetUsage() == ATTRIB_POSITION)
            hasPositions = true;
        
        _attrs |= stream->GetUsage();
    }
    
    if (!hasPositions)
    {
        printf("Specified mesh part doesn't contain position stream!\n");
        return false;
    }
    
    glGetError();
    
    GLBuffer glbuff;
    
    // VAO
    glGenVertexArrays(1, &glbuff.vertArrayObj);
    PrintGLError("generating VAO");
    glBindVertexArray(glbuff.vertArrayObj);
    PrintGLError("binding VAO");
    
    // VERTEX BUFFER
    glGenBuffers(1, &glbuff.vertBuffer);
    PrintGLError("generating vertex buffer");
    glBindBuffer(GL_ARRAY_BUFFER, glbuff.vertBuffer);
    PrintGLError("binding vertex buffer");
    
    unsigned vertlen = ComputeVertDataLen(_attrs);
    unsigned vbuflen = numVerts * vertlen;
    char* vbufdata = (char*)malloc(vbuflen);
    EVertexAttrib atr = ATTRIB_NONE;
    
    for (unsigned v=0; v<numVerts; v++)
    {
        char* const vertStart = vbufdata + v*vertlen;
        
        STD_CONST_FOREACH(CMeshPart::VertexStreamArray, part.GetVertexStreams(), it)
        {
            const CVertexStream* stream = *it;
            
            atr = ATTRIB_POSITION;
            if (stream->GetUsage() == atr)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)NULL);
                    PrintGLError("setting vertex attribute pointer");
                }
                const unsigned len = ComputeVertDataLen(atr);
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), stream->GetData()+len*v, len);
            }
            
            atr = ATTRIB_COLOR0;
            if (stream->GetUsage() == atr)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 4, GL_UNSIGNED_BYTE, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                const unsigned len = ComputeVertDataLen(atr);
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), stream->GetData()+len*v, len);
            }
            
            atr = ATTRIB_COORDS0;
            if (stream->GetUsage() == atr)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 2, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                const unsigned len = ComputeVertDataLen(atr);
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), stream->GetData()+len*v, len);
            }
            
            atr = ATTRIB_NORMAL;
            if (stream->GetUsage() == atr)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                const unsigned len = ComputeVertDataLen(atr);
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), stream->GetData()+len*v, len);
            }
            
            atr = ATTRIB_TANGENT;
            if (stream->GetUsage() == atr)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                const unsigned len = ComputeVertDataLen(atr);
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), stream->GetData()+len*v, len);
            }
            
            atr = ATTRIB_BITANGENT;
            if (stream->GetUsage() == atr)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                const unsigned len = ComputeVertDataLen(atr);
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), stream->GetData()+len*v, len);
            }
            
            atr = ATTRIB_PROJVEC;
            if (stream->GetUsage() == atr)
            {
                if (v==0)
                {
                    glEnableVertexAttribArray(Attrib2Index(atr));
                    glVertexAttribPointer(Attrib2Index(atr), 3, GL_FLOAT, 0, vertlen, (void*)(long)ComputeAttribOffset(atr, _attrs));
                    PrintGLError("setting vertex attribute pointer");
                }
                const unsigned len = ComputeVertDataLen(atr);
                memcpy(vertStart + ComputeAttribOffset(atr, _attrs), stream->GetData()+len*v, len);
            }
        }
        
    }
    
    // VERTEX BUFFER DATA
    glBufferData(GL_ARRAY_BUFFER, vbuflen, vbufdata, GL_STATIC_DRAW);
    PrintGLError("uploading vertex buffer data");
    free(vbufdata);
    
    glbuff.indBuffer= 0;
    if (part.GetIndexStream())
    {
        const CIndexStream* istream = part.GetIndexStream();
        
        // INDEX BUFFER
        glGenBuffers(1, &glbuff.indBuffer);
        PrintGLError("generating index buffer");
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glbuff.indBuffer);
        PrintGLError("binding index buffer");
        
        // INDEX BUFFER DATA
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, istream->GetByteLength(), istream->GetData(), GL_STATIC_DRAW);
        PrintGLError("sending index buffer data");
        
        glbuff.numInds = part.GetIndexStream()->GetByteLength()/GetTypeSize(istream->GetType());
        glbuff.indBufferType = s_types[istream->GetType()];
    }
    else
        glbuff.numInds = numVerts;
    
    glbuff.primType = s_primTypes[part.GetPrimitiveType()];
    
    // ADD GLMESH TO LIST
    _glbuff.push_back(glbuff);
    
    printf("%s: Mesh part added; Attributes: %s (%d bytes), Vertices: %u, Indices: %u\n",
           GetName(), GetAttribString(_attrs), ComputeVertDataLen(_attrs), numVerts, glbuff.numInds);
    
    // CLEAN.. just in case..
    glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glGetError();
    
    return true;
}

void CMesh::DrawNode(EDrawPass pass, const struct aiNode *nd)const
{
    // update transform
    /*aiMatrix4x4 m = nd->mTransformation;
    aiTransposeMatrix4(&m);
    glPushMatrix();
    glMultMatrixf((float*)&m);*/
    
    for (int m=0; m<nd->mNumMeshes; m++)
    {
        assert(m<_glbuff.size());
        
        int meshId = nd->mMeshes[m];
        const GLBuffer& glbuff = _glbuff[meshId];
        
        // MATERIAL
        IScene* scene = CEngine::Inst()->GetScene();
        CShaderProgram* prog = NULL;
        if (pass == DRAW_Z)
            prog = glbuff.zProg;
        else if (pass == DRAW_NORMAL)
        {
            prog = glbuff.normalProg;
            if (prog && glbuff.normalSpecularTex)
                prog->SetUniform("uTexNormalSpecular", *glbuff.normalSpecularTex, 0);
        }
        else if (pass == DRAW_MATERIAL)
        {
            prog = glbuff.materialProg;
            if (prog && glbuff.diffuseTex)
                prog->SetUniform("uTex0", *glbuff.diffuseTex, 0);
            
            if (prog)
            {
                prog->SetUniform("uAmbientColor", scene->GetAmbientColor());
                prog->SetUniform("uDiffuseAcc", scene->GetRTTexture(IScene::RT_DIFFUSE_ACC), 1);
            }
        }
        
        if (prog)
        {
            scene->SetCommonUniforms(prog,  glm::scale(glm::translate(glm::mat4(), _pos), _scale));
            prog->Use();
        }
        
        // BIND VAO
        glBindVertexArrayAPPLE(_glbuff[meshId].vertArrayObj);
        PrintGLError("binding VAO");
        
        // DRAW MESH
        if (_glbuff[meshId].indBuffer)
            glDrawElements((GLenum)glbuff.primType, glbuff.numInds, glbuff.indBufferType, 0);
        else
            glDrawArrays((GLenum)glbuff.primType, 0, glbuff.numInds);
        PrintGLError("drawing elements");
    }
    
    // draw all children
    for (int n = 0; n < nd->mNumChildren; n++)
        DrawNode(pass, nd->mChildren[n]);
    
    /*glPopMatrix();*/
    
    // just in case..
    //glBindVertexArrayAPPLE(0);
    //glGetError();
}

void CMesh::DrawBufferArray(EDrawPass pass)const
{
    STD_CONST_FOREACH(GLBufferArray, _glbuff, ib)
    {
        // MATERIAL
        const GLBuffer& glbuff = *ib;
        IScene* scene = CEngine::Inst()->GetScene();
        CShaderProgram* prog = NULL;
        if (pass == DRAW_Z)
            prog = glbuff.zProg;
        else if (pass == DRAW_NORMAL)
        {
            prog = glbuff.normalProg;
            if (prog && glbuff.normalSpecularTex)
                prog->SetUniform("uTexNormalSpecular", *glbuff.normalSpecularTex, 0);
        }
        else if (pass == DRAW_MATERIAL)
        {
            prog = glbuff.materialProg;
            if (prog && glbuff.diffuseTex)
                prog->SetUniform("uTex0", *glbuff.diffuseTex, 0);
            
            if (prog)
            {
                prog->SetUniform("uAmbientColor", scene->GetAmbientColor());
                prog->SetUniform("uDiffuseAcc", scene->GetRTTexture(IScene::RT_DIFFUSE_ACC), 1);
            }
        }
        
        if (prog)
        {
            scene->SetCommonUniforms(prog,  glm::translate(glm::scale(glm::mat4(), _scale), _pos));
            prog->Use();
        }
            
        // BIND VAO
        glBindVertexArrayAPPLE(ib->vertArrayObj);
        PrintGLError("binding VAO");
        
        // DRAW MESH
        if (ib->indBuffer)
            glDrawElements((GLenum)ib->primType, ib->numInds, ib->indBufferType, 0);
        else
            glDrawArrays((GLenum)ib->primType, 0, ib->numInds);
        
        PrintGLError("drawing elements");
    }
}

void CMesh::CalcStatsInNode(const struct aiNode *nd, unsigned& outVerts, unsigned& outInds)
{
    for (int n=0; n < nd->mNumMeshes; n++)
    {
        const struct aiMesh* mesh = _scene->mMeshes[nd->mMeshes[n]];
        outVerts += mesh->mNumVertices;
        
        for (int t = 0; t < mesh->mNumFaces; t++)
        {
            const struct aiFace* face = &mesh->mFaces[t];
            outInds += face->mNumIndices;
        }
    }
    
    // draw all children
    for (int n = 0; n < nd->mNumChildren; n++)
        CalcStatsInNode(nd->mChildren[n], outVerts, outInds);
}





