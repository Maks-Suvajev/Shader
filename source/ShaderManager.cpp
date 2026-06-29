#include "ShaderManager.h"


namespace gfx {

ShaderManager::ShaderManager(AssetRegistry* assetRegistry, QOpenGLExtraFunctions* openGLFunctions)
    : ResourceManager<ShaderSource>(assetRegistry, supportedShaderFileTypes), 
      m_openGLFunctions(openGLFunctions),
      m_shaderAvailable(false)
{
    m_activeDirectory = assetRegistry->getDefaultAssetPath<ShaderSource>();

    loadAndCompileDefaultShader();

    refreshElements();
}

Shader* ShaderManager::getShaderPtr(const std::string& shaderName)
{
    if (!m_shaders.contains(shaderName))
    {
        return nullptr;
    }

    return m_shaders[shaderName].get();
}

Shader* ShaderManager::getShaderPtr(GLuint shaderID)
{
    for (const auto& [key, shader] : m_shaders)
    {
        if (shaderID == shader->getShaderID())
        {
            return shader.get();
        }
    }

    return nullptr;
}

std::string ShaderManager::loadShaderCode(const std::string& shaderPath)
{
    std::ifstream shaderFile(shaderPath);

    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    if (!shaderFile.is_open())
    {
        throw std::ios_base::failure("ERROR::FAILED TO OPEN FILE: " + shaderPath); 
    }

    std::stringstream shaderStream;	

    shaderStream << shaderFile.rdbuf();

    return shaderStream.str();
}

void ShaderManager::compileShaderProgram(const std::string& shaderSourceKeyA, const std::string& shaderSourceKeyB, const std::string& name)
{
    bool vertexShaderInputted   = shaderSourceKeyA.ends_with(vertShaderExtension) || shaderSourceKeyB.ends_with(vertShaderExtension);
    bool fragmentShaderInputted = shaderSourceKeyA.ends_with(fragShaderExtension) || shaderSourceKeyB.ends_with(fragShaderExtension);

    if (!(vertexShaderInputted && fragmentShaderInputted))
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::ShaderManager::compileShaderProgram::Input must be 1 vertex and 1 fragment shader." << std::endl;
        #endif

        return;
    }

    std::string normalisedKeyA = normaliseStringKey(shaderSourceKeyA);
    std::string normalisedKeyB = normaliseStringKey(shaderSourceKeyB);

    if (!m_elements.contains(normalisedKeyA))
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::ShaderManager::compileShaderProgram::Selected vertex shader source doesn't exist: " << shaderSourceKeyA << std::endl;
        #endif

        return;
    }

    if (!m_elements.contains(normalisedKeyB))
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::ShaderManager::compileShaderProgram::Selected fragment shader source doesn't exist: " << shaderSourceKeyB << std::endl;
        #endif

        return;
    }

    if (m_shaders.contains(name) || name.empty())
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::ShaderManager::compileShaderProgram:::Shader with given name already exists or is null: " << name << std::endl;
        #endif

        return;
    }

    ShaderSource* sourceA = m_elements.at(normalisedKeyA).get();

    if (!ensureCompiled(sourceA))
    {
        return;
    }

    ShaderSource* sourceB = m_elements.at(normalisedKeyB).get();

    if (!ensureCompiled(sourceB))
    {
        return;
    }

    m_shaders[name] = std::make_unique<Shader>(sourceA, sourceB, name, m_openGLFunctions);

    m_shaderAvailable = true;
}

bool ShaderManager::ensureCompiled(ShaderSource* source)
{
    if (source->status != SourceStatus::Compiled)
    {
        source->compiledID = compileShader(source);

        if (source->status != SourceStatus::Compiled)
        {
            #ifdef ENABLE_DEBUG_MESSAGES
                std::cout << "ERROR::ShaderManager::compileShaderProgram::Failed to compile shader: " << source->name << std::endl;
            #endif
            return false;
        }
    }

    return true;
}

GLuint ShaderManager::compileShader(ShaderSource* source)
{
    if (source == nullptr)
    {
        return 0;
    }

    GLuint shaderObject;

    const char* rawSource = source->sourceCode.c_str();

    shaderObject = m_openGLFunctions->glCreateShader(source->type);

    m_openGLFunctions->glShaderSource(shaderObject, 1, &rawSource, NULL);

    m_openGLFunctions->glCompileShader(shaderObject);

    checkShaderCompilation(shaderObject, source);

    return shaderObject;
}

GLuint ShaderManager::compileShaderWithKey(const std::string& key)
{
    return compileShader(getSource(key));
}

std::string ShaderManager::normaliseStringKey(const std::string& key)
{
    namespace fs = std::filesystem;

    std::string normalisedString;

    try
    {
        normalisedString = fs::canonical(fs::path(key)).generic_string();    
    }
    catch(const std::exception& e)
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::ShaderManager::normaliseStringKey:: " <<  e.what() << std::endl;
        #else
            void(e);
        #endif
    }

    return normalisedString;
}

ShaderSource* ShaderManager::getSource(const std::string& key)
{
    std::string normalisedKey = normaliseStringKey(key);

    if (!m_elements.contains(normalisedKey))
    {
        return nullptr;
    }

    return m_elements[normalisedKey].get();
}

void ShaderManager::unloadShader(const std::string& key)
{
    std::string normalisedKey = normaliseStringKey(key);

    ShaderSource* source = m_elements.at(normalisedKey).get();

    if (source->status != SourceStatus::Compiled)
    {
        return;
    }

    m_openGLFunctions->glDeleteShader(source->compiledID);

    source->status = SourceStatus::Loaded;
}

void linkedKeyCleanup(ShaderSource* source, const std::string& key)
{
    std::erase_if(source->linkedShaderKeys, [key](auto& linkedKey){
        return (linkedKey == key);
    });
}

void ShaderManager::unloadShaderProgram(const std::string& key)
{
    if (!m_shaders.contains(key) || key.empty())
    {
        return;
    }

    std::for_each(m_elements.begin(), m_elements.end(), [key](auto& element){
        linkedKeyCleanup(element.second.get(), key);
    });
    
    m_shaders.erase(key);
}


void ShaderManager::checkShaderCompilation(GLuint shaderID, ShaderSource* source)
{
    int success;
    m_openGLFunctions->glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

    int logLength;
    m_openGLFunctions->glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);

    std::string infoLog(logLength, '\0');
    m_openGLFunctions->glGetShaderInfoLog(shaderID, logLength, nullptr, infoLog.data()); 
    
    source->infoLog = infoLog;

    if (!success)
    {
        source->status = SourceStatus::CompilationError;

        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl; // Replace with logging module
        #endif

        return;
    }
    
    source->compiledID = shaderID;
    source->status = SourceStatus::Compiled;
}

void ShaderManager::registerElement(const std::filesystem::path& sourcePath)
{
    std::filesystem::path normalisedPath;

    try
    {
        normalisedPath = std::filesystem::canonical(sourcePath);
    }
    catch(const std::exception& e)
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::ShaderManager::registerElement:: " << e.what() << std::endl;
        #else
            void(e);
        #endif
         
        return;
    }
    
    const auto key = normalisedPath.generic_string();

    if (m_elements.contains(key))
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::Source already loaded with the key: " << sourcePath.string() << std::endl;
        #endif

        return;
    }
 
    ShaderSource sourceData{};

    if (sourcePath.filename().extension() == vertShaderExtension)
    {
        sourceData.type = GL_VERTEX_SHADER;
    }
    else if (sourcePath.filename().extension() == fragShaderExtension)
    {
        sourceData.type = GL_FRAGMENT_SHADER;
    }
    else
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::ShaderManager::registerSource:: Not a valid fragment or vertex shader." << sourcePath.string() << std::endl;
        #endif
        return;
    }

    sourceData.name = normalisedPath.filename().string();
    sourceData.systemSourcePath = normalisedPath;

    try
    {
        sourceData.sourceCode = loadShaderCode(sourceData.systemSourcePath.string());
    }
    catch(const std::exception& e)
    {
        #ifdef ENABLE_DEBUG_MESSAGES
            std::cout << "ERROR::ShaderManager::registerSource::loadShaderCode::" <<  e.what() << std::endl;
        #else
            void(e);
        #endif

        return;
    }
    
    if (sourceData.sourceCode.empty())
    {
        sourceData.status = SourceStatus::EmptyFileError;
    }
    else
    {
        sourceData.status = SourceStatus::Loaded;
    }

    m_elements[key]  = std::make_unique<ShaderSource>(std::move(sourceData));     
}

GLuint ShaderManager::getShaderID(const std::string& shaderName)
{
    return getShaderPtr(shaderName)->getShaderID();
}

//TODO: depreciate this, non-ECS infrastructure
const std::unordered_map<std::string, std::unique_ptr<Shader>>& ShaderManager::getCompiledMap()
{
    return m_shaders;
}

void ShaderManager::loadAndCompileDefaultShader()
{
    bool compileCheck = false;
    ShaderSource defaultVertexData{};
    ShaderSource defaultFragmentData{};

    // Set up vertex source
    defaultVertexData.name          = defaultVertexShaderKey;
    defaultVertexData.sourceCode    = defaultVertexShader;
    defaultVertexData.status        = SourceStatus::Loaded;
    defaultVertexData.type          = GL_VERTEX_SHADER;

    compileCheck = ensureCompiled(&defaultVertexData);
    assert(compileCheck == true);

    m_elements[defaultVertexShaderKey]  = std::make_unique<ShaderSource>(std::move(defaultVertexData));   

    // Set up fragment source
    defaultFragmentData.name          = defaultFragmentShaderKey;
    defaultFragmentData.sourceCode    = defaultFragmentShader;
    defaultFragmentData.status        = SourceStatus::Loaded;
    defaultFragmentData.type          = GL_FRAGMENT_SHADER;

    compileCheck = ensureCompiled(&defaultFragmentData);
    assert(compileCheck == true);

    m_elements[defaultFragmentShaderKey]  = std::make_unique<ShaderSource>(std::move(defaultFragmentData));   

    m_shaders[defaultShaderProgramKey] = std::make_unique<Shader>(m_elements[defaultVertexShaderKey].get(), m_elements[defaultFragmentShaderKey].get(), defaultShaderProgramKey, m_openGLFunctions);

    m_shaderAvailable = true;
}

GLuint ShaderManager::getDefaultShaderID()
{
    return getShaderID(defaultShaderProgramKey);
}


}

         