#ifndef COMMONVALS
#define COMMONVALS

#include "stb_image.h"

enum PieceType
{
	KING = 0,
	QUEEN = 1,
	BISHOP = 2,
	KNIGHT = 3,
	ROOK = 4,
	PAWN = 5,
	EN_PASSANT = 6
};

enum PieceTeam
{
	WHITE = 0,
	BLACK = 1
};

#endif