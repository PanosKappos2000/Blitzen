#pragma once

#include <iostream>

#include "Core/blitMemory.h"

namespace BlitML
{

    struct vec2
    {
        union 
        {
            float x, r, s, u;
        };
        union 
        {
            float y, g, t, v;
        };

        inline vec2() : x{0.f}, y{0.f} {}
        inline vec2(float f) : x{f}, y{f} {}
        inline vec2(float first, float second) : x{first}, y{second} {}
        inline vec2(vec2& copy) : x{copy.x}, y{copy.y} {}
    };

    inline vec2 operator + (vec2& v1, vec2& v2) { return vec2(v1.x + v2.x, v1.y + v2.y); }

    inline vec2 operator - (vec2& v1, vec2& v2){ return vec2(v1.x - v2.x, v1.y - v2.y); }

    inline vec2 operator * (vec2& v1, vec2& v2) { return vec2(v1.x * v2.x, v1.y * v2.y); }

    inline vec2 operator / (vec2& v1, vec2& v2) { return vec2(v1.x / v2.x, v1.y / v2.y); }

    struct vec3
    {
        union
        {
            float x, r, s, u;
        };
        union
        {
            float y, g, t, v;
        };
        union
        {
            float z, b, p, w;
        };

        inline vec3() : x{0.f}, y{0.f}, z{0.f} {}
        inline vec3(float f) : x{f}, y{f}, z{f} {}
        inline vec3(float first, float second, float third) : x{first}, y{second}, z{third} {}
        inline vec3(vec2& partial, float third) : x{partial.x}, y{partial.y}, z{third} {}
        inline vec3(vec3& copy) : x{copy.x}, y{copy.y}, z{copy.z} {}
    };

    inline vec3 operator + (vec3& v1, vec3& v2) { return vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }

    inline vec3 operator - (vec3& v1, vec3& v2){ return vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }

    inline vec3 operator * (vec3& v1, vec3& v2) { return vec3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z); }

    inline vec3 operator *(vec3& vec, float scalar) { return vec3(vec.x * scalar, vec.y * scalar, vec.z * scalar); }

    inline vec3 operator / (vec3& v1, vec3& v2) { return vec3(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z); }

    union vec4 
    {
        float elements[4];
        struct
        {
            union
            {
                float x, r, s;
            };
            union
            {
                float y, g, t;
            };
            union
            {
                float z, b, p;
            };
            union 
            {
                float w, a, q;
            };
        };

        inline vec4() : x{0.f}, y{0.f}, z{0.f}, w{0.f} {}
        inline vec4(float f) : x{f}, y{f}, z{f}, w{f} {}
        inline vec4(float first, float second, float third, float fourth) : x{first}, y{second}, z{third}, w{fourth} {}
        inline vec4(vec2& partial , float third, float fourth) : x{partial.x}, y{partial.y}, z{third}, w{fourth} {}
        inline vec4(vec3& partial, float fourth = 0.f) : x{partial.x}, y{partial.y}, z{partial.z}, w{fourth} {}
        inline vec4(vec4& copy) : x{copy.x}, y{copy.y}, z{copy.z}, w{copy.z} {}  
    };

    inline vec4 operator + (vec4& v1, vec4& v2) { return vec4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w); }

    inline vec4 operator - (vec4& v1, vec4& v2){ return vec4(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w); }

    inline vec4 operator * (vec4& v1, vec4& v2) { return vec4(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w); }

    inline vec4 operator / (vec4& v1, vec4& v2) { return vec4(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w); }



    // Quaternion
    typedef vec4 quat;


    // 4x4 matrix
    union mat4
    {
        float data [16];

        // Creates and identity matrix if identity is defaulted or any value other than 1. Creates a matrix filled with zeroes otherwise
        inline mat4(uint8_t identity = 1)
        {
            BlitzenCore::BlitZeroMemory(this, sizeof(mat4));

            if(identity)
            {
                data[0] = 1.f;
                data[5] = 1.f;
                data[10] = 1.f;
                data[15] = 1.f;
            }
        }
    };

    inline mat4 operator * (mat4& mat1, mat4& mat2) 
    {
        mat4 res;
        const float* pMat1 = mat1.data;
        const float* pMat2 = mat2.data;
        float* pRes = res.data;
        for (int32_t i = 0; i < 4; ++i) 
        {
            for (int32_t j = 0; j < 4; ++j) 
            {
                *pRes = pMat1[0] * pMat2[0 + j] + pMat1[1] * pMat2[4 + j] + pMat1[2] * pMat2[8 + j] + pMat1[3] * pMat2[12 + j];
                pRes++;
            }
            pMat1 += 4;
        }
        return res;
    }





    struct alignas(16) Vertex
    {
        vec3 position;
    };
}