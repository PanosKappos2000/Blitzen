#include "Core/Dasher/Interface/dasherInterface.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

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
		if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)initRes))
		{
			BLIT_ERROR("Failed to initialize imgui");
			return BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_DEAR_DASHER_EDITOR_SYSTEM_NAME, DEAR_DASHER_ERROR_CODE_TO_STRING(initRes));
		}

		return true;
	}

	static void ImguiStartBar(uint32_t windowWidth, uint32_t windowHeight, DasherEditor* pEditor)
	{
		// HORIZONTAL TOP BAR
		ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight / 20));
		ImGui::SetNextWindowPos(ImVec2(0, 0));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.f, 0.3f, 1.0f)); // Apply background color

		ImGui::Begin("Top Bar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

		// Set the button style: yellow text, dark blue border (border color)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.1f, 1.0f)); // Darker blue button background
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.7f, 1.0f)); // Lighter blue on hover
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 1.0f)); // Even darker blue on click
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Bright yellow text color

		// Set button font size and padding
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f)); // Padding inside the button
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 0.0f)); // No spacing between items

		// CENTER
		ImGui::SetCursorPosX(((float)windowWidth - (ImGui::CalcTextSize("START").x + ImGui::GetStyle().FramePadding.x * 2.0f * BlitML::Sin(float(windowHeight)))) * 0.5f);

		uint32_t eventCount = pEditor->m_eventContext.m_currentID;

		if (ImGui::Button("START"))
		{
			if (eventCount < BlitzenCore::Ce_EditorButtonEventTypeCount)
			{
				pEditor->m_eventContext.m_events[eventCount].m_type = BlitzenCore::EditorEventType::BUTTON_CLICK;
				pEditor->m_eventContext.m_events[eventCount].m_eventTypeID = BlitzenCore::Ce_ImguiSceneStartButtonID;
				eventCount++;
			}
		}

		// Pop the styles after the button to reset them
		ImGui::PopStyleVar(2); // Pop the style variables we set for the button
		ImGui::PopStyleColor(4); // Pop the style colors we set for the button

		ImGui::PopStyleColor(); // Pop the top bar background color

		ImGui::End();
	}

	static void ImguiSideBar(uint32_t windowWidth, uint32_t windowHeight)
	{
		// Set the side bar size and position (10% of window width, full height)
		auto sideBarSize = ImVec2((float)windowWidth / 10, (float)windowHeight);
		ImGui::SetNextWindowSize(sideBarSize); // 10% of width, full height
		ImGui::SetNextWindowPos(ImVec2((float)windowWidth - sideBarSize.x, (float)windowHeight / 20)); // Position just below the top bar

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.2f, 1.0f)); // Apply the color for the window background

		ImGui::Begin("Side Bar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Sidebar content can go here");

		ImGui::End();

		ImGui::PopStyleColor();
	}

	static void ImguiDebugBar(uint32_t windowWidth, uint32_t windowHeight, DasherEditor* pEditor)
	{
		// HORIZONTAL TOP BAR
		ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight / 20));
		ImGui::SetNextWindowPos(ImVec2(0, 0));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.f, 0.3f, 1.0f)); // Apply background color

		ImGui::Begin("Top Bar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

		// Set the button style: yellow text, dark blue border (border color)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.1f, 1.0f)); // Darker blue button background
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.7f, 1.0f)); // Lighter blue on hover
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 1.0f)); // Even darker blue on click
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Bright yellow text color

		// Set button font size and padding
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f)); // Padding inside the button
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 0.0f)); // No spacing between items

		// CENTER
		ImGui::SetCursorPosX(0);

		uint32_t eventCount = pEditor->m_eventContext.m_currentID;

		if (ImGui::Button("Close"))
		{
			if (eventCount < BlitzenCore::Ce_EditorButtonEventTypeCount)
			{
				pEditor->m_eventContext.m_events[eventCount].m_type = BlitzenCore::EditorEventType::BUTTON_CLICK;
				pEditor->m_eventContext.m_events[eventCount].m_eventTypeID = BlitzenCore::Ce_ImguiDebugWindowCloseID;
				eventCount++;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Freeze Frustum"))
		{
			if (eventCount < BlitzenCore::Ce_EditorButtonEventTypeCount)
			{
				pEditor->m_eventContext.m_events[eventCount].m_type = BlitzenCore::EditorEventType::BUTTON_CLICK;
				pEditor->m_eventContext.m_events[eventCount].m_eventTypeID = BlitzenCore::Ce_ImguiFreezeFrustumButtonID;
				eventCount++;
			}
		}

		// Pop the styles after the button to reset them
		ImGui::PopStyleVar(2); // Pop the style variables we set for the button
		ImGui::PopStyleColor(4); // Pop the style colors we set for the button

		ImGui::PopStyleColor(); // Pop the top bar background color

		ImGui::End();
	}

	void DasherEditor::Draw(float deltaTime)
	{
#if defined(BLIT_VK_FORCE)
		ImguiStartRecording(m_apiData);

		BlitzenPlatform::DearDasherUpdate();

		ImguiDrawEditor(m_apiData, m_io, deltaTime);

		ImGui::NewFrame();

		m_io.DeltaTime = deltaTime;

		switch (m_state)
		{
		case BlitzenCore::DASHER_STATE::DASHER_EDITOR_FULL_BLIT_RENDERER_IDLE:
		{
			ImguiStartBar(m_windowWidth, m_windowHeight, this);
			ImguiSideBar(m_windowWidth, m_windowHeight);
			break;
		}
		case BlitzenCore::DASHER_STATE::BLIT_RENDERER_FULL_DASHER_EDITOR_NO_JOIN:
		{
			BLIT_ASSERT(true);
			break;
		}
		case BlitzenCore::DASHER_STATE::DASHER_EDITOR_DEBUG_RENDERER:
		{
			ImguiDebugBar(m_windowWidth, m_windowHeight, this);
			break;
		}
		}

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