#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include "GL/glew.h"

class Shader
{
public:
	Shader();
	~Shader();

	void CompileShaders(const char* vertexLocation, const char* fragmentLocation);
	void UseShader();

private:
	std::string ReadFile(const char* fileLocation);

	void AddShader(GLuint shaderProgram, const char* sourceLocation, GLenum shaderType);


	GLuint vertId, fragId;
	GLuint shaderProgramId;

};

