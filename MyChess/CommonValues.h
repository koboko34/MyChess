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
	NONE = 0,
	WHITE = 1,
	BLACK = 2
};

enum BoardDir
{
	TOP_LEFT = -9,
	UP = -8,
	TOP_RIGHT = -7,
	RIGHT = 1,
	BOT_RIGHT = 9,
	DOWN = 8,
	BOT_LEFT = 7,
	LEFT = -1
};

#endif