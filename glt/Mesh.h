//
//  Mesh.h
//  glt
//
//  Created by Mario Hros on 2. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef __glt__Mesh__
#define __glt__Mesh__

#include <string>
#include <vector>
#include <assert.h>
#include <algorithm> // std::sort

#include "Types.h"
#include "vec3.hpp"

struct aiScene;
struct aiNode;
struct aiMaterial;
struct aiMesh;

class CTexture;
class CShaderProgram;

// vertex attributes
enum EVertexAttrib {
	ATTRIB_NONE		= 0,
	ATTRIB_POSITION	= 1<<0,
	ATTRIB_COLOR0	= 1<<1,
	ATTRIB_COORDS0	= 1<<2,
	ATTRIB_NORMAL	= 1<<3,
	ATTRIB_TANGENT	= 1<<4,
	ATTRIB_BITANGENT= 1<<5,
	ATTRIB_PROJVEC	= 1<<6 // screen space projection direction
};

// must be defined in s_primTypes
enum EPrimitiveType
{
    PRIM_NONE=0,
    PRIM_POINTS,
    PRIM_LINE_STRIP,
    PRIM_LINES,
    PRIM_TRIANGLE_STRIP,
    PRIM_TRIANGLE_FAN,
    PRIM_TRIANGLES,
    PRIM_QUADS,
    PRIM_POLYGON
};

unsigned Attrib2Index(EVertexAttrib a); // 0 for ATTRIB_POSITION, 1 for ATTRIB_COLOR0, ...
unsigned ComputeVertDataLen(unsigned attribs);
const char* GetAttribString(unsigned attribs);
unsigned ComputeAttribOffset(unsigned attrib, unsigned allAttribs);

/// Helper class
class CMeshStreamData
{
public:
    const char* GetData()const{ return (const char*)_data; };
    unsigned GetByteLength()const{ return _byteSize; };
    
protected:
    CMeshStreamData(const void* data, unsigned byteSize, bool makeCopy);
    virtual ~CMeshStreamData();
    
    bool _shouldFreeData;
    void* _data;
    unsigned _byteSize;
};

/// Helper class used for filling CMeshPart with indices. Not used for actual rendering.
class CIndexStream : public CMeshStreamData
{
public:
    /// \param makeCopy will make a private copy of data. If setting false,
    /// make sure the data is valid for the livespan of the class
    CIndexStream(EType type, const void* data, unsigned byteSize, bool makeCopy):_type(type), CMeshStreamData(data, byteSize, makeCopy){};
    
    EType GetType()const{ return _type; };
    
private:
    EType _type;
};

/// Helper class used for filling CMeshPart with vertex data. Not used for actual rendering.
class CVertexStream : public CMeshStreamData
{
public:
    /// \note Data units must be standard length depending on usage, see ComputeVertDataLen()
    CVertexStream(EVertexAttrib usage, const void* data, unsigned byteSize, bool makeCopy=false):_usage(usage), CMeshStreamData(data, byteSize, makeCopy){};
    EVertexAttrib GetUsage()const{ return _usage; };
    
private:
    EVertexAttrib _usage;
};

inline bool vertexStreamPtrComparator(const CVertexStream* a, const CVertexStream* b)
{
    assert(a && b);
    return a->GetUsage() < b->GetUsage();
}

/// Helper class used for filling CMesh with data. Not used for actual rendering.
class CMeshPart
{
public:
    typedef std::vector<const CVertexStream*> VertexStreamArray;
    
    /// Empry mesh part
    CMeshPart():_indexStream(NULL),_primType(PRIM_NONE){};
    /// Mesh part with positions
    CMeshPart(EPrimitiveType primType, const CVertexStream& positions)
    : _primType(primType), _indexStream(NULL)
    {
        assert(positions.GetUsage() == ATTRIB_POSITION);
        AddVertexStream(positions);
    };
    /// Mesh part with positions and indices
    CMeshPart(EPrimitiveType primType, const CVertexStream& positions, const CIndexStream& indices)
    : _primType(primType),_indexStream(&indices)
    {
        assert(positions.GetUsage() == ATTRIB_POSITION);
        AddVertexStream(positions);
    }

    /// Sets a new index stream, replacing an existing one
    bool SetIndexStream(const CIndexStream& stream){ _indexStream = &stream; return true; };
    /// Adds a vertex stream
    bool AddVertexStream(const CVertexStream& stream)
    {
        _vertexStreams.push_back(&stream);
        std::sort(_vertexStreams.begin(), _vertexStreams.end(), vertexStreamPtrComparator);
        return true;
    }
    
    /// Can be NULL
    const CIndexStream* GetIndexStream()const{ return _indexStream; };
    const VertexStreamArray& GetVertexStreams()const{ return _vertexStreams; };
    EPrimitiveType GetPrimitiveType()const{ return _primType; };
    
private:
    const CIndexStream* _indexStream;
    VertexStreamArray _vertexStreams; // sorted by attribute usage!
    EPrimitiveType  _primType;
};

/// Mesh representing renderable sets of vertices
class CMesh
{
    /// Runtime data used for rendering
    struct GLBuffer
    {
        GLBuffer():vertBuffer(0),indBuffer(0),vertArrayObj(0),numInds(0),primType(0),
        normalProg(0), zProg(0), materialProg(0), diffuseTex(0),normalSpecularTex(0){};
        
        unsigned    vertBuffer;
        unsigned    indBuffer;
        unsigned    indBufferType; // glEnum data type of elements
        unsigned    vertArrayObj;
        unsigned    numInds;
        unsigned    primType;
        
        //material
        CTexture*   diffuseTex;
        CTexture*   normalSpecularTex;
        CShaderProgram* normalProg;
        CShaderProgram* zProg;
        CShaderProgram* materialProg;
    };
    typedef std::vector<GLBuffer> GLBufferArray;
    
public:
    enum EDrawPass {
        DRAW_Z=0,
        DRAW_NORMAL,
        DRAW_MATERIAL
    };
    
    
    CMesh(const char* name = "unnamed");
    ~CMesh();
    static CMesh* FromFile(const char* path, bool generateNormals=true, bool calcTangentSpace=false)
    {
        CMesh* mesh = new CMesh();
        return mesh->LoadFromFile(path,generateNormals,calcTangentSpace)?mesh:NULL;
    }
    
    static const CMesh& FullscreenQuad();
    static const CMesh& UnitPlane();
    static const CMesh& UnitIcosphere();
    static const CMesh& UnitBox();
    static const CMesh& AxisLines();
    
    
    const char* GetName()const{ return _name.c_str(); };
    void SetName(const char* name){ _name = name; };
    const glm::vec3& GetPosition()const{ return _pos; };
    void SetPosition(const glm::vec3& pos){ _pos = pos; };
    void SetScale(const glm::vec3& scale){ _scale = scale; };
    
    /// Loads mesh data from a file in a supported file format
    bool LoadFromFile(const char* path, bool generateNormals=true, bool calcTangentSpace=false);
    /// Adds mesh part from the in-memory structure
    bool AddMeshPart(const CMeshPart& part);
    
    void Draw(EDrawPass pass = DRAW_MATERIAL)const;

private:
    void DrawNode(EDrawPass pass, const struct aiNode* nd)const;
    void DrawBufferArray(EDrawPass pass)const;
    void CalcStatsInNode(const struct aiNode* nd, unsigned& outVerts, unsigned& outInds);
    void CreateGLMeshesFromAssimp();
    std::string LocateTexture(const char* path)const;
    
    const struct aiScene* _scene;
    std::string _name;
    GLBufferArray _glbuff; // contains opengl objects
    unsigned _attrs; // EVertexAttrib
    glm::vec3 _pos;
    glm::vec3 _scale;
};

#endif /* defined(__glt__Mesh__) */
