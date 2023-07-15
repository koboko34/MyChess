#include "EvalBoard.h"

#include <memory>
#include <thread>

#ifdef TESTING
#include "Timer.h"
#endif

EvalBoard::EvalBoard()
{
	for (int i = 0; i < 64; i++)
	{
		pieces[i].Init(PieceTeam::NONE, PieceType::NONE, true);
	}

	bTesting = false;
	bSearching = false;
	bShouldSearch = false;
	maxDepth = 2;
	eval = 0;
}

EvalBoard::~EvalBoard()
{
	for (PieceType type : promotionTypes)
	{
		delete promotionPieces[type];
	}
	promotionPieces.clear();
}

void EvalBoard::Init(irrklang::ISoundEngine* engine)
{
	soundEngine = engine;

	SetupPromotionPieces();
	SetBoardCoords();
	PrepEdges();
	CalculateEdges();
}

void EvalBoard::StartEval(const int depth)
{
	maxDepth = depth;

	// evalThread.~thread();
	evalThread = std::thread([this] { this->IterSearch(); });
	printf("TEST\n");
}

void EvalBoard::StopEval()
{
	bShouldSearch = false;
}

void EvalBoard::ShannonTestCallback()
{
	const int depth = 3;
	int moveCount;

	bTesting = true;
	soundEngine->setSoundVolume(0.f);

	printf("\nStarting test suite at depth %i:\n", depth);

	for (size_t currentDepth = 1; currentDepth <= depth; currentDepth++)
	{
		SetupGame(true);
		int ply = 1;

		auto start = std::chrono::high_resolution_clock::now();
		moveCount = ShannonTest(ply, currentDepth);
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<float> duration = end - start;

		printf("Depth %i: %i possible moves. Calculated in: %f seconds.\n", currentDepth, moveCount, duration.count());
	}

	printf("Testing complete!\n");

	bTesting = false;
	soundEngine->setSoundVolume(1.f);
}

int EvalBoard::ShannonTest(int ply, const int depth)
{
	int moveCount = 0;

	if (ply > depth || bGameOver)
	{
		return 1;
	}

	// save current board state (locations and whether piece moved for castling etc)
	std::unique_ptr<BoardState> boardState = std::make_unique<BoardState>(this);

	std::vector<int> attackMap[64];
	if (currentTurn == PieceTeam::WHITE)
	{
		std::copy(std::begin(attackMapWhite), std::end(attackMapWhite), std::begin(attackMap));
	}
	else
	{
		std::copy(std::begin(attackMapBlack), std::end(attackMapBlack), std::begin(attackMap));
	}

	// for each move
	for (size_t startTile = 0; startTile < 64; startTile++)
	{
		std::vector<int> movesFound;

		if (!IsActivePiece(startTile) || pieces[startTile].GetTeam() != currentTurn || attackMap[startTile].empty())
			continue;

		for (int move : attackMap[startTile])
		{
			if (TileInContainer(move, movesFound))
			{
				printf("MOVE ALREADY IN CONTAINER! FOUND MORE THAN ONCE!\n");
			}
			movesFound.push_back(move);

			// since CompleteTurn() wipes attack maps, copy the relevant attack map entry so that checks can be carried out
			currentTurn == PieceTeam::WHITE ? attackMapWhite[startTile].clear() : attackMapBlack[startTile].clear();
			for (int tileMove : attackMap[startTile])
			{
				currentTurn == PieceTeam::WHITE ? attackMapWhite[startTile].push_back(tileMove) : attackMapBlack[startTile].push_back(tileMove);
			}

			// play move
			MovePiece(startTile, move);

			// calc deeper moves with recursion and add
			moveCount += ShannonTest(ply + 1, depth);

			// undo move by restoring board state
			RecoverBoardState(boardState.get());
		}
	}

	return moveCount;
}

void EvalBoard::RecoverPieceMovedState(const std::vector<PieceMovedState>& pieceMovedStates)
{
	for (PieceMovedState boardState : pieceMovedStates)
	{
		pieces[boardState.tile].bMoved = boardState.bMoved;
	}
}

void EvalBoard::CapturePieceMovedState(std::vector<PieceMovedState>& pieceMovedState)
{
	for (size_t i = 0; i < 64; i++)
	{
		if (!IsActivePiece(i))
			continue;

		if (pieces[i].GetType() == PAWN || pieces[i].GetType() == ROOK || pieces[i].GetType() == KING)
		{
			PieceMovedState state = PieceMovedState(i, pieces[i].bMoved);
			pieceMovedState.push_back(state);
		}
	}
}

void EvalBoard::RecoverBoardState(BoardState* boardState)
{
	SetupBoardFromFEN(boardState->fen);
	RecoverPieceMovedState(boardState->pieceMovedStates);
	ClearPinnedPieces();
	currentTurn = boardState->turn;
	bGameOver = false;
	bInCheckWhite = boardState->bLocalCheckWhite;
	bInCheckBlack = boardState->bLocalCheckBlack;
	lastMoveStart = boardState->lastMoveStart;
	lastMoveEnd = boardState->lastMoveEnd;
}

//void EvalBoard::SetupBoardFromFEN(const std::string& fen)
//{
//	for (size_t i = 0; i < 64; i++)
//	{
//		if (!IsActivePiece(i))
//			continue;
//
//		pieces[i].ClearPiece();
//	}
//
//	int index = 0;
//	PieceTeam team;
//
//	for (char c : fen)
//	{
//		// if letter, place piece
//		if (isalpha(c))
//		{
//			if (islower(c))
//			{
//				team = PieceTeam::BLACK;
//			}
//			else
//			{
//				team = PieceTeam::WHITE;
//			}
//
//			switch (tolower(c))
//			{
//			case 'r':
//				pieces[index].SetPiece(team, ROOK);
//				pieces[index].bMoved = false;
//				index++;
//				continue;
//			case 'n':
//				pieces[index].SetPiece(team, KNIGHT);
//				pieces[index].bMoved = false;
//				index++;
//				continue;
//			case 'b':
//				pieces[index].SetPiece(team, BISHOP);
//				pieces[index].bMoved = false;
//				index++;
//				continue;
//			case 'q':
//				pieces[index].SetPiece(team, QUEEN);
//				pieces[index].bMoved = false;
//				index++;
//				continue;
//			case 'k':
//				pieces[index].SetPiece(team, KING);
//				pieces[index].bMoved = false;
//				index++;
//				continue;
//			case 'p':
//				pieces[index].SetPiece(team, PAWN);
//				pieces[index].bMoved = false;
//				index++;
//				continue;
//			case 'e':
//				pieces[index].SetPiece(team, EN_PASSANT);
//				lastEnPassantIndex = index;
//				index++;
//				continue;
//			}
//		}
//
//		// if number, traverse board
//		if (isdigit(c))
//		{
//			int moveDistance = c - '0';
//			index += moveDistance;
//		}
//	}
//
//	if (InMapRange(lastEnPassantIndex) && IsActivePiece(lastEnPassantIndex))
//	{
//		if (pieces[lastEnPassantIndex].GetTeam() == PieceTeam::WHITE)
//		{
//			enPassantOwner = lastEnPassantIndex - 8;
//		}
//		else if (pieces[lastEnPassantIndex].GetTeam() == PieceTeam::BLACK)
//		{
//			enPassantOwner = lastEnPassantIndex + 8;
//		}
//	}
//	lastEnPassantIndex = -1;
//}

int EvalBoard::EvaluatePosition() const
{
	int eval = CalcWhiteValue() - CalcBlackValue();
	int perspective = currentTurn == PieceTeam::WHITE ? 1 : -1;

	return eval * perspective;
}

void EvalBoard::HandleEval()
{
	return;
}

int EvalBoard::Search(const int ply, const int depth)
{
	int eval = 0;

	if (ply > depth)
	{
		return eval = EvaluatePosition();
	}

	int bestEval = -999;

	std::vector<Move> bestMoves;

	// save current board state (locations, whether piece moved for castling etc)
	std::unique_ptr<BoardState> boardState = std::make_unique<BoardState>(this);

	std::vector<int> attackMap[64];
	if (currentTurn == PieceTeam::WHITE)
	{
		std::copy(std::begin(attackMapWhite), std::end(attackMapWhite), std::begin(attackMap));
	}
	else
	{
		std::copy(std::begin(attackMapBlack), std::end(attackMapBlack), std::begin(attackMap));
	}

	std::vector<CheckingPiece> checkingPieces = currentTurn == PieceTeam::WHITE ? checkingPiecesBlack : checkingPiecesWhite;

	// for each move
	for (size_t startTile = 0; startTile < 64; startTile++)
	{
		std::vector<int> movesFound;

		if (!IsActivePiece(startTile) || pieces[startTile].GetTeam() != currentTurn || attackMap[startTile].empty())
			continue;

		for (int move : attackMap[startTile])
		{
#ifdef TESTING
			if (TileInContainer(move, movesFound))
			{
				printf("MOVE ALREADY IN CONTAINER! FOUND MORE THAN ONCE!\n");
			}
#endif
			movesFound.push_back(move);

			// since CompleteTurn() wipes attack maps, copy the relevant attack map entry so that checks can be carried out
			currentTurn == PieceTeam::WHITE ? attackMapWhite[startTile].clear() : attackMapBlack[startTile].clear();
			for (int tileMove : attackMap[startTile])
			{
				currentTurn == PieceTeam::WHITE ? attackMapWhite[startTile].push_back(tileMove) : attackMapBlack[startTile].push_back(tileMove);
			}

			// if next move reaches max depth, don't calculate further moves
			ply == depth ? bSearchEnd = true : bSearchEnd = false;

			// play move
			MovePiece(startTile, move);

			// calc deeper moves with recursion and add
			eval = -Search(ply + 1, depth);

			if (eval > bestEval && ply == 1)
			{
				bestEval = eval;
				bestMoves.clear();
				bestMoves.emplace(bestMoves.end(), startTile, move);
			}
			else if (eval == bestEval && ply == 1)
			{
				bestMoves.emplace(bestMoves.end(), startTile, move);
			}
			else if (eval >= bestEval)
			{
				bestEval = eval;
			}

			// undo move by restoring board state
			RecoverBoardState(boardState.get());
			if (currentTurn == PieceTeam::WHITE)
			{
				checkingPiecesBlack = checkingPieces;
			}
			else
			{
				checkingPiecesWhite = checkingPieces;
			}
		}

		// restore attackMap, recovering from cache rather than calculating again
		if (currentTurn == PieceTeam::WHITE)
		{
			std::copy(std::begin(attackMap), std::end(attackMap), std::begin(attackMapWhite));
		}
		else
		{
			std::copy(std::begin(attackMap), std::end(attackMap), std::begin(attackMapBlack));
		}

		if (movesFound.empty())
		{
			if ((currentTurn == PieceTeam::WHITE && bInCheckWhite) || (currentTurn == PieceTeam::BLACK && bInCheckBlack))
			{
				return -999; // nothing is worse than checkmate
			}
			else
			{
				return 0; // stalemate
			}
		}
	}

	if (ply == 1)
	{
		SetBestMoves(bestMoves);
	}

	return bestEval;
}

void EvalBoard::IterSearch()
{
	bShouldSearch = true;
	bSearching = true;
	SetupBoardFromFEN(fen);
	CalculateMoves();
	
	int depth = 1;
	while (depth <= maxDepth && bShouldSearch)
	{
		printf("Calculating eval at depth %i...\n", depth);
		int eval = Search(1, depth);
		printf("Depth %i, Eval: %i %s\n", depth, eval, currentTurn == PieceTeam::WHITE ? "WHITE" : "BLACK");
		depth++;
	}
	printf("Search done!\n");

	bSearching = false;
	bShouldSearch = false;
	return;
}
