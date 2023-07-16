#pragma once

#include <thread>

#include "Board.h"

class EvalBoard : public Board
{
public:
	EvalBoard();
	~EvalBoard();

	void Init(irrklang::ISoundEngine* engine);

	void StartEval(const int depth);
	void StopEval();

	void SetFEN(std::string fen) { this->fen = fen; }
	void SetCurrentTurn(PieceTeam team) { currentTurn = team; }

	int GetEval() const { return eval; }

	void IterDeepSearch();

	void ShannonTestCallback();

private:
	bool bTesting;
	bool bShouldSearch;
	bool bEarlyExit;
	int eval;
	int maxDepth;

	std::string fen;

	struct PieceMovedState
	{
		PieceMovedState(int tile, bool bMoved)
		{
			this->tile = tile;
			this->bMoved = bMoved;
		}

		int tile;
		bool bMoved;
	};

	void CapturePieceMovedState(std::vector<PieceMovedState>& pieceMovedState);
	void RecoverPieceMovedState(const std::vector<PieceMovedState>& pieceMovedStates);

	struct BoardState
	{
		BoardState(EvalBoard* board)
		{
			fen = board->BoardToFEN();
			board->CapturePieceMovedState(pieceMovedStates);
			this->kingXRay = board->kingXRay;
			turn = board->currentTurn;
			bLocalCheckWhite = board->bInCheckWhite;
			bLocalCheckBlack = board->bInCheckBlack;
			this->lastMoveStart = board->lastMoveStart;
			this->lastMoveEnd = board->lastMoveEnd;
		}

		~BoardState()
		{

		}

		std::string fen;
		std::vector<PieceMovedState> pieceMovedStates;
		std::set<int> kingXRay;
		PieceTeam turn;
		bool bLocalCheckWhite;
		bool bLocalCheckBlack;
		int lastMoveStart;
		int lastMoveEnd;
	};

	void RecoverBoardState(BoardState* boardState);

	int ShannonTest(int ply, const int depth);

	// virtual void SetupBoardFromFEN(const std::string& fen) override;

	virtual void HandleEval() override;
	int EvaluatePosition() const;
	int Search(const int ply, const int depth);

	std::thread evalThread;
};

