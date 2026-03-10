#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

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
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 baseColor = texture(texture_diffuse, TexCoord).rgb;
    vec3 specColor = texture(texture_specular, TexCoord).rgb;

    // Directional light
    vec3 dirLightDir = normalize(-dirLight.direction);
    vec3 dirResult = CalcPhong(
        dirLightDir,
        dirLight.ambient,
        dirLight.diffuse,
        dirLight.specular,
        norm,
        viewDir,
        baseColor,
        specColor
    );

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
