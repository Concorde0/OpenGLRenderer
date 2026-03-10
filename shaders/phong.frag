#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;
in mat3 TBN;

struct Material {
    vec3 ka;
    vec3 kd;
    vec3 ks;
    float shininess;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform PointLight pointLight;
uniform DirLight dirLight;
uniform SpotLight spotLight;

uniform vec3 viewPos;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;
uniform sampler2D shadowMap;
uniform sampler2D texture_normal;
uniform float shadowBiasSlope;
uniform float shadowBiasMin;
uniform int shadowPcfRadius;

const int MAX_PCF_RADIUS = 4;

float ShadowCalculation(vec3 normal, vec3 lightDir)
{
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) {
        return 0.0;
    }

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(shadowBiasSlope * (1.0 - dot(normal, lightDir)), shadowBiasMin);

    int radius = clamp(shadowPcfRadius, 0, MAX_PCF_RADIUS);

    float shadow = 0.0;
    int sampleCount = 0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -MAX_PCF_RADIUS; x <= MAX_PCF_RADIUS; ++x) {
        for (int y = -MAX_PCF_RADIUS; y <= MAX_PCF_RADIUS; ++y) {
            if (abs(x) > radius || abs(y) > radius) {
                continue;
            }
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
            sampleCount += 1;
        }
    }
    shadow /= max(float(sampleCount), 1.0);

    return shadow;
}

vec3 CalcPhong(vec3 lightDir, vec3 lightAmbient, vec3 lightDiffuse, vec3 lightSpecular,
               vec3 norm, vec3 viewDir, vec3 baseColor, vec3 specColor)
{
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient  = lightAmbient * material.ka * baseColor;
    vec3 diffuse  = lightDiffuse * diff * material.kd * baseColor;
    vec3 specular = lightSpecular * spec * material.ks * specColor;
    return ambient + diffuse + specular;
}

void main()
{
    vec3 tangentNormal = texture(texture_normal, TexCoord).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;
    vec3 norm = normalize(TBN * tangentNormal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 baseColor = texture(texture_diffuse, TexCoord).rgb;
    vec3 specColor = texture(texture_specular, TexCoord).rgb;

    // Directional light
    vec3 dirLightDir = normalize(-dirLight.direction);
    float dirDiff = max(dot(norm, dirLightDir), 0.0);
    vec3 dirReflectDir = reflect(-dirLightDir, norm);
    float dirSpec = pow(max(dot(viewDir, dirReflectDir), 0.0), material.shininess);
    vec3 dirAmbient = dirLight.ambient * material.ka * baseColor;
    vec3 dirDiffuse = dirLight.diffuse * dirDiff * material.kd * baseColor;
    vec3 dirSpecular = dirLight.specular * dirSpec * material.ks * specColor;
    float shadow = ShadowCalculation(norm, dirLightDir);
    vec3 dirResult = dirAmbient + (1.0 - shadow) * (dirDiffuse + dirSpecular);

    // Point light (distance attenuation)
    vec3 pointLightDir = normalize(pointLight.position - FragPos);
    float pointDist = length(pointLight.position - FragPos);
    float pointAtten = 1.0 / (pointLight.constant + pointLight.linear * pointDist + pointLight.quadratic * pointDist * pointDist);
    vec3 pointResult = CalcPhong(
        pointLightDir,
        pointLight.ambient,
        pointLight.diffuse,
        pointLight.specular,
        norm,
        viewDir,
        baseColor,
        specColor
    ) * pointAtten;

    // Spotlight (angle smooth + distance attenuation)
    vec3 spotLightDir = normalize(spotLight.position - FragPos);
    float theta = dot(spotLightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    float spotDist = length(spotLight.position - FragPos);
    float spotAtten = 1.0 / (spotLight.constant + spotLight.linear * spotDist + spotLight.quadratic * spotDist * spotDist);
    vec3 spotResult = CalcPhong(
        spotLightDir,
        spotLight.ambient,
        spotLight.diffuse,
        spotLight.specular,
        norm,
        viewDir,
        baseColor,
        specColor
    ) * intensity * spotAtten;

    FragColor = vec4(dirResult + pointResult + spotResult, 1.0);
}
