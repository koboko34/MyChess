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

	PieceTeam GetTeam() const { return pieceTeam; }
	PieceType GetType() const { return pieceType; }
	int GetValue() const { return pieceValue; }

	void SetPiece(PieceTeam newTeam, PieceType newType);
	void DrawPiece();

	bool bMoved;

private:
	PieceTeam pieceTeam;
	PieceType pieceType;
	int pieceValue;

	GLuint VAO, EBO, VBO;

	void Init();
};

