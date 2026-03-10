#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec4 aInstanceCol0;
layout (location = 5) in vec4 aInstanceCol1;
layout (location = 6) in vec4 aInstanceCol2;
layout (location = 7) in vec4 aInstanceCol3;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useInstancing;

void main()
{
    mat4 instanceModel = mat4(aInstanceCol0, aInstanceCol1, aInstanceCol2, aInstanceCol3);
    mat4 objectModel = useInstancing ? (model * instanceModel) : model;

    FragPos = vec3(objectModel * vec4(aPos, 1.0));

    mat3 normalMatrix = mat3(transpose(inverse(objectModel)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    Normal = N;
    TBN = mat3(T, B, N);
    TexCoord = aTexCoord;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
