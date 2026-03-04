add_library(ShaderLib 
    source/Shader.cpp
    source/ShaderManager.cpp
)

find_package(Qt6 REQUIRED COMPONENTS OpenGL)

target_link_libraries(ShaderLib 
    PUBLIC
        TypeLib
        Qt6::OpenGL
)

target_include_directories(ShaderLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${DEPS_INCLUDE_DIR}
)


