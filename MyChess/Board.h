#pragma once

#include <string>
#include <algorithm>
#include <array>

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
	bool IsCurrentTurn(int index) const { return pieces[index]->GetTeam() == currentTurn; }
	void CompleteTurn() { currentTurn = currentTurn == WHITE ? BLACK : WHITE; }

	int firstRank[8] = { 56, 57, 58, 59, 60, 61, 62, 63 };
	int eighthRank[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	int aFile[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };
	int bFile[8] = { 1, 9, 17, 25, 33, 41, 49, 57 };
	int gFile[8] = { 6, 14, 22, 30, 38, 46, 54, 62 };
	int hFile[8] = { 7, 15, 23, 31, 39, 47, 55, 63 };

	bool TileInArray(int target, const int arr[]) const;

	struct EdgesFromTile
	{
		int left;
		int right;
		int top;
		int bottom;

		int topLeft;
		int topRight;
		int bottomLeft;
		int bottomRight;

		void Print() { printf("Left: %i, Right: %i, Top: %i, Bottom: %i, TopLeft: %i, TopRight: %i, BottomLeft: %i, BottomRight: %i\n",
							left, right, top, bottom, topLeft, topRight, bottomLeft, bottomRight); }
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

	bool KnightMovesRight(int startTile, int endTile) const;
	bool KnightMovesLeft(int startTile, int endTile) const;

	void HandleEnPassant();
	int lastEnPassantIndex;

};

