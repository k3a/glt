//
//  Shaders.h
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef __glt__Shaders__
#define __glt__Shaders__

#include <string>
#include <map>
#include <vector>
#include "vec2.hpp"
#include "vec3.hpp"
#include "mat4x4.hpp"
#include "Types.h"
#include "Texture.h"

//////////////////////////////////////////////////////////////////////////////
class CShaderDefines
{
public:
    typedef std::map<std::string,std::string> ShaderDefinesMap;
    
    /// \param commaSeparatedValueAssignments Value doesn't need to be specified (exmaple "VS,ITERATIONS=1,BLUE")
    CShaderDefines(const char* commaSeparatedValueAssignments="");
    
    std::string GetCode();
    const char* ToString();
    
    CShaderDefines& Define(const char* name);
    CShaderDefines& Define(const char* name, const char* value);
    template <typename T> CShaderDefines& Define(const char* name, T value)
    {
        return Define(name, variable_to_string(value));
    }
    /// \param commaSeparatedValueAssignments Value doesn't need to be specified (exmaple "VS,ITERATIONS=1,BLUE")
    CShaderDefines& Undefine(const char* commaSeparatedValueAssignments);
    CShaderDefines& UndefineAll();
    
    ShaderDefinesMap GetDefines()const{ return _defines; };
    
    /// union
    CShaderDefines& operator+=(const CShaderDefines& b);
    /// intersection
    CShaderDefines& operator*=(const CShaderDefines& b);
    /// union
    CShaderDefines operator+(const CShaderDefines& b)const{ CShaderDefines r = *this; return r += b; };
    /// intersection
    CShaderDefines operator*(const CShaderDefines& b)const{ CShaderDefines r = *this; return r *= b; };
    
    /// comparison
    bool operator==(const CShaderDefines& b);
    
    SHType GetHash()const{ return _hash; };
    
private:
    void ComputeHash();
    
    ShaderDefinesMap _defines;
    SHType _hash;
};

//////////////////////////////////////////////////////////////////////////////
class CBaseShader
{
    friend class CShaderProgram;
    
public:
    enum Type
    {
        T_VERTEX=0,
        T_FRAGMENT
    };
    
    CBaseShader():_shader(0){};
    ~CBaseShader();
    
    bool IsValid()const{ return _shader != 0; };
    virtual Type GetType()const = 0;
    const char* GetName(){ return _name.c_str(); };
    
    /// Compiles the shader. Shader must be loaded first using LoadShaderFromFile.
    bool Compile(CShaderDefines* defines);
    /// Returns available defines in the shader (#ifdef lines). Known after successful LoadShaderFromFile call.
    const CShaderDefines& GetAvailableDefines()const{ return _availableDefines; };
    const CShaderDefines& GetCompiledDefines()const{ return _compiledDefines; };
    
protected:
    typedef std::vector<std::string> Lines;
    struct SIncludeInfo
    {
        std::string filename;
        unsigned numLines;
    };
    typedef std::vector<SIncludeInfo> IncludeInfoArray;
    
    /// Loads shader from the specified path and get list of defines which can be defined.
    bool LoadShaderFromFile(const char* path);
    
    Lines _lines;
    IncludeInfoArray _includeInfo;
    CShaderDefines _availableDefines;
    CShaderDefines _compiledDefines;
    unsigned int _shader;
    std::string _name;
    
private:
    char* ParsePreprocessorTokenValue(const char* cline, const char* tokenName);
    Lines LoadShaderSource(const char* path);
};

//////////////////////////////////////////////////////////////////////////////
class CVertexShader : public CBaseShader
{
public:
    static CVertexShader* FromFile(const char* path)
    {
        CVertexShader* s = new CVertexShader();
        if (s->LoadShaderFromFile(path))
            return s;
        
        delete s;
        return 0;
    };
    Type GetType()const{ return T_VERTEX; };
};

//////////////////////////////////////////////////////////////////////////////
class CFragmentShader : public CBaseShader
{
public:
    static CFragmentShader* FromFile(const char* path)
    {
        CFragmentShader* s = new CFragmentShader();
        if (s->LoadShaderFromFile(path))
            return s;
        
        delete s;
        return 0;
    };
    Type GetType()const{ return T_FRAGMENT; };
};

//////////////////////////////////////////////////////////////////////////////
class CShaderProgram
{
    typedef std::map<SHType, int> UniformMap;
    
public:
    CShaderProgram(const char* name="");
    CShaderProgram(CVertexShader* vs, CFragmentShader* fs);
    
    ~CShaderProgram();
    
    void Release(){}; //TODO: refcount
    
    CShaderProgram& SetShaders(CVertexShader* vs, CFragmentShader* fs);
    CShaderProgram& RemoveShaders();
    
    bool IsValid()const{ return _object && _linked; };
    bool Link();
    void Use()const;
    const char* GetName()const{ return _name.size()>0?_name.c_str():"<unnamed>"; };
    void SetName(const char* name){ _name = name; };
    const CShaderDefines& GetCompiledDefines()const{ return _defines; };
    
    int GetUniformLocation(SHArg name);
    bool SetUniform(SHArg uniform, float val);
    bool SetUniform(SHArg name, int val);
    bool SetUniform(SHArg name, const glm::vec2& v);
    bool SetUniform(SHArg name, const glm::vec3& v);
    bool SetUniform(SHArg name, unsigned count, const glm::vec3& v);
    bool SetUniform(SHArg name, const glm::vec4& v);
    bool SetUniform(SHArg name, const glm::mat4& mat);
    bool SetUniform(SHArg name, const CTexture& tex, unsigned textureUnit=0);
    
    static const CShaderProgram& None()
    {
        static CShaderProgram none;
        return none;
    }
    
private:
    void BindAttributes();
    
    std::string _name;
    CVertexShader* _vs;
    CFragmentShader* _fs;
    bool _linked;
    unsigned int _object;
    CShaderDefines _defines;
    
    UniformMap _uniforms;
    static const CShaderProgram* s_currentProgram;
};

class CShaderManager
{
    typedef std::map<SHType, CShaderProgram*> ProgMap;
    typedef std::map<SHType, CVertexShader*> VSMap;
    typedef std::map<SHType, CFragmentShader*> FSMap;
    
public:
    static CShaderManager* Inst()
    {
        static CShaderManager* inst = 0;
        if (!inst) inst = new CShaderManager();
        return inst;
    }
    
    CShaderProgram* GetProgram(SHArg name, CShaderDefines* defines=0, bool compileIfNotFound=true);
    CVertexShader* GetVShader(SHArg name, CShaderDefines* defines=0, bool compileIfNotFound=true);
    CFragmentShader* GetFShader(SHArg name, CShaderDefines* defines=0, bool compileIfNotFound=true);
    
    unsigned PurgeVSCache();
    unsigned PurgeFSCache();
    unsigned PurgeProgCache();
    unsigned PurgeShaderCaches(){ return PurgeFSCache() + PurgeVSCache(); }
    
private:
    ProgMap _progCache;
    VSMap _vsCache;
    FSMap _fsCache;
};

#endif /* defined(__glt__Shaders__) */






