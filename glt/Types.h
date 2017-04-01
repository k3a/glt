//
//  types.h
//  glt
//
//  Created by Mario Hros on 1. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef glt_types_h
#define glt_types_h

#include <string.h>
#include <stdlib.h>

enum EType
{
    T_UNKNOWN=0,
    T_BOOL,
    T_BYTE, // 1B
    T_UNSIGNED_BYTE,
    T_SHORT, // 2B
    T_UNSIGNED_SHORT,
    T_INT, // 3B
    T_UNSIGNED_INT,
    T_FLOAT, // sizeof(float)
    T_STRING // const char* or std::string
};

inline unsigned GetTypeSize(EType t)
{
    switch(t)
    {
        case T_BOOL: return sizeof(bool);
        case T_BYTE: return 1;
        case T_UNSIGNED_BYTE: return 1;
        case T_SHORT: return sizeof(short);
        case T_UNSIGNED_SHORT: return sizeof(unsigned short);
        case T_INT: return sizeof(int);
        case T_UNSIGNED_INT: return sizeof(unsigned int);
        case T_FLOAT: return sizeof(float);
        default: return 0;
    }
}

template< typename T >
struct variable_type{
    static const EType type = T_UNKNOWN;
};

template<> struct variable_type< bool >{
    static const EType type = T_BOOL;
};

template<> struct variable_type< int >{
    static const EType type = T_INT;
};

template<> struct variable_type< float >{
    static const EType type = T_FLOAT;
};

template<> struct variable_type< char* >{
    static const EType type = T_STRING;
};

template<> struct variable_type< const char* >{
    static const EType type = T_STRING;
};



template< int N >
struct type_from_enum{
    static int type;
};

template<> struct type_from_enum<T_BOOL> {
    static bool type;
};
template<> struct type_from_enum<T_INT> {
    static int type;
};
template<> struct type_from_enum<T_FLOAT> {
    static float type;
};
template<> struct type_from_enum<T_STRING> {
    static const char* type;
};


template< typename T >
inline const char* variable_to_string(char* buff, T v)
{
    return v.ToString();
}

template<> inline const char* variable_to_string<>(char* buff, bool v)
{
    return v?"true":"false";
};

template<> inline const char* variable_to_string<>(char* buff, int v)
{
    sprintf(buff, "%d", v);
    return buff;
};
template<> inline const char* variable_to_string<>(char* buff, float v)
{
    sprintf(buff, "%f", v);
    return buff;
};
template<> inline const char* variable_to_string<>(char* buff, double v)
{
    sprintf(buff, "%f", v);
    return buff;
};
template<> inline const char* variable_to_string<>(char* buff, const char* v)
{
    return v;
};





template< typename T >
inline T string_to_object(const char* str)
{
    return T::FromString(str);
}
inline bool string_to_bool(const char* str)
{
    return *str != '0' && strcmp(str, "false");
};
inline int string_to_int(const char* str)
{
    return atoi(str);
};
inline float string_to_float(const char* str)
{
    return atof(str);
};
inline const char* string_to_string(const char* str)
{
    return str;
};




template< typename T >
inline float variable_to_float(T v)
{
    char buf[32];
    const char* str = variable_to_string(buf, v);
    return string_to_float(str);
}
template<> inline float variable_to_float<>(float v){ return v; };
template<> inline float variable_to_float<>(double v){ return v; };



template< typename T >
inline bool variable_to_bool(T v)
{
    char buf[32];
    const char* str = variable_to_string(buf, v);
    return string_to_bool(str);
}
template<> inline bool variable_to_bool<>(int v){ return v != 0; };
template<> inline bool variable_to_bool<>(float v){ return v != 0; };
template<> inline bool variable_to_bool<>(bool v){ return v; };




struct SUniversalTypePointer
{
    template <typename T>
    SUniversalTypePointer(T* ptr): pointer(ptr), type(variable_type<T>::type) {}
    
    void* pointer;
    EType type;
};






// StringHash; based on: http://www.altdevblogaday.com/2011/10/27/quasi-compile-time-string-hashing/

// Type of hash value used in CStringHash
typedef unsigned int SHType;

#define _SH_REPEAT_0(op, data)
#define _SH_REPEAT_1(op, data)
#define _SH_REPEAT_2(op, data)				op(0, data)
#define _SH_REPEAT_3(op, data)				_SH_REPEAT_2(op, data) op(1, data)
#define _SH_REPEAT_4(op, data)				_SH_REPEAT_3(op, data) op(2, data)
#define _SH_REPEAT_5(op, data)				_SH_REPEAT_4(op, data) op(3, data)
#define _SH_REPEAT_6(op, data)				_SH_REPEAT_5(op, data) op(4, data)
#define _SH_REPEAT_7(op, data)				_SH_REPEAT_6(op, data) op(5, data)
#define _SH_REPEAT_8(op, data)				_SH_REPEAT_7(op, data) op(6, data)
#define _SH_REPEAT_9(op, data)				_SH_REPEAT_8(op, data) op(7, data)
#define _SH_REPEAT_10(op, data)				_SH_REPEAT_9(op, data) op(8, data)
#define _SH_REPEAT_11(op, data)				_SH_REPEAT_10(op, data) op(9, data)
#define _SH_REPEAT_12(op, data)				_SH_REPEAT_11(op, data) op(10, data)
#define _SH_REPEAT_13(op, data)				_SH_REPEAT_12(op, data) op(11, data)
#define _SH_REPEAT_14(op, data)				_SH_REPEAT_13(op, data) op(12, data)
#define _SH_REPEAT_15(op, data)				_SH_REPEAT_14(op, data) op(13, data)
#define _SH_REPEAT_16(op, data)				_SH_REPEAT_15(op, data) op(14, data)
#define _SH_REPEAT_17(op, data)				_SH_REPEAT_16(op, data) op(15, data)
#define _SH_REPEAT_18(op, data)				_SH_REPEAT_17(op, data) op(16, data)
#define _SH_REPEAT_19(op, data)				_SH_REPEAT_18(op, data) op(17, data)
#define _SH_REPEAT_20(op, data)				_SH_REPEAT_19(op, data) op(18, data)
#define _SH_REPEAT_21(op, data)				_SH_REPEAT_20(op, data) op(19, data)
#define _SH_REPEAT_22(op, data)				_SH_REPEAT_21(op, data) op(20, data)
#define _SH_REPEAT_23(op, data)				_SH_REPEAT_22(op, data) op(21, data)
#define _SH_REPEAT_24(op, data)				_SH_REPEAT_23(op, data) op(22, data)
#define _SH_REPEAT_25(op, data)				_SH_REPEAT_24(op, data) op(23, data)
#define _SH_REPEAT_26(op, data)				_SH_REPEAT_25(op, data) op(24, data)
#define _SH_REPEAT_27(op, data)				_SH_REPEAT_26(op, data) op(25, data)
#define _SH_REPEAT_28(op, data)				_SH_REPEAT_27(op, data) op(26, data)
#define _SH_REPEAT_29(op, data)				_SH_REPEAT_28(op, data) op(27, data)
#define _SH_REPEAT_30(op, data)				_SH_REPEAT_29(op, data) op(28, data)
#define _SH_REPEAT_31(op, data)				_SH_REPEAT_30(op, data) op(29, data)
#define _SH_REPEAT_32(op, data)				_SH_REPEAT_31(op, data) op(30, data)
#define _SH_REPEAT_33(op, data)				_SH_REPEAT_32(op, data) op(31, data)
#define _SH_REPEAT_34(op, data)				_SH_REPEAT_33(op, data) op(32, data)
#define _SH_REPEAT_35(op, data)				_SH_REPEAT_34(op, data) op(33, data)
#define _SH_REPEAT_36(op, data)				_SH_REPEAT_35(op, data) op(34, data)
#define _SH_REPEAT_37(op, data)				_SH_REPEAT_36(op, data) op(35, data)
#define _SH_REPEAT_38(op, data)				_SH_REPEAT_37(op, data) op(36, data)
#define _SH_REPEAT_39(op, data)				_SH_REPEAT_38(op, data) op(37, data)
#define _SH_REPEAT_40(op, data)				_SH_REPEAT_39(op, data) op(38, data)
#define _SH_REPEAT_41(op, data)				_SH_REPEAT_40(op, data) op(39, data)
#define _SH_REPEAT_42(op, data)				_SH_REPEAT_41(op, data) op(40, data)
#define _SH_REPEAT_43(op, data)				_SH_REPEAT_42(op, data) op(41, data)
#define _SH_REPEAT_44(op, data)				_SH_REPEAT_43(op, data) op(42, data)
#define _SH_REPEAT_45(op, data)				_SH_REPEAT_44(op, data) op(43, data)
#define _SH_REPEAT_46(op, data)				_SH_REPEAT_45(op, data) op(44, data)
#define _SH_REPEAT_47(op, data)				_SH_REPEAT_46(op, data) op(45, data)
#define _SH_REPEAT_48(op, data)				_SH_REPEAT_47(op, data) op(46, data)
#define _SH_REPEAT_49(op, data)				_SH_REPEAT_48(op, data) op(47, data)
#define _SH_REPEAT_50(op, data)				_SH_REPEAT_49(op, data) op(48, data)
#define _SH_REPEAT_51(op, data)				_SH_REPEAT_50(op, data) op(49, data)
#define _SH_REPEAT_52(op, data)				_SH_REPEAT_51(op, data) op(50, data)
#define _SH_REPEAT_53(op, data)				_SH_REPEAT_52(op, data) op(51, data)
#define _SH_REPEAT_54(op, data)				_SH_REPEAT_53(op, data) op(52, data)
#define _SH_REPEAT_55(op, data)				_SH_REPEAT_54(op, data) op(53, data)
#define _SH_REPEAT_56(op, data)				_SH_REPEAT_55(op, data) op(54, data)
#define _SH_REPEAT_57(op, data)				_SH_REPEAT_56(op, data) op(55, data)
#define _SH_REPEAT_58(op, data)				_SH_REPEAT_57(op, data) op(56, data)
#define _SH_REPEAT_59(op, data)				_SH_REPEAT_58(op, data) op(57, data)
#define _SH_REPEAT_60(op, data)				_SH_REPEAT_59(op, data) op(58, data)
#define _SH_REPEAT_61(op, data)				_SH_REPEAT_60(op, data) op(59, data)
#define _SH_REPEAT_62(op, data)				_SH_REPEAT_61(op, data) op(60, data)
#define _SH_REPEAT_63(op, data)				_SH_REPEAT_62(op, data) op(61, data)
#define _SH_JOIN(a, b) a##b
#define _SH_REPEAT(count, op, data)			_SH_JOIN(_SH_REPEAT_, count)(op, data)

#define _SH_PREFIX(n, data)		((
#define _SH_POSTFIX(n, data)	^ str[n]) * 16777619u)

#define _SH_CONSTRUCTOR(n)                                                      \
inline CStringHash(const char (&str)[n])                                  \
: _string(str), _hash(_SH_REPEAT(n, _SH_PREFIX, ~) 2166136261u _SH_REPEAT(n, _SH_POSTFIX, ~))	{};


class CStringHash
{
public:
    CStringHash(SHType hash):_hash(hash),_string(0){};
    CStringHash(const CStringHash& h):_hash(h._hash),_string(0){}; // not safe to copy string ptr
    
    _SH_CONSTRUCTOR(1)
    _SH_CONSTRUCTOR(2)
    _SH_CONSTRUCTOR(3)
    _SH_CONSTRUCTOR(4)
    _SH_CONSTRUCTOR(5)
    _SH_CONSTRUCTOR(6)
    _SH_CONSTRUCTOR(7)
    _SH_CONSTRUCTOR(8)
    _SH_CONSTRUCTOR(9)
    _SH_CONSTRUCTOR(10)
    _SH_CONSTRUCTOR(11)
    _SH_CONSTRUCTOR(12)
    _SH_CONSTRUCTOR(13)
    _SH_CONSTRUCTOR(14)
    _SH_CONSTRUCTOR(15)
    _SH_CONSTRUCTOR(16)
    _SH_CONSTRUCTOR(17)
    _SH_CONSTRUCTOR(18)
    _SH_CONSTRUCTOR(19)
    _SH_CONSTRUCTOR(20)
    _SH_CONSTRUCTOR(21)
    _SH_CONSTRUCTOR(22)
    _SH_CONSTRUCTOR(23)
    _SH_CONSTRUCTOR(24)
    _SH_CONSTRUCTOR(25)
    _SH_CONSTRUCTOR(26)
    _SH_CONSTRUCTOR(27)
    _SH_CONSTRUCTOR(28)
    _SH_CONSTRUCTOR(29)
    _SH_CONSTRUCTOR(30)
    _SH_CONSTRUCTOR(31)
    _SH_CONSTRUCTOR(32)
    _SH_CONSTRUCTOR(33)
    _SH_CONSTRUCTOR(34)
    _SH_CONSTRUCTOR(35)
    _SH_CONSTRUCTOR(36)
    _SH_CONSTRUCTOR(37)
    _SH_CONSTRUCTOR(38)
    _SH_CONSTRUCTOR(39)
    _SH_CONSTRUCTOR(40)
    _SH_CONSTRUCTOR(41)
    _SH_CONSTRUCTOR(42)
    _SH_CONSTRUCTOR(43)
    _SH_CONSTRUCTOR(44)
    _SH_CONSTRUCTOR(45)
    _SH_CONSTRUCTOR(46)
    _SH_CONSTRUCTOR(47)
    _SH_CONSTRUCTOR(48)
    _SH_CONSTRUCTOR(49)
    _SH_CONSTRUCTOR(50)
    _SH_CONSTRUCTOR(51)
    _SH_CONSTRUCTOR(52)
    _SH_CONSTRUCTOR(53)
    _SH_CONSTRUCTOR(54)
    _SH_CONSTRUCTOR(55)
    _SH_CONSTRUCTOR(56)
    _SH_CONSTRUCTOR(57)
    _SH_CONSTRUCTOR(58)
    _SH_CONSTRUCTOR(59)
    _SH_CONSTRUCTOR(60)
    _SH_CONSTRUCTOR(61)
    _SH_CONSTRUCTOR(62)
    _SH_CONSTRUCTOR(63)
    
    /// Use this method to get CStringHash for strings in memory with unknown life span
    /// (string ptr won't be stored = resulting CStringHash should not be used for file loading method arguments)
    inline static CStringHash FromDynamicString(const char* str)
    {
        unsigned int h = 2166136261u;
        while(*str)
        {
            h ^= *(str++);
            h *= 16777619u;
        }
        return CStringHash(h);
    }
    
    /// Use this method to get CStringHash for strings which will remain in memory for the whole life of this CStringHash
    inline static CStringHash FromStackString(const char* str)
    {
        CStringHash h = FromDynamicString(str);
        h._string = str;
        return h;
    }
    
    inline operator const SHType () const { return _hash; }
    /// Returns hash value
    inline const SHType GetHash()const{ return _hash; };
    /// Returns static string if known or NULL if not
    inline const char* GetString()const{ return _string?_string:"<!HASH_WITHOUT_STRING!>"; };
    
    /// Printable representation of the hash - returns either original string (if known) or #hash
    inline const char* ToString()const
    {
        if (_string)
            return _string;
        else
        {
            static char buf[64];
            sprintf(buf, "<#%u>", _hash);
            return buf;
        }
    }
    
private:
    SHType _hash;
    const char* _string;
};

// CStringHash argument to be used in function args
typedef const CStringHash& SHArg;





#endif











