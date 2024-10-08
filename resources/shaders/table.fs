#version 330 core
out vec4 FragColor;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_diffuse2;

    float shininess;
};

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform DirLight dirLight;
uniform PointLight pointLight;
uniform Material material;
uniform bool Blinn;

uniform vec3 viewPosition;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0;

    //Blinn-Phong
    if(Blinn){
       vec3 halfwayDir = normalize(lightDir + viewDir);
       spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    }else{
       vec3 reflectDir = reflect(-lightDir, normal);
       spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0/(light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results

    //ambient
    vec3 ambientFirst = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 ambientSecond = light.ambient * vec3(texture(material.texture_diffuse2, TexCoords));
    vec3 ambient = ambientFirst + ambientSecond;

    //diffuse
    vec3 diffuseFirst = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuseSecond = light.diffuse * diff * vec3(texture(material.texture_diffuse2, TexCoords));
    vec3 diffuse = diffuseFirst + diffuseSecond;

    //specular
    vec3 specular = light.specular * spec; //* vec3(texture(material.texture_specular1, TexCoords).xxx);

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec = 0.0;

    //Blinn-Phong

    if(Blinn){
           vec3 halfwayDir = normalize(lightDir + viewDir);
           spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
        }else{
           vec3 reflectDir = reflect(-lightDir, normal);
           spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        }

    // combine results

    //ambient
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));

    //diffuse
    vec3 diffuseFirst = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuseSecond = light.diffuse * diff * vec3(texture(material.texture_diffuse2, TexCoords));
    vec3 diffuse = diffuseFirst + diffuseSecond;

    vec3 specular = light.specular * spec; //* vec3(texture(material.texture_specular1, TexCoords).xxx);

    return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 result = CalcDirLight(dirLight, normal, viewDir);

    result += CalcPointLight(pointLight, normal, FragPos, viewDir);

    FragColor = vec4(result, 1.0);
}

