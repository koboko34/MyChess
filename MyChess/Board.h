#pragma once

#include <string>

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
	void DrawBoard(int selectedObjectId);
	void PickingPass();

	bool MovePiece(int startTile, int endTile);

	bool PieceExists(int index);

private:

	void SetupBoard(glm::mat4 view, glm::mat4 projection);
	void SetupPieces(glm::mat4 view, glm::mat4 projection);
	void SetupPickingShader(glm::mat4 view, glm::mat4 projection);

	void SetupBoardFromFEN(std::string fen);

	void RenderTiles(int selectedObjectId);
	void RenderPieces();

	Shader boardShader, pieceShader, pickingShader;
	
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

	GLuint tileColorLocation, tileColorModLocation, tileModelLocation, tileViewLocation, tileProjectionLocation;
	GLuint pieceModelLocation, pieceViewLocation, pieceProjectionLocation;
	GLuint pickingModelLocation, pickingViewLocation, pickingProjectionLocation, objectIdLocation, drawIdLocation;
	GLuint VAO, EBO, VBO;

	GLuint piecesTextureId;

	Piece* pieces[64];

	glm::vec3 blackTileColor = glm::vec3(0.4f, 0.6f, 0.4f);
	glm::vec3 whiteTileColor = glm::vec3(1.f, 1.f, 0.8f);

	float tileSize = 0.13f;

	float aspect;

	PieceTeam currentTurn;
	void CompleteTurn() { currentTurn = currentTurn == WHITE ? BLACK : WHITE; }

	struct EdgesFromTile
	{
		int left;
		int right;
		int top;
		int bottom;

		void Print() { printf("Left: %i, Right: %i, Top: %i, Bottom: %i\n", left, right, top, bottom); }
	};

	EdgesFromTile edgesFromTiles[64];
	void CalculateEdges();

	bool CheckLegalMove(int startTile, int endTile);
	bool CheckKingMove(int startTile, int endTile) const;
	bool CheckQueenMove(int startTile, int endTile) const;
	bool CheckBishopMove(int startTile, int endTile) const;
	bool CheckKnightMove(int startTile, int endTile) const;
	bool CheckRookMove(int startTile, int endTile) const;
	bool CheckPawnMove(int startTile, int endTile);

	void HandleEnPassant();
	int lastEnPassantIndex;

};

