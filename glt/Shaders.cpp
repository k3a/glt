//
//  Shaders.cpp
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#include "Shaders.h"
#include "glstuff.h"
#include "Shared.h"
#include "Types.h"
#include "Mesh.h"
#include "Engine.h"

#include <fstream>
#include <string>
#include <vector>
#include "type_ptr.hpp"

CShaderDefines::CShaderDefines(const char* commaSeparatedValueAssignments)
: _hash(0)
{
    Define(commaSeparatedValueAssignments);
}

CShaderDefines& CShaderDefines::operator+=(const CShaderDefines& b)
{
    const ShaderDefinesMap& bdefs = b.GetDefines();
    _defines.insert(bdefs.begin(), bdefs.end());
    ComputeHash();
    return *this;
}

CShaderDefines& CShaderDefines::operator*=(const CShaderDefines& b)
{
    const ShaderDefinesMap& bdefs = b.GetDefines();
    STD_FOREACH_NOINC(ShaderDefinesMap, _defines, it)
    {
        // skip "#define VAL 124" cases as we are interested in undefining 'defined' defines only when doing intersection
        if (it->second != "defined")
        {
            it++;
            continue;
        }
            
        // remove 'defined' defines from 'this' which are not present in the 'b'
        ShaderDefinesMap::const_iterator bit = bdefs.find(it->first);
        if (bit == bdefs.end())
            it = _defines.erase(it);
        else
            it++;
    }
    ComputeHash();
    return *this;
}

bool CShaderDefines::operator==(const CShaderDefines& b)
{
    ShaderDefinesMap bdefs = b.GetDefines();
    STD_FOREACH(ShaderDefinesMap, _defines, it)
    {
        // we are doing comparison on 'defined' defines
        if (it->second != "defined")
            continue;
        
        // one define not present in 'b' is enough to reject the equal test
        ShaderDefinesMap::iterator bit = bdefs.find(it->first);
        if (bit == bdefs.end())
            return false;
    }
    return true;
}


std::string CShaderDefines::GetCode()
{
    std::string str;
    
    STD_CONST_FOREACH(ShaderDefinesMap, _defines, d)
    {
        str += "#define " + d->first + " " + d->second + "\n";
    }
    
    return str;
}
const char* CShaderDefines::ToString()
{
    static std::string str;
    
    str = ""; bool fst = true;
    STD_CONST_FOREACH(ShaderDefinesMap, _defines, d)
    {
        if (d->second == "defined")
            str += (fst?"":",") + d->first;
        else
            str += (fst?"":",") + d->first + "=" + d->second;
        
        fst = false;
    }
    
    return str.c_str();
}

void CShaderDefines::ComputeHash()
{
    _hash = 0;
    STD_CONST_FOREACH(ShaderDefinesMap, _defines, it)
    {
        _hash += CStringHash::FromStackString(it->first.c_str()).GetHash();
        _hash += CStringHash::FromStackString(it->second.c_str()).GetHash();
    }
}

CShaderDefines& CShaderDefines::Define(const char *commaSeparatedValueAssignments)
{
    typedef std::vector<std::string> SegArray;
    SegArray segments;
    
    if (commaSeparatedValueAssignments && *commaSeparatedValueAssignments)
    {
        // separate string into segments by comma
        char* str = strdup(commaSeparatedValueAssignments);
        char* pch = strtok(str, ",");
        while (pch != NULL)
        {
            segments.push_back(pch);
            pch = strtok (NULL, ",");
        }
        free(str);
        
        // apply segments
        STD_CONST_FOREACH(SegArray, segments, s)
        {
            char* str = strdup(s->c_str());
            const char* name = strtok(str, " =");
            const char* value = strtok (NULL, " =");
            if (!value) value = "defined";
            Define(name, value);
            free(str);
        }
    }
    return *this;
}
CShaderDefines& CShaderDefines::Define(const char* name, const char* value)
{
    _defines.insert(std::pair<std::string, std::string>(name, value));
    ComputeHash();
    return *this;
}
CShaderDefines& CShaderDefines::Undefine(const char* name)
{
    _defines.erase(name);
    if (_defines.size() == 0)
        _hash = 0;
    else
        ComputeHash();
    return *this;
}
CShaderDefines& CShaderDefines::UndefineAll()
{
    _defines.clear();
    _hash = 0;
    return *this;
}


////////////////////////////////////////////////////////////////////////////////////////////////

char* CBaseShader::ParsePreprocessorTokenValue(const char* cline, const char* tokenName)
{
    // empty or not preprocessor line
    if (!cline || *cline != '#')
        return 0;
    
    // skip '#' and whitespaces
    while (*cline == '#' || *cline == ' ' || *cline == '\t')
        cline++;
        
    // does the line start with the token name?
    unsigned tokenNameLen = (unsigned)strlen(tokenName);
    if (!strncmp(cline, tokenName, tokenNameLen))
    {
        // remove leading characters before the token value
        char* start = const_cast<char*>(&cline[tokenNameLen]);
        while (*start == '"' || *start == ' ' || *start == '\t') start++;
        char* value = strdup(start);
        
        // remove ending characters after filename
        start = value;
        do
        {
            if (*start == '"' || *start == '\t' || *start == '\n' || *start == '\r' || *start == ' ' || *start == '/')
            {
                *start = 0;
                break;
            }
        }while(*(start++));
        
        return value;
    }
    
    return 0;
}

CBaseShader::Lines CBaseShader::LoadShaderSource(const char* path)
{
    std::ifstream file;
    
    file.open(path, std::ios::in);
    if(!file)
        return Lines();
    
    Lines lines;
    
    // load lines from file
    std::string line;
    while(std::getline(file, line))
        lines.push_back(line);
    
    // this will make the shader in "path" to start on line 0 as when error happens,
    // we will more likely be editing this shader than some included file
    //if (lines.size()) lines.insert(lines.begin(), "#line 0");
    
    file.close();
    
    // find and include all includes
    STD_FOREACH_NOINC(Lines, lines, lit)
    {
        std::string& line = *lit;
        
        char* fname = ParsePreprocessorTokenValue(line.c_str(), "include");
        
        if (fname)
        {
            lit = lines.erase(lit);
            //printf("Loading incl '%s'\n", fname);
            Lines incllines = LoadShaderSource( LocateFile(fname));
            lit = lines.insert(lit, incllines.begin(), incllines.end());
            
            SIncludeInfo ii;
            ii.filename = std::string(fname);
            ii.numLines = (unsigned)incllines.size();
            _includeInfo.push_back(ii);
            
            free(fname);
        }
        else
            lit++;
    }
    
    return lines;
}

CBaseShader::~CBaseShader()
{
    if (_shader) glDeleteShader(_shader);
}

bool CBaseShader::LoadShaderFromFile(const char* path)
{
    if (_shader) glDeleteShader(_shader);
    _shader = 0;
    
    _name = basename(const_cast<char*>(path));
    
    _lines = LoadShaderSource(path);
    if (_lines.size() == 0)
    {
        printf("Failed to load %s!", path);
        return false;
    }
    
    // get defines
    // TODO: already pre-process #ifdef VS/FS to get list of available #ifdefs for VS or FS part only
    _availableDefines.UndefineAll();
    STD_FOREACH(Lines, _lines, lit)
    {
        std::string& line = *lit;
        
        char* def = ParsePreprocessorTokenValue(line.c_str(), "ifdef");
        if (def)
        {
            _availableDefines.Define(def);
            free(def);
        }
    }
    
    return true;
}

#ifdef WIN32
void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}
#endif

bool CBaseShader::Compile(CShaderDefines *defines)
{
    if (_lines.size() == 0)
    {
        printf("Shader source not loaded; can't compile");
        return false;
    }
    
    // create complete source
    _compiledDefines.UndefineAll();
    if (defines) _compiledDefines += *defines;
    
    std::string finalSource = _compiledDefines.GetCode();
    
    finalSource += std::string("#define ") + ((GetType()==T_FRAGMENT)?"FS":"VS") + " defined\n";
    if (CEngine::Inst()->GetRendererCapabilities().MRT) finalSource += "#define MRT defined\n";
    //finalSource += "#define NORMAL_ENCODE_SPHEREMAP defined\n";
    
    // compute extra lines
    unsigned extraLines=0; // number of extra lines added at the beginning of shader source
    STD_CONST_FOREACH(std::string, finalSource, it)
    {
        if (*it == '\n')
            extraLines++;
    }
    
    // append loaded shader lines (including includes)
    STD_FOREACH(Lines, _lines, line)
    {
        finalSource += *line + "\n";
    }
    
    //printf("\n--\n%s\n--\n", finalSource.c_str());
    glGetError();
    _shader = glCreateShader((GetType()==T_FRAGMENT)?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);
    PrintGLError("creating shader object");
    
    if (!_shader)
    {
        printf("%s: Error creating shader object!\n", GetName());
        return false;
    }

#ifdef WIN32
	ReplaceStringInPlace(finalSource, "defined", "");
#endif
        
    const char* sourcePtr = finalSource.c_str();
    glShaderSource(_shader, 1, &sourcePtr, 0);
    PrintGLError("setting shader source");
    
    glCompileShader(_shader);
    PrintGLError("compiling shader");
    
    GLint compiled;
    glGetShaderiv(_shader, GL_COMPILE_STATUS, &compiled);
    PrintGLError("getting compile status");
    
    if (compiled)
    {
        // lines and available defines no longer needed
        _lines.clear();
        _availableDefines.UndefineAll();
        
        if (defines) _compiledDefines = *defines;
        
        printf("%s(%s): Shader compiled. Defines <#%u>: %s\n", GetName(), (GetType()==T_FRAGMENT)?"frag":"vert", _compiledDefines.GetHash(), _compiledDefines.ToString());
        return true;
    }
    
    GLint infolog_len;
    glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &infolog_len);
    PrintGLError("getting shader log length");
    
    if (infolog_len > 0)
    {
        char* infolog = (char*)malloc(infolog_len);
        glGetShaderInfoLog(_shader, infolog_len, &infolog_len, infolog);
        
        // try to determine include file in which the problem happened
        char* desc = (char*)malloc(infolog_len+512);
        unsigned col=0, line=0;
        if (sscanf(infolog, "ERROR: %u:%u: %8192[^\r]", &col, &line, desc) == 3)
        {
            unsigned cur = 0;  const SIncludeInfo* info = NULL;
            STD_CONST_FOREACH(IncludeInfoArray, _includeInfo, it)
            {
                if (line >= cur && line < cur + it->numLines)
                {
                    info = &(*it);
                    break;
                }
                cur += it->numLines;
            }
            if (info)
                sprintf(desc, "ERROR: %s %u:%u: %s", info->filename.c_str(), col, line-cur-extraLines-1, desc); // included file
            else
                sprintf(desc, "ERROR: %u:%u: %s", col, line-cur-extraLines-1, desc); // main file
        }
        free(infolog);
        
        printf("%s(%s): %s\n", _name.c_str(), (GetType()==T_FRAGMENT)?"frag":"vert", desc);
        free(desc);
        
        PrintGLError("getting shader log");
    }
    else
        printf("%s(%s): Unknown error\n", _name.c_str(), (GetType()==T_FRAGMENT)?"frag":"vert");
    
    glDeleteShader(_shader);
    PrintGLError("deleting shader object");
    _shader = 0;
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////

const CShaderProgram* CShaderProgram::s_currentProgram = NULL;

CShaderProgram::CShaderProgram(const char* name)
: _linked(false), _object(0), _name(name)
{
    
}

CShaderProgram::CShaderProgram(CVertexShader* vs, CFragmentShader* fs)
: _linked(false), _vs(vs), _fs(fs), _object(0)
{
}

CShaderProgram::~CShaderProgram()
{
    if (_object)
    {
        glDeleteProgram(_object);
        PrintGLError("deleting shader object (2)");
    }
}

CShaderProgram& CShaderProgram::SetShaders(CVertexShader* vs, CFragmentShader* fs)
{
    _vs = vs;
    _fs = fs;
    
    if (_vs && _fs)
        _defines = _vs->GetCompiledDefines() + _fs->GetCompiledDefines();
    else
        _defines.UndefineAll();
    return *this;
};
CShaderProgram& CShaderProgram::RemoveShaders()
{
    SetShaders(0, 0);
    return *this;
};

void CShaderProgram::BindAttributes()
{
	glBindAttribLocation(_object,	Attrib2Index(ATTRIB_POSITION),	"aPosition");
    PrintGLError("binding vertex attribute location");
	glBindAttribLocation(_object,	Attrib2Index(ATTRIB_COLOR0),	"aColor0");
    PrintGLError("binding vertex attribute location");
	glBindAttribLocation(_object,	Attrib2Index(ATTRIB_COORDS0),	"aCoords0");
    PrintGLError("binding vertex attribute location");
	glBindAttribLocation(_object,	Attrib2Index(ATTRIB_NORMAL),	"aNormal");
    PrintGLError("binding vertex attribute location");
	glBindAttribLocation(_object,	Attrib2Index(ATTRIB_TANGENT),	"aTangent");
    PrintGLError("binding vertex attribute location");
    glBindAttribLocation(_object,	Attrib2Index(ATTRIB_BITANGENT),	"aBitangent");
    PrintGLError("binding vertex attribute location");
	glBindAttribLocation(_object,	Attrib2Index(ATTRIB_PROJVEC),	"aProjVec");
    PrintGLError("binding vertex attribute location");
}

bool CShaderProgram::Link()
{
    glGetError();
    
    if (!_vs || !_vs->IsValid())
    {
        const char* name = 0;
        if (_vs) name = _vs->GetName();
        printf("%s: Trying to link invalid VS %s!\n", GetName(), name);
        return false;
    }
    else if (!_fs || !_fs->IsValid())
    {
        const char* name = 0;
        if (_fs) name = _fs->GetName();
        printf("%s: Trying to link invalid FS %s!\n", GetName(), name);
        return false;
    }
    
    if (_object) glDeleteProgram(_object);
    _object = glCreateProgram();
    PrintGLError("creating program object");
    if (!_object) return false;
    
    glAttachShader(_object, _vs->_shader);
    PrintGLError("attaching VS");
    glAttachShader(_object, _fs->_shader);
    PrintGLError("attaching FS");
    
    // bind attribute locations
	// this needs to be done prior to linking
    BindAttributes();
    
    glLinkProgram(_object);
    PrintGLError("linking program object");
    
    int linked = 0;
    glGetProgramiv(_object, GL_LINK_STATUS, &linked);
    PrintGLError("getting program object status");
    
    _linked = linked != 0;
    
    if (!_linked)
    {
        GLint infolog_len;
        glGetProgramiv(_object, GL_INFO_LOG_LENGTH, &infolog_len);
        PrintGLError("getting program log length");
        
        if (infolog_len > 0)
        {
            char* infolog = (char*)malloc(infolog_len);
            GLint nb_chars;
            glGetProgramInfoLog(_object, infolog_len, &nb_chars, infolog);
            PrintGLError("getting shader log");
            printf("%s: Error linking %s and %s shaders: %s\n", GetName(), _vs->GetName(), _fs->GetName(), infolog);
            free(infolog);
        }
        else
            printf("%s: Error linking %s and %s shaders!\n", GetName(), _vs->GetName(), _fs->GetName());
        
        glDeleteProgram(_object);
        _object = 0;
        return false;
    }

    GLint success=0;
    glValidateProgram(_object);
    PrintGLError("validationg program object");
    glGetProgramiv(_object, GL_VALIDATE_STATUS, &success);
    PrintGLError("getting program object validation status");
    if (!success)
    {
        printf("%s: Validation failed for %s and %s shaders!\n", GetName(), _vs->GetName(), _fs->GetName());
        _linked = false;
        glDeleteProgram(_object);
        _object = 0;
        return false;
    }

    // print information
    int attr=0, unif=0;
    glGetProgramiv(_object, GL_ACTIVE_ATTRIBUTES, &attr);
    glGetProgramiv(_object, GL_ACTIVE_UNIFORMS, &unif);
    printf("%s: Shader program linked; Attributes: %d, Uniforms: %d, Defines <#%u>: %s\n", GetName(), attr, unif, _defines.GetHash(), _defines.ToString());
    
    // get uniforms
    int total = -1;
    glGetProgramiv( _object, GL_ACTIVE_UNIFORMS, &total );
    for(GLuint i=0; i<total; ++i)
    {
        int name_len = 0, num = 0; char name[100]; GLenum type = GL_ZERO;
        
        glGetActiveUniform( _object, i, sizeof(name)-1, &name_len, &num, &type, name );
        name[name_len] = 0;
        
        // array type?
        if (name_len > 3 && !strcmp(&name[name_len-3], "[0]"))
            name[name_len-3] = 0;
        
        GLuint location = glGetUniformLocation(_object, name);
        SHArg hash = CStringHash::FromStackString(name);
        printf(" : %s/%u <#%u> : %u\n", name, num, hash.GetHash(), location);

        _uniforms.insert(std::pair<SHType, int>(hash, location));
    }
    
    return _linked;
}

void CShaderProgram::Use()const
{
    if (s_currentProgram == this)
        return;
    
    if (!IsValid())
    {
        glUseProgram(0);
        s_currentProgram = this;
        return;
    }
    
    glGetError();
    
    glUseProgram(_object);
    s_currentProgram = this;
    PrintGLError("using program object");
}


int CShaderProgram::GetUniformLocation(SHArg name)
{
    UniformMap::iterator it = _uniforms.find(name);
    if (it != _uniforms.end())
        return it->second;

    _uniforms.insert(std::pair<SHType, int>(name, -1));
    
    return -1;
}

bool CShaderProgram::SetUniform(SHArg name, const glm::mat4& mat)
{
    Use();
    int loc = GetUniformLocation(name);
    if (loc < 0) return false;
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
    return true;
}

bool CShaderProgram::SetUniform(SHArg name, int val)
{
    Use();
    int loc = GetUniformLocation(name);
    if (loc < 0) return false;
	glUniform1i(loc, val);
    return true;
}
bool CShaderProgram::SetUniform(SHArg name, float val)
{
    Use();
    int loc = GetUniformLocation(name);
    if (loc < 0) return false;
    glUniform1f(loc, val);
    return true;
}
bool CShaderProgram::SetUniform(SHArg name, const glm::vec2& v)
{
    Use();
    int loc = GetUniformLocation(name);
    if (loc < 0) return false;
	glUniform2fv(loc, 1, glm::value_ptr(v));
    return true;
}
bool CShaderProgram::SetUniform(SHArg name, unsigned count, const glm::vec3& v)
{
    Use();
    int loc = GetUniformLocation(name);
    if (loc < 0) return false;
	glUniform3fv(loc, count, glm::value_ptr(v));
    return true;
}
bool CShaderProgram::SetUniform(SHArg name, const glm::vec3& v)
{
    return SetUniform(name, 1, v);
}
bool CShaderProgram::SetUniform(SHArg name, const glm::vec4& v)
{
    Use();
    int loc = GetUniformLocation(name);
    if (loc < 0) return false;
	glUniform4fv(loc, 1, glm::value_ptr(v));
    return true;
}
bool CShaderProgram::SetUniform(SHArg name, const CTexture& tex, unsigned textureUnit)
{
    Use();
    tex.Use(textureUnit);
    int loc = GetUniformLocation(name);
    if (loc < 0) return false;
	glUniform1i(loc, textureUnit); // we are setting texture unit number to the uniform actually (not a texture ID)!
    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////

CShaderProgram* CShaderManager::GetProgram(SHArg name, CShaderDefines* defines, bool compileIfNotFound)
{
    SHType hash = name.GetHash();
    if (defines) hash += defines->GetHash();
    
    ProgMap::iterator it = _progCache.find(hash);
    if (it != _progCache.end())
        return it->second;
    
    if (compileIfNotFound)
    {
        CShaderProgram* prog = new CShaderProgram(name.GetString());
        
        CVertexShader* vs = GetVShader(name, defines, compileIfNotFound);
        CFragmentShader* fs = GetFShader(name, defines, compileIfNotFound);
        
        prog->SetShaders(vs, fs);
        prog->Link();
        prog->RemoveShaders();
        
        _progCache.insert(std::pair<SHType, CShaderProgram*>(hash, prog));
        return prog;
    }
        
    return 0;
}

CVertexShader* CShaderManager::GetVShader(SHArg name, CShaderDefines* defines, bool compileIfNotFound)
{
    // get complete hash
    SHType hash = name.GetHash();
    if (defines) hash += defines->GetHash();
    
    // try to find exact match
    VSMap::iterator it = _vsCache.find(hash);
    if (it != _vsCache.end())
        return it->second;
    
    CVertexShader* sh = NULL;
    // try to find any permutation having the specified defines
    /*STD_CONST_FOREACH(VSMap, _vsCache, it)
    {
        CVertexShader* s = it->second;
        if (!s) continue;
        
        // not the name we are looking for
        if ( strcmp(name.ToString(), s->GetName()) )
            continue;
        
        // when no defines needed, use the first shader with the exact name
        // or check that intersection of shader defines with requested defines is requeted defines
        if (!defines || (s->GetCompiledDefines() * *defines == *defines))
        {
            sh = s;
            break;
        }
    }*/
    
    if (!sh && compileIfNotFound)
    {
        sh = CVertexShader::FromFile(LocateFile(name.GetString()));
        if (sh)
        {
            CShaderDefines finalDefines;
            if (defines)
            {
                finalDefines = sh->GetAvailableDefines();
                finalDefines *= *defines;
            }
            
            sh->Compile(&finalDefines);
            
            hash = name.GetHash() + finalDefines.GetHash();
        }
    }
    
    _vsCache.insert(std::pair<SHType, CVertexShader*>(hash, sh));
    return sh;
}
CFragmentShader* CShaderManager::GetFShader(SHArg name, CShaderDefines* defines, bool compileIfNotFound)
{
    // get complete hash
    SHType hash = name.GetHash();
    if (defines) hash += defines->GetHash();
    
    // try to find exact match
    FSMap::iterator it = _fsCache.find(hash);
    if (it != _fsCache.end())
        return it->second;
    
    // try to find any permutation having the specified defines
    CFragmentShader* sh = NULL;
    /*STD_CONST_FOREACH(FSMap, _fsCache, it)
    {
        CFragmentShader* s = it->second;
        if (!s) continue;
        
        // not the name we are looking for
        if ( strcmp(name.ToString(), s->GetName()) )
            continue;
        
        // when no defines needed, use the first shader with the exact name
        // or check that intersection of shader defines with requested defines is requeted defines
        if (!defines || (s->GetCompiledDefines() * *defines == *defines))
        {
            sh = s;
            break;
        }
    }*/
    
    if (!sh && compileIfNotFound)
    {
        sh = CFragmentShader::FromFile(LocateFile(name.GetString()));
        if (sh)
        {
            CShaderDefines finalDefines;
            if (defines)
            {
                finalDefines = sh->GetAvailableDefines();
                finalDefines *= *defines;
            }
                
            sh->Compile(&finalDefines);
            
            hash = name.GetHash() + finalDefines.GetHash();
        }
    }
    
    _fsCache.insert(std::pair<SHType, CFragmentShader*>(hash, sh));
    return sh;
}

unsigned CShaderManager::PurgeVSCache()
{
    STD_FOREACH(VSMap, _vsCache, it)
    {
        delete it->second;
    }
    unsigned r = (unsigned)_vsCache.size();
    _vsCache.clear();
    
    return r;
}
unsigned CShaderManager::PurgeFSCache()
{
    STD_FOREACH(FSMap, _fsCache, it)
    {
        delete it->second;
    }
    unsigned r = (unsigned)_fsCache.size();
    _fsCache.clear();
    
    return r;
}
unsigned CShaderManager::PurgeProgCache()
{
    STD_FOREACH(ProgMap, _progCache, it)
    {
        delete it->second;
    }
    unsigned r = (unsigned)_progCache.size();
    _progCache.clear();
    
    return r;
}








