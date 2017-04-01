//
//  CVar.h
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef __glt__CVar__
#define __glt__CVar__

#include <string>
#include <set>

#include "Types.h"
#include "Shared.h"

class CVar;

class ICVarListener
{
    friend class CVar;
    typedef std::set<CVar*> RefSet;
    
private:
    void AddRef(CVar* cv);
    
protected:
    virtual ~ICVarListener();
    /// Return true to signal handling this cvar change
    virtual bool CVarChanged(CVar* cv){ return false;};
    /// Return true to signal handling this cvar command
    /// \param outResult Result status of the call (implicitly true - success)
    virtual bool CVarCalled(CVar* cv, unsigned argc, const char* argv[], bool& outResult){ return false; };
    
private:
    RefSet _refs;
};

/// Universal console or UI editable variable
/// Setting/changing is slow, getting is very fast
class CVar
{
    typedef std::set<ICVarListener*> ListenerSet;

    void Init();

public:
    enum {
        FLAG_NONE=0,
        FLAG_GUI_TWEAKABLE = 1,
        FLAG_GUI_PRINT = 2,
        FLAG_COMMAND = 4
    };
    
    CVar(const char* name)
    : _name(name), _type(T_UNKNOWN), _minVal(0), _maxVal(1), _flags(FLAG_COMMAND)
    { Init(); Set(0); };
    
    template <typename T>
    CVar(const char* name, const T& defaultVal, int flags = FLAG_NONE, float minVal = 0, float maxVal = 1)
    : _name(name), _type(variable_type<T>::type), _minVal(minVal), _maxVal(maxVal), _flags(flags)
    { Init(); Set(defaultVal); };
    
    static CVar* FirstCVar(){ return s_first; };
    CVar* NextCVar(){ return _next; };
    const char* GetName()const{ return _name; };
    EType GetType()const{ return _type; };
    float GetMinVal()const{ return _minVal; };
    float GetMaxVal()const{ return _maxVal; };
    unsigned char GetFlags()const{ return _flags; };
    
    template <typename T>
    void Set(T v)
    {
        char buf[64];
        const char* newStringVal = variable_to_string(buf, v);
        bool changed = !strcmp(newStringVal, _stringVal.c_str());
        
        _stringVal = newStringVal;
        _floatVal = variable_to_float(v);
        _boolVal = variable_to_bool(v);
        _intVal = (int)_floatVal;
        
        if (changed)
        {
            STD_CONST_FOREACH(ListenerSet, s_listeners, l)
            {
                if ((*l)->CVarChanged(this))
                    break;
            }
        }
    }
    
    /// Call this cvar (it can either be a variable assignment "x_var value" or command call "doSomehing")
    /// \param outResult Success result (ie when command is handled but failed, Call returns true but outResult is set to false). Can be NULL.
    /// \return Returns true if the call has been handled
    bool Call(unsigned argc, const char* argv[], bool* outResult = NULL)
    {
        // act as a variable
        if ( !(_flags & FLAG_COMMAND) )
        {
            // not enough args -> print
            if (argc == 0)
            {
                printf("%s = %s\n", _name, _stringVal.c_str());
                return true;
            }
            
            Set(argv[0]);
            if (outResult) *outResult = true;
            
            return true;
        }
        
        // act as a command
        STD_CONST_FOREACH(ListenerSet, s_listeners, l)
        {
            bool tmpRet = true;
            if ((*l)->CVarCalled(this, argc, argv, tmpRet))
            {
                if (outResult) *outResult = tmpRet;
                return true;
            }
        }
        return false;
    }
    
    const char* GetString()const{  return _stringVal.c_str(); };
    int GetInt()const{ return _intVal; };
    float GetFloat()const{ return _floatVal; };
    bool GetBool()const{ return _boolVal; };
    
    operator bool ()const{ return GetBool(); };
    operator int ()const{ return GetInt(); };
    operator float ()const{ return GetFloat(); };
    operator const char* ()const{ return GetString(); };
    
    /// Adds a listener to the CVar. Once the listener is released, it is automatically removed from the cvar
    static void AddListener(ICVarListener* list) { s_listeners.insert(list); }
    /// Removes a listener from the CVar. No need to do it in listener class's destructors manually
    static void RemoveListener(ICVarListener* list){ s_listeners.erase(list); }
    
private:
    EType _type;
    const char* _name;
    
    std::string _stringVal;
    int _intVal;
    float _floatVal;
    bool _boolVal;
    
    unsigned char _flags;
    float _minVal;
    float _maxVal;

    CVar* _next;
    static CVar* s_first;
    static ListenerSet s_listeners;
};

#endif /* defined(__glt__CVar__) */
