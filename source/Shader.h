#ifndef SHADER_H
#define SHADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <type_traits>
#include <vector>
#include <ranges>
#include <algorithm>

#include "ShaderTypes.h"

// QT
#include <QOpenGLExtraFunctions>

namespace gfx {

class Shader
{
    public:
	    Shader(ShaderSource* sourceA, ShaderSource* sourceB, const std::string name, QOpenGLExtraFunctions* openGLFunctions);
        ~Shader();

        template<typename T>
        bool updateUniformValue(const GLchar * const name, const T& value);

        void useProgram()
        {
            m_openGLFunctions->glUseProgram(m_shaderID);
        }

        std::string getShaderName()
        {
            return m_shaderName;
        }

        GLuint getShaderID()
        {
            return m_shaderID;
        }

        ShaderProgramStatus getShaderProgramStatus()
        {
            return m_programLinkStatus;
        }

        std::string getLinkLog()
        {
            return m_log;
        }

        std::vector<std::filesystem::path>& getSourcePaths()
        {
            return m_sourceFiles;
        }

        GLint getUniformLocation(const char * const name);

        // MVP calculated on GPU side right now, maybe will be switched to CPU once I have more information on this
        bool updateModelMatrixValue(const glm::mat4& value);
        bool updateViewMatrixValue(const glm::mat4& value);
        bool updateProjectionMatrixValue(const glm::mat4& value);

    private:
        GLuint      m_shaderID;
        std::string m_shaderName;

        ShaderProgramStatus m_programLinkStatus;
        std::string         m_log;

        std::vector<std::filesystem::path> m_sourceFiles;

        GLint m_modelMatrixLocation;
        GLint m_viewMatrixLocation;
        GLint m_projectionMatrixLocation;

        glm::mat4 m_modelMatrixCache;
        glm::mat4 m_viewMatrixCache;
        glm::mat4 m_projectionMatrixCache;

        TransformMatrixUniforms  m_matrixUniforms;
        std::vector<GlslUniform> m_nonTransformUniforms;

        QOpenGLExtraFunctions* m_openGLFunctions;

        void documentSourcefiles(ShaderSource* sourceA, ShaderSource* sourceB);

        void linkShaderProgram(GLuint vertexShader, GLuint fragmentShader);

        void loadMvpMatricesLocations();

        void initialiseMvpMatricesValues();

        void initialiseMvpMatrices();

        void loadShaderUniformVariables(const std::string& shaderCode);

        void loadEachFileShaderVariables(const std::string& VertShaderCode, const std::string& FragShaderCode);

        void storeUniform(const GlslUniform& uniform);
};

template<typename T>
bool Shader::updateUniformValue(const GLchar * const name, const T& value)
{
    GLint valueLocation = m_openGLFunctions->glGetUniformLocation(m_shaderID, name); // Needs to be replaced with an interface with a caching interface

    if (valueLocation == glUniformLocationLoadError)
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::" << name << " uniform not found in linked shader program." << std::endl;
        #endif

        return false;
    }

    // This if statement will need to be expanded to accomodate new data types required by the system
    if constexpr ((std::is_same_v<T, GLint> || std::is_same_v<T, bool> || std::is_same_v<T, int>)) // glsl doesn't support bools, need to treat as int
    {
        m_openGLFunctions->glUniform1i(valueLocation, value);
    }
    else if constexpr (std::is_same_v<T, GLfloat>) 
    {
        m_openGLFunctions->glUniform1f(valueLocation, value);
    }
    else if constexpr (std::is_same_v<T, glm::vec2>) 
    {
        m_openGLFunctions->glUniform2f(valueLocation, value[0], value[1]);
    }
    else if constexpr (std::is_same_v<T,glm::vec3>) 
    {
        m_openGLFunctions->glUniform3f(valueLocation, value[0], value[1], value[2]);
    }
    else if constexpr (std::is_same_v<T,glm::vec4>) 
    {
        m_openGLFunctions->glUniform4f(valueLocation, value[0], value[1], value[2], value[3]);
    }
    else if constexpr (std::is_same_v<T,glm::mat2>) 
    {
        m_openGLFunctions->glUniformMatrix2fv(valueLocation, 1, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::is_same_v<T, glm::mat3>) 
    {
        m_openGLFunctions->glUniformMatrix3fv(valueLocation, 1, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::is_same_v<T,glm::mat4>) 
    {
        m_openGLFunctions->glUniformMatrix4fv(valueLocation, 1, GL_FALSE, glm::value_ptr(value));
    }
    else
    {
        static_assert(always_false<T>::value, "Type provided for uniform update not currently supported.");

    }

    GLenum errorCheck = m_openGLFunctions->glGetError();

    if (errorCheck != GL_NO_ERROR)
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR:: Could not write to uniform: " << name << "  Error code: " << errorCheck << std::endl;
        #endif
        
        return false;
    }

    return true;
}

}

#endif