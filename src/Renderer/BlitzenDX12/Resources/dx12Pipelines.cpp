#if defined(_WIN32)

#include "dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "dx12Resources.h"
#include <string>
#include "BlitCL/blitString.h"
#include "Core/DbLog/blitLogger.h"
// Temporary
#include <fstream>
#include <sstream>
#include <iostream>

namespace BlitzenDX12
{
    class ShaderIncludeHandler : public ID3DInclude
    {
    public:

        // Ref counting
        inline STDMETHOD_(ULONG, AddRef)() { return 1; }
        inline STDMETHOD_(ULONG, Release)() { return 1; }

        STDMETHOD(Open)(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override;

        STDMETHOD(Close)(LPCVOID pData) override;
    private:
        std::string m_content;
    };

    // Create an instance of ShaderIncludeHandler
    inline ShaderIncludeHandler inl_shaderIncludeHandler;

    HRESULT ShaderIncludeHandler::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
    {
        std::ifstream file(pFileName);
        if (!file.is_open())
        {
            return E_FAIL;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        m_content = buffer.str();

        // Allocate memory for the content
        *ppData = m_content.c_str();
        *pBytes = (UINT)m_content.size();

        return S_OK;
    }

    HRESULT ShaderIncludeHandler::Close(LPCVOID pData)
    {
        return S_OK;
    }

    constexpr size_t GCGetShaderBytesErrorCode = 0;
    static size_t GetShaderBytes(ID3D12Device* device, const char* filepath, BlitCL::String& bytes)
    {
        BlitzenPlatform::C_FILE_SCOPE scopedFILE;

        if (!scopedFILE.Open(filepath, BlitzenPlatform::FileModes::Read, 1))
        {
            BLIT_ERROR("Failed to open shader file");
            return GCGetShaderBytesErrorCode;
        }

        size_t filesize = 0;
        if (!BlitzenPlatform::FilesystemReadAllBytes(scopedFILE, bytes, &filesize))
        {
            BLIT_ERROR("Failed to read shader file");
            return GCGetShaderBytesErrorCode;
        }

        return filesize;
    }

    void CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE& range, D3D12_DESCRIPTOR_RANGE_TYPE rangeType, UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace /*=0*/)
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

    void CreateRootParameterUAV(D3D12_ROOT_PARAMETER& rootParameter, UINT baseRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
    {
        rootParameter = {};

        rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        rootParameter.Descriptor.ShaderRegister = baseRegister;
        rootParameter.Descriptor.RegisterSpace = registerSpace;

        rootParameter.ShaderVisibility = shaderVisibility;
    }

    void CreateRootParameterSrv(D3D12_ROOT_PARAMETER& rootParameter, UINT baseRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
    {
        rootParameter = {};

        rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
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

        DX12WRAPPER<ID3DBlob> serializedRootSignature;
        DX12WRAPPER<ID3DBlob> errorBlob;

        HRESULT serializeRes = D3D12SerializeRootSignature(&rootSignatureDesc, rootSignatureVersion, serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
        if (FAILED(serializeRes))
        {
            BLIT_ERROR("Failed to serialize root signature");

            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            }

            return LOG_ERROR_MESSAGE_AND_RETURN(serializeRes);
        }

        HRESULT rootSignatureResult = device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(ppRootSignature));
        if (FAILED(rootSignatureResult))
        {
            BLIT_ERROR("Failed to create root signature");
            return LOG_ERROR_MESSAGE_AND_RETURN(rootSignatureResult);
        }

        return 1;
    }

    uint8_t CreateShaderProgram(const WCHAR* filepath, const char* target, const char* entryPoint, ID3DBlob** shaderBlob)
    {
        DX12WRAPPER<ID3DBlob> errorBlob;

        HRESULT compileResult = D3DCompileFromFile(filepath, nullptr, &inl_shaderIncludeHandler, entryPoint, target, 0, 0, shaderBlob, errorBlob.GetAddressOf());
        if (FAILED(compileResult))
        {
            BLIT_ERROR("Failed to compile shader program");

            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            }

            return LOG_ERROR_MESSAGE_AND_RETURN(compileResult);
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
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
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
            BLIT_ERROR("Failed to create triangle root signature");
            return 0;
        }

        DX12WRAPPER<ID3DBlob> vertexShader;
        if (!CreateShaderProgram(L"HlslShadersLegacy/loadingTriangle.vs.hlsl", "vs_5_0", "main", vertexShader.ReleaseAndGetAddressOf()))
        {
            BLIT_ERROR("Failed to create triangle loading vertex shader");
            return 0;
        }
        DX12WRAPPER<ID3DBlob> pixelShader;
        if (!CreateShaderProgram(L"HlslShadersLegacy/loadingTriangle.ps.hlsl", "ps_5_0", "main", pixelShader.ReleaseAndGetAddressOf()))
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

        HRESULT psoResult = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPso));
        if (FAILED(psoResult))
        {
            BLIT_ERROR("Failed to create triangle pipeline");
            return LOG_ERROR_MESSAGE_AND_RETURN(psoResult);
        }

        return 1;
    }

    uint8_t CreateGraphicsPipelines(ID3D12Device* device, PipelineContext& ctx)
    {
        BlitCL::String vsBytes;
        size_t vsSize{ 0 };

        vsSize = GetShaderBytes(device, "HlslShaders/VS/opaqueDraw.vs.hlsl.bin", vsBytes);
        if (!vsSize)
        {
            BLIT_ERROR("%s: Failed to create main opaque vertex shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        D3D12_SHADER_BYTECODE vsCode{};
        vsCode.BytecodeLength = vsSize;
        vsCode.pShaderBytecode = vsBytes.Data();

        BlitCL::String psBytes;
        size_t psSize{ GetShaderBytes(device, "HlslShaders/PS/opaqueDraw.ps.hlsl.bin", psBytes) };
        if (!psSize)
        {
            BLIT_ERROR("Failed to create main opaque pixel shader");
            return 0;
        }
        D3D12_SHADER_BYTECODE psCode{};
        psCode.BytecodeLength = psSize;
        psCode.pShaderBytecode = psBytes.Data();

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        CreateDefaultPsoDescription(psoDesc);
        psoDesc.pRootSignature = ctx.m_graphicsRoot.Get();
        psoDesc.VS = vsCode;
        psoDesc.PS = psCode;
        psoDesc.DSVFormat = Ce_DepthTargetFormat;

        HRESULT psoResult = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ctx.m_staticDrawPso.ReleaseAndGetAddressOf()));
        if (FAILED(psoResult))
        {
            BLIT_ERROR("%s: Failed to create opaque draw pipeline state object", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(psoResult);
        }

        vsBytes.Clear();
        vsSize = 0;

        vsSize = GetShaderBytes(device, "HlslShaders/VS/dynamicDraw.vs.hlsl.bin", vsBytes);
        if (vsSize == 0)
        {
            BLIT_ERROR("%s: Failed to create dynamic object vertex shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        vsCode.BytecodeLength = vsSize;
        vsCode.pShaderBytecode = vsBytes.Data();
        psoDesc.VS = vsCode;

        HRESULT dynamicPsoRes{ device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ctx.m_dynamicDrawPso.ReleaseAndGetAddressOf())) };
        if (FAILED(dynamicPsoRes))
        {
            BLIT_ERROR("%s: Failed to create dynamic draw pipeline state object", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        vsBytes.Clear();
        vsSize = 0;

        vsSize = GetShaderBytes(device, "HlslShaders/VS/terrainDraw.vs.hlsl.bin", vsBytes);
        if (vsSize == 0)
        {
            BLIT_ERROR("%s: Failed to create terrain draw vertex shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        psBytes.Clear();
        psSize = 0;

        psSize = GetShaderBytes(device, "HlslShaders/PS/terrainDraw.ps.hlsl.bin", psBytes);
        if (psSize == 0)
        {
            BLIT_ERROR("%s: Failed to create terrain draw pixel shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        vsCode.BytecodeLength = vsSize;
        vsCode.pShaderBytecode = vsBytes.Data();
        psoDesc.VS = vsCode;

        psCode.BytecodeLength = psSize;
        psCode.pShaderBytecode = psBytes.Data();
        psoDesc.PS = psCode;

        HRESULT terrainPsoRes{ device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ctx.m_terrainDrawPso.ReleaseAndGetAddressOf())) };
        if (FAILED(terrainPsoRes))
        {
            BLIT_ERROR("%s: Failed to create terrain draw pipeline state object", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(terrainPsoRes);
        }

        if constexpr (BlitzenCore::Ce_InstanceCulling)
        {
            vsBytes.Clear();
            vsSize = 0;

            vsSize = GetShaderBytes(device, "HlslShaders/VS/opaqueDrawInst.vs.hlsl.bin", vsBytes);
            if (vsSize == 0)
            {
                BLIT_ERROR("%s: Failed to create opaque instanced vertex shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
                return 0;
            }

            vsCode.BytecodeLength = vsSize;
            vsCode.pShaderBytecode = vsBytes.Data();

            psoDesc.VS = vsCode;
            psoDesc.pRootSignature = ctx.m_graphicsRoot.Get();

            HRESULT psoInstResult{ device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ctx.m_drawInstPso.ReleaseAndGetAddressOf())) };
            if (FAILED(psoInstResult))
            {
                BLIT_ERROR("%s: Failed to create opaque draw instanced pipeline state object", BlitzenCore::CE_DX12_SYSTEM_NAME);
                return LOG_ERROR_MESSAGE_AND_RETURN(psoInstResult);
            }
        }

        return 1;
    }

    uint8_t CreateColliderVisualDebugPipelines(ID3D12Device* device, PipelineContext& ctx)
    {
#if defined(BLIT_VISUAL_DEBUG)
        BlitCL::String vsBytes;
        size_t vsSize{ 0 };

        vsSize = GetShaderBytes(device, "HlslShaders/VS/colliderDraw.vs.hlsl.bin", vsBytes);
        if (vsSize == GCGetShaderBytesErrorCode)
        {
            BLIT_ERROR("%s: Failed to create collider visual debug  vertex shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }
        D3D12_SHADER_BYTECODE vsCode{};
        vsCode.BytecodeLength = vsSize;
        vsCode.pShaderBytecode = vsBytes.Data();

        BlitCL::String psBytes;
        size_t psSize{ GetShaderBytes(device, "HlslShaders/PS/colliderDraw.ps.hlsl.bin", psBytes) };
        if (psSize == GCGetShaderBytesErrorCode)
        {
            BLIT_ERROR("%s: Failed to create collider visual debug pixel shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }
        D3D12_SHADER_BYTECODE psCode{};
        psCode.BytecodeLength = psSize;
        psCode.pShaderBytecode = psBytes.Data();

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        CreateDefaultPsoDescription(psoDesc);
        psoDesc.pRootSignature = ctx.m_graphicsRoot.Get();
        psoDesc.VS = vsCode;
        psoDesc.PS = psCode;
        psoDesc.DSVFormat = Ce_DepthTargetFormat;

        // Resets Blend state to setup for transparency
        psoDesc.BlendState = {};
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // No writing to depth
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        psoDesc.DepthStencilState.StencilEnable = FALSE;

        HRESULT psoResult = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ctx.mColliderDrawPso.ReleaseAndGetAddressOf()));
        if (FAILED(psoResult))
        {
            BLIT_ERROR("%s: Failed to create collider visual debug pipeline state object", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(psoResult);
        }

        vsBytes.Clear();
        vsSize = 0;
        vsSize = GetShaderBytes(device, "HlslShaders/VS/gridCellDraw.vs.hlsl.bin", vsBytes);
        if (vsSize == GCGetShaderBytesErrorCode)
        {
            BLIT_ERROR("%s: Failed to create grid cell visual debug vertex shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        psBytes.Clear();
        psSize = 0;
        psSize = GetShaderBytes(device, "HlslShaders/PS/gridCellDraw.ps.hlsl.bin", psBytes);
        if (psSize == GCGetShaderBytesErrorCode)
        {
            BLIT_ERROR("%s: Failed to create grid cell visual debug pixel shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        D3D12_SHADER_BYTECODE gridCellVSDesc{};
        gridCellVSDesc.BytecodeLength = vsSize;
        gridCellVSDesc.pShaderBytecode = vsBytes.Data();
        D3D12_SHADER_BYTECODE gridCellPSDesc{};
        gridCellPSDesc.BytecodeLength = psSize;
        gridCellPSDesc.pShaderBytecode = psBytes.Data();
        D3D12_GRAPHICS_PIPELINE_STATE_DESC gridCellPsoDesc{};
        CreateDefaultPsoDescription(gridCellPsoDesc);
        gridCellPsoDesc.pRootSignature = ctx.m_graphicsRoot.Get();
        gridCellPsoDesc.VS = gridCellVSDesc;
        gridCellPsoDesc.PS = gridCellPSDesc;

        HRESULT gridCellPsoResult = device->CreateGraphicsPipelineState(&gridCellPsoDesc, IID_PPV_ARGS(ctx.mGridCellDrawPso.ReleaseAndGetAddressOf()));
        if (FAILED(psoResult))
        {
            BLIT_ERROR("%s: Failed to create grid cell visual debug pipeline state object", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(psoResult);
        }

        return 1;
#endif
    }

    uint8_t CreateBlitzenLogoPipeline(ID3D12Device* device, PipelineContext& ctx)
    {
        D3D12_DESCRIPTOR_RANGE textureRange{};
        CreateDescriptorRange(textureRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_BLITZEN_LOGO_TEX_REGISTER);

        D3D12_DESCRIPTOR_RANGE textureSamplerRange{};
        CreateDescriptorRange(textureSamplerRange, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, BLIT_HLSL_TEX_SAMPLER_REGISTER);

        D3D12_ROOT_PARAMETER rootParams[CE_BLITZEN_LOGO_PIPELINE_PARAM_COUNT]{};
        CreateRootParameterDescriptorTable(rootParams[CE_BLITZEN_LOGO_TEX_ID], &textureRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        CreateRootParameterDescriptorTable(rootParams[CE_BLITZEN_LOGO_SAMPLER_ID], &textureSamplerRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);

        if (!CreateRootSignature(device, ctx.m_blitzenLogoRoot.ReleaseAndGetAddressOf(), CE_BLITZEN_LOGO_PIPELINE_PARAM_COUNT, rootParams))
        {
            BLIT_ERROR("%s: Failed to create Blitzen logo root signature", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        BlitCL::String vsBytes;
        size_t vsSize{ GetShaderBytes(device, "HlslShaders/VS/blitzenLogo.vs.hlsl.bin", vsBytes) };
        if (vsSize == 0)
        {
            BLIT_ERROR("%s: Failed to create blitzen logo draw vertex shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        D3D12_SHADER_BYTECODE vsCode{};
        vsCode.BytecodeLength = vsSize;
        vsCode.pShaderBytecode = vsBytes.Data();

        BlitCL::String psBytes;
        size_t psSize{ GetShaderBytes(device, "HlslShaders/PS/blitzenLogo.ps.hlsl.bin", psBytes) };
        if (psSize == 0)
        {
            BLIT_ERROR("%s: Failed to create blitzen logo draw pixel shader", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return 0;
        }

        D3D12_SHADER_BYTECODE psCode{};
        psCode.BytecodeLength = psSize;
        psCode.pShaderBytecode = psBytes.Data();

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        CreateDefaultPsoDescription(psoDesc);
        psoDesc.DepthStencilState = {};
        psoDesc.pRootSignature = ctx.m_blitzenLogoRoot.Get();
        psoDesc.VS = vsCode;
        psoDesc.PS = psCode;

        HRESULT psoResult = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ctx.m_blitzenLogoPipelineState.ReleaseAndGetAddressOf()));
        if (FAILED(psoResult))
        {
            BLIT_ERROR("%s: Failed to create Blitzen logo pipeline state object", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(psoResult);
        }

        // SUCCESS
        return 1;
    }

    uint8_t CreateBMPRDrivenCollisionComputeShaders(ID3D12Device* device, PipelineContext& ctx)
    {
        if constexpr (BLITGCBroadPhaseCollisionBumper || BLITGCNarrowPhaseCollisionBumper)
        {
            // First this shader will go over all grid cells, and reset their count
            if (!CreateComputeShaderProgram(device, ctx.m_cullRoot.Get(), ctx.MCellsColliderCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/wvGridCellReset.cs.hlsl.bin"))
            {
                BLIT_ERROR("%s: Failed to create wvGridCellReset.cs.hlsl compute shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
                return 0;
            }

            // Then this shader will go over all world variables and get a flat index from their position on the x and z axis
            // That index will place them inside the appropriate grid by incrementing the grid count and having the resident point to it
            if (!CreateComputeShaderProgram(device, ctx.m_cullRoot.Get(), ctx.MCellsColliderCountPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/wvGridCellColliderCount.cs.hlsl.bin"))
            {
                BLIT_ERROR("%s: Failed to create wvGridCellColliderCount.cs.hlsl compute shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
                return 0;
            }

            // Goes over colliders again, this time checks their count and gives them an offset for the collider index array, using a global counter. Their count is reset
            if (!CreateComputeShaderProgram(device, ctx.m_cullRoot.Get(), ctx.MCellsColliderOffsetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/wvGridCellColliderOffset.cs.hlsl.bin"))
            {
                BLIT_ERROR("%s: Failed to create wvGridCellColliderOffset.cs.hlsl compute shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
                return 0;
            }

            if (!CreateComputeShaderProgram(device, ctx.m_cullRoot.Get(), ctx.MColliderIDXsPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/wvColliderIndices.cs.hlsl.bin"))
            {
                BLIT_ERROR("%s: Failed to create wvColliderIndices.cs.hlsl compute shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
                return 0;
            }

            if (!CreateComputeShaderProgram(device, ctx.m_cullRoot.Get(), ctx.MColliderTransformPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/wvColliderTransform.cs.hlsl.bin"))
            {
                BLIT_ERROR("%s: Failed to create wvColliderTransform.cs.hlsl.bin compute shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
                return 0;
            }
        }

        if constexpr (BLITGCNarrowPhaseCollisionBumper)
        {

        }

        return 1;
    }

    uint8_t CreateComputeShaderProgram(ID3D12Device* device, ID3D12RootSignature* root, ID3D12PipelineState** pso, const char* filename)
    {
        BlitCL::String csBytes;
        size_t csSize{ GetShaderBytes(device, filename, csBytes) };
        if (!csSize)
        {
            BLIT_ERROR("Failed to create compute shader program");
            return 0;
        }

        D3D12_SHADER_BYTECODE csCode{};
        csCode.BytecodeLength = csSize;
        csCode.pShaderBytecode = csBytes.Data();

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.CS = csCode;
        psoDesc.pRootSignature = root;

        HRESULT cullPsoResult{ device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(pso)) };
        if (FAILED(cullPsoResult))
        {
            BLIT_ERROR("Failed to create compute pipeline");
            return LOG_ERROR_MESSAGE_AND_RETURN(cullPsoResult);
        }

        return 1;
    }

    void CreateOMSTargetDescs(D3D12_RENDER_PASS_RENDER_TARGET_DESC* renderTargetDesc, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* depthTargetDesc, 
        D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetHandle, D3D12_CPU_DESCRIPTOR_HANDLE* depthTargetHandle)
    {
        for (UINT frame = 0; frame < ce_framesInFlight; ++frame)
        {
            // DEPTH TARGET
            depthTargetDesc[frame] = {};
            depthTargetDesc[frame].cpuDescriptor = depthTargetHandle[frame];

            depthTargetDesc[frame].DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = Ce_ClearDepth;

            depthTargetDesc[frame].DepthBeginningAccess.Clear.ClearValue.Format = Ce_DepthTargetFormat;

            depthTargetDesc[frame].DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

            // RENDER TARGET
            renderTargetDesc[frame] = {};
            renderTargetDesc[frame].cpuDescriptor = renderTargetHandle[frame];

            renderTargetDesc[frame].BeginningAccess.Clear.ClearValue.Color[0] = BlitzenCore::Ce_DefaultWindowBackgroundColor[0];
            renderTargetDesc[frame].BeginningAccess.Clear.ClearValue.Color[1] = BlitzenCore::Ce_DefaultWindowBackgroundColor[1];
            renderTargetDesc[frame].BeginningAccess.Clear.ClearValue.Color[2] = BlitzenCore::Ce_DefaultWindowBackgroundColor[2];
            renderTargetDesc[frame].BeginningAccess.Clear.ClearValue.Color[3] = BlitzenCore::Ce_DefaultWindowBackgroundColor[3];

            renderTargetDesc[frame].BeginningAccess.Clear.ClearValue.Format = Ce_SwapchainFormat;

            renderTargetDesc[frame].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        }
    }

    void DefineViewportAndScissor(ID3D12GraphicsCommandList* commandList, float width, float height)
    {
        D3D12_VIEWPORT viewport = {};

        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;

        viewport.Width = width;
        viewport.Height = height;

        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissorRect = { 0, 0, LONG(width), LONG(height) };

        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);
    }

    void ClearWindow(ID3D12GraphicsCommandList* cmdList, float swapchainWidth, float swapchainHeight, ID3D12Resource* swapchainBackBuffer,
        DescriptorContext& descriptorContext, UINT swapchainIndex)
    {
        DefineViewportAndScissor(cmdList, swapchainWidth, swapchainHeight);

        // Render target barrier
        D3D12_RESOURCE_BARRIER renderTargetBarrier{};
        CreateResourcesTransitionBarrier(renderTargetBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &renderTargetBarrier);

        cmdList->OMSetRenderTargets(1, &descriptorContext.m_swapchainRtvHandle[swapchainIndex], FALSE, &descriptorContext.m_depthTargetDSVHandle[swapchainIndex]);

        cmdList->ClearRenderTargetView(descriptorContext.m_swapchainRtvHandle[swapchainIndex], BlitzenCore::Ce_DefaultWindowBackgroundColor, 0, nullptr);
        cmdList->ClearDepthStencilView(descriptorContext.m_depthTargetDSVHandle[swapchainIndex], D3D12_CLEAR_FLAG_DEPTH, Ce_ClearDepth, 0, 0, nullptr);
    }

    void BeginRenderPassClear(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* swapchainBackBuffer, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, UINT swapchainIndex)
    {
        cmdList->OMSetRenderTargets(1, &descriptorContext.m_swapchainRtvHandle[swapchainIndex], FALSE, &descriptorContext.m_depthTargetDSVHandle[swapchainIndex]);

        pipelineContext.m_depthTargetPassDesc[swapchainIndex].DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            
        pipelineContext.m_renderTargetPassDesc[swapchainIndex].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;

        cmdList->BeginRenderPass(1, &pipelineContext.m_renderTargetPassDesc[swapchainIndex], &pipelineContext.m_depthTargetPassDesc[swapchainIndex], D3D12_RENDER_PASS_FLAG_NONE);
    }

    void BeginRenderPassPreserve(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* swapchainBackBuffer, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, UINT swapchainIndex)
    {
        cmdList->OMSetRenderTargets(1, &descriptorContext.m_swapchainRtvHandle[swapchainIndex], FALSE, &descriptorContext.m_depthTargetDSVHandle[swapchainIndex]);

        pipelineContext.m_depthTargetPassDesc[swapchainIndex].DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;

        pipelineContext.m_renderTargetPassDesc[swapchainIndex].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;

        cmdList->BeginRenderPass(1, &pipelineContext.m_renderTargetPassDesc[swapchainIndex], &pipelineContext.m_depthTargetPassDesc[swapchainIndex], D3D12_RENDER_PASS_FLAG_NONE);
    }
}

#endif