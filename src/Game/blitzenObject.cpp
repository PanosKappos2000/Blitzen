#include "blitObject.h"

namespace BlitzenEngine
{
    void GameObject::Update()
	{
		BLIT_WARN("Update function not implemented, if this is intended, change the bDynamic boolean to 0")
		// This is a placeholder
	}

	GameObject::GameObject(const char* meshName /*="undefined"*/, uint8_t bDynamic /*=0*/) :
		m_meshId{ 0/*pResources->meshMap.Get(meshName).transformIndex*/ },
		m_bDynamic{ bDynamic }, m_transformId{ 0 }
	{}

	ClientTest::ClientTest(const char* meshName /*="undefined"*/, uint8_t bDynamic /*=0*/) :
		GameObject(meshName, bDynamic)
	{}

	void ClientTest::Update()
	{
		auto pResources = RenderingResources::GetRenderingResources();

		m_yaw += m_speed;
		m_pitch += m_speed;

		BlitML::vec3 yAxis(0.f, -1.f, 0.f);
		BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, m_yaw, 0);

		BlitML::vec3 xAxis(1.f, 0.f, 0.f);
		BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, m_pitch, 0);
		
		BlitzenEngine::MeshTransform& transform = pResources->transforms[m_transformId];
		transform.orientation = yawOrientation + pitchOrientation;
	}
}