#include "blitRenderingResources.h"
#include "Renderer/Interface/blitRenderer.h"
// Not necessary since I have my own math library, but I use glm for random values in textures
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"

#include <string>

namespace BlitzenEngine
{
    RenderingResources::RenderingResources()
    {
        DefineMaterial(BlitML::vec4{ 0.1f }, 65.f, "dds_texture_default", "unknown", "loaded_material");
        LoadMeshFromObj(m_meshContext, "Assets/Meshes/bunny.obj", BlitzenCore::Ce_DefaultMeshName);
    }

    void RenderingResources::DefineMaterial(const BlitML::vec4& diffuseColor, float shininess, 
        const char* diffuseMapName, const char* specularMapName, const char* materialName)
    {
        auto& mat = m_materials[m_materialCount];

        mat.albedoTag = 0; //pResources->textureTable[diffuseMapName].textureTag;
        mat.normalTag = 0; //pResources->textureTable[specularMapName].textureTag;
        mat.specularTag = 0;
        mat.emissiveTag = 0;

        mat.materialId = m_materialCount;

        m_materialTable.Insert(materialName, mat);
        m_materialCount++;
    }

    
}