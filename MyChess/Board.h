#pragma once

#include <string>
#include <algorithm>
#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <random>
#include <functional>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include <irrKlang/irrKlang.h>

#include "CommonValues.h"

#include "Shader.h"
#include "Piece.h"

class Board
{
public:
	Board();
	~Board();

	void Init(unsigned int windowWidth, unsigned int windowHeight, GLFWwindow* window, irrklang::ISoundEngine* sEngine);
	void RenderScene(int selectedObjectId);
	void DrawBoard(int selectedObjectId);
	void PickingPass();

	bool MovePiece(int startTile, int endTile);

	bool PieceExists(int index);
	bool IsChoosingPromotion() { return bChoosingPromotion; }
	bool IsGameOver() { return bGameOver; }
	bool InMainMenu() { return bInMainMenu; }

	void Promote(PieceType pieceType);

	void ButtonCallback(int id);

private:

	GLFWwindow* window;

	unsigned int width, height;

	void SetupBoard(glm::mat4 view, glm::mat4 projection);
	void SetupPieces(glm::mat4 view, glm::mat4 projection);
	void SetupPickingShader(glm::mat4 view, glm::mat4 projection);
	void SetupPromotionPieces();
	void SetupBoardFromFEN(std::string fen);

	bool ShouldHighlightSelectedObject(int selectedObjectId, int objectId);
	bool ShouldHighlightLastMove(int objectId);

	void RenderTiles(int selectedObjectId);
	void RenderPieces();

	void RenderPromotionTiles();
	void RenderPromotionPieces();

	struct Button
	{
		Button(Board* board, float xPositon, float xPositionOffset, float yPosition, float xSize, float xSizeOffset, float ySize)
		{
			this->board = board;

			xPos = xPositon;
			xPosOffset = xPositionOffset;
			yPos = yPosition;
			this->xSize = xSize;
			this->xSizeOffset = xSizeOffset;
			this->ySize = ySize;
		}

		void SetCallback(std::function<void()> fn) { callback = fn; }

		void RemoveButton()
		{
			while (0 < board->buttons.size())
			{
				if (board->buttons[0]->id == id)
				{
					board->buttons.erase(board->buttons.begin());
					return;
				}
			}
		}

		~Button()
		{
			RemoveButton();
		}

		Board* board;
		std::function<void()> callback;

		int id;

		float xPos;
		float xPosOffset;
		float yPos;
		float xSize;
		float xSizeOffset;
		float ySize;
	};

	std::vector<Button*> buttons;
	void ClearButtons();
	void RenderButton(Button* button);
	void RenderButtons();

	void RenderContinueButton();
	float buttonHeight = 0.13f;
	float buttonWidth = 0.35f;

	bool bInMainMenu;
	bool bInGame;
	void ShowMenuButtons();

	void PlaySingleplayerCallback();
	void PlayWhiteCallback();
	void PlayBlackCallback();
	void PlayMultiplayerCallback();
	void QuitGameCallback();
	void ContinueCallback();

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

	GLuint tileColorLocation, tileModelLocation, tileViewLocation, tileProjectionLocation;
	GLuint pieceModelLocation, pieceViewLocation, pieceProjectionLocation;
	GLuint pickingModelLocation, pickingViewLocation, pickingProjectionLocation, objectIdLocation, drawIdLocation;
	GLuint VAO, EBO, VBO;

	GLuint piecesTextureId;

	Piece* pieces[64];
	
	PieceType promotionTypes[4] = { QUEEN, ROOK, BISHOP, KNIGHT };
	std::unordered_map<int, Piece*> promotionPieces;

	glm::vec3 blackTileColor = glm::vec3(0.4f, 0.6f, 0.4f);
	glm::vec3 whiteTileColor = glm::vec3(1.f, 1.f, 0.8f);
	glm::vec3 selectColor = glm::vec3(1.f, 0.3f, 0.2f);
	glm::vec3 lastMoveColor = glm::vec3(0.9f, 0.5f, 0.3f);

	float tileSize = 0.13f;

	float aspect;

	irrklang::ISoundEngine* soundEngine;

	enum MoveSounds
	{
		NONE = 0,
		MOVE_SELF = 1,
		MOVE_OPP = 2,
		CAPTURE = 3,
		CHECK = 4,
		CASTLE = 5,
		CHECKMATE = 6,
		PROMOTE = 7,
	};

	MoveSounds lastMoveSound;
	void PlayMoveSound();
	bool bSetPromoSound;

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

	void CalcSlidingMovesOneDir(int startTile, int min, int max, int dir, int kingPos, bool& foundKing, std::vector<int>& checkLOS, std::vector<int>& attackingTiles);
	void CalcKnightMovesOneDir(int startTile, int dir, int kingPos, std::vector<int>& checkLOS, std::vector<int>& attackingTiles);
	void HandleFoundMoves(int startTile, bool& foundKing, std::vector<int>& checkLOS, std::vector<int>& attackingTiles);

	void CalcKingMoves(int startTile);
	void CalcQueenMoves(int startTile);
	void CalcBishopMoves(int startTile);
	void CalcKnightMoves(int startTile);
	void CalcRookMoves(int startTile);
	void CalcPawnMoves(int startTile);

	int lastMoveStart;
	int lastMoveEnd;

	bool BlockedByOwnPiece(int startTile, int target) const;
	bool BlockedByEnemyPiece(int startTile, int target) const;

	void AddNotBlocked(int startTile, int target, std::vector<int>& validMoves);

	void CalculateCastling();
	bool CheckCanCastle(int startTile, int target, int rookPos, int dir) const;
	void HandleCastling(int startTile, int endTile);

	void CreateEnPassant(int startTile, int endTile);
	void ClearEnPassant();
	void TakeByEnPassant();
	int lastEnPassantIndex;
	Piece* enPassantOwner;

	bool bChoosingPromotion;
	int pieceToPromote;
	void HandlePromotion(int endTile);

	std::vector<int> attackMapWhite[64];
	std::vector<int> attackMapBlack[64];
	void CalculateMoves();

	std::set<int> attackSetWhite;
	std::set<int> attackSetBlack;
	void CalculateAttacks();
	void AddToAttackSet(int startTile, int target);

	std::set<int> kingXRay; // squares behind the king which checking pieces can see, king cannot escape to these squares

	template <typename T>
	bool TileInContainer(int target, T container) const;

	bool InMapRange(int index) const { return 0 <= index && index < 64; }

	void AddToMap(int startTile, std::vector<int> validMoves);

	int kingPosWhite;
	int kingPosBlack;
	void FindKings();
	void SetKingPos(int target);

	bool bInCheckWhite;
	bool bInCheckBlack;
	void CalculateCheck();
	void ClearMoves(int team);
	void SetCheckMoves();

	struct CheckingPiece
	{
		PieceType pieceType;
		int tile;
		std::vector<int> lineOfSight;
	};

	struct PinnedPiece
	{
		int tile;
		std::vector<int> lineOfSight;
	};

	std::vector<PinnedPiece*> pinnedPiecesWhite;
	std::vector<PinnedPiece*> pinnedPiecesBlack;

	void ClearPinnedPieces();
	void AddPinnedPiece(int startTile, const std::vector<int>& pinLOS);
	void HandlePinnedPieces();

	bool MoveBlocksCheck(int startTile, int endTile);
	bool CanBlockCheck(int kingPos, int& moveCount);
	bool KingEscapesCheck(int endTile);
	bool CanKingEscape(int startTile, int& moveCount);
	bool MoveTakesCheckingPiece(int endTile);
	bool CanTakeCheckingPiece(int kingPos, int& moveCount);

	std::vector<int> validCheckMoves[64];
	int CalcValidCheckMoves();
	void ClearValidCheckMoves();

	void AddCheckingPiece(int startTile, const std::vector<int>& checkLOS);
	void AddProtectedPieceToSet(int target);

	std::vector<CheckingPiece*> checkingPiecesWhite;
	std::vector<CheckingPiece*> checkingPiecesBlack;

	void ClearCheckingPieces();

	void GameOver(int winningTeam);
	void SetupGame();

	bool bGameOver;
	int winner;

	bool bVsComputer = false; // set dynamically on main menu once added
	PieceTeam compTeam = BLACK; // set dynamically when selecting team

	void PlayCompMove();

};

