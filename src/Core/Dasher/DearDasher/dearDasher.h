#pragma once

#include "Core/blitzenEngine.h"
#include "Imgui.h"
#include "dearDasherVK.h"

namespace BlitzenIMGUI
{
	using IMGUI_API_CONTEXT = BlitzenIMGUI::ImguiVK;

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
}