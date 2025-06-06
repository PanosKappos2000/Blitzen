#pragma once

#include "Core/blitzenEngine.h"
#include "Imgui.h"
#include "dearDasherVK.h"
#include "dearDasherDX12.h"

namespace BlitzenIMGUI
{

#if defined(linux)

	static_assert(true, "Linux does not have imgui yet");

#elif defined(_WIN32) && defined(BLIT_VK_FORCE)

	using IMGUI_API_CONTEXT = BlitzenIMGUI::ImguiVK;

#elif defined(_WIN32) && defined(BLIT_GL_LEGACY_OVERRIDE) 

	static_assert(true, "Imgui cannot use opengl yet");

#elif defined(_WIN32)

	using IMGUI_API_CONTEXT = BlitzenIMGUI::ImguiDX12;

#else

	static_assert(true);

#endif

	class DasherUI
	{
	public:
		DasherUI();

		bool Init(BlitzenEngine::RendererPtrType pRenderer);

		void Draw(float deltaTime);
	
	private:

		ImGuiIO m_io;

		IMGUI_API_CONTEXT m_apiData;
	};

	void ImguiDrawEditor(BlitzenIMGUI::ImguiVK& imguiVk, ImGuiIO& io, float deltaTime);

	void ImguiDrawEditor(BlitzenIMGUI::ImguiDX12& imguiDX, ImGuiIO& io, float deltaTime);
}

namespace BlitzenPlatform
{
	void DearDasherUpdate();
}