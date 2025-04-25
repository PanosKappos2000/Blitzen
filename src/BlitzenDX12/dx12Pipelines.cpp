#include "dx12Pipelines.h"
#include "dx12Renderer.h"
#include <d3dcompiler.h>

namespace BlitzenDX12
{
    uint8_t CreateShaderProgram(const WCHAR* filepath, const char* target, ID3DBlob** shaderBlob)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        auto compileResult = D3DCompileFromFile(filepath, nullptr, nullptr, "main", target, 0, 0, shaderBlob, errorBlob.GetAddressOf());
        if (FAILED(compileResult))
        {
            if (errorBlob) 
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            }
            BLIT_ERROR("Failed to compile shader from %s", filepath);
            return 0;
        }

        // Success
        return 1;
    }

    void CreateDefaultPsoDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
    {
		psoDesc = {};
        psoDesc.InputLayout = {};

        psoDesc.RasterizerState = {};
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        psoDesc.BlendState = {};
        psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        psoDesc.DepthStencilState = {};
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        psoDesc.DepthStencilState.BackFace = psoDesc.DepthStencilState.FrontFace;

        
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }

    uint8_t CreateOpaqueGraphicsPipeline(ID3D12Device* device, ID3D12RootSignature* rootSignature, ID3D12PipelineState** ppPso)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
		CreateShaderProgram(L"Shaders/VertexShader.hlsl", "vs_5_0", &vertexShader);
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
		CreateShaderProgram(L"Shaders/PixelShader.hlsl", "ps_5_0", &pixelShader);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		CreateDefaultPsoDescription(psoDesc);
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

        auto psoResult = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPso));
        if (FAILED(psoResult))
        {
            LOG_ERROR_MESSAGE_AND_RETURN(psoResult);
            return 0;
        }

        return 1;
    }
}