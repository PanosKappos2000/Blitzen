#include "dx12Renderer.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenDX12
{
	void Dx12Renderer::SetupResourceManagement()
	{
		for (auto& tools : m_frameTools)
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&tools.commandQueue))))
			{
				m_stats.ce_bResourceManagement = 0;
				return;
			}

			if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&tools.commandAllocator))))
			{
				m_stats.ce_bResourceManagement = 0;
				return;
			}

			if(FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&tools.inFlightFence))))
			{
				m_stats.ce_bResourceManagement = 0;
				return;
			}
		}
		m_stats.ce_bResourceManagement = 1;
	}

	uint8_t Dx12Renderer::UploadTexture(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
	void* pData, const char* filepath)
	{
		return 1;
	}

	uint8_t Dx12Renderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, 
	float& pyramidWidth, float& pyramidHeight)
	{
		if (!m_stats.ce_bResourceManagement)
			SetupResourceManagement();
		if (!m_stats.ce_bResourceManagement)
			return 0;

		if(!CreateSwapchain(m_swapchain, m_swapchain.extent.right, m_swapchain.extent.top, factory, m_frameTools[0].commandQueue.Get()))
			return 0;

		BLIT_INFO("Dx12 renderer setup")
		return 1;
	}

	uint8_t CreateSwapchain(Swapchain& swapchain, uint32_t windowWidth, uint32_t windowHeight, 
	IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue)
	{
		D3D12_VIEWPORT viewport;
	
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
	
		viewport.Width = static_cast<float>(windowWidth);
		viewport.Height = static_cast<float>(windowHeight);
	
		viewport.MinDepth = BlitzenEngine::ce_znear;
		viewport.MaxDepth = BlitzenEngine::ce_initialDrawDistance;
	
		if(swapchain.handle != nullptr)
		{
			if(swapchain.handle->ResizeBuffers(2, windowWidth, windowHeight, 
			DXGI_FORMAT_R8G8_B8G8_UNORM, 0) != S_OK)
				return 0;
		}
		else
		{
			DXGI_SWAP_CHAIN_DESC1 swapDesc{};
			swapDesc.BufferCount = 2;// I think this is about double buffering, not sure
			swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapDesc.SampleDesc.Count = 1;
			swapDesc.SampleDesc.Quality = 0;
			swapDesc.Width = windowWidth;
			swapDesc.Height = windowHeight;
			swapDesc.Format = DXGI_FORMAT_R8G8_B8G8_UNORM;// Hardcoding this, I don't need anything else for now
			swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; 
			swapDesc.Scaling = DXGI_SCALING_STRETCH; 
			swapDesc.Stereo = FALSE; 
				
			Microsoft::WRL::ComPtr<IDXGISwapChain1> newSwapchain;
			HWND hwnd = reinterpret_cast<HWND>(BlitzenPlatform::GetWindowHandle());

			// Validate HWND
			if (!IsWindow(hwnd))
			{
				BLIT_ERROR("Invalid HWND");
				return 0;
			}

			// Validate Command Queue
			if (!commandQueue)
			{
				BLIT_ERROR("Invalid Command Queue");
				return 0;
			}

			if(FAILED(factory->CreateSwapChainForHwnd(commandQueue, hwnd, 
			&swapDesc, nullptr, nullptr, &newSwapchain)))
				return 0;
	
			newSwapchain.As(&swapchain.handle);
		}
		return 1;
	}
}