#include "dearDasher.h"

namespace BlitzenIMGUI
{
	DasherEditor::DasherEditor()
	{
		ImGui::CreateContext();
		m_io = ImGui::GetIO();
	}

	bool DasherEditor::Init(BlitzenEngine::RendererPtrType pRenderer)
	{
		auto initRes{ m_apiData.Init(pRenderer) };
		if (BlitzenCore::BLIT_CHECK_FAIL(initRes))
		{
			BLIT_ERROR("Failed to initialize imgui");
			return LOG_IMGUI_ERROR_MSG_AND_RETURN(initRes);
		}

		return true;
	}

	void DasherEditor::Draw(float deltaTime)
	{
		ImguiDrawEditor(m_apiData, m_io, deltaTime);
	}

	void DasherEditor::UpdateWindowSize()
	{
		ImguiUpdateEditorWindow(m_apiData);
	}
}