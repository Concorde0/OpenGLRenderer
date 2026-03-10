#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gMaterial;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} fs_in;

uniform sampler2D albedoMap;
uniform bool useAlbedoMap;

uniform vec3 albedoColor;
uniform float metallic;
uniform float roughness;
uniform float ao;

void main()
{
    vec3 albedo = albedoColor;
    if (useAlbedoMap) {
        vec3 texAlbedo = texture(albedoMap, fs_in.TexCoord).rgb;
        albedo *= pow(texAlbedo, vec3(2.2));
    }

    gPosition = fs_in.FragPos;
    gNormal = normalize(fs_in.Normal);
    gAlbedo = vec4(albedo, 1.0);
    gMaterial = vec4(clamp(metallic, 0.0, 1.0), clamp(roughness, 0.04, 1.0), clamp(ao, 0.0, 1.0), 1.0);
}
