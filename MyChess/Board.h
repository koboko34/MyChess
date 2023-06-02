#pragma once

#include <string>
#include <algorithm>
#include <array>
#include <vector>

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

	bool IsGameOver() { return bGameOver; }

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
	void CompleteTurn();

	std::vector<int> firstRank;
	std::vector<int> eighthRank;
	std::vector<int> aFile;
	std::vector<int> bFile;
	std::vector<int> gFile;
	std::vector<int> hFile;
	std::vector<int> topLeft;
	std::vector<int> topRight;
	std::vector<int> bottomLeft;
	std::vector<int> bottomRight;

	bool TileInArray(int target, std::vector<int> arr) const;

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
	void PrepEdges();
	void CalculateEdges();

	bool CheckLegalMove(int startTile, int endTile);

	void CalcKingMoves(int startTile);
	void CalcQueenMoves(int startTile);
	void CalcBishopMoves(int startTile);
	void CalcKnightMoves(int startTile);
	void CalcRookMoves(int startTile);
	void CalcPawnMoves(int startTile);

	bool BlockedByOwnPiece(int startTile, int target) const;
	bool BlockedByEnemyPiece(int startTile, int target) const;

	void AddNotBlocked(int startTile, int target, std::vector<int>& validMoves);

	bool CheckCanCastle(int startTile, int target, int rookPos, int dir) const;
	void HandleCastling(int startTile, int endTile);

	void CreateEnPassant(int startTile, int endTile);
	void ClearEnPassant();
	void TakeByEnPassant();
	int lastEnPassantIndex;
	Piece* enPassantOwner;

	std::vector<int> attackMapWhite[64];
	std::vector<int> attackMapBlack[64];
	void CalculateMoves();

	bool InMapRange(int index) const { return 0 <= index && index < 64; }

	void AddToMap(int startTile, std::vector<int> validMoves);

	bool bInCheckWhite;
	bool bInCheckBlack;

	// calculate pinned pieces

	void GameOver(int winningTeam);

	bool bGameOver;
	int winner;

};

