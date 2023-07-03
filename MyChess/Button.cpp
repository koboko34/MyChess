#include "Button.h"

#include "CommonValues.h"

#include "Board.h"

Button::Button(Board* board, float xPositon, float xPositionOffset, float yPosition, float xSize, float xSizeOffset, float ySize)
{
	this->board = board;

	xPos = xPositon;
	xPosOffset = xPositionOffset;
	yPos = yPosition;
	this->xSize = xSize;
	this->xSizeOffset = xSizeOffset;
	this->ySize = ySize;
}

Button::~Button()
{
	RemoveButton();
}

void Button::SetColor(GLfloat x, GLfloat y, GLfloat z)
{
	color.x = x;
	color.y = y;
	color.z = z;
}

void Button::SetTexture(std::string fileLoc)
{
	if (bTextureSet)
	{
		printf("Cannot set texture twice! Create brand new button instead.\n");
	}

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int texWidth, texHeight, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(fileLoc.c_str(), &texWidth, &texHeight, &nrChannels, STBI_rgb_alpha);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		printf("Failed to load texture!");
	}
	stbi_image_free(data);




	GLfloat buttonVertices[] = {
	 0.5f,  0.5f, 0.f,		1.f,	1.f,
	 0.5f, -0.5f, 0.f,		1.f,	0.f,
	-0.5f, -0.5f, 0.f,		0.f,	0.f,
	-0.5f,  0.5f, 0.f,		0.f,	1.f
	};

	GLuint buttonIndices[] = {
	0, 1, 3,
	1, 2, 3
	};

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(buttonIndices), buttonIndices, GL_STATIC_DRAW);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(buttonVertices), buttonVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	bTextureSet = true;
}

void Button::RemoveButton()
{
	while (0 < board->GetButtons().size())
	{
		if (board->GetButtons()[0]->id == id)
		{
			board->GetButtons().erase(board->GetButtons().begin());
			return;
		}
	}
}