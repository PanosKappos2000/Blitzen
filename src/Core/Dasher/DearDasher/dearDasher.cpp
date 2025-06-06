#include "dearDasher.h"

namespace BlitzenIMGUI
{
	DasherUI::DasherUI()
	{
		ImGui::CreateContext();
		m_io = ImGui::GetIO();
	}

	bool DasherUI::Init(BlitzenEngine::RendererPtrType pRenderer)
	{
		auto initRes{ m_apiData.Init(pRenderer) };
		if (BlitzenCore::BLIT_CHECK_FAIL(initRes))
		{
			BLIT_ERROR("Failed to initialize imgui");
			return LOG_IMGUI_ERROR_MSG_AND_RETURN(initRes);
		}

		return true;
	}

	void DasherUI::Draw(float deltaTime)
	{
		ImguiDrawEditor(m_apiData, m_io, deltaTime);
	}
}