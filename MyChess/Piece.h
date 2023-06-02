#pragma once

#include <GL/glew.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "CommonValues.h"

class Piece
{
public:
	Piece();
	Piece(PieceTeam team, PieceType type);
	~Piece();

	PieceTeam GetTeam() { return pieceTeam; }
	PieceType GetType() { return pieceType; }

	void DrawPiece();

	bool bMoved;

private:
	PieceTeam pieceTeam;
	PieceType pieceType;

	GLuint VAO, EBO, VBO;

	void Init();
};

