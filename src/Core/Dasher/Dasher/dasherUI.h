#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenEngine
{
	constexpr uint32_t GCDasherMaxPanelCount = 10;

	enum class DSEvent
	{
		PLACEHOLDER
	};

	struct DSButton
	{
		DSQuad render;
	};

	struct DSTextField
	{
		float xOffset;
		float yOffset;

		float width;
		float height;
	};

	class DSPanel
	{
	public:
		DSQuad mFatherQuad;
		DSButton* mButtons = nullptr;
		DSEvent* mButtonEvents = nullptr;
		uint32_t mButtonCount = 0;
		
		~DSPanel();
	};

	struct DSPanelContext
	{
		DSQuad fatherQuad;
		uint32_t buttonCount;
	};

	class DasherUI
	{
	public:
		BlitML::vec2 mWindowExtent;
		BlitML::mat4 mProjection;
		RenderingLoadingContextDSUI mLoadingContext;

		BLIT_STRAIGHTHANDLE mAccessors = nullptr;
		uint32_t mAccessorCount;

		DSPanel mPanels[GCDasherMaxPanelCount];
		DSQuad mPanelQuads[GCDasherMaxPanelCount];
		uint32_t mPanelCount = 0;

		DSTextField* mTextFields;
		char** mTextFieldBuffers;
		uint32_t textFieldCount;

		DasherUI(uint32_t windowWidth, uint32_t windowHeight);

		void AllocRenderingLoadingContext(RendererPtrType bmpr);

		bool AllocPanel(const DSPanelContext& panelContext);
	};

	bool DasherDefineEditor(RendererPtrType pRenderer, DasherUI* pDasher);
}