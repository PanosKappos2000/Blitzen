#if defined(_WIN32)

#include "dearDasherDX12.h"
#include "ImGui.h"

namespace BlitzenIMGUI
{
	
	DEAR_DASHER_RETURN_CODE ImguiDX12::Init(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		m_pDX = pRenderer;

		return DEAR_DASHER_RETURN_CODE::SUCCESS;
	}

	void ImguiDrawEditor(ImguiDX12& imguiDX, ImGuiIO& io, float deltaTime)
	{

	}

	void ImguiStartRecording(ImguiDX12& imguiDX)
	{

	}

	void ImguiBeginRenderPass(ImguiDX12& imguiDX)
	{

	}

	void ImguiSubmitEditorRender(ImguiDX12& imageDX)
	{

	}

	void ImguiUpdateEditorWindow(ImguiDX12& imguiDX)
	{

	}
}

#endif