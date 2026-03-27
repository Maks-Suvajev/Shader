#include "Shader.h"

#include <fstream>
#include <sstream>

namespace gfx {

	Shader::Shader(ShaderSource* sourceA, ShaderSource* sourceB, const std::string name, QOpenGLExtraFunctions* openGLFunctions)
	: m_modelMatrixLocation(glUniformLocationLoadError),
	  m_viewMatrixLocation(glUniformLocationLoadError),
	  m_projectionMatrixLocation(glUniformLocationLoadError),
      m_openGLFunctions(openGLFunctions),
      m_shaderName(name),
      m_programLinkStatus(ShaderProgramStatus::Unlinked)
    {
		linkShaderProgram(sourceA->compiledID, sourceB->compiledID);

		useProgram();

		loadEachFileShaderVariables(sourceA->sourceCode.c_str(), sourceB->sourceCode.c_str());

		initialiseMvpMatrices();

	}

    Shader::~Shader()
    {
        m_openGLFunctions->glDeleteProgram(m_shaderID);
    }

    void Shader::documentSourcefiles(ShaderSource* sourceA, ShaderSource* sourceB)
    {
        m_sourceFiles.push_back(sourceA->systemSourcePath);
        m_sourceFiles.push_back(sourceB->systemSourcePath);
    }

	void Shader::initialiseMvpMatrices()
	{
		loadMvpMatricesLocations();
		initialiseMvpMatricesValues();
	}

	void Shader::initialiseMvpMatricesValues()
	{
		// Initiliase all MVP matrices with identity matrices
		updateModelMatrixValue(glm::mat4(1.0f));
		updateViewMatrixValue(glm::mat4(1.0f));
		updateProjectionMatrixValue(glm::mat4(1.0f));
	}


	void Shader::linkShaderProgram(GLuint shaderA, GLuint shaderB)
	{
		m_shaderID = m_openGLFunctions->glCreateProgram();

		m_openGLFunctions->glAttachShader(m_shaderID, shaderA);
		m_openGLFunctions->glAttachShader(m_shaderID, shaderB);
		m_openGLFunctions->glLinkProgram(m_shaderID);

		//Check linking
		int success;
		m_openGLFunctions->glGetProgramiv(m_shaderID, GL_LINK_STATUS, &success);

		char infoLog[512];
		m_openGLFunctions->glGetProgramInfoLog(m_shaderID, 512, NULL, infoLog);

        m_log = infoLog;

		if (!success) {

            m_programLinkStatus = ShaderProgramStatus::Error;

            #ifdef ENABLE_DEBUG_MESSAGES
			    std::cout << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n" << infoLog << std::endl;
            #endif

            return;
		}
        

        m_programLinkStatus = ShaderProgramStatus::Linked;
	}

	// Pull the uniform locations for the transformation matrices Model-View-Projection (MVP)
	void Shader::loadMvpMatricesLocations()
	{
		m_modelMatrixLocation = m_openGLFunctions->glGetUniformLocation(m_shaderID, modelMatrixUniformName);

		if	(m_modelMatrixLocation == glUniformLocationLoadError)
		{
            #ifdef ENABLE_DEBUG_MESSAGES
			    std::cout << "ERROR::\"model\" transformation matrix uniform not found in linked shader program." << std::endl;
            #endif
		}

		m_viewMatrixLocation = m_openGLFunctions->glGetUniformLocation(m_shaderID, viewMatrixUniformName);

		if	(m_viewMatrixLocation == glUniformLocationLoadError)
		{
            #ifdef ENABLE_DEBUG_MESSAGES
			    std::cout << "ERROR::\"view\" transformation matrix uniform not found in linked shader program." << std::endl;
            #endif
		}

		m_projectionMatrixLocation = m_openGLFunctions->glGetUniformLocation(m_shaderID, projectionMatrixUniformName);

		if	(m_projectionMatrixLocation == glUniformLocationLoadError)
		{
            #ifdef ENABLE_DEBUG_MESSAGES
			    std::cout << "ERROR::\"projection\" transformation matrix uniform not found in linked shader program." << std::endl;
            #endif
        }
	}

	GLint Shader::getUniformLocation(const char * const name)
	{
			GLint uniformLocation = m_openGLFunctions->glGetUniformLocation(m_shaderID, name);

			if	(uniformLocation == glUniformLocationLoadError)
			{
                #ifdef ENABLE_DEBUG_MESSAGES
				    std::cout << "ERROR::\"model\" transformation matrix uniform not found in linked shader program." << std::endl;
                #endif
			}

			return uniformLocation;
	}

	bool Shader::updateModelMatrixValue(const glm::mat4& value)
	{
        static bool firstWrite = true;

        if (!firstWrite)
        {
            if (value == m_modelMatrixCache)
            {
                return true;
            }
            
            if (updateUniformValue(modelMatrixUniformName, value))
            {
                m_modelMatrixCache = value;
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
		    updateUniformValue(modelMatrixUniformName, value);

            firstWrite = false;

            return true;

        }
	}

	bool Shader::updateViewMatrixValue(const glm::mat4& value)
	{

        static bool firstWrite = true;

        if (!firstWrite)
        {
            if (value == m_viewMatrixCache)
            {
                return true;
            }
            
            if (updateUniformValue(viewMatrixUniformName, value))
            {
                m_viewMatrixCache = value;
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            updateUniformValue(viewMatrixUniformName, value);

            firstWrite = false;

            return true;
        }
	}

	bool Shader::updateProjectionMatrixValue(const glm::mat4& value)
	{
        static bool firstWrite = true;

        if (!firstWrite)
        {
            if (value == m_projectionMatrixCache)
            {
                return true;
            }
            
            if (updateUniformValue(projectionMatrixUniformName, value))
            {
                m_projectionMatrixCache = value;
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            updateUniformValue(projectionMatrixUniformName, value);

            firstWrite = false;
            
            return true;
        }
	}

	void Shader::loadShaderUniformVariables(const std::string& shaderCode)
	{
		std::istringstream shaderCodeStream(shaderCode);
		std::string currLine;
		std::vector<std::string> lines;

		bool uniformDetected = false;
		bool uniformTypeSet = false;
		bool uniformValueSet = false;
		bool skip = false; // Skip other token checks if token has already been assigned

		GlslUniform uniform;

		while (std::getline(shaderCodeStream, currLine, ';')) {

			std::istringstream lineParse(currLine); // Create stream for line
			std::string currToken; 

			uniformDetected = false;
			uniformTypeSet = false;
			uniformValueSet = false;

			while(lineParse >> currToken)
			{
				skip = false;

				if (currToken == "uniform")
				{
					uniformDetected = true;
					skip = true;
				}

				if (uniformDetected && !uniformTypeSet && !skip)
				{
					uniform.typeString = currToken;

                    #ifdef ENABLE_DEBUG_MESSAGES
					    std::cout << "DEBUG::Uniform type string = " << uniform.typeString << std::endl;
                    #endif

					uniformTypeSet = true;
					skip = true;
				}
				else if (uniformDetected && !uniformValueSet && !skip)
				{
					uniform.uniformName = currToken;
					uniformValueSet = true;

                    #ifdef ENABLE_DEBUG_MESSAGES
					    std::cout << "DEBUG::Uniform type name = " << uniform.uniformName << std::endl;
                    #endif

					storeUniform(uniform);

					uniform = GlslUniform{}; //reset
				}
			}
		}
	}

	void Shader::loadEachFileShaderVariables(const std::string& VertShaderCode, const std::string& FragShaderCode)
	{
		loadShaderUniformVariables(VertShaderCode);
		loadShaderUniformVariables(FragShaderCode);
	}

	void Shader::storeUniform(const GlslUniform &uniform)
	{
		// need to check if keywords "model", "view" or "projection" are in uniform name.

		auto toLower = [](char c){ return std::tolower(static_cast<unsigned char>(c)); }; // Convert char to lowercase

		auto lowerCaseUniformNameView = uniform.uniformName | std::views::transform(toLower);
        
        #ifdef ENABLE_DEBUG_MESSAGES                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
		    std::cout << "DEBUG::Storing uniform: " << uniform.uniformName << std::endl;
        #endif

		if (std::ranges::search(lowerCaseUniformNameView,  std::string_view("model")).begin() != lowerCaseUniformNameView.end())
		{
			m_matrixUniforms.model = uniform;

            #ifdef ENABLE_DEBUG_MESSAGES
			    std::cout << "DEBUG::Stored as model uniform." << std::endl;
            #endif
		}
		else if (std::ranges::search(lowerCaseUniformNameView, std::string_view("view")).begin() != lowerCaseUniformNameView.end())
		{
			m_matrixUniforms.view = uniform;

            #ifdef ENABLE_DEBUG_MESSAGES
			    std::cout << "DEBUG::Stored as view uniform." << std::endl;
            #endif
		}
		else if (std::ranges::search(lowerCaseUniformNameView, std::string_view("projection")).begin() != lowerCaseUniformNameView.end())
		{
			m_matrixUniforms.projection = uniform;
            #ifdef ENABLE_DEBUG_MESSAGES
			    std::cout << "DEBUG::Stored as projection uniform." << std::endl;
            #endif
		}
		else
		{
            #ifdef ENABLE_DEBUG_MESSAGES
			    std::cout << "DEBUG::Stored as non-MVP uniform." << std::endl;
            #endif

			m_nonTransformUniforms.push_back(uniform);
		}
	}
}

