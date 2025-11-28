#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;
uniform sampler2D texSampler;
uniform int useTexture; // 0 = 텍스처 미사용, 1 = 사용

out vec4 FragColor;

void main(void)
{
    // ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    // normal / fragment position
    vec3 normalVector = normalize(Normal);
    vec3 fragPos = FragPos;

    // light direction
    vec3 lightDir = normalize(lightPos - fragPos);

    // diffuse
    float diffuseLight = max(dot(normalVector, lightDir), 0.0);
    vec3 diffuse = diffuseLight * lightColor;

    // specular
    float shininess = 128.0;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normalVector);
    float specularLight = max(dot(viewDir, reflectDir), 0.0);
    specularLight = pow(specularLight, shininess);
    vec3 specular = specularLight * lightColor;

    vec3 baseColor = objectColor;
    if (useTexture == 1) {
        vec4 texc = texture(texSampler, TexCoord);
        baseColor = texc.rgb;
    }

    vec3 result = (ambient + diffuse + specular) * baseColor;
    FragColor = vec4(result, 1.0);
}