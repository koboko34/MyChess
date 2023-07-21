#pragma once

#include <thread>

#include "Board.h"

class EvalBoard : public Board
{
public:
	EvalBoard();
	~EvalBoard();

	void Init(Board* newBoard, irrklang::ISoundEngine* engine);

	void StartEval(const int depth);
	void StopEval();

	void SetFEN(std::string fen) { this->fen = fen; }
	void SetMovedStates(const std::vector<PieceMovedState>& pieceMovedStates);
	void SetCurrentTurn(PieceTeam team) { currentTurn = team; }

	int GetEval() const { return eval; }
	bool IsSearching() const { return bSearching; }

	void IterDeepSearch();

	void ShannonTestCallback();

private:
	Board* board;
	
	bool bTesting;
	bool bShouldSearch;
	bool bEarlyExit;
	int eval;
	int maxDepth;

	std::string fen;

	std::vector<PieceMovedState> pieceMovedStates;

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

	int ShannonTest(const int ply, const int depth);

	virtual void HandleEval() override;
	int EvaluatePosition() const;

	// Returns eval as experienced by currentTeam. For example, if black is up by 2 pawns and it is black's turn, the function will return 2.
	// For normalised eval, multiply by 1 if currentTeam == WHITE and multiply by -1 if currentTeam == BLACK
	int Search(const int ply, const int depth);
	int SearchAllCaptures(bool bCaptureFound);
};

