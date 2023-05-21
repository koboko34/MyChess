#include "Shader.h"

Shader::Shader()
{
    shaderProgramId = 0;
}

Shader::~Shader()
{
    if (shaderProgramId != 0)
    {
        glDeleteProgram(shaderProgramId);
    }
}

void Shader::UseShader()
{
    glUseProgram(shaderProgramId);
}

std::string Shader::ReadFile(const char* fileLocation)
{
    std::string content;
    std::ifstream filestream(fileLocation, std::ios::in);

    if (!filestream.is_open())
    {
        printf("Failed to open %s! File does not exist!\n", fileLocation);
        return "";
    }

    std::string line;

    while (!filestream.eof())
    {
        std::getline(filestream, line);
        content.append(line + "\n");
    }
    
    filestream.close();
    return content;
}

void Shader::AddShader(GLuint shaderProgram, const char* sourceLocation, GLenum shaderType)
{
    GLuint shaderId = glCreateShader(shaderType);
    std::string shaderString = ReadFile(sourceLocation);
    const char* shaderCode = shaderString.c_str();
    glShaderSource(shaderId, 1, &shaderCode, NULL);
    glCompileShader(shaderId);

    int success;
    char infoLog[512];
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shaderId, sizeof(infoLog), NULL, infoLog);
        printf("Error compiling %d shader! '%s'\n", shaderType, infoLog);
        return;
    }

    glAttachShader(shaderProgram, shaderId);

    if (shaderType == GL_VERTEX_SHADER)
    {
        vertId = shaderId;
    }
    else if (shaderType == GL_FRAGMENT_SHADER)
    {
        fragId = shaderId;
    }
}

void Shader::CompileShaders(const char* vertexLocation, const char* fragmentLocation)
{
    shaderProgramId = glCreateProgram();

    if (!shaderProgramId)
    {
        printf("Failed to create shader program!\n");
        return;
    }
    
    AddShader(shaderProgramId, vertexLocation, GL_VERTEX_SHADER);
    AddShader(shaderProgramId, fragmentLocation, GL_FRAGMENT_SHADER);

    glLinkProgram(shaderProgramId);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(shaderProgramId, sizeof(infoLog), NULL, infoLog);
        printf("Shader program linking failed! %s\n", infoLog);
    }

    glDeleteShader(vertId);
    glDeleteShader(fragId);
}
