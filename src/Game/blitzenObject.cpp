#include "blitObject.h"
#include "Renderer/blitRenderer.h"

namespace BlitzenEngine
{
	GameObject::GameObject(bool bDynamic /*=0*/, const char* meshName /*=ce_defaultMeshName*/) :
		m_bDynamic{ bDynamic }, m_transformId{ 0 }
	{
		auto pResources{ BlitzenEngine::RenderingResources::GetRenderingResources() };
		m_meshId = pResources->meshMap[meshName].meshId;
	}

#if !defined(LAMBDA_GAME_OBJECT_TEST)

	ClientTest::ClientTest(bool bDynamic /*=0*/, const char* meshName /*=ce_defaultMeshName*/) :
		GameObject{bDynamic, meshName}
	{}

	void ClientTest::Update()
	{
		RotateObject(m_yaw, m_pitch, m_transformId, m_speed);
	}

	void GameObject::Update()
	{
		BLIT_WARN("Update function not implemented, if this is intended, change the bDynamic boolean to 0")
			// This is a placeholder
	}
#endif
	
	void RotateObject(float& yaw, float& pitch, uint32_t transformId, float speed)
	{
		auto pResources = RenderingResources::GetRenderingResources();
		float deltaTime = float(Engine::GetEngineInstancePointer()->GetDeltaTime());
		yaw += speed * deltaTime;
		pitch += speed * deltaTime;

		BlitML::vec3 yAxis(0.f, -1.f, 0.f);
		BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, yaw, 0);

		BlitML::vec3 xAxis(1.f, 0.f, 0.f);
		BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, pitch, 0);

		BlitzenEngine::MeshTransform& transform = pResources->transforms[transformId];
		transform.orientation = yawOrientation + pitchOrientation;

		// Updates the renderer
		auto pr = RendererType::GetRendererInstance();
		pr->UpdateObjectTransform(transformId, transform);
	}
}