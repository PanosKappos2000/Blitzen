#include "blitObject.h"
#include "Renderer/blitRenderer.h"

namespace BlitzenEngine
{
	void GameObject::Update()
	{
		BLIT_WARN("Update function not implemented, if this is intended, change the bDynamic boolean to 0")
			// This is a placeholder
	}

	GameObject::GameObject(uint8_t bDynamic /*=0*/, const char* meshName /*=ce_defaultMeshName*/) :
		m_bDynamic{ bDynamic }, m_transformId{ 0 }
	{
		auto pResources{ BlitzenEngine::RenderingResources::GetRenderingResources() };
		m_meshId = pResources->meshMap[meshName].meshId;
	}

	ClientTest::ClientTest(uint8_t bDynamic /*=0*/, const char* meshName /*=ce_defaultMeshName*/) :
		GameObject{bDynamic, meshName}
	{}

	void ClientTest::Update()
	{
		RotateObject(m_yaw, m_pitch, m_transformId, m_speed);
	}
	
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