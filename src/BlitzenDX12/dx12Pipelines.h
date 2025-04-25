#pragma once
#include "dx12Data.h"

namespace BlitzenDX12
{
    uint8_t CreateShaderProgram(const WCHAR* filepath, const char* target, ID3DBlob** shaderBlob);

    void CreateDefaultPsoDescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);

    uint8_t CreateOpaqueGraphicsPipeline(ID3D12Device* device, ID3D12RootSignature* rootSignature, ID3D12PipelineState** ppPso);
}