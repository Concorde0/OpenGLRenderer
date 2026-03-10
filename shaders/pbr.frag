#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in mat3 TBN;

const float PI = 3.14159265359;

struct PBRMaterial {
    vec3 albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    float aoFactor;

    bool useAlbedoMap;
    bool useNormalMap;
    bool useMetallicMap;
    bool useRoughnessMap;
    bool useAOMap;

    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
};

uniform PBRMaterial material;
uniform DirectionalLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;

uniform vec3 camPos;
uniform bool enableIBL;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float numerator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;

    return numerator / max(denominator, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float numerator = NdotV;
    float denominator = NdotV * (1.0 - k) + k;

    return numerator / max(denominator, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 GetNormalWS()
{
    if (!material.useNormalMap) {
        return normalize(Normal);
    }

    vec3 tangentNormal = texture(material.normalMap, TexCoord).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;
    return normalize(TBN * tangentNormal);
}

vec3 EvaluateDirectBRDF(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness)
{
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{
    vec3 albedo = material.albedoFactor;
    if (material.useAlbedoMap) {
        vec3 texAlbedo = texture(material.albedoMap, TexCoord).rgb;
        albedo *= pow(texAlbedo, vec3(2.2));
    }

    float metallic = material.metallicFactor;
    if (material.useMetallicMap) {
        metallic *= texture(material.metallicMap, TexCoord).r;
    }
    metallic = clamp(metallic, 0.0, 1.0);

    float roughness = material.roughnessFactor;
    if (material.useRoughnessMap) {
        roughness *= texture(material.roughnessMap, TexCoord).r;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    float ao = material.aoFactor;
    if (material.useAOMap) {
        ao *= texture(material.aoMap, TexCoord).r;
    }
    ao = clamp(ao, 0.0, 1.0);

    vec3 N = GetNormalWS();
    vec3 V = normalize(camPos - FragPos);

    vec3 Lo = vec3(0.0);

    vec3 Ld = normalize(-dirLight.direction);
    vec3 dirRadiance = dirLight.color * dirLight.intensity;
    Lo += EvaluateDirectBRDF(N, V, Ld, dirRadiance, albedo, metallic, roughness);

    vec3 Lp = normalize(pointLight.position - FragPos);
    float pointDistance = length(pointLight.position - FragPos);
    float pointAttenuation = 1.0 / (pointLight.constant + pointLight.linear * pointDistance + pointLight.quadratic * pointDistance * pointDistance);
    vec3 pointRadiance = pointLight.color * pointLight.intensity * pointAttenuation;
    Lo += EvaluateDirectBRDF(N, V, Lp, pointRadiance, albedo, metallic, roughness);

    vec3 Ls = normalize(spotLight.position - FragPos);
    float theta = dot(Ls, normalize(-spotLight.direction));
    float epsilon = max(spotLight.cutOff - spotLight.outerCutOff, 0.0001);
    float spotEdge = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    float spotDistance = length(spotLight.position - FragPos);
    float spotAttenuation = 1.0 / (spotLight.constant + spotLight.linear * spotDistance + spotLight.quadratic * spotDistance * spotDistance);
    vec3 spotRadiance = spotLight.color * spotLight.intensity * spotEdge * spotAttenuation;
    Lo += EvaluateDirectBRDF(N, V, Ls, spotRadiance, albedo, metallic, roughness);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 ambient = vec3(0.03) * albedo * ao;
    if (enableIBL) {
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * albedo;

        vec3 R = reflect(-V, N);
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

        ambient = (kD * diffuseIBL + specularIBL) * ao;
    }

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
