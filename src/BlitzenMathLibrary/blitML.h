#pragma once 
#include "blitMLTypes.h"
#include "Platform/platform.h"

#include <math.h>
#include <stdlib.h>

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
    inline float Sin(float x) {return sinf(x);}
    inline float Cos(float x) {return cosf(x);}
    inline float Tan(float x) {return tanf(x);}
    inline float Acos(float x) {return acosf(x);}
    inline float Sqrt(float x) {return sqrtf(x);}
    inline float Abs(float x) {return fabsf(x);}
    inline float Max(float x, float y) {return (x > y) ? x : y;}

    inline uint8_t IsPowerOf2(uint64_t value) { return (value != 0) && ((value & (value - 1)) == 0); }

    static uint8_t sRandSeeded = 0;

    inline int32_t Rand() 
    {
        if(!sRandSeeded)
        {
            srand(static_cast<uint32_t>(BlitzenPlatform::PlatformGetAbsoluteTime()));
            sRandSeeded = 1;
        }
        return rand();
    }
    inline int32_t RandInRange(int32_t min, int32_t max)
    {
        if (!sRandSeeded) 
        {
            srand(static_cast<uint32_t>(BlitzenPlatform::PlatformGetAbsoluteTime()));
            sRandSeeded = 1;
        }
        return (rand() % (max - min + 1)) + min;
    }

    inline float FRand();
    inline float FRandInRange(float min, float max);


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




    /*-----------------------
        Matrix operations
    --------------------------*/

    // Creates and returns an orthographic projection matrix. Typically used to render flat or 2D scenes
    inline mat4 Orthographic(float left, float right, float bottom, float top, float near, float far) 
    {
        mat4 res;
        float lr = 1.0f / (left - right);
        float bt = 1.0f / (bottom - top);
        float nf = 1.0f / (near - far);
        res.data[0] = -2.0f * lr;
        res.data[5] = -2.0f * bt;
        res.data[10] = 2.0f * nf;
        res.data[12] = (left + right) * lr;
        res.data[13] = (top + bottom) * bt;
        res.data[14] = (far + near) * nf;
        return res;
    }

    // Creates and returns a perspective matrix, typically used to render 3d scenes.
    inline mat4 Perspective(float fov, float aspectRatio, float near, float far) 
    {
        float halfTanFov = Tan(fov * 0.5f);
        mat4 res(0);
        res.data[0] = 1.0f / (aspectRatio * halfTanFov);
        res.data[5] = 1.0f / halfTanFov;
        res.data[10] = far / (near - far);
        res.data[11] = -1.0f;
        res.data[14] = -((far * near) / (far - near));
        return res;
    }

    inline mat4 InfiniteZPerspective(float fov, float aspectRatio, float near)
    {
	    float halfTanFov = 1.0f / tanf(fov / 2.0f);
        mat4 res(0);
        res.data[0] = halfTanFov / aspectRatio;
        res.data[5] = halfTanFov;
        res.data[11] = 1.0f;
        res.data[14] = near;
        return res;

	    /*return glm::mat4(
		f / aspectRatio, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, zNear, 0.0f); Reference to write this with my own types*/
    }

    // Creates and returns a look-at matrix, or a matrix looking at target from the perspective of position.
    inline mat4 LookAt(vec3& position, vec3& target, vec3& up)
    {
        mat4 res;
        vec3 zAxis;
        zAxis.x = target.x - position.x;
        zAxis.y = target.y - position.y;
        zAxis.z = target.z - position.z;
        zAxis = GetNormalized(zAxis);
        vec3 cross = Cross(zAxis, up);
        vec3 xAxis = GetNormalized(cross);
        vec3 yAxis = Cross(xAxis, zAxis);
        res.data[0] = xAxis.x;
        res.data[1] = yAxis.x;
        res.data[2] = -zAxis.x;
        res.data[3] = 0;
        res.data[4] = xAxis.y;
        res.data[5] = yAxis.y;
        res.data[6] = -zAxis.y;
        res.data[7] = 0;
        res.data[8] = xAxis.z;
        res.data[9] = yAxis.z;
        res.data[10] = -zAxis.z;
        res.data[11] = 0;
        res.data[12] = -Dot(xAxis, position);
        res.data[13] = -Dot(yAxis, position);
        res.data[14] = Dot(zAxis, position);
        res.data[15] = 1.0f;
        return res;
    }

    // Returns a transposed copy of the provided matrix (rows->colums)
    inline mat4 Transpose(mat4& matrix)
    {
        mat4 res;
        res.data[0] = matrix.data[0];
        res.data[1] = matrix.data[4];
        res.data[2] = matrix.data[8];
        res.data[3] = matrix.data[12];
        res.data[4] = matrix.data[1];
        res.data[5] = matrix.data[5];
        res.data[6] = matrix.data[9];
        res.data[7] = matrix.data[13];
        res.data[8] = matrix.data[2];
        res.data[9] = matrix.data[6];
        res.data[10] = matrix.data[10];
        res.data[11] = matrix.data[14];
        res.data[12] = matrix.data[3];
        res.data[13] = matrix.data[7];
        res.data[14] = matrix.data[11];
        res.data[15] = matrix.data[15];
        return res;
    }


    // Creates and returns an inverse of the provided matrix.
    inline mat4 Mat4Inverse(mat4 matrix) 
    {
        const float* m = matrix.data;
        float t0 = m[10] * m[15];
        float t1 = m[14] * m[11];
        float t2 = m[6] * m[15];
        float t3 = m[14] * m[7];
        float t4 = m[6] * m[11];
        float t5 = m[10] * m[7];
        float t6 = m[2] * m[15];
        float t7 = m[14] * m[3];
        float t8 = m[2] * m[11];
        float t9 = m[10] * m[3];
        float t10 = m[2] * m[7];
        float t11 = m[6] * m[3];
        float t12 = m[8] * m[13];
        float t13 = m[12] * m[9];
        float t14 = m[4] * m[13];
        float t15 = m[12] * m[5];
        float t16 = m[4] * m[9];
        float t17 = m[8] * m[5];
        float t18 = m[0] * m[13];
        float t19 = m[12] * m[1];
        float t20 = m[0] * m[9];
        float t21 = m[8] * m[1];
        float t22 = m[0] * m[5];
        float t23 = m[4] * m[1];
        mat4 res;
        float* pRes = res.data;
        pRes[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) - (t1 * m[5] + t2 * m[9] + t5 * m[13]);
        pRes[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) - (t0 * m[1] + t7 * m[9] + t8 * m[13]);
        pRes[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
        pRes[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) - (t4 * m[1] + t9 * m[5] + t10 * m[9]);
        float d = 1.0f / (m[0] * pRes[0] + m[4] * pRes[1] + m[8] * pRes[2] + m[12] * pRes[3]);
        pRes[0] = d * pRes[0];
        pRes[1] = d * pRes[1];
        pRes[2] = d * pRes[2];
        pRes[3] = d * pRes[3];
        pRes[4] = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) - (t0 * m[4] + t3 * m[8] + t4 * m[12]));
        pRes[5] = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) - (t1 * m[0] + t6 * m[8] + t9 * m[12]));
        pRes[6] = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) - (t2 * m[0] + t7 * m[4] + t10 * m[12]));
        pRes[7] = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) - (t5 * m[0] + t8 * m[4] + t11 * m[8]));
        pRes[8] = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) - (t13 * m[7] + t14 * m[11] + t17 * m[15]));
        pRes[9] = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) - (t12 * m[3] + t19 * m[11] + t20 * m[15]));
        pRes[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) - (t15 * m[3] + t18 * m[7] + t23 * m[15]));
        pRes[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) - (t16 * m[3] + t21 * m[7] + t22 * m[11]));
        pRes[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) - (t16 * m[14] + t12 * m[6] + t15 * m[10]));
        pRes[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
        pRes[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) - (t22 * m[14] + t14 * m[2] + t19 * m[6]));
        pRes[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) - (t20 * m[6] + t23 * m[10] + t17 * m[2]));
        return res;
    }

    inline mat4 Translate(vec3& translation)
    {
        mat4 res;
        res.data[12] = translation.x;
        res.data[13] = translation.y;
        res.data[14] = translation.z;
        return res;
    }

    inline mat4 Scale(vec3& scale) 
    {
        mat4 res;
        res.data[0] = scale.x;
        res.data[5] = scale.y;
        res.data[10] = scale.z;
        return res;
    }

    inline mat4 Mat4EulerX(float radians) 
    {
        mat4 res;
        float c = Cos(radians);
        float s = Sin(radians);
        res.data[5] = c;
        res.data[6] = s;
        res.data[9] = -s;
        res.data[10] = c;
        return res;
    }
    inline mat4 Mat4EulerY(float radians) 
    {
        mat4 res;
        float c = Cos(radians);
        float s = Sin(radians);
        res.data[0] = c;
        res.data[2] = -s;
        res.data[8] = s;
        res.data[10] = c;
        return res;
    }
    inline mat4 Mat4EulerZ(float angle_radians) 
    {
        mat4 res;
        float c = Cos(angle_radians);
        float s = Sin(angle_radians);
        res.data[0] = c;
        res.data[1] = s;
        res.data[4] = -s;
        res.data[5] = c;
        return res;
    }
    inline mat4 Mat4EulerXYZ(float xRadians, float yRadians, float zRadians) 
    {
        mat4 rx = Mat4EulerX(xRadians);
        mat4 ry = Mat4EulerY(yRadians);
        mat4 rz = Mat4EulerZ(zRadians);
        mat4 res = rx * ry;
        res = res * rz;
        return res;
    }

    // Derives a forward vector from a provided matrix
    inline vec3 Mat4Forward(mat4& matrix) 
    {
        vec3 forward;
        forward.x = -matrix.data[2];
        forward.y = -matrix.data[6];
        forward.z = -matrix.data[10];
        Normalize(forward);
        return forward;
    }

    // Derives a backward vector from a derived matrix
    inline vec3 Mat4Backward(mat4& matrix) 
    {
        vec3 backward;
        backward.x = matrix.data[2];
        backward.y = matrix.data[6];
        backward.z = matrix.data[10];
        Normalize(backward);
        return backward;
    }

    // Derives an up vector from a derived matrix
    inline vec3 Mat4Up(mat4& matrix) 
    {
        vec3 up;
        up.x = matrix.data[1];
        up.y = matrix.data[5];
        up.z = matrix.data[9];
        Normalize(up);
        return up;
    }

    // You get the idea
    inline vec3 Mat4Down(mat4& matrix) 
    {
        vec3 down;
        down.x = -matrix.data[1];
        down.y = -matrix.data[5];
        down.z = -matrix.data[9];
        Normalize(down);
        return down;
    }

    inline vec3 Mat4Left(mat4& matrix) 
    {
        vec3 left;
        left.x = -matrix.data[0];
        left.y = -matrix.data[4];
        left.z = -matrix.data[8];
        Normalize(left);
        return left;
    }

    inline vec3 Mat4Right(mat4& matrix) 
    {
        vec3 right;
        right.x = matrix.data[0];
        right.y = matrix.data[4];
        right.z = matrix.data[8];
        Normalize(right);
        return right;
    }




    /*-------------------------
        Quaternion operations
    ---------------------------*/

    inline float QuatNormal(quat& q){ return Sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w); }

    inline quat NormalizeQuat(quat q) {
        float normal = QuatNormal(q);
        return quat(q.x / normal, q.y / normal , q.z / normal, q.w / normal);
    }

    inline quat QuatConjugate(quat& q) { return quat(-q.x, -q.y, -q.z, q.w); }

    inline quat QuatInverse(quat& q) { return NormalizeQuat(QuatConjugate(q)); }

    inline quat MulitplyQuat(quat& q1, quat& q2) 
    {
        quat res;
        res.x = q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
        res.y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
        res.z = q1.x * q1.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
        res.w = -q1.x * q2.x -q1.y * q2.y -q1.z * q2.z + q1.w * q2.w;
        return res;
    }

    inline float QuatDot(quat& q1, quat& q2) { return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w; }

    inline mat4 QuatToMat4(quat& q) 
    {
        mat4 res;
        quat n = NormalizeQuat(q);
        res.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
        res.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
        res.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;
        res.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
        res.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
        res.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;
        res.data[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
        res.data[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
        res.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;
        return res;
    }

    inline mat4 QuatToRotationMatrix(quat& q, vec3& center) 
    {
        mat4 res;
        float* f = res.data;
        f[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
        f[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
        f[2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
        f[3] = center.x - center.x * f[0] - center.y * f[1] - center.z * f[2];
        f[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
        f[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
        f[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
        f[7] = center.y - center.x * f[4] - center.y * f[5] - center.z * f[6];
        f[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
        f[9] = 2.0f * ((q.y * q.z) - (q.x * q.w));
        f[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
        f[11] = center.z - center.x * f[8] - center.y * f[9] - center.z * f[10];
        f[12] = 0.0f;
        f[13] = 0.0f;
        f[14] = 0.0f;
        f[15] = 1.0f;
        return res;
    }

    inline quat QuatFromAngleAxis(vec3& axis, float angle, uint8_t normalize) 
    {
        const float half_angle = 0.5f * angle;
        float s = Sin(half_angle);
        float c = Cos(half_angle);
        quat q = quat(s * axis.x, s * axis.y, s * axis.z, c);
        if (normalize) 
        {
            return NormalizeQuat(q);
        }
        return q;
    }

    // Spherical linear interpolation
    inline quat QuatSlerp(quat q_0, quat q_1, float percentage) 
    {
        quat res;
        // Source: https://en.wikipedia.org/wiki/Slerp
        // Only unit quaternions are valid rotations.
        // Normalize to avoid undefined behavior.
        quat v0 = NormalizeQuat(q_0);
        quat v1 = NormalizeQuat(q_1);
        // Compute the cosine of the angle between the two vectors.
        float dot = QuatDot(v0, v1);
        // If the dot product is negative, slerp won't take
        // the shorter path. Note that v1 and -v1 are equivalent when
        // the negation is applied to all four components. Fix by
        // reversing one quaternion.
        if (dot < 0.0f) {
            v1.x = -v1.x;
            v1.y = -v1.y;
            v1.z = -v1.z;
            v1.w = -v1.w;
            dot = -dot;
        }
        const float DOT_THRESHOLD = 0.9995f;
        if (dot > DOT_THRESHOLD) {
            // If the inputs are too close for comfort, linearly interpolate
            // and normalize the result.
            res = quat( v0.x + ((v1.x - v0.x) * percentage), v0.y + ((v1.y - v0.y) * percentage), v0.z + ((v1.z - v0.z) * percentage),
            v0.w + ((v1.w - v0.w) * percentage));
            return NormalizeQuat(res);
        }
        // Since dot is in range [0, DOT_THRESHOLD], acos is safe
        float theta_0 = Acos(dot);          // theta_0 = angle between input vectors
        float theta = theta_0 * percentage;  // theta = angle between v0 and result
        float sin_theta = Sin(theta);       // compute this value only once
        float sin_theta_0 = Sin(theta_0);   // compute this value only once
        float s0 = Cos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
        float s1 = sin_theta / sin_theta_0;
        return quat((v0.x * s0) + (v1.x * s1), (v0.y * s0) + (v1.y * s1), (v0.z * s0) + (v1.z * s1), (v0.w * s0) + (v1.w * s1));
    }

    inline float Radians(float degrees) { return degrees * BLIT_DEG2RAD_MULTIPLIER; }  
    inline float Degrees(float radians) {return radians * BLIT_RAD2DEG_MULTIPLIER; } 
}
