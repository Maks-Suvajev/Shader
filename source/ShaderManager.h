#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <unordered_map>
#include <memory>
#include <filesystem>

#include "Shader.h"
#include "ShaderTypes.h"

// QT
#include <QOpenGLExtraFunctions>

namespace gfx {

class ShaderManager
{
    public:
        ShaderManager(std::vector<ShaderProgramFilePaths> shaderSources, QOpenGLExtraFunctions* openGLFunctions);
        Shader* getShaderPtr(std::string shaderName);
        Shader* getShaderPtr(GLuint shaderID);
        GLuint getShaderID(std::string shaderName);
        void printAllShaderPrograms();

    private:
        std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;

        QOpenGLExtraFunctions* m_openGLFunctions;

};

}

#endif