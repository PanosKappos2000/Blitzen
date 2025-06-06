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
		m_apiData.StartFrame();
		ImGui::NewFrame();

		m_io.DeltaTime = deltaTime;

		// Create a basic window with a button
		ImGui::Begin("Test Window");

		ImGui::Text("Hello, Blitzen!");

		if (ImGui::Button("Click Me"))
		{
			// Button pressed logic here
		}

		ImGui::End();

		ImGui::Render();
	}
}