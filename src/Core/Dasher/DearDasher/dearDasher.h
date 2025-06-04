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

		IMGUI_API_CONTEXT m_apiData;
	
	private:

		ImGuiIO m_io;
	};
}