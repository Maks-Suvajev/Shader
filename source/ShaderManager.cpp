#include "ShaderManager.h"


namespace gfx {

ShaderManager::ShaderManager(std::vector<ShaderProgramFilePaths> shaderSources, QOpenGLExtraFunctions* openGLFunctions)
    : m_openGLFunctions(openGLFunctions)
{
    for (const auto& shaderSource : shaderSources)
    {
        Shader compiledShader = Shader(shaderSource.fragmentShader.string(), shaderSource.vertexShader.string(), shaderSource.setName, m_openGLFunctions);

        shaders[shaderSource.setName] = std::make_unique<Shader>(compiledShader);
    }
}

Shader* ShaderManager::getShaderPtr(std::string shaderName)
{
    return shaders[shaderName].get();
}

Shader* ShaderManager::getShaderPtr(GLuint shaderID)
{
    for (const auto& [key, shader] : shaders)
    {
        if (shaderID == shader->getShaderID())
        {
            return shader.get();
        }
    }

    return nullptr;
}

void ShaderManager::printAllShaderPrograms()
{
    std::cout << "-----------------------------------------------------------------" << std::endl;
    std::cout << "| ----- Printing currently available shaders and their IDs----- |" << std::endl;

    for (auto& [key, item] : shaders)
    {
        std::cout << "-----------------------------------------------------------------" << std::endl;
        std::cout << "Key: " << key << std::endl;

        if (item)
        {
            std::cout << "Name: " << item->getShaderName() << "Shader ID: " << item->getShaderID() << std::endl;
        }
        else
        {
            std::cout << "Path: NULLPTR" << std::endl;
        }        
    }

    std::cout << "-----------------------------------------------------------------" << std::endl;
}

GLuint ShaderManager::getShaderID(std::string shaderName)
{
    return getShaderPtr(shaderName)->getShaderID();
}

}