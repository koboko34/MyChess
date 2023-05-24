#pragma once

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "CommonValues.h"

#include "Shader.h"
#include "Piece.h"

class Board
{
public:
	Board();
	~Board();

	void Init(unsigned int width, unsigned int height);
	void DrawBoard();


private:

	Shader boardShader, pieceShader;
	
	GLfloat tileVertices[12] = {
		 0.5f,  0.5f, 0.f,
		 0.5f, -0.5f, 0.f,
		-0.5f, -0.5f, 0.f,
		-0.5f,  0.5f, 0.f
	};

	GLuint tileIndices[6] = {
		0, 1, 3,
		1, 2, 3
	};

	GLuint tileColorLocation, tileModelLocation, tileViewLocation, tileProjectionLocation;
	GLuint pieceModelLocation, pieceViewLocation, pieceProjectionLocation;
	GLuint VAO, EBO, VBO;

	GLuint piecesTextureId;

	Piece* pieces[64];

	glm::vec3 blackColor = glm::vec3(0.1f, 0.1f, 0.1f);
	glm::vec3 whiteColor = glm::vec3(0.9f, 0.9f, 0.9f);

	float aspect;
	
};

