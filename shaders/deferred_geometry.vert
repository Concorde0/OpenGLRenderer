#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 4) in vec4 aInstanceCol0;
layout (location = 5) in vec4 aInstanceCol1;
layout (location = 6) in vec4 aInstanceCol2;
layout (location = 7) in vec4 aInstanceCol3;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useInstancing;

void main()
{
    mat4 instanceModel = mat4(aInstanceCol0, aInstanceCol1, aInstanceCol2, aInstanceCol3);
    mat4 objectModel = useInstancing ? (model * instanceModel) : model;

    vec4 worldPos = objectModel * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;
    vs_out.Normal = normalize(mat3(transpose(inverse(objectModel))) * aNormal);
    vs_out.TexCoord = aTexCoord;

    gl_Position = projection * view * worldPos;
}
