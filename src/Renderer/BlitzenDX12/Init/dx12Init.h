#if defined(_WIN32)

#pragma once
#include "Renderer/BlitzenDX12/Context/dx12Context.h"

namespace BlitzenDX12
{
	uint8_t CreateFactory(IDXGIFactory6** ppFactory, DX12WRAPPER<ID3D12Debug>& debugController);

	uint8_t ChooseAdapter(IDXGIFactory6* factory, IDXGIAdapter4** ppAdapter);

	uint8_t CreateDevice(IDXGIAdapter4* adapter, ID3D12Device** ppDevice);

	uint8_t CreateDebugController(ID3D12Debug* pDebugController, DX12WRAPPER<ID3D12Debug1>& debugController1, ID3D12Device* device);

	uint8_t CreateRootSignatures(ID3D12Device* device, PipelineContext& context, DescriptorContext& descriptorContext);

	uint8_t CreateCmdSignatures(ID3D12Device* device, PipelineContext& ctx, DescriptorContext& descriptorContext);

	uint8_t CreatePipelines(ID3D12Device* device, PipelineContext& context);

	uint8_t CreateROResources(ID3D12Device* device, ReadOnlyResources& roResources);

	uint8_t CreateRWResources(ID3D12Device* device, ReadWriteResources* rwResourcesArray, uint32_t swapchainWidth, uint32_t swapchainHeight);

	uint8_t CreateCpuLogicBuffers(ID3D12Device* device, CPU_LOGIC_BUFFERS& resources);
}

#endif