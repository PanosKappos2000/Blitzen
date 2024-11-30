#pragma once 
#include "blitMLTypes.h"

#define BLIT_PI                         3.14159265358979323846f
#define BLIT_PI_2                       2.0f * BLIT_PI
#define BLIT_HALF_PI                    0.5f * BLIT_PI
#define BLIT_QUARTER_PI                 0.25f * BLIT_PI
#define BLIT_ONE_OVER_PI                1.0f / BLIT_PI
#define BLIT_ONVER_OVER_2_PI            1.0f / BLIT_PI_2
#define BLIT_SQRT_TWO                   1.41421356237309504880f
#define BLIT_SQRT_THREE                 1.73205080756887729352f
#define BLIT_SQRT_ONE_OVER_TWO          0.70710678118654752440f
#define BLIT_SQRT_ONE_OVER_THREE        0.57735026918962576450f
#define BLIT_DEG2RAD_MULTIPLIER         BLIT_PI / 180.0f
#define BLIT_RAD2DEG_MULTIPLIER         180.0f / BLIT_PI

#define BLIT_SEC_TO_MS_MULTIPLIER       1000.f
#define BLIT_MS_TO_SEC_MULTIPLIER       0.001f

#define BLIT_INFINITY                   1e30f
#define BLIT_FLOAT_EPSILON              1.192092896e-07f

namespace BlitML
{
    /* ------------------------------------------
        General math functions
    ------------------------------------------ */
    float Sin(float x);
    float Cos(float x);
    float Tan(float x);
    float Acos(float x);
    float Sqrt(float x);
    float Abs(float x);

    inline uint8_t IsPowerOf2(uint64_t value) { return (value != 0) && ((value & (value - 1)) == 0); }

    int32_t Rand();
    int32_t RandInRange(int32_t min, int32_t max);

    float FRand();
    float FRandInRange(float min, float max);


    /*-----------------------
        Vector operations
    ------------------------*/

    // Does not return the true length but is adequate if a comparison or something similar is needed, where a square root operation can be avoided
    inline float LengthSquared(vec2& vec) { return vec.x * vec.x + vec.y * vec.y; }
    inline float LengthSquared(vec3& vec) { return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z; }
    inline float LengthSquared(vec4& vec) { return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z, vec.w * vec.w; }

    inline float Length(vec2& vec) { return Sqrt(LengthSquared(vec)); }
    inline float Length(vec3& vec) { return Sqrt(LengthSquared(vec)); }
    inline float Length(vec4& vec) { return Sqrt(LengthSquared(vec)); } 
    

    inline void Normalize(vec2& vec) {
        float len = Length(vec);
        vec.x /= len;
        vec.y /= len;
    }
    inline void Normalize(vec3& vec){
        float len = Length(vec);
        vec.x /= len;
        vec.y /= len;
        vec.z /= len;
    }
    inline void Normalize(vec4& vec){
        float len = Length(vec);
        vec.x /= len;
        vec.y /= len;
        vec.z /= len;
        vec.w /= len;
    }

    inline vec2 GetNormalized(vec2& vec){
        float len = Length(vec);
        return vec2(vec.x / len, vec.y / len);
    }
    inline vec3 GetNormalized(vec3& vec){
        float len = Length(vec);
        return vec3(vec.x / len, vec.y / len, vec.z / len);
    }
    inline vec4 GetNormalized(vec4& vec){
        float len = Length(vec);
        return vec4(vec.x / len, vec.y / len, vec.z / len, vec.w / len);
    }

    inline float Distance(vec2& v1, vec2& v2){
        vec2 d(v1 - v2);
        return Length(d);
    }
    inline float Distance(vec3& v1, vec3& v2){
        vec3 d(v1 - v2);
        return Length(d);
    }
    inline float Distance(vec4& v1, vec4& v2){
        vec4 d(v1 - v2);
        return Length(d);
    }

    inline vec3 ToVec3(vec4& vec) { return vec3(vec.x, vec.y, vec.z); }

    inline uint8_t Compare(vec2& v1, vec2& v2, float tolerance)
    {
        if (Abs(v1.x - v2.x) > tolerance) 
        {
            return 0;
        }
        if (Abs(v1.y - v2.y) > tolerance) 
        {
            return 0;
        }
        return 1;
    }
    inline uint8_t Compare(vec3& v1, vec3& v2, float tolerance)
    {
        if (Abs(v1.x - v2.x) > tolerance) 
        {
            return 0;
        }
        if (Abs(v1.y - v2.y) > tolerance) 
        {
            return 0;
        }
        if(Abs(v1.z - v2.z) > tolerance)
        {
            return 0;
        }
        return 1;
    }

    inline float Dot(vec3& v1, vec3& v2) 
    {
        float f = 0;
        f += v1.x * v2.x;
        f += v1.y * v2.y;
        f += v1.z * v2.z;
        return f;
    }

    inline float Dot(vec4& v1, vec4& v2){
        float f = 0;
        f += v1.x * v2.x;
        f += v1.y * v2.y;
        f += v1.z * v2.z;
        f += v1.w * v2.w;
        return f;
    }

    inline vec3 Cross(vec3& v1, vec3& v2){
        return vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
    }
}