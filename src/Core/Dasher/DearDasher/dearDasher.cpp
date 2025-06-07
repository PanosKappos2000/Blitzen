#include "Core/Dasher/Interface/dasherInterface.h"

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
#if defined(BLIT_VK_FORCE)
		ImguiStartRecording(m_apiData);

		BlitzenPlatform::DearDasherUpdate();

		ImguiDrawEditor(m_apiData, m_io, deltaTime);

		ImGui::NewFrame();

		m_io.DeltaTime = deltaTime;

		// HORIZONTAL TOP BAR
		ImGui::SetNextWindowSize(ImVec2((float)m_windowWidth, 30)); 
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::Begin("Top Bar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

		// CENTER
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (ImGui::CalcTextSize("START").x + ImGui::GetStyle().FramePadding.x * 2.0f)) * 0.5f); 

		if (ImGui::Button("START"))
		{
			// TODO
		}

		//ImGui::SameLine(); More

		ImGui::End(); // Close the top bar window

		ImGui::Begin("DearDasher Test");

		ImGui::Text("DearDasher");

		uint32_t eventCount = m_eventContext.m_currentID;

		if (ImGui::Button("Freeze Frustum"))
		{
			if (eventCount < BlitzenCore::Ce_EditorButtonEventTypeCount)
			{
				m_eventContext.m_events[eventCount].m_type = BlitzenCore::EditorEventType::BUTTON_CLICK;
				m_eventContext.m_events[eventCount].m_eventTypeID = BlitzenCore::Ce_FreezeFrustumButtonID;
				eventCount++;
			}
		}

		ImGui::End();

		ImguiBeginRenderPass(m_apiData);

		// After begin render pass
		ImGui::Render();

		ImguiSubmitEditorRender(m_apiData);

#endif
	}

	void DasherEditor::UpdateWindowSize(uint32_t width, uint32_t height)
	{
		m_windowWidth = width;
		m_windowHeight = height;

		ImguiUpdateEditorWindow(m_apiData);
	}
}