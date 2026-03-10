#ifndef PBR_MATERIAL_H
#define PBR_MATERIAL_H

#include <algorithm>
#include <string>

#include <glm/glm.hpp>

#include "Shader.h"
#include "Texture.h"

class PBRMaterial
{
public:
    PBRMaterial()
        : m_AlbedoFactor(1.0f, 1.0f, 1.0f),
          m_MetallicFactor(0.0f),
          m_RoughnessFactor(0.5f),
          m_AOFactor(1.0f),
          m_AlbedoMap(nullptr),
          m_NormalMap(nullptr),
          m_MetallicMap(nullptr),
          m_RoughnessMap(nullptr),
          m_AOMap(nullptr)
    {
    }

    void SetAlbedoFactor(const glm::vec3& value) { m_AlbedoFactor = value; }
    void SetMetallicFactor(float value) { m_MetallicFactor = std::clamp(value, 0.0f, 1.0f); }
    void SetRoughnessFactor(float value) { m_RoughnessFactor = std::clamp(value, 0.04f, 1.0f); }
    void SetAOFactor(float value) { m_AOFactor = std::clamp(value, 0.0f, 1.0f); }

    void SetAlbedoMap(const Texture* texture) { m_AlbedoMap = texture; }
    void SetNormalMap(const Texture* texture) { m_NormalMap = texture; }
    void SetMetallicMap(const Texture* texture) { m_MetallicMap = texture; }
    void SetRoughnessMap(const Texture* texture) { m_RoughnessMap = texture; }
    void SetAOMap(const Texture* texture) { m_AOMap = texture; }

    void Bind(const Shader& shader, const std::string& uniformPrefix = "material", unsigned int baseTextureUnit = 0) const
    {
        shader.SetVec3(uniformPrefix + ".albedoFactor", m_AlbedoFactor);
        shader.SetFloat(uniformPrefix + ".metallicFactor", m_MetallicFactor);
        shader.SetFloat(uniformPrefix + ".roughnessFactor", m_RoughnessFactor);
        shader.SetFloat(uniformPrefix + ".aoFactor", m_AOFactor);

        unsigned int unit = baseTextureUnit;
        BindOptionalMap(shader, uniformPrefix, "albedoMap", "useAlbedoMap", m_AlbedoMap, unit);
        BindOptionalMap(shader, uniformPrefix, "normalMap", "useNormalMap", m_NormalMap, unit);
        BindOptionalMap(shader, uniformPrefix, "metallicMap", "useMetallicMap", m_MetallicMap, unit);
        BindOptionalMap(shader, uniformPrefix, "roughnessMap", "useRoughnessMap", m_RoughnessMap, unit);
        BindOptionalMap(shader, uniformPrefix, "aoMap", "useAOMap", m_AOMap, unit);
    }

private:
    static void BindOptionalMap(const Shader& shader,
                                const std::string& prefix,
                                const std::string& samplerName,
                                const std::string& toggleName,
                                const Texture* texture,
                                unsigned int& unit)
    {
        const std::string samplerPath = prefix + "." + samplerName;
        const std::string togglePath = prefix + "." + toggleName;

        if (texture != nullptr && texture->IsLoaded()) {
            texture->Bind(unit);
            shader.SetInt(samplerPath, static_cast<int>(unit));
            shader.SetBool(togglePath, true);
            ++unit;
            return;
        }

        shader.SetBool(togglePath, false);
    }

    glm::vec3 m_AlbedoFactor;
    float m_MetallicFactor;
    float m_RoughnessFactor;
    float m_AOFactor;

    const Texture* m_AlbedoMap;
    const Texture* m_NormalMap;
    const Texture* m_MetallicMap;
    const Texture* m_RoughnessMap;
    const Texture* m_AOMap;
};

#endif
