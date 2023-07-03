#pragma once

#include <string>
#include <functional>

#include <GL/glew.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

class Board;

class Button
{
public:
	Button(Board* board, float xPositon, float xPositionOffset, float yPosition, float xSize, float xSizeOffset, float ySize);
	~Button();

	void SetCallback(std::function<void()> fn) { callback = fn; }
	void SetColor(GLfloat x, GLfloat y, GLfloat z);
	void SetTexture(std::string fileLoc);

	int id;
	std::function<void()> callback;

	glm::vec3 color = glm::vec3(0.3f, 0.3f, 0.9f);

	float xPos;
	float xPosOffset;
	float yPos;
	float xSize;
	float xSizeOffset;
	float ySize;

	bool bUseTexture = false;
	bool bTextureSet = false;
	GLuint VAO, EBO, VBO, textureId = 0;

private:
	void RemoveButton();

	Board* board;
};

