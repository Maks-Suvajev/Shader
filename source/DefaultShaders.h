#ifndef DEFAULT_SHADERS_H
#define DEFAULT_SHADERS_H

namespace gfx
{
    const char* const defaultShaderProgramKey   = "DEFAULT_SHADER_PROGRAM";
    const char* const defaultVertexShaderKey    = "DEFAULT_VERTEX_SHADER";
    const char* const defaultFragmentShaderKey  = "DEFAULT_FRAGMENT_SHADER";

    const char* const defaultVertexShader = R"(
    #version 330 core

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;

    out vec2 TexCoords;
    out vec3 Normal;
    out vec3 FragPos; //Fragment position in world space

    uniform mat4 modelMatrix;
    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;

    void main()
    {
        gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aPos, 1.0);
        FragPos = vec3(modelMatrix * vec4(aPos, 1.0));

        // Required for non-uniform scaling in order to keep normal perp. to surface - should be done on CPU side
        Normal = mat3(transpose(inverse(modelMatrix))) * aNormal; 
        //Normal = aNormal;

        TexCoords = aTexCoords;
    }
    )";

    const char* const defaultFragmentShader = R"(
    #version 330 core

    // I/O
    out vec4 FragColor;
    in vec2 TexCoords;
    in vec3 Normal;
    in vec3 FragPos;

    // Types ----------
    struct Material 
    {
        sampler2D diffuse;
        int       numDiffuse;
        sampler2D specular;
        int       numSpecular;
        float     shininess;
    };

    struct LightColours
    {
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
    };

    struct DirectionalLight 
    {
        vec3 direction;
        LightColours lightColours;
    };

    struct PointLightConstants
    {
        float constant;
        float linear;
        float quadratic;
    };

    struct CutOff
    {
        float outer;
        float inner;
    };

    struct PointLight 
    {
        vec3 position;
        LightColours lightColours;
        PointLightConstants constants;
    };

    struct SpotLight 
    {
        vec3 position;
        vec3 direction;
        CutOff cutOff;
        LightColours lightColours;
    };
    // ------------

    // Max number of point lights
    #define NR_POINT_LIGHTS_MAX 32

    // Uniforms
    uniform int numberOfActiveLights;
    uniform DirectionalLight directionalLight;
    uniform PointLight pointLights[NR_POINT_LIGHTS_MAX];
    uniform SpotLight spotLight;
    uniform vec3 viewPos;
    uniform Material material;

    // Functions
    vec3 CalcDirLight(vec3 normal, vec3 viewDirection);
    vec3 CalcPointLight(PointLight pointLightInstance, vec3 normal, vec3 fragPosition, vec3 viewDirection);
    vec3 CalcSpotLight(SpotLight spotLightInstance, vec3 normal, vec3 fragPosition, vec3 viewDirection);

    void main()
    {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);

        // Directional lighting
        vec3 result = CalcDirLight(norm, viewDir);

        int numberOfLights = 0;

        // Point lights
        if (numberOfActiveLights > NR_POINT_LIGHTS_MAX)
        {
            numberOfLights = NR_POINT_LIGHTS_MAX;
        }
        else
        {
            numberOfLights = numberOfActiveLights;
        }

        for (int i = 0; i < numberOfLights; i++)
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

        // Spot light
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

        FragColor = vec4(result, 1.0);
    }

    vec3 CalcDirLight(vec3 normal, vec3 viewDirection)
    {
        vec3 lightDir = normalize(-directionalLight.direction);
        float diff = max(dot(normal, lightDir), 0.0); // if dot < 0 max will return 0.0 instead 

        vec3 reflectDir = reflect(-lightDir, normal); // Have to negate because its pointing from frag to light currently
        float spec = pow(max(dot(viewDirection, reflectDir), 0.0), material.shininess);

        vec3 ambient = directionalLight.lightColours.ambient * vec3(texture(material.diffuse, TexCoords));
        vec3 diffuse = directionalLight.lightColours.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
        vec3 specular = (directionalLight.lightColours.specular * spec) * vec3(texture(material.specular, TexCoords));

        return ambient + diffuse + specular;
    }

    vec3 CalcPointLight(PointLight pointLightInstance, vec3 normal, vec3 fragPosition, vec3 viewDirection)
    {
        vec3 ambient = pointLightInstance.lightColours.ambient * vec3(texture(material.diffuse, TexCoords));

        vec3 lightDir = normalize(vec3(pointLightInstance.position) - fragPosition);

        float diff = max(dot(normal, lightDir), 0.0); // if dot < 0 max will return 0.0 instead 

        vec3 diffuse = pointLightInstance.lightColours.diffuse * diff * vec3(texture(material.diffuse, TexCoords));

        vec3 reflectrDir = reflect(-lightDir, normal); // Have to negate because its pointing from frag to light currently

        float spec = pow(max( dot(viewDirection, reflectrDir), 0.0), material.shininess);

        vec3 specular = (pointLightInstance.lightColours.specular * spec) * vec3(texture(material.specular, TexCoords));

        float distance = length(pointLightInstance.position - fragPosition);
        float attenuation = 1.0f / (pointLightInstance.constants.constant + (pointLightInstance.constants.linear * distance) + (pointLightInstance.constants.quadratic * (distance * distance)));

        ambient  *= attenuation;
        diffuse  *= attenuation;
        specular *= attenuation;

        return ambient + diffuse + specular;
    }

    vec3 CalcSpotLight(SpotLight spotLightInstance, vec3 normal, vec3 fragPosition, vec3 viewDirection)
    {
        vec3 ambient = spotLightInstance.lightColours.ambient * vec3(texture(material.diffuse, TexCoords));

        vec3 lightDir = normalize(spotLightInstance.position - fragPosition);

        float diff = max(dot(normal, lightDir), 0.0); // if dot < 0 max will return 0.0 instead 

        vec3 diffuse = spotLightInstance.lightColours.diffuse * diff * vec3(texture(material.diffuse, TexCoords));

        vec3 reflectrDir = reflect(-lightDir, normal); // Have to negate because its pointing from frag to light currently

        float spec = pow(max(dot(viewDirection, reflectrDir), 0.0), material.shininess);

        vec3 specular = (spotLightInstance.lightColours.specular * spec) * vec3(texture(material.specular, TexCoords));

        float theta = dot(lightDir, normalize(-spotLightInstance.direction));
        float epsilon = spotLightInstance.cutOff.inner - spotLightInstance.cutOff.outer;
        float intensity = clamp((theta - spotLightInstance.cutOff.outer)/ epsilon, 0.0, 1.0);

        diffuse  *= intensity;
        specular *= intensity;

        return ambient + diffuse + specular;
    }
    )";
}


#endif