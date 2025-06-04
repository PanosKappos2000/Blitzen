#include "dearDasher.h"

namespace BlitzenIMGUI
{
	DasherUI::DasherUI()
	{
		ImGui::CreateContext();
		m_io = ImGui::GetIO();
	}
}