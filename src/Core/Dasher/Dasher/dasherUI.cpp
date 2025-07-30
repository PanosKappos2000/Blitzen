#include "dasherUI.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{
	constexpr BlitML::vec4 GCEditorTopHorizontalPanelColor = { 0.f, 0.2f, 0.f, 1.f };
	constexpr BlitML::vec4 GCEditorRightVerticalPanelColor = { 0.f, 0.f, 0.f, 1.f };
	constexpr uint32_t GCEditorTopHorizontalPanelHeightMultiplier = 1 / 10;
	constexpr uint32_t GCEditorRightVerticalPanelWidthMultiplier = 1 / 4;

	DasherUI::DasherUI(uint32_t windowWidth, uint32_t windowHeight) : mWindowExtent{(float)windowWidth, (float)windowHeight}
	{
		mProjection = BlitML::UIPixelProjection((float)windowWidth, (float)windowHeight);
	}

	void DasherUI::AllocRenderingLoadingContext(RendererPtrType bmpr)
	{
		BLIT_ASSERT(AllocateLoadingStagingBufferDSUI(bmpr, mLoadingContext));
	}

	bool DasherDefineEditor(RendererPtrType pRenderer, DasherUI* pDasher)
	{
		DSPanelContext topHorizontalPanelContext;
		topHorizontalPanelContext.fatherQuad.color = GCEditorTopHorizontalPanelColor;
		topHorizontalPanelContext.buttonCount = 0;
		topHorizontalPanelContext.fatherQuad.position = BlitML::vec2{ 0.0f, 0.0f };
		topHorizontalPanelContext.fatherQuad.scale = BlitML::vec2{ pDasher->mWindowExtent.x, pDasher->mWindowExtent.y * GCEditorTopHorizontalPanelHeightMultiplier };
		if (!pDasher->AllocPanel(topHorizontalPanelContext))
		{
			BLIT_ERROR("%s: Failed to create editor top horizontal panel", BlitzenCore::GCDasherEditorSystemName);
			return false;
		}

		DSPanelContext rightVerticalPanelContext;
		rightVerticalPanelContext.fatherQuad.color = GCEditorRightVerticalPanelColor;
		rightVerticalPanelContext.buttonCount = 0;
		rightVerticalPanelContext.fatherQuad.position = { pDasher->mWindowExtent.x - (pDasher->mWindowExtent.y * GCEditorRightVerticalPanelWidthMultiplier) };
		rightVerticalPanelContext.fatherQuad.scale = { pDasher->mWindowExtent.x / 5.0f, pDasher->mWindowExtent.y - (pDasher->mWindowExtent.y / 10.0f) };
		if (!pDasher->AllocPanel(rightVerticalPanelContext))
		{
			BLIT_ERROR("%s: Failed to create editor right vertical panel", BlitzenCore::GCDasherEditorSystemName);
			return false;
		}

		if (!UploadPanelQuadsToStagingBuffer(pDasher->mLoadingContext, pDasher->mPanelQuads, pDasher->mPanelCount))
		{
			BLIT_ERROR("%s: Renderer failed to copy ui quads to staging buffer", BlitzenCore::GCDasherEditorSystemName);
			return false;
		}

		return true;
	}

	bool DasherUI::AllocPanel(const DSPanelContext& panelContext)
	{
		if (mPanelCount >= GCDasherMaxPanelCount)
		{
			BLIT_ERROR("%s: Cannot add any more panels for current UI context", BlitzenCore::GCDasherUISystemName);
			return false;
		}

		mPanelQuads[mPanelCount] = panelContext.fatherQuad;

		auto& panel = mPanels[mPanelCount];
		if (panelContext.buttonCount != 0)
		{
			panel.mButtons = reinterpret_cast<DSButton*>(BlitzenCore::BlitAlloc<DSButton*>(BlitzenCore::AllocationType::DSUI, panelContext.buttonCount));
			panel.mButtonCount = panelContext.buttonCount;
		}

		++mPanelCount;

		return true;
	}

	DSPanel::~DSPanel()
	{
		if (mButtons != nullptr)
		{
			BLIT_ASSERT(mButtonEvents != nullptr && mButtonCount != 0);

			BlitzenCore::BlitFree<DSButton>(BlitzenCore::AllocationType::DSUI, mButtons, mButtonCount);
			BlitzenCore::BlitFree<DSEvent>(BlitzenCore::AllocationType::DSUI, mButtonEvents, mButtonCount);
		}
	}
}