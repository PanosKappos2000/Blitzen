#pragma once
#include "dx12Data.h"
#include <d3dcompiler.h>
#include <string>
#include "BlitCL/blitString.h"

namespace BlitzenDX12
{
    void CreateRootParameterPushConstants(D3D12_ROOT_PARAMETER& rootParameter, UINT shaderRegister, UINT registerSpace,
        UINT num32BitValues, D3D12_SHADER_VISIBILITY shaderVisibility);

    void CreateRootParameterDescriptorTable(D3D12_ROOT_PARAMETER& rootParameter, D3D12_DESCRIPTOR_RANGE* pRanges, UINT numRanges,
        D3D12_SHADER_VISIBILITY shaderVisibility);

    void CreateRootParameterCBV(D3D12_ROOT_PARAMETER& rootParameter, UINT baseRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility);

    void CreateRootParameterUAV(D3D12_ROOT_PARAMETER& rootParameter, UINT baseRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility);

    void CreateRootParameterSrv(D3D12_ROOT_PARAMETER& rootParameter, UINT baseRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility);

    // Descriptor parameter to be passed to root parameter of type descriptor table
	void CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE& range, D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0);

    // Single root signature creation for use by one or more pipelines
    uint8_t CreateRootSignature(ID3D12Device* device, ID3D12RootSignature** ppRootSignature,
        UINT numParameters, D3D12_ROOT_PARAMETER* pParameters,
        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE,
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1);

    // Compiles a specified shader inside the shader blob parameter
    uint8_t CreateShaderProgram(const WCHAR* filepath, const char* target, const char* entryPoint, ID3DBlob** shaderBlob);

    // Compiles a shader with shader model 6.6
    size_t GetShaderBytes(ID3D12Device* device, const char* filepath, BlitCL::String& bytes);

    // Compiles a compute shader and creates its pipeline state object
    uint8_t CreateComputeShaderProgram(ID3D12Device* device, ID3D12RootSignature* root, ID3D12PipelineState** pso, const char* filename);

    // Sets the most used values of a pso description 
    void CreateDefaultPsoDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);

    // Creates a pipeline that draws a single static triangle
	uint8_t CreateTriangleGraphicsPipeline(ID3D12Device* device, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature, ID3D12PipelineState** ppPso);

    // Main drawing pipeline creation
    uint8_t CreateOpaqueGraphicsPipeline(ID3D12Device* device, ID3D12RootSignature* rootSignature, ID3D12PipelineState** ppPso);

    // Creates the all shaders used for draw culling
    uint8_t CreateCullingShaders(ID3D12Device* device, ID3D12RootSignature* cullingRootSignature, ID3D12PipelineState** ppCullPso);

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

    
}