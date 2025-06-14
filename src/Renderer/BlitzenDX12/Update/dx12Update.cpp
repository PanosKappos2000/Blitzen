#if defined(_WIN32)

#include "dx12Update.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"

namespace BlitzenDX12
{
	void WaitForFrame(Dx12Renderer* pContext)
	{
		auto& cmdContext = pContext->m_cmdContext[pContext->m_currentFrame];

		PlaceFence(cmdContext.m_frameFence.m_value, pContext->m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);
	}

	void Generate_HI_Z(Dx12Renderer* pContext)
	{

	}

	void Dx12Renderer::UpdateTransforms(BlitzenEngine::MeshTransform* pDynamicTransformArr, uint32_t transformCount, BlitzenEngine::MeshTransform* transforms)
	{
		for (uint32_t move = 0; move < transformCount; ++move)
		{
			
		}
	}

	void PassDynamicTransformToRenderer(Dx12Renderer* pContext, BlitzenEngine::MeshTransform* dynamicTransforms, uint32_t transformCount, BlitzenEngine::MeshTransform* transforms)
	{
		/*for (uint32_t move = 0; move < transformCount; ++move)
		{
			auto& dynamic{ dynamicTransforms[move] };
			auto& transform{ transforms[move] };

			if (!dynamic.m_isMoving || dynamic.m_isBlock)
			{
				continue;
			}

			if (dynamic.m_rotation != 0)
			{
				BlitML::vec3 yAxis(0.f, -1.f, 0.f);
				BlitML::quat yawVelocity = dynamic.m_rotation.x != 0 ? BlitML::NormalizedQuatFromAngleAxis(yAxis, dynamic.m_rotation.x) : 0;

				BlitML::vec3 xAxis(1.f, 0.f, 0.f);
				BlitML::quat pitchVelocity = dynamic.m_rotation.y != 0 ? BlitML::NormalizedQuatFromAngleAxis(xAxis, dynamic.m_rotation.y) : 0;

				// TODO: Roll velocity

				BlitML::quat angularVelocity = BlitML::MulitplyQuat(pitchVelocity, yawVelocity);
				transform.orientation = BlitML::MulitplyQuat(transform.orientation, angularVelocity);
			}

			transform.pos += dynamic.m_velocity;
		}*/

		
	}
}
#endif