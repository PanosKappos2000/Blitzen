#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Core/DbLog/blitLogger.h"
#include "BlitCL/blitDynamicArr.h"

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
		D3D12_DESCRIPTOR_RANGE opaqueSrvRanges[CE_VERTEX_ODS_RANGE_COUNT]{};
		CreateDescriptorRange(opaqueSrvRanges[CE_VERTEX_ODS_VTXPOS_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_VTX_POSITIONS_REGISTER);
		CreateDescriptorRange(opaqueSrvRanges[CE_VERTEX_ODS_VTXNORMAL_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_VTX_NORMALS_REGISTER);
		CreateDescriptorRange(opaqueSrvRanges[CE_VERTEX_ODS_VTXTANGENT_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_VTX_TANGENTS_REGISTER);
		CreateDescriptorRange(opaqueSrvRanges[CE_VERTEX_ODS_VTXTEXCOORD_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_VTX_TEXCOORDS_REGISTER);

		// Ranges for descrtiptor table for SRVs that are allocated in the shared section of the heap
		D3D12_DESCRIPTOR_RANGE sharedSrvRanges[CE_GLOBAL_DESCRIPTOR_RANGE_COUNT]{};
		CreateDescriptorRange(sharedSrvRanges[CE_GDESC_RENDER_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_RENDER_BUFFER_REGISTER);
		CreateDescriptorRange(sharedSrvRanges[CE_GDESC_TRANSFORM_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_SHADER_TRANSFORM_BUFFER_REGISTER);
		CreateDescriptorRange(sharedSrvRanges[CE_GDESC_SURFACE_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_SURFACE_BUFFER_REGISTER);
		CreateDescriptorRange(sharedSrvRanges[CE_GDESC_VIEW_ID], D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, BLIT_HLSL_VIEW_DATA_REGISTER);

		// Texture sampler
		D3D12_DESCRIPTOR_RANGE textureSamplerRange{};
		CreateDescriptorRange(textureSamplerRange, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, BLIT_HLSL_TEX_SAMPLER_REGISTER);

		// Range for material buffer (pixel shader only)
		D3D12_DESCRIPTOR_RANGE materialSrvRange[CE_PIXEL_ODS_RANGE_COUNT]{};
		CreateDescriptorRange(materialSrvRange[CE_PIXEL_ODS_MATERIAL_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_MATERIAL_BUFFER_REGISTER);

		// Range for textures
		D3D12_DESCRIPTOR_RANGE textureSrvsRange{};
		CreateDescriptorRange(textureSrvsRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, CE_TEXTURE_DESCRIPTOR_COUNT, BLIT_HLSL_TEXTURE_DESCRIPTORS_REGISTER);

		D3D12_DESCRIPTOR_RANGE terrainDescriptors[CE_VERTEX_TERRAIN_RANGE_COUNT]{};
		CreateDescriptorRange(terrainDescriptors[CE_VERTEX_TERRAIN_VTXPOS_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_TERRAIN_VERTEX_POSITIONS_REGISTER);

		// ROOT PARAMS
		BlitCL::DynamicArray<D3D12_ROOT_PARAMETER> opaqueDrawRootParams{ CE_GRAPHICS_ODS_ROOT_COUNT, {} };
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[CE_GRAPHICS_ODS_VTX_TABLE_ID], opaqueSrvRanges, CE_VERTEX_ODS_RANGE_COUNT, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[CE_GRAPHICS_ODS_PS_TABLE_ID], materialSrvRange, CE_PIXEL_ODS_RANGE_COUNT, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[CE_GRAPHICS_ODS_GLOBAL_ID], sharedSrvRanges, CE_GLOBAL_DESCRIPTOR_RANGE_COUNT, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[CE_GRAPHICS_ODS_TEXSMP_ID], &textureSamplerRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[CE_GRAPHICS_ODS_TEX_ID], &textureSrvsRange, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		CreateRootParameterPushConstants(opaqueDrawRootParams[CE_GRAPHICS_ODS_STATIC_OBJIDX_ID], BLIT_HLSL_OPAQUE_STATIC_OBJID_REGISTER, 0, CE_DRAW_OBJ_ID_32_BIT_COUNT, 
			D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterPushConstants(opaqueDrawRootParams[CE_GRAPHICS_ODS_DYNAMIC_OBJIDX_ID], BLIT_HLSL_OPAQUE_DYNAMIC_OBJID_REGISTER, 0, CE_DRAW_OBJ_ID_32_BIT_COUNT,
			D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(opaqueDrawRootParams[CE_GRAPHICS_TERRAIN_VERTICES_ID], terrainDescriptors, BLIT_ARRAY_SIZE(terrainDescriptors), D3D12_SHADER_VISIBILITY_VERTEX);

		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			// Additional roots
			D3D12_ROOT_PARAMETER clusterDrawParameter{};
			CreateRootParameterPushConstants(clusterDrawParameter, BLIT_HLSL_CLUSTER_OBJID_REGISTER, 0, CE_DRAW_OBJ_ID_32_BIT_COUNT, D3D12_SHADER_VISIBILITY_VERTEX);

			opaqueDrawRootParams.PushBack(clusterDrawParameter);
			context.m_clusterObjidxContantRootID = (UINT)opaqueDrawRootParams.GetSize();
		}

		// OPAQUE DRAW ROOT
		if (!CreateRootSignature(device, context.m_graphicsRoot.ReleaseAndGetAddressOf(), (UINT)opaqueDrawRootParams.GetSize(), opaqueDrawRootParams.Data(),
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED))
		{
			BLIT_ERROR("%s: Failed to create graphics root signature", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		// CULLING DESCRIPTORS
		D3D12_DESCRIPTOR_RANGE cullGlobalRanges[CE_CULL_GLOBAL_RANGE_COUNT]{};
		CreateDescriptorRange(cullGlobalRanges[CE_CULL_GLOBAL_BOUNDS_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_BOUNDING_SPHERE_REGISTER);
		CreateDescriptorRange(cullGlobalRanges[CE_CULL_GLOBAL_LOD_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_LOD_BUFFER_REGISTER);

		// CULLING OPAQUE STATIC ONLY
		D3D12_DESCRIPTOR_RANGE cullOSRanges[CE_CULL_OS_RANGE_COUNT]{};
		CreateDescriptorRange(cullOSRanges[CE_CULL_OS_DRAW_CMD_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_OPAQUE_STATIC_CMD_BUFFER_REGISTER);
		CreateDescriptorRange(cullOSRanges[CE_CULL_OS_DRAW_COUNTER_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_OPAQUE_STATIC_CMD_COUNTER_REGISTER);

		// HIERARCHICHAL Z BUFFER DESCRIPTOR
		D3D12_DESCRIPTOR_RANGE HI_Z_MAP_cullRange{};
		CreateDescriptorRange(HI_Z_MAP_cullRange, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_HI_Z_MAP_REGISTER);

		// ROOT PARAMS
		BlitCL::DynamicArray<D3D12_ROOT_PARAMETER> drawCullRootParameters{ CE_CULL_ROOT_PARAMETER_COUNT, {} };
		CreateRootParameterDescriptorTable(drawCullRootParameters[CE_CULL_ROOT_GLOBAL_ID], sharedSrvRanges, CE_GLOBAL_DESCRIPTOR_RANGE_COUNT, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterDescriptorTable(drawCullRootParameters[CE_CULL_ROOT_STATIC_TABLE_ID], cullOSRanges, CE_CULL_GLOBAL_RANGE_COUNT, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterDescriptorTable(drawCullRootParameters[CE_CULL_ROOT_CULL_GLOBAL_ID], cullGlobalRanges, CE_CULL_OS_RANGE_COUNT, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterDescriptorTable(drawCullRootParameters[CE_CULL_ROOT_HI_Z_MAP_ID], &HI_Z_MAP_cullRange, 1, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterPushConstants(drawCullRootParameters[CE_CULL_ROOT_STATIC_WORK_CONSTANT_ID], BLIT_HLSL_OPAQUE_STATIC_COUNT_CONSTANT_REGISTER, 0, CE_CULL_WORK_COUNT_CONSTANT_32_BIT_COUNT,
			D3D12_SHADER_VISIBILITY_ALL);

		// CULLING OPAQUE DYNAMIC ONLY
		D3D12_DESCRIPTOR_RANGE cullODRanges[CE_CULL_OD_RANGE_COUNT]{};
		CreateDescriptorRange(cullODRanges[CE_CULL_OD_DRAW_CMD_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_OPAQUE_DYNAMIC_CMD_BUFFER_REGISTER);
		CreateDescriptorRange(cullODRanges[CE_CULL_OD_DRAW_COUNTER_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_OPAQUE_DYNAMIC_CMD_COUNTER_REGISTER);
		CreateDescriptorRange(cullODRanges[CE_CULL_OD_MOVEMENT_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_CPU_TRANSFORM_BUFFER_REGISTER);
		CreateDescriptorRange(cullODRanges[CE_CULL_OD_TERRAIN_HEIGHT_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_TERRAIN_HEIGHT_BUFFER_REGISTER);

		// ROOT PARAMETER RECONFIGURATION
		CreateRootParameterDescriptorTable(drawCullRootParameters[CE_CULL_ROOT_DYNAMIC_TABLE_ID], cullODRanges, CE_CULL_OD_RANGE_COUNT, D3D12_SHADER_VISIBILITY_ALL);
		CreateRootParameterPushConstants(drawCullRootParameters[CE_CULL_ROOT_DYNAMIC_WORK_CONSTANT_ID], BLIT_HLSL_OPAQUE_DYNAMIC_COUNT_CONSTANT_REGISTER, 0, CE_CULL_WORK_COUNT_CONSTANT_32_BIT_COUNT,
			D3D12_SHADER_VISIBILITY_ALL);

		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			D3D12_DESCRIPTOR_RANGE cullInstRanges[CE_CULL_INST_RANGE_COUNT]{};
			CreateDescriptorRange(cullInstRanges[CE_CULL_INST_DRAW_CMD_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_INSTANCED_CMD_BUFFER_REGISTER);
			CreateDescriptorRange(cullInstRanges[CE_CULL_INST_DRAW_COUNTER_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_INSTANCED_CMD_COUNTER_REGISTER);

			// RECONFIGURE ROOT
			D3D12_ROOT_PARAMETER cullInstRangeParameter{};
			CreateRootParameterDescriptorTable(cullInstRangeParameter, cullInstRanges, CE_CULL_INST_RANGE_COUNT, D3D12_SHADER_VISIBILITY_ALL);
			drawCullRootParameters.PushBack(cullInstRangeParameter);
			context.m_cullInstTableRootID = (UINT)drawCullRootParameters.GetSize();

			D3D12_ROOT_PARAMETER cullInstWorkParameter{};
			CreateRootParameterPushConstants(cullInstWorkParameter, BLIT_HLSL_OPAQUE_INSTANCED_OBJID_REGISTER, 0, CE_CULL_WORK_COUNT_CONSTANT_32_BIT_COUNT,
				D3D12_SHADER_VISIBILITY_ALL);
			drawCullRootParameters.PushBack(cullInstWorkParameter);
			context.m_cullInstWorkRootID = (UINT)drawCullRootParameters.GetSize();
		}

		// DOUBLE PASS OCCLUSION
		if constexpr (BlitzenCore::CE_OCCLUSION_DOUBLE_PASS)
		{

		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			D3D12_DESCRIPTOR_RANGE clusterRanges[CE_CULL_CLUSTERS_RANGE_COUNT]{};
			CreateDescriptorRange(clusterRanges[CE_CULL_CLUSTERS_CMD_RANGE_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_CLUSTER_CMD_BUFFER_REGISTER);
			CreateDescriptorRange(clusterRanges[CE_CULL_CLUSTERS_CMD_COUNTER_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_CLUSTER_CMD_COUNTER_REGISTER);
			CreateDescriptorRange(clusterRanges[CE_CULL_CLUSTERS_GROUP_DATA_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_CLUSTER_GROUP_BUFFER_REGISTER);
			CreateDescriptorRange(clusterRanges[CE_CULL_CLUSTERS_GROUP_COUNTER_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_CLUSTER_COUNTER_BUFFER_REGISTER);
			CreateDescriptorRange(clusterRanges[CE_CULL_CLUSTERS_VISIBILITY_ID], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_CLUSTER_VISIBILITY_BUFFER_REGISTER);
			CreateDescriptorRange(clusterRanges[CE_CULL_CLUSTERS_VTXS_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_CLUSTER_VTXS_REGISTER);
			CreateDescriptorRange(clusterRanges[CE_CULL_CLUSTERS_SPHERES_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_CLUSTER_SPHERES_REGISTER);
			CreateDescriptorRange(clusterRanges[CE_CULL_CLUSTERS_CONES_ID], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_CLUSTER_CONES_REGISTER);

			D3D12_ROOT_PARAMETER clusterRangeParameter{};
			CreateRootParameterDescriptorTable(clusterRangeParameter, clusterRanges, CE_CULL_CLUSTERS_RANGE_COUNT, D3D12_SHADER_VISIBILITY_ALL);
			drawCullRootParameters.PushBack(clusterRangeParameter);
			context.m_clusterCullTableRootID = (UINT)drawCullRootParameters.GetSize();

			D3D12_ROOT_PARAMETER clusterContantParameter{};
			CreateRootParameterPushConstants(clusterContantParameter, BLIT_HLSL_CLUSTER_WORK_COUNT_CONSTANT_REGISTER, 0, CE_CULL_WORK_COUNT_CONSTANT_32_BIT_COUNT,
				D3D12_SHADER_VISIBILITY_ALL);
			drawCullRootParameters.PushBack(clusterContantParameter);
			context.m_clusterCullWorkRootID = (UINT)drawCullRootParameters.GetSize();
		}

		// DRAW CULL ROOT
		if (!CreateRootSignature(device, context.m_cullRoot.ReleaseAndGetAddressOf(), (UINT)drawCullRootParameters.GetSize(), drawCullRootParameters.Data()))
		{
			BLIT_ERROR("%s: Failed to create draw cull root signature", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		// HIERARCHICAL Z BUFFER DESCRIPTORS
		if (BlitzenCore::Ce_Build_HI_Z)
		{
			// Additional for hi_z_map generation
			D3D12_DESCRIPTOR_RANGE HI_Z_ouput{};
			CreateDescriptorRange(HI_Z_ouput, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, BLIT_HLSL_HI_Z_OUTPUT_REGISTER);

			D3D12_DESCRIPTOR_RANGE HI_Z_input{};
			CreateDescriptorRange(HI_Z_input, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, BLIT_HLSL_HI_Z_INPUT_REGISTER);

			D3D12_ROOT_PARAMETER depthPyramidGenParameters[CE_HI_Z_MAP_ROOT_COUNT]{};
			CreateRootParameterDescriptorTable(depthPyramidGenParameters[CE_HI_Z_MAP_OUTPUT_ID], &HI_Z_ouput, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterDescriptorTable(depthPyramidGenParameters[CE_HI_Z_MAP_INPUT_ID], &HI_Z_input, 1, D3D12_SHADER_VISIBILITY_ALL);
			CreateRootParameterPushConstants(depthPyramidGenParameters[CE_HI_Z_MAP_CONSTANT_ID], BLIT_HLSL_HI_Z_CONTANT_REGISTER, 0, CE_HI_Z_MAP_CONSTANT_32BIT_COUNT,
				D3D12_SHADER_VISIBILITY_ALL);

			// HI_Z MAP ROOT
			if (!CreateRootSignature(device, context.m_HI_Z_MapRoot.ReleaseAndGetAddressOf(), CE_HI_Z_MAP_ROOT_COUNT, depthPyramidGenParameters))
			{
				BLIT_ERROR("%s: Failed to create depth pyramid root parameter", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

#if !defined(NDEBUG)
		D3D12_ROOT_PARAMETER boundingSphereRootParameters[Ce_BoundingSphereRootParameterCount]{};
		CreateRootParameterSrv(boundingSphereRootParameters[Ce_BoundingSphereSphereRootParameterID], Ce_BoundingSphereSphereSRVRegister, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterCBV(boundingSphereRootParameters[Ce_BoundingSphereViewDataRootParameterID], Ce_BoundingSphereViewDataCBVRegister, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterPushConstants(boundingSphereRootParameters[Ce_BoundingSphereObjectIDRootParameterID], Ce_BoundingSphereObjectIDConstantRegister, 0, Ce_BoundingSphereObjectIDConstant32BitCount,
			D3D12_SHADER_VISIBILITY_VERTEX);

		if (!CreateRootSignature(device, context.m_boundingSphereRoot.ReleaseAndGetAddressOf(), Ce_BoundingSphereRootParameterCount, boundingSphereRootParameters))
		{
			BLIT_ERROR("%s: Failed to create bounding sphere root signature", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}
#endif

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
		indirectDescs[0].Constant.Num32BitValuesToSet = CE_DRAW_OBJ_ID_32_BIT_COUNT;
		indirectDescs[0].Constant.RootParameterIndex = CE_GRAPHICS_ODS_STATIC_OBJIDX_ID;

		D3D12_COMMAND_SIGNATURE_DESC sigDesc{};
		sigDesc.NodeMask = 0;
		sigDesc.NumArgumentDescs = 2;
		sigDesc.pArgumentDescs = indirectDescs;
		sigDesc.ByteStride = sizeof(IndirectDrawCmd);

		// regular opaque draw cmd signature
		HRESULT opaqueCmdRes{ device->CreateCommandSignature(&sigDesc, ctx.m_graphicsRoot.Get(), IID_PPV_ARGS(ctx.m_staticDrawCmdSignature.ReleaseAndGetAddressOf())) };
		if (FAILED(opaqueCmdRes))
		{
			BLIT_ERROR("%s: Failed to create opaque draw command signature", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return LOG_ERROR_MESSAGE_AND_RETURN(opaqueCmdRes);
		}

		indirectDescs[0].Constant.RootParameterIndex = CE_GRAPHICS_ODS_DYNAMIC_OBJIDX_ID;
		HRESULT dynamicCmdRes{ device->CreateCommandSignature(&sigDesc, ctx.m_graphicsRoot.Get(), IID_PPV_ARGS(ctx.m_dynamicDrawCmdSignature.ReleaseAndGetAddressOf())) };
		if (FAILED(dynamicCmdRes))
		{
			BLIT_ERROR("%s: Failed to create dynamic draw command signature", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return LOG_ERROR_MESSAGE_AND_RETURN(dynamicCmdRes);
		}

		D3D12_INDIRECT_ARGUMENT_DESC indirectDescsTerrain[1]{};
		indirectDescsTerrain[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		HRESULT terrainCmdRes{ device->CreateCommandSignature(&sigDesc, ctx.m_graphicsRoot.Get(), IID_PPV_ARGS(ctx.m_terrainDrawCmdSignature.ReleaseAndGetAddressOf())) };
		if (FAILED(terrainCmdRes))
		{
			BLIT_ERROR("%s: Failed to create terrain draw command signature", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return LOG_ERROR_MESSAGE_AND_RETURN(terrainCmdRes);
		}

		// opaque draw inst cmd signature
		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			//indirectDescs[0].Constant.RootParameterIndex = CE_GRAPHICS;
			HRESULT opaqueInstCmdRes{ device->CreateCommandSignature(&sigDesc, ctx.m_graphicsRoot.Get(), IID_PPV_ARGS(ctx.m_drawInstCmdSignature.ReleaseAndGetAddressOf())) };
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
		if (!CreateGraphicsPipelines(device, context))
		{
			BLIT_ERROR("%s: Failed to create graphics pipelines", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_opaqueStaticCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawCountReset.cs.hlsl.bin"))
		{
			BLIT_ERROR("%s: Failed to create drawCountReset.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_staticCullPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawCull.cs.hlsl.bin"))
		{
			BLIT_ERROR("%s: Failed to create drawCull.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_dynamicCullPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/dynamicCull.cs.hlsl.bin"))
		{
			BLIT_ERROR("%s: Failed to create dynamicCull.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_opaqueDynamicCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/dynamicCountReset.cs.hlsl.bin"))
		{
			BLIT_ERROR("%s: Failed to create dynamicCountReset.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_drawOccTemporalPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccTemporal.cs.hlsl.bin"))
		{
			BLIT_ERROR("%s: Failed to create drawOccTemporal.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if constexpr (BlitzenCore::Ce_Build_HI_Z)
		{
			if (!CreateComputeShaderProgram(device, context.m_HI_Z_MapRoot.Get(), context.m_HI_Z_MapPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/hi_z_map.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create depthPyramid.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		if constexpr (BlitzenCore::CE_OCCLUSION_DOUBLE_PASS)
		{
			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_drawOccFirstPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccFirst.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create drawCull.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_drawOccLatePso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawOccLate.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create drawOccLate.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_drawCullInstPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawInstCull.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create drawInstCull.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_instanceCountResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/drawInstCountReset.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create drawInstCountReset.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_clusterCullCmdResetPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullCmdReset.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create clusterDispatchCmdReset.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_clusterCullDispatchPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullDispatch.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create clusterCullDispatch.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_clusterCullPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCull.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create clusterCull.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_clusterCullBatchCmdPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullBatchCmd.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create clusterCullBatchCmd.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateComputeShaderProgram(device, context.m_cullRoot.Get(), context.m_clusterCullBatchPso.ReleaseAndGetAddressOf(), "HlslShaders/CS/clusterCullBatch.cs.hlsl.bin"))
			{
				BLIT_ERROR("%s: Failed to create clusterCullBatch.cs shader program", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

#if !defined(NDEBUG)
		if(!CreateBoundingSphereDebugDrawPipeline(device, context))
		{
			BLIT_ERROR("%s: Failed to create bounding sphere debug draw pipeline", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}
#endif

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

			if (!CreateSSBO<BlitzenEngine::MeshTransform>(device, rwResources.m_transformBuffer, BLIT_MAX_WORLD_TRANSFORM_COUNT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("%s: Failed to create transform buffer resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateSSBO<BlitzenEngine::CPU_TRANSFORM>(device, rwResources.m_movementBuffer, BLIT_MAX_WORLD_VARIABLE_COUNT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("%s: Failed to create movement buffer resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			UINT64 indirectBufferSize{ BLIT_MAX_STATIC_DRAW_COMMANDS * sizeof(IndirectDrawCmd) };
			if (!CreateSSBO<IndirectDrawCmd>(device, rwResources.m_staticDrawCmdBuffer, indirectBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("%s: Failed to create indirect draw command buffer resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateSSBO<uint32_t>(device, rwResources.m_staticDrawCmdCounter, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("%s: Failed to create draw command counter resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			UINT64 dynamicCmdSize{ BLIT_MAX_DYNAMIC_DRAW_COMMANDS * sizeof(IndirectDrawCmd) };
			if (!CreateSSBO<IndirectDrawCmd>(device, rwResources.m_dynamicDrawCmdBuffer, dynamicCmdSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("%s: Failed to create dynamic indirect draw command buffer resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}

			if (!CreateSSBO<uint32_t>(device, rwResources.m_dynamicDrawCmdCounter, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("%s: Failed to create dynamic draw command counter resource", BlitzenCore::CE_DX12_SYSTEM_NAME);
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
				if (CreateSSBO<IndirectDrawCmd>(device, rwResources.m_clusterDrawCmdBuffer, BlitzenCore::Ce_MaxLodCountPerSurface, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
				{
					BLIT_ERROR("%s: Failed to create instance draw commands buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}
			}

			// CLUSTER CULL MODE
			if constexpr (BlitzenCore::Ce_BuildClusters)
			{
				if (CreateSSBO<IndirectDrawCmd>(device, rwResources.m_clusterDrawCmdBuffer, BLIT_MAX_CLUSTER_DRAW_COMMANDS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
				{
					BLIT_ERROR("%s: Failed to create cluster draw commands buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}

				if (CreateSSBO<uint32_t>(device, rwResources.m_clusterDrawCounter, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
				{
					BLIT_ERROR("%s: Failed to create cluster draw counter buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}

				if (CreateSSBO<ClusterDispatchCmd>(device, rwResources.m_clusterGroupCounter, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
				{
					BLIT_ERROR("%s: Failed to create cluster dispatch buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
					return 0;
				}

				if (CreateSSBO<BlitzenCore::FAT_BOOL>(device, rwResources.m_clusterVisibilityBuffer, Ce_ClusterGroupDataBufferSize * BLIT_MAX_CLUSTERS_PER_GROUP, 
					D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
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

		if (CreateSSBO<BlitzenEngine::VtxPos>(device, roResources.m_terrainVtxBuffer, BlitzenEngine::CE_MAX_TERRAIN_VERTICES) == 0)
		{
			BLIT_ERROR("%s: Failed to create terrain vertex buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (CreateIndexBuffer(device, roResources.m_terrainIdxBuffer, BlitzenEngine::CE_MAX_TERRAIN_INDICES) == 0)
		{
			BLIT_ERROR("%s: Failed to create terrain index buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (CreateSSBO<float>(device, roResources.m_terrainHeightBuffer, BLIT_MAX_HEIGHT_MAP_DATA_COUNT) == 0)
		{
			BLIT_ERROR("%s: Failed to create terrain height buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
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

		if (CreateSSBO<BlitzenEngine::BoundingSphere>(device, roResources.m_boundingSpheres, BlitzenEngine::CE_MAX_WORLD_BOUNDING_SPHERE_COUNT) == 0)
		{
			BLIT_ERROR("%s: Failed to create bounding sphere buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
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

		if (!CreateStaging(device, roResources.CPU_MOVING_OBJECT_BUFFER, BLIT_MAX_WORLD_VARIABLE_COUNT, (BlitzenEngine::CPU_TRANSFORM*)nullptr))
		{
			BLIT_ERROR("%s: Failed to create moving object peristently mapped buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return 0;
		}

		if (!CreateReadback(device, roResources.GPU_MOVING_OBJECT_READBACK, BLIT_MAX_WORLD_VARIABLE_COUNT))
		{
			BLIT_ERROR("%s: Failed to create moving object GPU readback buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
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

			if (CreateIndexBuffer(device, roResources.m_clusterIdxBuffer, BlitzenCore::Ce_MaxWorldVertexIndicesCount) == 0)
			{
				BLIT_ERROR("%s: Failed to create cluster indices buffer", BlitzenCore::CE_DX12_SYSTEM_NAME);
				return 0;
			}
		}

		// Success
		return 1;
	}
}

#endif