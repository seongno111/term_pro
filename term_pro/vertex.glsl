#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 3) in vec2 vTexCoord; // 추가: UV
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord; // 추가: 프래그먼트로 전달
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    gl_Position = projection * view * model * vec4(vPos, 1.0);
    FragPos = vec3(model * vec4(vPos, 1.0));
    // 정규화된 법선 계산
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * vNormal);
    TexCoord = vTexCoord;
}