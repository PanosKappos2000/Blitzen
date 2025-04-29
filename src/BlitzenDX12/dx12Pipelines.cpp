#include "dx12Pipelines.h"
#include "dx12Renderer.h"
#include <d3dcompiler.h>

namespace BlitzenDX12
{
    void CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE& range, D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
        UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace /*=0*/)
    {
		range = {};

		range.BaseShaderRegister = baseShaderRegister;
		range.RegisterSpace = registerSpace;
		range.NumDescriptors = numDescriptors;
		range.RangeType = rangeType;

		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    void CreateRootParameterPushConstants(D3D12_ROOT_PARAMETER& rootParameter, UINT shaderRegister, UINT registerSpace, 
        UINT num32BitValues, D3D12_SHADER_VISIBILITY shaderVisibility)
    {
        rootParameter = {};

		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameter.Constants.ShaderRegister = shaderRegister;
        rootParameter.Constants.RegisterSpace = registerSpace;
		rootParameter.Constants.Num32BitValues = num32BitValues;

		rootParameter.ShaderVisibility = shaderVisibility;
    }

    void CreateRootParameterDescriptorTable(D3D12_ROOT_PARAMETER& rootParameter, D3D12_DESCRIPTOR_RANGE* pRanges, UINT numRanges, 
        D3D12_SHADER_VISIBILITY shaderVisibility)
    {
        rootParameter = {};

		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameter.DescriptorTable.NumDescriptorRanges = numRanges;
		rootParameter.DescriptorTable.pDescriptorRanges = pRanges;

		rootParameter.ShaderVisibility = shaderVisibility;
    }

    void CreateRootParameterCBV(D3D12_ROOT_PARAMETER& rootParameter, UINT baseRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
    {
        rootParameter = {};

        rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameter.Descriptor.ShaderRegister = baseRegister;
        rootParameter.Descriptor.RegisterSpace = registerSpace;

        rootParameter.ShaderVisibility = shaderVisibility;
    }


    uint8_t CreateRootSignature(ID3D12Device* device, ID3D12RootSignature** ppRootSignature, 
        UINT numParameters, D3D12_ROOT_PARAMETER* pParameters,
        D3D12_ROOT_SIGNATURE_FLAGS flags /*=D3D12_ROOT_SIGNATURE_FLAG_NONE*/, 
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion /*=D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1*/)
    {
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = numParameters;
        rootSignatureDesc.pParameters = pParameters;
        rootSignatureDesc.Flags = flags;

        Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        auto serializeRes = D3D12SerializeRootSignature(&rootSignatureDesc, rootSignatureVersion,
            serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
        if (FAILED(serializeRes))
        {
            // Handle serialization error, log message from errorBlob if available
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            }
            return LOG_ERROR_MESSAGE_AND_RETURN(serializeRes);
        }

        auto rootSignatureResult = device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(),
            IID_PPV_ARGS(ppRootSignature));
        if (FAILED(rootSignatureResult))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(rootSignatureResult);
        }

        return 1;
    }

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
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        psoDesc.DepthStencilState.BackFace = psoDesc.DepthStencilState.FrontFace;

        
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = Ce_SwapchainFormat;  
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }

    uint8_t CreateTriangleGraphicsPipeline(ID3D12Device* device, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature, ID3D12PipelineState** ppPso)
    {
        D3D12_ROOT_PARAMETER rootParameters[1] = {};
        CreateRootParameterPushConstants(rootParameters[0], 0, 0, 3, D3D12_SHADER_VISIBILITY_VERTEX);

        if (!CreateRootSignature(device, rootSignature.ReleaseAndGetAddressOf(), 0, nullptr, 
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT))
        {
            BLIT_ERROR("Failed to create opaque root signature");
            return 0;
        }

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
        if (!CreateShaderProgram(L"HlslShaders/loadingTriangle.vs.hlsl", "vs_5_0", vertexShader.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to create triangle loading vertex shader");
            return 0;
        }
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
        if (!CreateShaderProgram(L"HlslShaders/loadingTriangle.ps.hlsl", "ps_5_0", pixelShader.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to create triangle loading pixel shader");
            return 0;
        }

        // Sets default values
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        CreateDefaultPsoDescription(psoDesc);

        // Adds specialized values (shader and root signature)
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

        auto psoResult = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPso));
        if (FAILED(psoResult))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(psoResult);
        }

        return 1;
    }

    uint8_t CreateOpaqueGraphicsPipeline(ID3D12Device* device, ID3D12RootSignature* rootSignature, ID3D12PipelineState** ppPso)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
        if (!CreateShaderProgram(L"HlslShaders/opaqueDraw.vs.hlsl", "vs_5_0", &vertexShader))
        {
			BLIT_ERROR("Failed to create main opaque vertex shader");
			return 0;
        }
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
        if (!CreateShaderProgram(L"HlslShaders/opaqueDraw.ps.hlsl", "ps_5_0", &pixelShader))
        {
			BLIT_ERROR("Failed to create main opaque pixel shader");
            return 0;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		CreateDefaultPsoDescription(psoDesc);
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

        auto psoResult = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPso));
        if (FAILED(psoResult))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(psoResult);
        }

        return 1;
    }
}