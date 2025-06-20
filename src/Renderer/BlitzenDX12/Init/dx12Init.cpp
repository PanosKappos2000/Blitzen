#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenDX12
{
    uint8_t CreateFactory(IDXGIFactory6** ppFactory, DX12WRAPPER<ID3D12Debug>& debugController)
    {
        UINT dxgiFactoryFlags = 0;

        if constexpr (ce_bDebugController)
        {
            HRESULT debugRes{ D3D12GetDebugInterface(IID_PPV_ARGS(debugController.ReleaseAndGetAddressOf())) };
            if (SUCCEEDED(debugRes))
            {
                debugController->EnableDebugLayer();
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
            else
            {
                BLIT_ERROR("Failed to create debug controller");
                return LOG_ERROR_MESSAGE_AND_RETURN(debugRes);
            }
        }

        HRESULT factoryRes{ CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(ppFactory)) };
        if (FAILED(factoryRes))
        {
            BLIT_ERROR("Failed to create factory for %s", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(factoryRes);
        }

        // success
        return 1;
    }

    uint8_t ChooseAdapter(IDXGIFactory6* factory, IDXGIAdapter4** ppAdapter)
    {
        HRESULT res{ (factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(ppAdapter))) };
        if (FAILED(res))
        {
            BLIT_ERROR("Failed to choose adapter for %s", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(res);
        }
        
        // TODO: Consider a fallback with IDXGIAdapter1 and manual adapter selction

        // success
        return 1;
    }

    uint8_t CreateDevice(IDXGIAdapter4* adapter, ID3D12Device** ppDevice)
    {
        HRESULT deviceRes{ D3D12CreateDevice(adapter, Ce_DeviceFeatureLevel, IID_PPV_ARGS(ppDevice)) };
        if (FAILED(deviceRes))
        {
            BLIT_ERROR("% s: Failed to create device", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(deviceRes);
        }

        return 1;
    }

    uint8_t CreateDebugController(ID3D12Debug* pDebugController, DX12WRAPPER<ID3D12Debug1>& debugController1, ID3D12Device* device)
    {
        HRESULT interfaceRes{ pDebugController->QueryInterface(IID_PPV_ARGS(debugController1.ReleaseAndGetAddressOf())) };
        if (SUCCEEDED(interfaceRes))
        {
            debugController1->SetEnableGPUBasedValidation(TRUE);
            if (!CheckForDeviceRemoval(device))
            {
                BLIT_ERROR("Device removed");
                return 0;
            }
        }
        else
        {
            BLIT_ERROR("%s: Failed to create debug controller", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(interfaceRes);
        }

        return 1;
    }

    uint8_t CreateRootSignatures(ID3D12Device* device, PipelineContext& context)
    {
		// Range for descriptor table for SRVs that are allocated in the exclusive region of the heap for this root
		D3D12_DESCRIPTOR_RANGE opaqueSrvRanges[Ce_OpaqueDrawExclusiveSRVsRangeCount]{};
		CreateDescriptorRange(opaqueSrvRanges[Ce_OpaqueDrawVtxPosSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_OpaqueDrawVtxPosSRVRegister);
		CreateDescriptorRange(opaqueSrvRanges[Ce_OpaqueDrawVtxNormalSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_OpaqueDrawVtxNormalSRVRegister);
		CreateDescriptorRange(opaqueSrvRanges[Ce_OpaqueDrawVtxTangentSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_OpaqueDrawVtxTangentSRVRegister);
		CreateDescriptorRange(opaqueSrvRanges[Ce_OpaqueDrawVtxTexCoordSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_OpaqueDrawVtxTexCoordSRVRegister);

		// Ranges for descrtiptor table for SRVs that are allocated in the shared section of the heap
		D3D12_DESCRIPTOR_RANGE sharedSrvRanges[Ce_SharedSRVsRangeCount]{};
		CreateDescriptorRange(sharedSrvRanges[Ce_SurfaceSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_SurfaceSRVRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_TransformSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_TransformSRVRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_RenderSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_RenderSRVRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_ViewCBVRootID], D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, Ce_ViewCBVRegister);

		// Texture sampler
		D3D12_DESCRIPTOR_RANGE textureSamplerRange{};
		CreateDescriptorRange(textureSamplerRange, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, Ce_OpaqueDrawTexSMPRegister);

		// Range for material buffer (pixel shader only)
		D3D12_DESCRIPTOR_RANGE materialSrvRange[Ce_OpaqueDrawPSExclusiveSRVsRangeCount]{};
		CreateDescriptorRange(materialSrvRange[Ce_OpaqueDrawPSMaterialSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_OpaqueDrawPSMaterialSRVRegister);

		// Range for textures
		D3D12_DESCRIPTOR_RANGE textureSrvsRange{};
		CreateDescriptorRange(textureSrvsRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_OpaqueDrawTexDescriptorCount, Ce_OpaqueDrawTexRegister);

		// ROOT PARAMS
		D3D12_ROOT_PARAMETER opaqueDrawRootParams[Ce_OpaqueDrawRootParameterCount]{};
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawExclusiveSRVsRootID], opaqueSrvRanges, Ce_OpaqueDrawExclusiveSRVsRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterPushConstants(opaqueDrawRootParams[Ce_OpaqueDrawObjIDRootID], Ce_OpaqueDrawObjIDConstantRegister, 0, Ce_OpaqueDrawObjIDConstant32BitCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawTexSMPRootID], &textureSamplerRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawMatSRVRootID], materialSrvRange, Ce_OpaqueDrawPSExclusiveSRVsRangeCount, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[Ce_OpaqueDrawTexSRVRootID], &textureSrvsRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);

		// OPAQUE DRAW ROOT
		if (!CreateRootSignature(device, context.m_opaqueDrawRoot.ReleaseAndGetAddressOf(), Ce_OpaqueDrawRootParameterCount, opaqueDrawRootParams, D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED))
		{
			BLIT_ERROR("Failed to create opaque root signature");
			return 0;
		}

		// Range for descriptor table for SRVs that are allocated in the exclusive region of the heap for this root
		D3D12_DESCRIPTOR_RANGE drawCullSrvRanges[Ce_DrawCullSRVsRangeCount]{};
		CreateDescriptorRange(drawCullSrvRanges[Ce_DrawCullDrawCmdUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawCullDrawCmdUAVRegister);
		CreateDescriptorRange(drawCullSrvRanges[Ce_DrawCullDrawCmdCountUAVRegister], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawCullDrawCmdCountUAVRegister);
		CreateDescriptorRange(drawCullSrvRanges[Ce_DrawCullLODSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_DrawCullLODSRVRegister);

		// ROOT PARAMS
		D3D12_ROOT_PARAMETER drawCullRootParameters[Ce_DrawCullRootParameterCount]{};
		CreateRootParameterDescriptorTable(drawCullRootParameters[Ce_DrawCullExclusiveSRVsRootID], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterDescriptorTable(drawCullRootParameters[Ce_DrawCullSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterPushConstants(drawCullRootParameters[Ce_DrawCullDrawCountConstantRootID], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);

		// DRAW CULL ROOT
		if (!CreateRootSignature(device, context.m_drawCullRoot.ReleaseAndGetAddressOf(), Ce_DrawCullRootParameterCount, drawCullRootParameters))
		{
			BLIT_ERROR("Failed to create draw cull root signature");
			return 0;
		}

		D3D12_ROOT_PARAMETER resetShaderRootParameter{};
		CreateRootParameterDescriptorTable(resetShaderRootParameter, drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);

		// DRAW COUNT RESET ROOT
		if (!CreateRootSignature(device, context.m_drawCountResetRoot.ReleaseAndGetAddressOf(), 1, &resetShaderRootParameter))
		{
			BLIT_ERROR("Failed to create draw count reset shader push constant");
			return 0;
		}

		// DRAW INST ADDITIONAL
		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			// Range for inst buffer
			D3D12_DESCRIPTOR_RANGE instanceBufferRange{};
			CreateDescriptorRange(instanceBufferRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_OpaqueDrawInstInstUAVRegister);

			// ROOT PARAMS
			D3D12_ROOT_PARAMETER opaqueDrawInstRootParams[Ce_OpaqueDrawInstRootParameterCount]{};
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstExclusiveSRVsRootID], opaqueSrvRanges, Ce_OpaqueDrawExclusiveSRVsRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
			CreateRootParameterPushConstants(opaqueDrawInstRootParams[Ce_OpaqueDrawObjIDRootID], Ce_OpaqueDrawObjIDConstantRegister, 0, Ce_OpaqueDrawObjIDConstant32BitCount, D3D12_SHADER_VISIBILITY_VERTEX);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstTexSMPRootID], &textureSamplerRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstMatSRVRootID], materialSrvRange, Ce_OpaqueDrawPSExclusiveSRVsRangeCount, D3D12_SHADER_VISIBILITY_PIXEL);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstTexSRVRootID], &textureSrvsRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
			CreateRootParameterDescriptorTable(opaqueDrawInstRootParams[Ce_OpaqueDrawInstInstSRVRootID], &instanceBufferRange, 1, D3D12_SHADER_VISIBILITY_VERTEX);

			// OPAQUE DRAW INST ROOT
			if (!CreateRootSignature(device, context.m_opaqueDrawInstRoot.ReleaseAndGetAddressOf(), Ce_OpaqueDrawInstRootParameterCount, opaqueDrawInstRootParams, D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED))
			{
				BLIT_ERROR("Failed to create opaque root signature");
				return 0;
			}

			// Draw cull inst additional ranges
			D3D12_DESCRIPTOR_RANGE drawCullInstRanges[Ce_DrawCullInstSRVsRangeCount]{};
			CreateDescriptorRange(drawCullInstRanges[Ce_DrawCullInstInstIdsxUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawCullInstInstIdsxUAVRegister);
			CreateDescriptorRange(drawCullInstRanges[Ce_DrawCullInstInstCounterUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawCullInstInstCounterUAVRegister);

			D3D12_ROOT_PARAMETER drawInstCullRootParameters[Ce_DrawCullInstRootParameterCount]{};
			CreateRootParameterDescriptorTable(drawInstCullRootParameters[Ce_DrawCullInstExclusiveSRVsRootID], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawInstCullRootParameters[Ce_DrawCullInstSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(drawInstCullRootParameters[Ce_DrawCullInstDrawCountConstantRootID], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawInstCullRootParameters[Ce_DrawCullInstAdditionalSRVsRootID], drawCullInstRanges, Ce_DrawCullInstSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);

			// DRAW CULL INST ROOT
			if (!CreateRootSignature(device, context.m_drawCullInstRoot.ReleaseAndGetAddressOf(), Ce_DrawCullInstRootParameterCount, drawInstCullRootParameters))
			{
				BLIT_ERROR("Failed to create draw cull inst root signature");
				return 0;
			}
		}

		// DRAW OCC ADDITIONAL
		if constexpr (BlitzenCore::Ce_OcclusionCulling)
		{
			// Additional Draw visibility srv 
			D3D12_DESCRIPTOR_RANGE drawVisibilityBufferRange{};
			CreateDescriptorRange(drawVisibilityBufferRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_DrawOccFirstDrawVisUAVRegister);

			D3D12_ROOT_PARAMETER drawOccFirstRootParameters[Ce_DrawOccFirstRootParameterCount]{};
			CreateRootParameterDescriptorTable(drawOccFirstRootParameters[Ce_DrawOccFirstExclusiveSRVsRootId], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccFirstRootParameters[Ce_DrawOccFirstSharedSRVsRootId], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(drawOccFirstRootParameters[Ce_DrawOccFirstDrawCountRootId], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccFirstRootParameters[Ce_DrawOccFirstDrawVisUAVRootId], &drawVisibilityBufferRange, 1, D3D12_SHADER_VISIBILITY_ALL);

			// DRAW OCC FIRST PASS ROOT
			if (!CreateRootSignature(device, context.m_drawOccFirstRoot.ReleaseAndGetAddressOf(), Ce_DrawOccFirstRootParameterCount, drawOccFirstRootParameters))
			{
				BLIT_ERROR("Failed to create draw occ first root signature");
				return 0;
			}

			// Additional Depth pyramid srv
			D3D12_DESCRIPTOR_RANGE depthPyramidCullRange{};
			CreateDescriptorRange(depthPyramidCullRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_DrawOccLateHI_Z_MapSRVRegister);

			D3D12_ROOT_PARAMETER drawOccLateRootParameters[Ce_DrawOccLateRootParameterCount]{};
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateExclusiveSRVsRootId], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateSharedSRVsRootId], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(drawOccLateRootParameters[Ce_DrawOccLateDrawCountRootId], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateDrawVisUAVRootId], &drawVisibilityBufferRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(drawOccLateRootParameters[Ce_DrawOccLateHI_Z_MapRootId], &depthPyramidCullRange, 1, D3D12_SHADER_VISIBILITY_ALL);

			// DRAW OCC LATE PASS ROOT
			if (!CreateRootSignature(device, context.m_drawOccLateRoot.ReleaseAndGetAddressOf(), Ce_DrawOccLateRootParameterCount, drawOccLateRootParameters))
			{
				BLIT_ERROR("Failed to create late cull (occlusion culling) root parameter");
				return 0;
			}
		}

		if constexpr (CE_DX12_BUILD_HI_Z_MAP)
		{
			// Additional for hi_z_map generation
			D3D12_DESCRIPTOR_RANGE depthPyramidUAVRange{};
			CreateDescriptorRange(depthPyramidUAVRange, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_HI_Z_MapUAVRegister);

			D3D12_DESCRIPTOR_RANGE depthPyramidSRVRange{};
			CreateDescriptorRange(depthPyramidSRVRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_HI_Z_MapSRVRegister);

			D3D12_ROOT_PARAMETER depthPyramidGenParameters[Ce_HI_Z_MapRootParameterCount]{};
			CreateRootParameterDescriptorTable(depthPyramidGenParameters[Ce_HI_Z_MapUAVRootID], &depthPyramidUAVRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(depthPyramidGenParameters[Ce_HI_Z_MapSRVRootID], &depthPyramidSRVRange, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(depthPyramidGenParameters[Ce_HI_Z_MapMipLvlConstantRootID], Ce_HI_Z_MapMipLvlConstantRegister, 0, Ce_HI_Z_MapMipLvlContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);

			// HI_Z MAP ROOT
			if (!CreateRootSignature(device, context.m_HI_Z_MapRoot.ReleaseAndGetAddressOf(), Ce_HI_Z_MapRootParameterCount, depthPyramidGenParameters))
			{
				BLIT_ERROR("Failed to create depth pyramid root parameter");
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_DESCRIPTOR_RANGE clusterDispatchAdditionalViewRanges[Ce_ClusterDispatchAdditionalViewsRangeCount]{};
			CreateDescriptorRange(clusterDispatchAdditionalViewRanges[Ce_ClusterCullCmdUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_ClusterCullCmdUAVRegister);
			CreateDescriptorRange(clusterDispatchAdditionalViewRanges[Ce_ClusterCullCounterUAVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_ClusterCullCounterUAVRegister);
			CreateDescriptorRange(clusterDispatchAdditionalViewRanges[Ce_ClusterCullGroupDataUAVRangerID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, Ce_ClusterCullGroupDataUAVRegister);
			CreateDescriptorRange(clusterDispatchAdditionalViewRanges[Ce_ClusterCullClusterVtxsSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_ClusterCullClusterVtxsSRVRegister);
			CreateDescriptorRange(clusterDispatchAdditionalViewRanges[Ce_ClusterCullClusterSpheresSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_ClusterCullClusterSpheresSRVRegister);
			CreateDescriptorRange(clusterDispatchAdditionalViewRanges[Ce_ClusterCullClusterConesSRVRangeID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_ClusterCullClusterConesSRVRegister);

			D3D12_DESCRIPTOR_RANGE depthPyramidCullRange{};
			CreateDescriptorRange(depthPyramidCullRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, Ce_DrawOccLateHI_Z_MapSRVRegister);

			D3D12_ROOT_PARAMETER clusterCullRootParameters[Ce_ClusterCullRootParameterCount]{};
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullExclusiveSRVsRootID], drawCullSrvRanges, Ce_DrawCullSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullSharedSRVsRootID], sharedSrvRanges, Ce_SharedSRVsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(clusterCullRootParameters[Ce_ClusterCullDrawCountRootID], Ce_DrawCullDrawCountContantRegister, 0, Ce_DrawCullDrawCountContant32BitCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullAdditionalViewsRootID], clusterDispatchAdditionalViewRanges, Ce_ClusterDispatchAdditionalViewsRangeCount, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(clusterCullRootParameters[Ce_ClusterCullHI_Z_MapSrvRootID], &depthPyramidCullRange, 1, D3D12_SHADER_VISIBILITY_ALL);

			if (!CreateRootSignature(device, context.m_clusterCullRoot.ReleaseAndGetAddressOf(), Ce_ClusterCullRootParameterCount, clusterCullRootParameters))
			{
				BLIT_ERROR("Failed to create cluster cull root signature");
				return 0;
			}
		}

		// success
		return 1;
    }

	uint8_t CreateCmdSignatures(ID3D12Device* device, PipelineContext& ctx)
	{
		// Draw command
		D3D12_INDIRECT_ARGUMENT_DESC indirectDescs[2]{};
		indirectDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		// Object id root constant
		indirectDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		indirectDescs[0].Constant.DestOffsetIn32BitValues = 0;
		indirectDescs[0].Constant.Num32BitValuesToSet = Ce_OpaqueDrawObjIDConstant32BitCount;
		indirectDescs[0].Constant.RootParameterIndex = Ce_OpaqueDrawObjIDRootID;

		D3D12_COMMAND_SIGNATURE_DESC sigDesc{};
		sigDesc.NodeMask = 0;
		sigDesc.NumArgumentDescs = 2;
		sigDesc.pArgumentDescs = indirectDescs;
		sigDesc.ByteStride = sizeof(IndirectDrawCmd);

		// regular opaque draw cmd signature
		HRESULT opaqueCmdRes{ device->CreateCommandSignature(&sigDesc, ctx.m_opaqueDrawRoot.Get(), IID_PPV_ARGS(ctx.m_opaqueDrawCmdSign.ReleaseAndGetAddressOf())) };
		if (FAILED(opaqueCmdRes))
		{
			BLIT_ERROR("Failed to create opaque draw command signature");
			return LOG_ERROR_MESSAGE_AND_RETURN(opaqueCmdRes);
		}

		// opaque draw inst cmd signature
		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			HRESULT opaqueInstCmdRes{ device->CreateCommandSignature(&sigDesc, ctx.m_opaqueDrawInstRoot.Get(), IID_PPV_ARGS(ctx.m_opaqueDrawInstCmdSign.ReleaseAndGetAddressOf())) };
			if (FAILED(opaqueInstCmdRes))
			{
				BLIT_ERROR("Failed to create opaque draw instanced command signature");
				return LOG_ERROR_MESSAGE_AND_RETURN(opaqueInstCmdRes);
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_INDIRECT_ARGUMENT_DESC clusterDispatchIndirectDesc{};

			clusterDispatchIndirectDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

			D3D12_COMMAND_SIGNATURE_DESC clusterSigDesc{};
			clusterSigDesc.NodeMask = 0;
			clusterSigDesc.NumArgumentDescs = 1;
			clusterSigDesc.pArgumentDescs = &clusterDispatchIndirectDesc;
			clusterSigDesc.ByteStride = sizeof(ClusterDispatchCmd);

			HRESULT clusterDispatchCmdRes{ device->CreateCommandSignature(&clusterSigDesc, nullptr, IID_PPV_ARGS(ctx.m_clusterCullCmdSign.ReleaseAndGetAddressOf())) };
			if (FAILED(clusterDispatchCmdRes))
			{
				BLIT_ERROR("Failed to create cluster dispatch command signature");
				return LOG_ERROR_MESSAGE_AND_RETURN(clusterDispatchCmdRes);
			}
		}

		// success
		return 1;
	}

	uint8_t CreatePipelines(ID3D12Device* device, PipelineContext& context)
	{
		if (!CreateOpaqueGraphicsPipeline(device, context))
		{
			BLIT_ERROR("Failed to create opaque grahics pipeline");
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_drawCountResetRoot.Get(), context.m_drawCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawCountReset.cs.hlsl.bin"))
		{
			BLIT_ERROR("Failed to create drawCountReset.cs shader program");
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_drawCullRoot.Get(), context.m_drawCullPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawCull.cs.hlsl.bin"))
		{
			BLIT_ERROR("Failed to create drawCull.cs shader program");
			return 0;
		}

		if constexpr (CE_DX12_BUILD_HI_Z_MAP)
		{
			if (!CreateComputeShaderProgram(device, context.m_HI_Z_MapRoot.Get(), context.m_HI_Z_MapPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/hi_z_map.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create depthPyramid.cs shader program");
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_OcclusionCulling)
		{
			if (!CreateComputeShaderProgram(device, context.m_drawOccFirstRoot.Get(), context.m_drawOccFirstPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccFirst.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_drawOccLateRoot.Get(), context.m_drawOccLatePso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccLate.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawOccLate.cs shader program");
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_DrawTemporalOcclusion)
		{
			if (!CreateComputeShaderProgram(device, context.m_drawOccLateRoot.Get(), context.m_drawOccTemporalPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccTemporal.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawOccTemporal.cs shader program");
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			if (!CreateComputeShaderProgram(device, context.m_drawCullInstRoot.Get(), context.m_drawCullInstPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawInstCull.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawInstCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_drawCullInstRoot.Get(), context.m_drawInstCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawInstCountReset.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create drawInstCountReset.cs shader program");
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterCullCmdResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullCmdReset.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterDispatchCmdReset.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterCullDispatchPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullDispatch.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterCullDispatch.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterCullCmdSetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullCmdSet.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterCullCmdSet.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterCullPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCull.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterCull.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterCullBatchCmdPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullBatchCmd.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterCullBatchCmd.cs shader program");
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_clusterCullRoot.Get(), context.m_clusterCullBatchPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullBatch.cs.hlsl.bin"))
			{
				BLIT_ERROR("Failed to create clusterCullBatch.cs shader program");
				return 0;
			}
		}

		return 1;
	}

	uint8_t CreateRWResources(ID3D12Device* device, ReadWriteResources* rwResourcesArray, uint32_t swapchainWidth, uint32_t swapchainHeight)
	{
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& rwResources = rwResourcesArray[i];

			if (!CreateBuffer(device, rwResources.m_viewBuffer.buffer.ReleaseAndGetAddressOf(), sizeof(BlitzenEngine::CameraViewData), D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
			{
				BLIT_ERROR("%s: Failed to create view data buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			void* pViewDataMapping{ nullptr };
			HRESULT viewDataBufferMappingRes{ rwResources.m_viewBuffer.buffer->Map(0, nullptr, &pViewDataMapping) };
			if (FAILED(viewDataBufferMappingRes))
			{
				BLIT_ERROR("%s: Failed to map view data buffer pointer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return LOG_ERROR_MESSAGE_AND_RETURN(viewDataBufferMappingRes);
			}

			rwResources.m_viewBuffer.pData = reinterpret_cast<BlitzenEngine::CameraViewData*>(pViewDataMapping);
			if (!rwResources.m_viewBuffer.pData)
			{
				BLIT_ERROR("%s: View data mapped pointer initialized as null", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateSSBO<BlitzenEngine::MeshTransform>(device, rwResources.m_transformBuffer.m_ssbo, BLIT_MAX_WORLD_TRANSFORM_COUNT))
			{
				BLIT_ERROR("%s: Failed to create transform buffer resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			UINT64 indirectBufferSize{ Ce_IndirectDrawCmdBufferSize * sizeof(IndirectDrawCmd) };
			if (!CreateSSBO<IndirectDrawCmd>(device, rwResources.m_drawCmdBuffer, indirectBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("%s: Failed to create indirect draw cmd buffer resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateSSBO<uint32_t>(device, rwResources.m_drawCmdCounterBuffer, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("%s: Failed to create draw cmd counter resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			// DRAW OCC MODE
			if constexpr (BlitzenCore::Ce_OcclusionCulling)
			{
				if (!CreateSSBO<uint32_t>(device, rwResources.m_drawVisBuffer, BLIT_MAX_WORLD_RENDERS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
				{
					BLIT_ERROR("%s: Failed to create draw visibility buffer resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}
			}

			if constexpr (CE_DX12_BUILD_HI_Z_MAP)
			{
				if (!CreateDepthPyramidResource(device, rwResources.m_HI_Z, swapchainWidth, swapchainHeight))
				{
					BLIT_ERROR("%s: Failed to create HI Z MAP", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}
			}

			// DRAW CULL INST MODE
			if constexpr (BlitzenCore::Ce_InstanceCulling)
			{
				// MIGHT NOT BE PRE ALLOCATING ANYTHING FOR INSTANCING MODE, BUT NOT SURE YET
			}

			// CLUSTER CULL MODE
			if constexpr (BlitzenCore::Ce_BuildClusters)
			{
				if (CreateSSBO<ClusterDispatchCmd>(device, rwResources.m_clusterDispatchBuffer, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
				{
					BLIT_ERROR("%s: Failed to create cluster dispatch buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}

				if (CreateSSBO<BlitzenCore::FAT_BOOL>(device, rwResources.m_clusterVisibilityBuffer, Ce_ClusterGroupDataBufferSize * 64, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
				{
					BLIT_ERROR("%s: Failed to create cluster visibility buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}

				if (CreateSSBO<ClusterGroupData>(device, rwResources.m_clusterGroupDataBuffer, Ce_ClusterGroupDataBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
				{
					BLIT_ERROR("%s: Failed to create cluster group data buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}
			}
		}
		// Success
		return 1;
	}

	uint8_t CreateROResources(ID3D12Device* device, ReadOnlyResources& roResources)
	{
		if (CreateSSBO<BlitzenEngine::VtxPos>(device, roResources.m_vtxPosBuffer, BlitzenCore::Ce_MaxWorldVertexCount) == 0)
		{
			BLIT_ERROR("%s: Failed to create vertex positions buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (CreateSSBO<BlitzenEngine::VtxNormals>(device, roResources.m_vtxNrmBuffer, BlitzenCore::Ce_MaxWorldVertexCount) == 0)
		{
			BLIT_ERROR("%s: Failed to create vertex normals buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (CreateSSBO<BlitzenEngine::VtxTangents>(device, roResources.m_vtxTangentBuffer, BlitzenCore::Ce_MaxWorldVertexCount) == 0)
		{
			BLIT_ERROR("%s: Failed to create vertex tangents buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (CreateSSBO<BlitzenEngine::VtxTexCoords>(device, roResources.m_vtxTexCoordBuffer, BlitzenCore::Ce_MaxWorldVertexCount) == 0)
		{
			BLIT_ERROR("%s: Failed to create vertex tex coords buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (CreateIndexBuffer(device, roResources.m_idxBuffer, BlitzenCore::Ce_MaxWorldVertexIndicesCount) == 0)
		{
			BLIT_ERROR("%s: Failed to create index buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		UINT64 surfaceBufferSize{  };
		if (CreateSSBO<BlitzenEngine::PrimitiveSurface>(device, roResources.m_surfaceBuffer, BlitzenCore::Ce_MaxMeshPrimitivesCount) == 0)
		{
			BLIT_ERROR("%s: Failed to create surface buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (CreateSSBO<BlitzenEngine::RenderObject>(device, roResources.m_renderBuffer, BLIT_MAX_WORLD_RENDERS) == 0)
		{
			BLIT_ERROR("%s: Failed to create render buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		
		if (CreateSSBO<BlitzenEngine::LodData>(device, roResources.m_LODBuffer, BlitzenEngine::CE_MAX_LOD_COUNT) == 0)
		{
			BLIT_ERROR("%s: Failed to create lod buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (CreateSSBO<BlitzenEngine::Material>(device, roResources.m_matBuffer, BlitzenCore::Ce_MaxMaterialCount) == 0)
		{
			BLIT_ERROR("%s: Failed to create material buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (CreateSSBO<BlitzenEngine::ClusterVertices>(device, roResources.m_clusterVtxsBuffer, BlitzenEngine::CE_MAX_WORLD_CLUSTER_COUNT) == 0)
			{
				BLIT_ERROR("%s: Failed to create cluster vertices buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (CreateSSBO<BlitzenEngine::ClusterSphere>(device, roResources.m_clusterSpheresBuffer, BlitzenEngine::CE_MAX_WORLD_CLUSTER_COUNT) == 0)
			{
				BLIT_ERROR("%s: Failed to create cluster spheres buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (CreateSSBO<BlitzenEngine::ClusterCone>(device, roResources.m_clusterConesBuffer, BlitzenEngine::CE_MAX_WORLD_CLUSTER_COUNT) == 0)
			{
				BLIT_ERROR("%s: Failed to create cluster cones buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		// Success
		return 1;
	}
}

#endif