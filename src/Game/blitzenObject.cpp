#include "blitObject.h"
#include "Renderer/blitRenderer.h"
#include "Core/blitTimeManager.h"

namespace BlitzenEngine
{
	GameObject::GameObject(RenderingResources* pResources, bool bDynamic /*=0*/, const char* meshName /*=ce_defaultMeshName*/) :
		m_bDynamic{ bDynamic }, m_transformId{ 0 }
	{
		m_meshId = pResources->meshMap[meshName].meshId;
	}

#if !defined(LAMBDA_GAME_OBJECT_TEST)

	ClientTest::ClientTest(RenderingResources* pResources, bool bDynamic /*=0*/, const char* meshName /*=ce_defaultMeshName*/) :
		GameObject{ pResources, bDynamic, meshName }
	{}

	void ClientTest::Update(void** ppContext)
	{
		float deltaTime{ float(BlitzenCore::BlitzenWorld_GetWorldTimeManager(ppContext)->GetDeltaTime()) };
		auto pRenderer{ BlitzenWorld_GetRenderer(ppContext) };
		auto pResources{ BlitzenWorld_GetRenderingResources(ppContext) };

		RotateObject(m_yaw, m_pitch, m_transformId, m_speed, deltaTime, pResources, pRenderer);
	}

	void GameObject::Update(void** ppContext)
	{
		BLIT_WARN("Update function not implemented, if this is intended, change the bDynamic boolean to 0");
	}
#endif
	
	void RotateObject(float& yaw, float& pitch, uint32_t transformId, float speed, float deltaTime, 
		RenderingResources* pResources, void* pRenderer)
	{
		yaw += speed * deltaTime;
		pitch += speed * deltaTime;

		BlitML::vec3 yAxis(0.f, -1.f, 0.f);
		BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, yaw, 0);

		BlitML::vec3 xAxis(1.f, 0.f, 0.f);
		BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, pitch, 0);

		// I may not need to do this
		auto& transform = pResources->transforms[transformId];
		transform.orientation = yawOrientation + pitchOrientation;

		// Updates the renderer
		reinterpret_cast<RendererPtrType>(pRenderer)->UpdateObjectTransform(transformId, transform);
	}
}