#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMaterial;

uniform vec3 viewPos;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};
uniform PointLight light;

void main()
{
    vec3 fragPos = texture(gPosition, TexCoord).rgb;
    vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
    vec3 albedo = texture(gAlbedo, TexCoord).rgb;
    vec3 material = texture(gMaterial, TexCoord).rgb;

    float metallic = material.r;
    float roughness = material.g;
    float ao = material.b;

    vec3 ambient = 0.05 * albedo * ao;

    vec3 lightDir = normalize(light.position - fragPos);
    float distanceToLight = length(light.position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distanceToLight + 0.032 * distanceToLight * distanceToLight);
    vec3 radiance = light.color * light.intensity * attenuation;

    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = NdotL * albedo * radiance * (1.0 - 0.5 * metallic);

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float shininess = mix(128.0, 8.0, roughness);
    float spec = pow(max(dot(normal, halfDir), 0.0), shininess);
    vec3 specColor = mix(vec3(0.04), albedo, metallic);
    vec3 specular = spec * specColor * radiance;

    vec3 color = ambient + diffuse + specular;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
