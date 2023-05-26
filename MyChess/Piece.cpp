#include "Piece.h"

Piece::Piece()
{
	VAO, EBO, VBO = 0;
	bPawnMoved = false;
}

Piece::Piece(PieceTeam team, PieceType type)
{
	pieceTeam = team;
	pieceType = type;

	VAO, EBO, VBO = 0;

	bPawnMoved = false;

	Init();
}

Piece::~Piece()
{
	if (VAO != 0)
	{
		glDeleteVertexArrays(1, &VAO);
	}
	if (EBO != 0)
	{
		glDeleteBuffers(1, &EBO);
	}
	if (VBO != 0)
	{
		glDeleteBuffers(1, &VBO);
	}
}

void Piece::DrawPiece()
{
	glBindVertexArray(VAO);

	// GL_LINE for wireframe, GL_FILL for standard
	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Piece::Init()
{
	float offsetPixels = 1920.f / 6.f;
	float pieceTextureOffsetU = (offsetPixels / 1920.f);
	float pieceTextureOffsetV = pieceTeam == WHITE ? 0.5f : 0.f;

	
	GLfloat pieceVertices[] = {
	 0.5f,  0.5f, 0.f,		pieceTextureOffsetU * (pieceType + 1),	0.5f + pieceTextureOffsetV,
	 0.5f, -0.5f, 0.f,		pieceTextureOffsetU * (pieceType + 1),	pieceTextureOffsetV,
	-0.5f, -0.5f, 0.f,		pieceTextureOffsetU * pieceType,		pieceTextureOffsetV,
	-0.5f,  0.5f, 0.f,		pieceTextureOffsetU * pieceType,		0.5f + pieceTextureOffsetV
	};
	
	GLuint pieceIndices[] = {
	0, 1, 3,
	1, 2, 3
	};

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pieceIndices), pieceIndices, GL_STATIC_DRAW);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pieceVertices), pieceVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}
