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

// QT
#include <QOpenGLExtraFunctions>

namespace gfx {

inline constexpr GLint glUniformLocationLoadError 	= -1; // If a uniform location does not exist then opengl will return a -1
inline constexpr char modelMatrixUniformName[] 		= "modelMatrix";
inline constexpr char viewMatrixUniformName[] 		= "viewMatrix";
inline constexpr char projectionMatrixUniformName[] = "projectionMatrix";

// Utility template for checking if constexpr else statement is hit
template<typename T>
struct always_false : std::false_type {}; 

struct GlslUniform
{
    std::string typeString; // Type string pulled from glsl source
    std::string uniformName;
};

struct TransformMatrixUniforms
{
    GlslUniform model;
    GlslUniform view;
    GlslUniform projection;
};

class Shader
{
    public:
        Shader(const std::string &fragmentShaderPath, const std::string &vertexShaderPath, const std::string uniqueShaderName, QOpenGLExtraFunctions* openGLFunctions);

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

        GLint getUniformLocation(const char * const name);

        // MVP calculated on GPU side right now, maybe will be switched to CPU once I have more information on this
        bool updateModelMatrixValue(const glm::mat4& value);
        bool updateViewMatrixValue(const glm::mat4& value);
        bool updateProjectionMatrixValue(const glm::mat4& value);



    private:
        GLuint      m_shaderID;
        std::string m_shaderName;

        GLint m_modelMatrixLocation;
        GLint m_viewMatrixLocation;
        GLint m_projectionMatrixLocation;

        glm::mat4 m_modelMatrixCache;
        glm::mat4 m_viewMatrixCache;
        glm::mat4 m_projectionMatrixCache;

        TransformMatrixUniforms  m_matrixUniforms;
        std::vector<GlslUniform> m_nonTransformUniforms;

        QOpenGLExtraFunctions* m_openGLFunctions;

        void checkShaderCompilation(GLuint shaderID);

        std::string loadShaderCode(const std::string& shaderPath);

        GLuint compileShader(GLenum shaderType, char const * shaderCode);

        void compileShaderProgram(GLuint vertexShader, GLuint fragmentShader);

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