//
// Lol Engine
//
// Copyright: (c) 2010-2011 Sam Hocevar <sam@hocevar.net>
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the Do What The Fuck You Want To
//   Public License, Version 2, as published by Sam Hocevar. See
//   http://sam.zoy.org/projects/COPYING.WTFPL for more details.
//

//
// The Matrix classes
// ------------------
//

#if !defined __LOL_MATRIX_H__
#define __LOL_MATRIX_H__

#include <cmath>
#if !defined __ANDROID__
#   include <iostream>
#endif

namespace lol
{

#define VECTOR_OP(elems, op) \
    template<typename U> \
    inline Vec##elems<T> operator op(Vec##elems<U> const &val) const \
    { \
        Vec##elems<T> ret; \
        for (int n = 0; n < elems; n++) \
            ret[n] = (*this)[n] op val[n]; \
        return ret; \
    } \
    \
    template<typename U> \
    inline Vec##elems<T> operator op##=(Vec##elems<U> const &val) \
    { \
        return *this = (*this) op val; \
    }

#define BOOL_OP(elems, op, op2, ret) \
    inline bool operator op(Vec##elems<T> const &val) const \
    { \
        for (int n = 0; n < elems; n++) \
            if (!((*this)[n] op2 val[n])) \
                return !ret; \
        return ret; \
    }

#define SCALAR_OP(elems, op) \
    inline Vec##elems<T> operator op(T const &val) const \
    { \
        Vec##elems<T> ret; \
        for (int n = 0; n < elems; n++) \
            ret[n] = (*this)[n] op val; \
        return ret; \
    } \
    \
    inline Vec##elems<T> operator op##=(T const &val) \
    { \
        return *this = (*this) op val; \
    }

#define CAST_OP(elems, dest) \
    inline operator Vec##dest<T>() const \
    { \
        Vec##dest<T> ret; \
        for (int n = 0; n < elems && n < dest; n++) \
            ret[n] = (*this)[n]; \
        for (int n = elems; n < dest; n++) \
            ret[n] = 0; \
        return ret; \
    }

#define OPERATORS_1(elems) \
    inline T& operator[](int n) { return *(&x + n); } \
    inline T const& operator[](int n) const { return *(&x + n); } \
    \
    VECTOR_OP(elems, -) \
    VECTOR_OP(elems, +) \
    VECTOR_OP(elems, *) \
    VECTOR_OP(elems, /) \
    \
    BOOL_OP(elems, ==, ==, true) \
    BOOL_OP(elems, !=, ==, false) \
    BOOL_OP(elems, <=, <=, true) \
    BOOL_OP(elems, >=, >=, true) \
    BOOL_OP(elems, <, <, true) \
    BOOL_OP(elems, >, >, true) \
    \
    SCALAR_OP(elems, -) \
    SCALAR_OP(elems, +) \
    SCALAR_OP(elems, *) \
    SCALAR_OP(elems, /) \
    \
    CAST_OP(elems, 2) \
    CAST_OP(elems, 3) \
    CAST_OP(elems, 4)

#define OPERATORS_2(elems) \
    template<typename U> \
    inline operator Vec##elems<U>() const \
    { \
        Vec##elems<U> ret; \
        for (int n = 0; n < elems; n++) \
            ret[n] = static_cast<U>((*this)[n]); \
        return ret; \
    } \
    \
    inline Vec##elems<T> operator -() const \
    { \
        Vec##elems<T> ret; \
        for (int n = 0; n < elems; n++) \
            ret[n] = -(*this)[n]; \
        return ret; \
    } \
    \
    inline T sqlen() const \
    { \
        T acc = 0; \
        for (int n = 0; n < elems; n++) \
            acc += (*this)[n] * (*this)[n]; \
        return acc; \
    } \
    \
    inline float len() const \
    { \
        using namespace std; \
        return sqrtf((float)sqlen()); \
    }

template <typename T> struct Vec2;
template <typename T> struct Vec3;
template <typename T> struct Vec4;

template <typename T> struct Vec2
{
    inline Vec2() { }
    inline Vec2(T val) { x = y = val; }
    inline Vec2(T _x, T _y) { x = _x; y = _y; }

    OPERATORS_1(2)
    OPERATORS_2(2)

#if !defined __ANDROID__
    template<typename U>
    friend std::ostream &operator<<(std::ostream &stream, Vec2<U> const &v);
#endif

    union { T x; T a; T i; };
    union { T y; T b; T j; };
};

typedef Vec2<float> vec2;
typedef Vec2<int> ivec2;

template <typename T> struct Vec3
{
    inline Vec3() { }
    inline Vec3(T val) { x = y = z = val; }
    inline Vec3(T _x, T _y, T _z) { x = _x; y = _y; z = _z; }

    OPERATORS_1(3)
    OPERATORS_2(3)

#if !defined __ANDROID__
    template<typename U>
    friend std::ostream &operator<<(std::ostream &stream, Vec3<U> const &v);
#endif

    union { T x; T a; T i; };
    union { T y; T b; T j; };
    union { T z; T c; T k; };
};

typedef Vec3<float> vec3;
typedef Vec3<int> ivec3;

template <typename T> struct Vec4
{
    inline Vec4() { }
    inline Vec4(T val) { x = y = z = w = val; }
    inline Vec4(T _x, T _y, T _z, T _w) { x = _x; y = _y; z = _z; w = _w; }

    OPERATORS_1(4)
    OPERATORS_2(4)

#if !defined __ANDROID__
    template<typename U>
    friend std::ostream &operator<<(std::ostream &stream, Vec4<U> const &v);
#endif

    union { T x; T a; T i; };
    union { T y; T b; T j; };
    union { T z; T c; T k; };
    union { T w; T d; T l; };
};

typedef Vec4<float> vec4;
typedef Vec4<int> ivec4;

#define SCALAR_GLOBAL(elems, op, U) \
    template<typename T> \
    static inline Vec##elems<U> operator op(U const &val, \
                                            Vec##elems<T> const &that) \
    { \
        Vec##elems<U> ret; \
        for (int n = 0; n < elems; n++) \
            ret[n] = val op that[n]; \
        return ret; \
    }

#define SCALAR_GLOBAL2(elems, op) \
    SCALAR_GLOBAL(elems, op, int) \
    SCALAR_GLOBAL(elems, op, float)

#define GLOBALS(elems) \
    SCALAR_GLOBAL2(elems, -) \
    SCALAR_GLOBAL2(elems, +) \
    SCALAR_GLOBAL2(elems, *) \
    SCALAR_GLOBAL2(elems, /)

GLOBALS(2)
GLOBALS(3)
GLOBALS(4)

template <typename T> struct Mat4
{
    inline Mat4() { }
    inline Mat4(T val)
    {
        for (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++)
                v[i][j] = (i == j) ? val : 0;
    }
    inline Mat4(Vec4<T> v0, Vec4<T> v1, Vec4<T> v2, Vec4<T> v3)
    {
        v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3;
    }

    inline Vec4<T>& operator[](int n) { return v[n]; }
    inline Vec4<T> const& operator[](int n) const { return v[n]; }

    T det() const;
    Mat4<T> invert() const;

    static Mat4<T> ortho(T left, T right, T bottom, T top, T near, T far);
    static Mat4<T> frustum(T left, T right, T bottom, T top, T near, T far);
    static Mat4<T> perspective(T theta, T width, T height, T near, T far);
    static Mat4<T> translate(T x, T y, T z);
    static Mat4<T> rotate(T theta, T x, T y, T z);

    void printf() const;

#if !defined __ANDROID__
    template<class U>
    friend std::ostream &operator<<(std::ostream &stream, Mat4<U> const &m);
#endif

    inline Mat4<T> operator +(Mat4<T> const val) const
    {
        Mat4<T> ret;
        for (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++)
                ret[i][j] = v[i][j] + val[i][j];
        return ret;
    }

    inline Mat4<T> operator +=(Mat4<T> const val)
    {
        return *this = *this + val;
    }

    inline Mat4<T> operator -(Mat4<T> const val) const
    {
        Mat4<T> ret;
        for (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++)
                ret[i][j] = v[i][j] - val[i][j];
        return ret;
    }

    inline Mat4<T> operator -=(Mat4<T> const val)
    {
        return *this = *this - val;
    }

    inline Mat4<T> operator *(Mat4<T> const val) const
    {
        Mat4<T> ret;
        for (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++)
            {
                T tmp = 0;
                for (int k = 0; k < 4; k++)
                    tmp += v[k][j] * val[i][k];
                ret[i][j] = tmp;
            }
        return ret;
    }

    inline Mat4<T> operator *=(Mat4<T> const val)
    {
        return *this = *this * val;
    }

    inline Vec4<T> operator *(Vec4<T> const val) const
    {
        Vec4<T> ret;
        for (int j = 0; j < 4; j++)
        {
            T tmp = 0;
            for (int i = 0; i < 4; i++)
                tmp += v[i][j] * val[i];
            ret[j] = tmp;
        }
        return ret;
    }

    Vec4<T> v[4];
};

typedef Mat4<float> mat4;
typedef Mat4<int> imat4;

} /* namespace lol */

#endif // __LOL_MATRIX_H__

