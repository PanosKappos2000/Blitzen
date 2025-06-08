#pragma once

#include "Core/blitzenEngine.h"
#include "Imgui.h"
#include "dearDasherVK.h"
#include "dearDasherDX12.h"
#include "Core/Events/blitEditorEvents.h"
#include "Core/Dasher/Interface/dasherState.h"

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

	class DasherEditor
	{
	public:
		DasherEditor();

		bool Init(BlitzenEngine::RendererPtrType pRenderer);

		void Draw(float deltaTime);

		void UpdateWindowSize(uint32_t width, uint32_t height);

		BlitzenCore::EditorEventContext m_eventContext;

		BlitzenCore::DASHER_STATE m_state{ BlitzenCore::DASHER_STATE::DASHER_EDITOR_FULL_BLIT_RENDERER_IDLE };

		inline void SetWindow(uint32_t width, uint32_t height)
		{
			m_windowWidth = width;
			m_windowHeight = height;
		}
	
	private:

		ImGuiIO m_io;

		IMGUI_API_CONTEXT m_apiData;

		uint32_t m_windowWidth;

		uint32_t m_windowHeight;
	};

	void ImguiStartRecording(ImguiVK& imguiVk);

	void ImguiStartRecording(ImguiDX12& imguiDX);

	void ImguiBeginRenderPass(ImguiVK& imguiVk);

	void ImguiBeginRenderPass(ImguiDX12& imguiDX);

	void ImguiDrawEditor(ImguiVK& imguiVk, ImGuiIO& io, float deltaTime);

	void ImguiDrawEditor(ImguiDX12& imguiDX, ImGuiIO& io, float deltaTime);

	void ImguiSubmitEditorRender(ImguiVK& imguiVk);

	void ImguiSubmitEditorRender(ImguiDX12& imageDX);

	void ImguiUpdateEditorWindow(ImguiVK& imguiVk);

	void ImguiUpdateEditorWindow(ImguiDX12& imageDX);
}

namespace BlitzenPlatform
{
	void DearDasherUpdate();
}