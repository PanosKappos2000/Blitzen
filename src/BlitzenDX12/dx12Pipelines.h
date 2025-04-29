#pragma once
#include "dx12Data.h"

namespace BlitzenDX12
{
    void CreateRootParameterPushConstants(D3D12_ROOT_PARAMETER& rootParameter, UINT shaderRegister, UINT registerSpace,
        UINT num32BitValues, D3D12_SHADER_VISIBILITY shaderVisibility);

    void CreateRootParameterDescriptorTable(D3D12_ROOT_PARAMETER& rootParameter, D3D12_DESCRIPTOR_RANGE* pRanges, UINT numRanges,
        D3D12_SHADER_VISIBILITY shaderVisibility);

    void CreateRootParameterCBV(D3D12_ROOT_PARAMETER& rootParameter, UINT baseRegister,
        UINT registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility);

	void CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE& range, D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0);

    uint8_t CreateRootSignature(ID3D12Device* device, ID3D12RootSignature** ppRootSignature,
        UINT numParameters, D3D12_ROOT_PARAMETER* pParameters,
        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE,
        D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1);

    uint8_t CreateShaderProgram(const WCHAR* filepath, const char* target, ID3DBlob** shaderBlob);

    void CreateDefaultPsoDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);

	uint8_t CreateTriangleGraphicsPipeline(ID3D12Device* device, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature, ID3D12PipelineState** ppPso);

    uint8_t CreateOpaqueGraphicsPipeline(ID3D12Device* device, ID3D12RootSignature* rootSignature, ID3D12PipelineState** ppPso);
}