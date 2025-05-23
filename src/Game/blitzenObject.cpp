#include "blitObject.h"

namespace BlitzenEngine
{
	GameObject::GameObject(MeshTransform* pTransform, uint32_t transformId, Mesh* pMesh, bool isDynamic) :
		m_pTransform{ pTransform }, m_transformId{transformId}, m_pMesh{pMesh}, m_bDynamic{isDynamic}
	{
		
	}

#if !defined(LAMBDA_GAME_OBJECT_TEST)

	ClientTest::ClientTest(MeshTransform* pTransform, uint32_t transformId, Mesh* pMesh, bool isDynamic) :
		GameObject{ pTransform, transformId, pMesh, isDynamic }
	{

	}

	void ClientTest::Update(BlitzenWorld::BlitzenWorldContext& context)
	{
		RotateObject(m_yaw, m_pitch, m_speed, GetTransform(), context);
	}

	void GameObject::Update(BlitzenWorld::BlitzenWorldContext& context)
	{
		BLIT_WARN("Update function not implemented, if this is intended, change the bDynamic boolean to 0");
	}
#endif
	
	void RotateObject(float& yaw, float& pitch, float speed, MeshTransform* pTransform, BlitzenWorld::BlitzenWorldContext& context)
	{
		float deltaTime{ float(context.pCoreClock->GetDeltaTime()) };

		yaw += speed * deltaTime;
		pitch += speed * deltaTime;

		BlitML::vec3 yAxis(0.f, -1.f, 0.f);
		BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, yaw, 0);

		BlitML::vec3 xAxis(1.f, 0.f, 0.f);
		BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, pitch, 0);

		pTransform->orientation = yawOrientation + pitchOrientation;

		context.rendererEvent = BlitzenEngine::RendererEvent::RENDERER_TRANSFORM_UPDATE;
	}
}