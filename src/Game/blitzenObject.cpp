#include "blitObject.h"

namespace BlitzenEngine
{
	GameObject::GameObject(RenderContainer& renders, MeshResources& meshes, bool isDynamic, BlitzenEngine::MeshTransform& initialTransform, const char* meshName) :
		m_bDynamic{ isDynamic }, m_transformId{ 0 }
	{
		m_meshId = meshes.m_meshMap[meshName].meshId;
		m_transformId = CreateRenderObjectFromMesh(renders, meshes, m_meshId, initialTransform, isDynamic);
	}

#if !defined(LAMBDA_GAME_OBJECT_TEST)

	ClientTest::ClientTest(RenderContainer& renders, MeshResources& meshes, bool isDynamic, BlitzenEngine::MeshTransform& initialTransform, const char* meshName) :
		GameObject{ renders, meshes, isDynamic, initialTransform, meshName }
	{

	}

	void ClientTest::Update(BlitzenWorld::BlitzenWorldContext& context)
	{
		RotateObject(m_yaw, m_pitch, m_speed, GetTransformId(), context);
	}

	void GameObject::Update(BlitzenWorld::BlitzenWorldContext& context)
	{
		BLIT_WARN("Update function not implemented, if this is intended, change the bDynamic boolean to 0");
	}
#endif
	
	void RotateObject(float& yaw, float& pitch, float speed, uint32_t transformId, BlitzenWorld::BlitzenWorldContext& context)
	{
		float deltaTime{ float(context.pCoreClock->GetDeltaTime()) };

		yaw += speed * deltaTime;
		pitch += speed * deltaTime;

		BlitML::vec3 yAxis(0.f, -1.f, 0.f);
		BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, yaw, 0);

		BlitML::vec3 xAxis(1.f, 0.f, 0.f);
		BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, pitch, 0);

		context.rendererTransformUpdate.pTransform = &context.pRenders->m_transforms[transformId];
		context.rendererTransformUpdate.pTransform->orientation = yawOrientation + pitchOrientation;
		context.rendererTransformUpdate.transformId = transformId;

		context.rendererEvent = BlitzenEngine::RendererEvent::RENDERER_TRANSFORM_UPDATE;
	}
}