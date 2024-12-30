#pragma once

#include <cstdint>

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

    inline vec3 operator + (vec3 v1, vec3 v2) { return vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }

    inline vec3 operator - (vec3& v1, vec3& v2){ return vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }

    inline vec3 operator * (vec3& v1, vec3& v2) { return vec3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z); }

    inline vec3 operator *(vec3 vec, float scalar) { return vec3(vec.x * scalar, vec.y * scalar, vec.z * scalar); }

    inline vec3 operator /(vec3 vec, float scalar) { return vec3(vec.x / scalar, vec.y / scalar, vec.z / scalar); }

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
        inline vec4(vec4& copy) : x{copy.x}, y{copy.y}, z{copy.z}, w{copy.w} {}  
    };

    inline vec4 operator + (vec4& v1, vec4& v2) { return vec4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w); }

    inline vec4 operator - (vec4& v1, vec4& v2){ return vec4(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w); }

    inline vec4 operator * (vec4& v1, vec4& v2) { return vec4(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w); }

    inline vec4 operator / (vec4& v1, vec4& v2) { return vec4(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w); }

    inline vec4 operator / (vec4& v1, float f) { return vec4(v1.x / f, v1.y / f, v1.z / f, v1.w / f); }



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

        inline float& operator [] (size_t index) { return this->data[index]; }

        inline vec4 GetRow(uint8_t row) { 
            return vec4(this->data[0 + row * 4], this->data[1 + row * 4], this->data[2 + row * 4], this->data[3 + row * 4]); 
        }
    };

    inline mat4 operator * (mat4& mat1, mat4& mat2) 
    {
        mat4 res;
        for (uint8_t i = 0; i < 4; ++i) {
            for (uint8_t j = 0; j < 4; ++j) 
            {
                res[j + i * 4] = mat1[0 + j] * mat2[0 + i * 4] + mat1[4 + j] * mat2[1 + i * 4] + mat1[8 + j] * mat2[2 + i * 4] + mat1[12 + j] * mat2[3 + i * 4];
            }
        }
        return res;
    }

    inline vec4 operator * (mat4& mat, vec4& vec)
    {
        vec4 res;
        res.x = mat[0] * vec.x + vec.y * mat[4] + vec.z * mat[8] + vec.w * mat[12];
        res.y = mat[1] * vec.x + vec.y * mat[5] + vec.z * mat[9] + vec.w * mat[13];
        res.z = mat[2] * vec.x + vec.y * mat[6] + vec.z * mat[10] + vec.w * mat[14];
        res.w = mat[3] * vec.x + vec.y * mat[7] + vec.z * mat[11] + vec.w * mat[15];

        return res;
    }





    struct alignas(16) Vertex
    {
        vec3 position;
        uint16_t uvX;
        vec3 normal;
        uint16_t uvY;
    };

    struct alignas(16) Meshlet
    {
	    uint32_t vertices[64]; // index into the original index buffer
	    uint32_t indices[126 * 3]; // up to 124 triangles, index into the above array of vertex indices
	    uint32_t triangleCount = 0; // The number of used elements in indices
	    uint32_t vertexCount; // The number of used elements in vertices
    };
}