#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <unordered_map>
#include <memory>
#include <filesystem>

#include "Shader.h"
#include "ShaderTypes.h"
#include "DefaultShaders.h"

#include "AssetRegistry.h"
#include "ResourceManager.h"


// QT
#include <QOpenGLExtraFunctions>

namespace gfx {

class ShaderManager : public ResourceManager<ShaderSource>
{
    public:
        ShaderManager(AssetRegistry* assetRegistry, QOpenGLExtraFunctions* openGLFunctions);

        Shader* getShaderPtr(const std::string& shaderName);
        Shader* getShaderPtr(const GLuint shaderID);

        GLuint getShaderID(const std::string& shaderName);
        
        void registerElement(const std::filesystem::path& sourcePath) override;
        std::string loadShaderCode(const std::string& shaderPath);

        GLuint compileShaderWithKey(const std::string& key);
        GLuint compileShader(ShaderSource* source);
        void unloadShader(const std::string& key);
        void unloadShaderProgram(const std::string& key);

        void loadAndCompileDefaultShader();
        GLuint getDefaultShaderID();

        void compileShaderProgram(const std::string& shaderSourceKeyA, const std::string& shaderSourceKeyB, const std::string& name);
        bool ensureCompiled(ShaderSource* source);

        ShaderSource* getSource(const std::string& key);
        std::string normaliseStringKey(const std::string& key);

        const std::unordered_map<std::string, std::unique_ptr<Shader>>& getCompiledMap();

        const bool shaderAvailable()
        {
            return m_shaderAvailable;
        }

 
    private:
        void checkShaderCompilation(const GLuint shaderID, ShaderSource* source);

        std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
        QOpenGLExtraFunctions* m_openGLFunctions;
        bool m_shaderAvailable;

};

}

#endif