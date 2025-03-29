#pragma once
#include "renderingResourcesTypes.h"
#include "Core/blitzenContainerLibrary.h"
#include "Game/blitCamera.h"

#include "Cgltf/cgltf.h"
// Algorithms for building meshlets, loading LODs, optimizing vertex caches etc.
// https://github.com/zeux/meshoptimizer
#include "Meshoptimizer/meshoptimizer.h"

namespace BlitzenEngine
{
    // Rendering resources container
    class RenderingResources
    {
    public:

        inline static RenderingResources* GetRenderingResources() { return s_pResources; }
        RenderingResources::RenderingResources(void* rendererData);

    public:

        // Holds all textures. No dynamic allocation.
        TextureStats textures[ce_maxTextureCount];
        BlitCL::HashMap<TextureStats> textureTable;
        size_t textureCount = 0;

        // Holds all materials. No dynamic allocation. Includes hashmap for separate access
        Material materials[ce_maxMaterialCount];
        BlitCL::HashMap<Material> materialTable;
        size_t materialCount = 0;


        /*
            Per primitive data
        */
        // Holds all the primitives / surfaces
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface> surfaces;

        // Holds the vertex count of each primitive. This does not need to be passed to shader for now. But I do need it for ray tracing
        BlitCL::DynamicArray<uint32_t> primitiveVertexCounts;

        // Holds the vertices of all the primitives that were loaded
        BlitCL::DynamicArray<Vertex> vertices;

        // Holds the indices of all the primitives that were loaded
        BlitCL::DynamicArray<uint32_t> indices;

        // Holds all clusters for all the primitives that were loaded
        BlitCL::DynamicArray<Meshlet> meshlets;

        // Holds the meshlet indices to index into the clusters above
        BlitCL::DynamicArray<uint32_t> meshletData;


        /*
            Per instance data
        */
        // Holds the transforms of every render object / instance on the scene 
        BlitCL::DynamicArray<MeshTransform> transforms;

        // Holds all the render objects / primitives. They index into one primitive and one transform each
        RenderObject renders[ce_maxRenderObjects];
        uint32_t renderObjectCount = 0; 

        RenderObject onpcReflectiveRenderObjects[ce_maxONPC_Objects];
        uint32_t onpcReflectiveRenderObjectCount = 0;
        
        // Holds the meshes that were loaded for the scene. Meshes are a collection of primitives.
        Mesh meshes[ce_maxMeshCount];
        BlitCL::HashMap<Mesh> meshMap;
        size_t meshCount = 0;



    public:

        // Takes a mesh id and adds a render object based on that ID and a transform
		inline uint32_t AddRenderObjectsFromMesh(uint32_t meshId, 
            const BlitzenEngine::MeshTransform& transform)
		{
            BlitzenEngine::Mesh& mesh = meshes[meshId];
			if (renderObjectCount + mesh.surfaceCount >= ce_maxRenderObjects)
			{
				BLIT_ERROR("Adding renderer objects from mesh will exceed the render object count")
				return 0;
			}

            uint32_t transformId = static_cast<uint32_t>(transforms.GetSize());
            transforms.PushBack(transform);

			for (uint32_t i = mesh.firstSurface; i < mesh.firstSurface + mesh.surfaceCount; ++i)
			{
                RenderObject& object = renders[renderObjectCount++];

                object.surfaceId = i;
                object.transformId = transformId;
			}
			
			return transformId;
		}

        uint8_t LoadMeshFromObj(const char* filename, const char* meshName);

    public:

        // Load a texture from a specified file. Assumes DDS format
		template<class RENDERER>
		bool LoadTextureFromFile(const char* filename, const char* texName,
			RENDERER* pRenderer)
		{
			// Don't go over the texture limit, might want to throw a warning here
			if (textureCount >= ce_maxTextureCount)
			{
				BLIT_WARN("Max texture count: %i, has been reached", ce_maxTextureCount)
				BLIT_ERROR("Texture from file: %s, failed to load", filename)
				return 0;
			}

			TextureStats& texture = textures[textureCount];
			// If texture upload to the renderer succeeds, the texture count is incremented and the function returns successfully
			if (pRenderer->UploadTexture(texture.pTextureData, filename))
			{
				textureCount++;
				return true;
			}
            else
            {
                BLIT_ERROR("Texture from file: %s, failed to load", filename)
                return false;
            }
		}

        // Loads a scene from gltf file
		template <class RENDERER>
        bool LoadGltfScene(const char* path, RENDERER* pRenderer)
        {
            if (renderObjectCount >= ce_maxRenderObjects)
            {
                BLIT_WARN("BLITZEN_MAX_DRAW_OBJECT already reached, no more geometry can be loaded. GLTF LOADING FAILED!")
                    return 0;
            }

            cgltf_options options = {};

            cgltf_data* pData = nullptr;

            cgltf_result res = cgltf_parse_file(&options, path, &pData);
            if (res != cgltf_result_success)
            {
                BLIT_WARN("Failed to load gltf file: %s", path)
                    return 0;
            }

            // Struct only used for cgltf_data pointer, will call its specialized free function when done
            struct CgltfScope
            {
                cgltf_data* pData;

                inline ~CgltfScope() { cgltf_free(pData); }
            };

            CgltfScope cgltfScope{ pData };

            res = cgltf_load_buffers(&options, pData, path);
            if (res != cgltf_result_success)
            {
                return 0;
            }

            res = cgltf_validate(pData);
            if (res != cgltf_result_success)
            {
                return 0;
            }

            BLIT_INFO("Loading GLTF scene from file: %s", path)

                // Defining a lambda here for finding the accessor since it will probably not be used outside of this
                auto findAccessor = [](const cgltf_primitive* prim, cgltf_attribute_type type, cgltf_int index = 0) {
                for (size_t i = 0; i < prim->attributes_count; ++i)
                {
                    const cgltf_attribute& attr = prim->attributes[i];
                    if (attr.type == type && attr.index == index)
                        return attr.data;
                }

                // This might seem redundant but compilation fails if I do not do this
                cgltf_accessor* scratch = nullptr;
                return scratch;
                };

            // Before loading textures save the previous texture size, to use for indexing
            size_t previousTextureSize = textureCount;

            BLIT_INFO("Loading textures")

                // I had to fold and use the STL
                BlitCL::DynamicArray<std::string> texturePaths(pData->textures_count);
            for (size_t i = 0; i < pData->textures_count; ++i)
            {
                cgltf_texture* texture = &(pData->textures[i]);
                if (!texture->image)
                    break;

                cgltf_image* image = texture->image;
                if (!image->uri)
                    break;

                std::string ipath = path;
                std::string::size_type pos = ipath.find_last_of('/');
                if (pos == std::string::npos)
                    ipath = "";
                else
                    ipath = ipath.substr(0, pos + 1);

                std::string uri = image->uri;
                uri.resize(cgltf_decode_uri(&uri[0]));
                std::string::size_type dot = uri.find_last_of('.');

                if (dot != std::string::npos)
                    uri.replace(dot, uri.size() - dot, ".dds");

                texturePaths[i] = ipath + uri;
            }

            for (auto path : texturePaths)
            {
                LoadTextureFromFile(path.c_str(), path.c_str(), pRenderer);
            }

            BLIT_INFO("Loading materials")

                // Saves the previous material count
                size_t previousMaterialCount = materialCount;
            // Creates one BlitzenEngine::Material for each material in the gltf
            for (size_t i = 0; i < pData->materials_count; ++i)
            {
                cgltf_material& cgltf_mat = pData->materials[i];

                Material& mat = materials[materialCount++];
                mat.materialId = static_cast<uint32_t>(materialCount - 1);

                mat.albedoTag = cgltf_mat.pbr_metallic_roughness.base_color_texture.texture ?
                    uint32_t(previousTextureSize + cgltf_texture_index(pData,
                        cgltf_mat.pbr_metallic_roughness.base_color_texture.texture
                    ))
                    : cgltf_mat.pbr_specular_glossiness.diffuse_texture.texture ?
                    uint32_t(previousTextureSize + cgltf_texture_index(pData,
                        cgltf_mat.pbr_specular_glossiness.diffuse_texture.texture
                    ))
                    : 0;

                mat.normalTag =
                    cgltf_mat.normal_texture.texture ?
                    uint32_t(previousTextureSize + cgltf_texture_index(pData,
                        cgltf_mat.normal_texture.texture
                    ))
                    : 0;

                mat.specularTag =
                    cgltf_mat.pbr_specular_glossiness.specular_glossiness_texture.texture ?
                    uint32_t(previousTextureSize + cgltf_texture_index(pData,
                        cgltf_mat.pbr_specular_glossiness.specular_glossiness_texture.texture
                    ))
                    : 0;

                mat.emissiveTag =
                    cgltf_mat.emissive_texture.texture ?
                    uint32_t(previousTextureSize + cgltf_texture_index(pData,
                        cgltf_mat.emissive_texture.texture
                    ))
                    : 0;

            }

            BLIT_INFO("Loading meshes and primitives")

            // The surface indices is a list of the first surface of each mesh. Used to create the render object struct
            BlitCL::DynamicArray<uint32_t> surfaceIndices(pData->meshes_count);

            for (size_t i = 0; i < pData->meshes_count; ++i)
            {
                // Get the current mesh
                const cgltf_mesh& mesh = pData->meshes[i];

                // Find the first surface of the current mesh. 
                // It is important for the mesh struct and to save the data for later to create the render objects
                uint32_t firstSurface = static_cast<uint32_t>(surfaces.GetSize());

                // Give the new mesh the surface that it owns and increment the mesh count
                meshes[meshCount].firstSurface = firstSurface;
                meshes[meshCount].surfaceCount = static_cast<uint32_t>(mesh.primitives_count);
                meshCount++;

                // Pass the first surface here so that it can be accessed by the nodes
                surfaceIndices[i] = firstSurface;

                for (size_t j = 0; j < mesh.primitives_count; ++j)
                {
                    const cgltf_primitive& prim = mesh.primitives[j];

                    // Skip primitives that are not triangles
                    if (prim.type != cgltf_primitive_type_triangles || !prim.indices)
                        continue;

                    size_t vertexCount = prim.attributes[0].data->count;

                    BlitCL::DynamicArray<Vertex> vertices(vertexCount);

                    // Will temporarily hold each aspect of the vertices (pos, tangent, normals, uvMaps) from the primitive
                    BlitCL::DynamicArray<float> scratch(vertexCount * 4);

                    if (const cgltf_accessor* pos = cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0))
                    {
                        // No choice but to assert here, as some data might already have been loaded
                        BLIT_ASSERT(cgltf_num_components(pos->type) == 3);

                        cgltf_accessor_unpack_floats(pos, scratch.Data(), vertexCount * 3);
                        for (size_t j = 0; j < vertexCount; ++j)
                        {
                            vertices[j].position = BlitML::vec3(scratch[j * 3 + 0], scratch[j * 3 + 1], scratch[j * 3 + 2]);
                        }
                    }

                    if (const cgltf_accessor* nrm = cgltf_find_accessor(&prim, cgltf_attribute_type_normal, 0))
                    {
                        BLIT_ASSERT(cgltf_num_components(nrm->type) == 3);

                        cgltf_accessor_unpack_floats(nrm, scratch.Data(), vertexCount * 3);
                        for (size_t j = 0; j < vertexCount; ++j)
                        {
                            vertices[j].normalX = static_cast<uint8_t>(scratch[j * 3 + 0] * 127.f + 127.5f);
                            vertices[j].normalY = static_cast<uint8_t>(scratch[j * 3 + 1] * 127.f + 127.5f);
                            vertices[j].normalZ = static_cast<uint8_t>(scratch[j * 3 + 2] * 127.f + 127.5f);
                        }
                    }

                    if (const cgltf_accessor* tang = cgltf_find_accessor(&prim, cgltf_attribute_type_tangent, 0))
                    {
                        BLIT_ASSERT(cgltf_num_components(tang->type) == 4)

                            cgltf_accessor_unpack_floats(tang, scratch.Data(), vertexCount * 4);
                        for (size_t j = 0; j < vertexCount; ++j)
                        {
                            vertices[j].tangentX = uint8_t(scratch[j * 4 + 0] * 127.f + 127.5f);
                            vertices[j].tangentY = uint8_t(scratch[j * 4 + 1] * 127.f + 127.5f);
                            vertices[j].tangentZ = uint8_t(scratch[j * 4 + 2] * 127.f + 127.5f);
                            vertices[j].tangentW = uint8_t(scratch[j * 4 + 3] * 127.f + 127.5f);
                        }
                    }

                    if (const cgltf_accessor* tex = cgltf_find_accessor(&prim, cgltf_attribute_type_texcoord, 0))
                    {
                        BLIT_ASSERT(cgltf_num_components(tex->type) == 2);
                        cgltf_accessor_unpack_floats(tex, scratch.Data(), vertexCount * 2);
                        for (size_t j = 0; j < vertexCount; ++j)
                        {
                            vertices[j].uvX = meshopt_quantizeHalf(scratch[j * 2 + 0]);
                            vertices[j].uvY = meshopt_quantizeHalf(scratch[j * 2 + 1]);
                        }
                    }

                    BlitCL::DynamicArray<uint32_t> indices(prim.indices->count);
                    cgltf_accessor_unpack_indices(prim.indices, indices.Data(), 4, indices.GetSize());

                    LoadPrimitiveSurface(vertices, indices);

                    // Get the material index and pass it to the surface if there is material index
                    if (prim.material)
                    {
                        surfaces.Back().materialId =
                            materials[previousMaterialCount +
                            cgltf_material_index(pData, prim.material)].materialId;

                        if (prim.material->alpha_mode != cgltf_alpha_mode_opaque)
                            surfaces.Back().postPass = 1;
                    }
                }
            }

            BLIT_INFO("Loading scene nodes")

            for (size_t i = 0; i < pData->nodes_count; ++i)
            {
                const cgltf_node* node = &(pData->nodes[i]);
                if (node->mesh)
                {
                    // Gets the model matrix
                    float matrix[16];
                    cgltf_node_transform_world(node, matrix);

                    // Uses these float arrays to hold the decomposed matrix
                    float translation[3];
                    float rotation[4];
                    float scale[3];

                    // Decomposes the model transform and tranlates the data to the engine's transform structure
                    BlitML::decomposeTransform(translation, rotation, scale, matrix);
                    MeshTransform transform;
                    transform.pos = BlitML::vec3(translation[0], translation[1], translation[2]);
                    transform.scale = BlitML::Max(scale[0], BlitML::Max(scale[1], scale[2]));
                    transform.orientation = BlitML::quat(rotation[0], rotation[1], rotation[2], rotation[3]);

                    // TODO: better warnings for non-uniform or negative scale

                    // Hold the offset of the first surface of the mesh and the transform id to give to the render objects
                    uint32_t surfaceOffset = surfaceIndices[cgltf_mesh_index(pData, node->mesh)];
                    uint32_t transformId = static_cast<uint32_t>(transforms.GetSize());

                    for (unsigned int j = 0; j < node->mesh->primitives_count; ++j)
                    {
                        // If the gltf goes over BLITZEN_MAX_DRAW_OBJECTS after already loading resources, I have no choice but to assert
                        BLIT_ASSERT_MESSAGE(renderObjectCount <= ce_maxRenderObjects,
                            "While Loading a GLTF, \
                            additional geometry was loaded \
                            which surpassed the BLITZEN_MAX_DRAW_OBJECT limiter value"
                        )

                            RenderObject& current = renders[renderObjectCount];
                        current.surfaceId = surfaceOffset + j;
                        current.transformId = transformId;

                        // Increment the render object count
                        renderObjectCount++;
                    }

                    transforms.PushBack(transform);
                }
            }

            return true;
        }


    private:

        // Loads a single primitive and adds it to the global array
        void LoadPrimitiveSurface(BlitCL::DynamicArray<Vertex>& vertices,
            BlitCL::DynamicArray<uint32_t>& indices);

        // Generates clusters for a given array of vertices and indices
        size_t GenerateClusters(BlitCL::DynamicArray<Vertex>& vertices,
            BlitCL::DynamicArray<uint32_t>& indices);

        void GenerateTangents(BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices,
            BlitCL::DynamicArray<uint32_t>& indices);

        // Generates LODs for the vertices of a given surface
        void AutomaticLevelOfDetailGenration(PrimitiveSurface& surface, 
            BlitCL::DynamicArray<Vertex>& surfaceVertices, 
            BlitCL::DynamicArray<uint32_t>& surfaceIndices
        );

        void GenerateBoundingSphere(PrimitiveSurface& surface,
            BlitCL::DynamicArray<Vertex>& surfaceVertices,
            BlitCL::DynamicArray<uint32_t>& surfaceIndices
        );


    private:

		static RenderingResources* s_pResources;
    };

    // Draw context needs to be given to draw frame function, so that it can update uniform values
    struct DrawContext
    {
        // The camera holds crucial data for the shaders. 
        // The structs in the shader try to be aligned with the structs in the camera
        BlitzenEngine::Camera* pCamera;

        RenderingResources* pResources;

        // Debug values
        uint8_t bOcclusionCulling;
        uint8_t bLOD;

        // Tells the renderer if it should do call Oblique near clipping shaders
        uint8_t bOnpc = 0;

        inline DrawContext(BlitzenEngine::Camera* pCam, 
            RenderingResources* pr, 
            uint8_t onpc = 0, uint8_t bOC = 1, uint8_t bLod = 1
        ) : pCamera(pCam), pResources(pr), bOcclusionCulling{bOC}, bLOD{bLod}, bOnpc{onpc} 
        {}
    };





    // Sets random transform
    inline void RandomizeTransform(MeshTransform& transform, float multiplier, float scale)
    {
        transform.pos = BlitML::vec3(
            (float(rand()) / RAND_MAX) * multiplier,//x 
            (float(rand()) / RAND_MAX) * multiplier,//y
            (float(rand()) / RAND_MAX) * multiplier
        );

        transform.scale = scale;

        transform.orientation = BlitML::QuatFromAngleAxis(
            BlitML::vec3(
                (float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1
            ),
            BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), // Angle 
            0
        );
    }

    void DefineMaterial(RenderingResources* pResources, const BlitML::vec4& diffuseColor, 
        float shininess, const char* diffuseMapName, 
        const char* specularMapName, const char* materialName);

    // Placeholder to load some default resources while testing the systems
    void LoadTestGeometry(RenderingResources* pResources);

    // Functions for testing aspects of the renderer
    void LoadGeometryStressTest(RenderingResources* pResources);
    void CreateObliqueNearPlaneClippingTestObject(RenderingResources* pResources);

    template<typename MT>
    void CreateDynamicObjectRendererTest(RenderingResources* pResources, MT* pManager)
    {
        constexpr uint32_t ce_objectCount = 1000;
        if (pResources->renderObjectCount + ce_objectCount > ce_maxRenderObjects)
        {
            BLIT_ERROR("Could not add dynamic object renderer test, object count exceeds limit")
                return;
        }

        for (size_t i = 0; i < ce_objectCount; ++i)
        {
            BlitzenEngine::MeshTransform transform;
            RandomizeTransform(transform, 100.f, 1.f);

            if (!pManager->AddObject<BlitzenEngine::ClientTest>(
                pResources, transform, 1, "kitten"
            ))
                return;
        }
    }

    // Takes the command line arguments to form a scene (this is pretty ill formed honestly)
    template<class RT, class MT>
    void CreateSceneFromArguments(int argc, char** argv,
        RenderingResources* pResources, RT* pRenderer, MT* pManager, uint8_t& bOnpc)
    {
        if (argc > 1)
        {
            LoadTestGeometry(pResources);
            #if defined(BLIT_DYNAMIC_OBJECT_TEST)
                CreateDynamicObjectRendererTest(pResources, pManager);
            #endif
            // Special argument. Loads heavy scene to stress test the culling
            if (strcmp(argv[1], "RenderingStressTest") == 0)
            {
                LoadGeometryStressTest(pResources);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }

            // Special argument. Test oblique near-plane clipping technique. Not working yet.
            else if (strcmp(argv[1], "ONPC_ReflectionTest") == 0)
            {
                LoadTestGeometry(pResources);
                CreateObliqueNearPlaneClippingTestObject(pResources);
                bOnpc = 1;

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }

            // If there are no special arguments everything is treated as a filepath for a gltf scene
            else
            {
                for (int32_t i = 1; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }
        }
    }
}