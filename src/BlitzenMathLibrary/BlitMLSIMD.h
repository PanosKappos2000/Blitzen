#include "blitML.h"
#include <xmmintrin.h> // SSE

#if defined(BLIT_CPU_SIMD_SAVIOR)
constexpr bool GCBlitzenSimd = true;
#else
constexpr bool GCBlitzenSimd = false;
#endif

namespace BCPSS
{
	// SIMD version of Matrix 4x4 and 4 component vector multiplication.
	// Crucial for directional velocity.
	inline BlitML::float4 MulMat4Vec4(BlitML::float16& matrix, BlitML::float4& vector)
	{
		// FALLBACK
		if constexpr (!GCBlitzenSimd)
		{
			return matrix * vector;
		}

		//-------------------------------------------------------------------------------------------------
		// Matrix is in column-major layout (as used in BlitML, GLSL, and HLSL by default).
		// This means each set of 4 consecutive floats in the matrix array represents one column:
		//   column 0 = mat[0]  to mat[3]   -> X axis
		//   column 1 = mat[4]  to mat[7]   -> Y axis
		//   column 2 = mat[8]  to mat[11]  -> Z axis
		//   column 3 = mat[12] to mat[15]  -> Translation (W axis)
		//
		// Each column is loaded into an __m128 register for SIMD math.
		// We use _mm_loadu_ps (unaligned load) to safely load even if &matrix[i] isn’t 16-byte aligned.
		//-------------------------------------------------------------------------------------------------
		__m128 col0 = _mm_loadu_ps(&matrix[0]);   // mat[0], mat[1], mat[2], mat[3]
		__m128 col1 = _mm_loadu_ps(&matrix[4]);   // mat[4], mat[5], mat[6], mat[7]
		__m128 col2 = _mm_loadu_ps(&matrix[8]);   // mat[8], mat[9], mat[10], mat[11]
		__m128 col3 = _mm_loadu_ps(&matrix[12]);  // mat[12], mat[13], mat[14], mat[15]

		// Broadcast each component of the input vector to all elements of a SIMD register.
		// This prepares the values for column-wise multiply-accumulate:
		// result = (vec.x * col0) + (vec.y * col1) + (vec.z * col2) + (vec.w * col3)
		// Note: We use scalar values here instead of extracting from __m128 vec because
		// _mm_set1_ps is cheaper than shuffling individual components from 'vec'.
		__m128 vecX = _mm_set1_ps(vector.x);
		__m128 vecY = _mm_set1_ps(vector.y);
		__m128 vecZ = _mm_set1_ps(vector.z);
		__m128 vecW = _mm_set1_ps(vector.w);

		// Multiply each column by corresponding scalar.
		// A look at the mat4 x vec4 mutliplication operator overload will show what is going on.
		// Basically multiplication of every component on the column of the corresponding component of the vector.
		// Since both are in 4 component vector form, SIMD shines.
		__m128 mul0 = _mm_mul_ps(col0, vecX);
		__m128 mul1 = _mm_mul_ps(col1, vecY);
		__m128 mul2 = _mm_mul_ps(col2, vecZ);
		__m128 mul3 = _mm_mul_ps(col3, vecW);

		// Sum the results to get the final transformed vector
		// Using SIMD on the first two mul results and the last two, since they are four component vectors. 
		// Those results get one final SIMD addition
		__m128 result = _mm_add_ps(_mm_add_ps(mul0, mul1), _mm_add_ps(mul2, mul3));

		BlitML::float4 out;
		_mm_storeu_ps(&out.x, result); // Write back to output
		return out;
	}

	inline float Float3DotProduct(BlitML::float4 vector1, BlitML::float4 vector2)
	{
		// FALLBACK
		if constexpr (!GCBlitzenSimd)
		{
			return BlitML::Dot(BlitML::ToVec3(vector1), BlitML::ToVec3(vector2));
		}

		__m128 vec1 = _mm_loadu_ps(&vector1.x);
		__m128 vec2 = _mm_loadu_ps(&vector2.x);

		__m128 mul = _mm_mul_ps(vec1, vec2); // x, y, z, w
		__m128 temp = _mm_add_ps(mul, _mm_movehl_ps(mul, mul)); // (x + z), (y + w), ?, ?
		__m128 result = _mm_add_ss(temp, _mm_shuffle_ps(temp, temp, 1)); // x + z + y

		return _mm_cvtss_f32(result); // get .x
	}

	inline bool CheckCollisionSphereToSphere(const BlitML::float4& aMaxRadOne, const BlitML::float4& aMaxRadTwo)
	{
		BlitML::vec3 delta = aMaxRadOne.xyz() - aMaxRadTwo.xyz();
		float distanceSquared = BlitML::Dot(delta, delta);
		float radiusSum = aMaxRadOne.w + aMaxRadTwo.w;
		return distanceSquared <= (radiusSum * radiusSum);
	}

	inline bool CheckCollisionCapsuleToCapsule(const BlitML::float4& aMaxRadOne, const BlitML::float4& bMinTypeOne, const BlitML::float4& aMaxRadTwo, const BlitML::float4& bMinTypeTwo)
	{
		return false;
	}

	inline bool CheckCollisionAABBToAABB(const BlitML::float4& aMaxRadOne, const BlitML::float4& bMinTypeOne, const BlitML::float4& aMaxRadTwo, const BlitML::float4& bMinTypeTwo)
	{
		return false;
	}

	inline bool CheckCollisionCapsuleToAABB(const BlitML::float4& aMaxRadCapsule, const BlitML::float4& bMinTypeCapsule, const BlitML::float4& aMaxRadAABB, const BlitML::float4& bMinTypeAABB)
	{
		return false;
	}

	inline bool CheckCollisionCapusleToSphere(const BlitML::float4& aMaxRadCapsule, const BlitML::float4& bMinTypeCapsule, const BlitML::float4& aMaxRadSphere)
	{
		return false;
	}

	inline bool CheckCollisionAABBToSphere(const BlitML::float4& aMaxRadAABB, const BlitML::float4& bMinTypeAABB, const BlitML::float4& aMaxRadSphere)
	{
		return false;
	}

	inline void CheckCapsuleCollisionOnMultipleCapsules(const BlitML::float4& hitterAMaxRad, const BlitML::mat4& hitterBMinType, BlitML::float4* receiverAMaxRadArr, BlitML::float4* receiverBMinTypeArr,
		uint32_t receiverCount, BlitzenCore::FAT_BOOL* results)
	{
		
	}

	inline void CheckCapsuleCollisionOnMultipleAABBs(const BlitML::float4& hitterAMaxRad, const BlitML::mat4& hitterBMinType, BlitML::float4* receiverAMaxRadArr, BlitML::float4* receiverBMinTypeArr, 
		uint32_t receiverCount, BlitzenCore::FAT_BOOL* results)
	{

	}

	inline void CheckCapsuleCollisionOnMultipleSpheres(const BlitML::float4& hitterAMaxRad, const BlitML::mat4& hitterBMinType, BlitML::float4* receiverAMaxRadArr, uint32_t receiverCount,
		BlitzenCore::FAT_BOOL* results)
	{

	}

	inline void CheckAABBCollisionOnMultipleCapsules(const BlitML::float4& hitterAMaxRad, const BlitML::mat4& hitterBMinType, BlitML::float4* receiverAMaxRadArr, BlitML::float4* receiverBMinTypeArr,
		uint32_t receiverCount, BlitzenCore::FAT_BOOL* results)
	{

	}

	inline void CheckAABBCollisionOnMultipleAABBs(const BlitML::float4& hitterAMaxRad, const BlitML::mat4& hitterBMinType, BlitML::float4* receiverAMaxRadArr, BlitML::float4* receiverBMinTypeArr,
		uint32_t receiverCount, BlitzenCore::FAT_BOOL* results)
	{

	}

	inline void CheckAABBCollisionOnMultipleSpheres(const BlitML::float4& hitterAMaxRad, const BlitML::mat4& hitterBMinType, BlitML::float4* receiverAMaxRadArr, uint32_t receiverCount,
		BlitzenCore::FAT_BOOL* results)
	{

	}

	inline void CheckSphereCollsionOnMutlipleCapsules(const BlitML::float4& hitterAMaxRad, BlitML::float4* receiverAMaxRadArr, BlitML::float4* receiverBMinTypeArr, uint32_t receiverCount,
		BlitzenCore::FAT_BOOL* results)
	{

	}

	inline void CheckSphereCollsionOnMutlipleAABBs(const BlitML::float4& hitterAMaxRad, BlitML::float4* receiverAMaxRadArr, BlitML::float4* receiverBMinTypeArr, uint32_t receiverCount,
		BlitzenCore::FAT_BOOL* results)
	{

	}

	inline void CheckSphereCollisionOnMultpleSpheres(const BlitML::float4& hitterAMaxRad, BlitML::float4* receiverAMaxRadArr, uint32_t receiverCount, BlitzenCore::FAT_BOOL* results)
	{

	}
}