#include "Board.h"

Board::Board()
{
	VAO, EBO, VBO = 0;
	currentTurn = WHITE;
	lastEnPassantIndex = -1;
	enPassantOwner = nullptr;

	bInCheckBlack = false;
	bInCheckWhite = false;
	bGameOver = false;
	winner = 0;
}

Board::~Board()
{
	for (Piece* piece : pieces)
	{
		delete piece;
		piece = nullptr;
	}

	if (VAO != 0)
	{
		glDeleteVertexArrays(1, &VAO);
	}
	if (EBO != 0)
	{
		glDeleteBuffers(1, &EBO);
	}
	if (VBO != 0)
	{
		glDeleteBuffers(1, &VBO);
	}

	if (piecesTextureId != 0)
	{
		glDeleteTextures(1, &piecesTextureId);
	}
}

void Board::Init(unsigned int width, unsigned int height)
{
	glm::mat4 view = glm::mat4(1.f);
	view = glm::translate(view, glm::vec3(0.f, 0.f, -1.f));

	aspect = (float)width / (float)height;
	glm::mat4 projection = glm::ortho(0.f, aspect, 0.f, 1.f, 0.f, 100.f);

	SetupBoard(view, projection);
	SetupPieces(view, projection);
	SetupPickingShader(view, projection);

	SetupBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

	PrepEdges();
	CalculateEdges();
	CalculateMoves();
}

// ========================================== RENDERING ==========================================

void Board::DrawBoard(int selectedObjectId)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.5f, 0.3f, 0.2f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	boardShader.UseShader();
	RenderTiles(selectedObjectId);
	
	pieceShader.UseShader();
	RenderPieces();
}

void Board::SetupBoard(glm::mat4 view, glm::mat4 projection)
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tileIndices), tileIndices, GL_STATIC_DRAW);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tileVertices), tileVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	boardShader.CompileShaders("shaders/boardShader.vert", "shaders/boardShader.frag");
	boardShader.UseShader();

	tileColorLocation = glGetUniformLocation(boardShader.GetShaderId(), "tileColor");
	tileColorModLocation = glGetUniformLocation(boardShader.GetShaderId(), "colorMod");
	tileModelLocation = glGetUniformLocation(boardShader.GetShaderId(), "model");
	tileViewLocation = glGetUniformLocation(boardShader.GetShaderId(), "view");
	tileProjectionLocation = glGetUniformLocation(boardShader.GetShaderId(), "projection");

	glUniformMatrix4fv(tileViewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(tileProjectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
}

void Board::SetupPieces(glm::mat4 view, glm::mat4 projection)
{
	glActiveTexture(GL_TEXTURE0);
	
	glGenTextures(1, &piecesTextureId);
	glBindTexture(GL_TEXTURE_2D, piecesTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int texWidth, texHeight, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load("textures/ChessPiecesArray.png", &texWidth, &texHeight, &nrChannels, STBI_rgb_alpha);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		printf("Failed to load texture!");
	}
	stbi_image_free(data);

	pieceShader.CompileShaders("shaders/pieceShader.vert", "shaders/pieceShader.frag");
	pieceShader.UseShader();

	pieceModelLocation = glGetUniformLocation(pieceShader.GetShaderId(), "model");
	pieceViewLocation = glGetUniformLocation(pieceShader.GetShaderId(), "view");
	pieceProjectionLocation = glGetUniformLocation(pieceShader.GetShaderId(), "projection");

	glUniformMatrix4fv(pieceViewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(pieceProjectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

	for (int i = 0; i < 64; i++)
	{
		pieces[i] = nullptr;
	}
}

void Board::SetupPickingShader(glm::mat4 view, glm::mat4 projection)
{
	pickingShader.CompileShaders("shaders/pickingShader.vert", "shaders/pickingShader.frag");
	pickingShader.UseShader();

	pickingModelLocation = glGetUniformLocation(pickingShader.GetShaderId(), "model");
	pickingViewLocation = glGetUniformLocation(pickingShader.GetShaderId(), "view");
	pickingProjectionLocation = glGetUniformLocation(pickingShader.GetShaderId(), "projection");
	objectIdLocation = glGetUniformLocation(pickingShader.GetShaderId(), "objectId");
	drawIdLocation = glGetUniformLocation(pickingShader.GetShaderId(), "drawId");

	unsigned int tempDrawId = 3;
	glUniform1ui(drawIdLocation, tempDrawId);

	glUniformMatrix4fv(pickingViewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(pickingProjectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
}

void Board::PickingPass()
{
	pickingShader.UseShader();
	glBindVertexArray(VAO);

	for (size_t rank = 8; rank >= 1; rank--)
	{
		for (size_t file = 1; file <= 8; file++)
		{
			unsigned int objectId = (8 - rank) * 8 + file - 1;
			GLuint adjustedObjectId = objectId + 1;
			glUniform1ui(objectIdLocation, adjustedObjectId);

			glm::mat4 tileModel = glm::mat4(1.f);
			tileModel = glm::translate(tileModel, glm::vec3((aspect / 13.7f) * file + 0.305f, (rank * 2 - 1) / 16.f, 1.f));
			tileModel = glm::scale(tileModel, glm::vec3(tileSize, tileSize, 1.f));
			glUniformMatrix4fv(pickingModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
	}

	glBindVertexArray(0);
}

void Board::RenderTiles(int selectedObjectId)
{
	glBindVertexArray(VAO);

	for (size_t rank = 8; rank >= 1; rank--)
	{
		for (size_t file = 1; file <= 8; file++)
		{
			if ((rank % 2 == 0) != (file % 2 == 0))
			{
				glUniform3f(tileColorLocation, whiteTileColor.x, whiteTileColor.y, whiteTileColor.z);
			}
			else
			{
				glUniform3f(tileColorLocation, blackTileColor.x, blackTileColor.y, blackTileColor.z);
			}

			unsigned int objectId = (8 - rank) * 8 + file - 1;
			if (InMapRange(selectedObjectId) && ((selectedObjectId == objectId && pieces[objectId] != nullptr) ||
				TileInContainer(objectId, currentTurn == WHITE ? attackMapWhite[selectedObjectId] : attackMapBlack[selectedObjectId])))
			{
				glUniform1i(tileColorModLocation, 0);
			}
			else
			{
				glUniform1i(tileColorModLocation, 1);
			}

			glm::mat4 tileModel = glm::mat4(1.f);
			tileModel = glm::translate(tileModel, glm::vec3((aspect / 13.7f) * file + 0.305f, (rank * 2 - 1) / 16.f, -2.f));
			tileModel = glm::scale(tileModel, glm::vec3(tileSize, tileSize, 1.f));
			glUniformMatrix4fv(tileModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
	}

	glBindVertexArray(0);
}

void Board::RenderPieces()
{	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, piecesTextureId);
	
	for (size_t rank = 8; rank >= 1; rank--)
	{
		for (size_t file = 1; file <= 8; file++)
		{
			int objectId = (8 - rank) * 8 + file - 1;

			if (pieces[objectId] == nullptr)
			{
				continue;
			}

			glm::mat4 pieceModel = glm::mat4(1.f);
			pieceModel = glm::translate(pieceModel, glm::vec3((aspect / 13.7f) * file + 0.305f, (rank * 2 - 1) / 16.f, 0.f));
			pieceModel = glm::scale(pieceModel, glm::vec3(tileSize, tileSize, 1.f));
			glUniformMatrix4fv(pieceModelLocation, 1, GL_FALSE, glm::value_ptr(pieceModel));

			pieces[objectId]->DrawPiece();
		}
	}

	glBindVertexArray(0);
}

// ========================================== MOVE LOGIC ==========================================

bool Board::MovePiece(int startTile, int endTile)
{
	// printf("Attempting to play a move...\n");

	if (pieces[startTile] == nullptr)
		return false;

	if (endTile < 0 || endTile >= 64 || startTile < 0 || startTile >= 64)
		return false;

	if (!IsCurrentTurn(startTile))
	{
		printf("It is %s's move!\n", currentTurn == WHITE ? "white" : "black");
		return false;
	}

	if (pieces[endTile] && pieces[startTile]->GetTeam() == pieces[endTile]->GetTeam())
	{
		printf("Cannot take your own team's piece!\n");
		return false;
	}

	// check piece specific move
	if (!CheckLegalMove(startTile, endTile))
	{
		printf("Not a legal move for this piece!\n");
		return false;
	}

	// check if move puts self in check by king moving into attacked square
	if (pieces[startTile]->GetType() == KING && TileInContainer(endTile, pieces[startTile]->GetTeam() == WHITE ? attackSetBlack : attackSetWhite))
	{
		printf("Cannot put yourself in check!\n");
		return false;
	}

	// check if move puts self in check by discovered check

	



	// checks complete
	if (pieces[endTile])
	{
		if (pieces[startTile]->GetType() == PAWN && pieces[endTile]->GetType() == EN_PASSANT)
		{
			// handle destruction of en passant piece owner when move is confirmed
			TakeByEnPassant();
		}
		delete pieces[endTile];
	}

	pieces[endTile] = pieces[startTile];
	pieces[startTile] = nullptr;
	pieces[endTile]->bMoved = true;

	switch (pieces[endTile]->GetType())
	{
	case PAWN:
		CreateEnPassant(startTile, endTile);
		break;
	case KING:
		HandleCastling(startTile, endTile);
		break;
	default:
		break;
	}

	CompleteTurn();
	return true;
}

bool Board::CheckLegalMove(int startTile, int endTile)
{	
	return TileInContainer(endTile, currentTurn == WHITE ? attackMapWhite[startTile] : attackMapBlack[startTile]);
}

void Board::CalcKingMoves(int startTile)
{
	std::vector<int> attackingTiles;

	int top = edgesFromTiles[startTile].top;
	int bottom = edgesFromTiles[startTile].bottom;
	int left = edgesFromTiles[startTile].left;
	int right = edgesFromTiles[startTile].right;
	int topLeft = edgesFromTiles[startTile].topLeft;
	int topRight = edgesFromTiles[startTile].topRight;
	int bottomLeft = edgesFromTiles[startTile].bottomLeft;
	int bottomRight = edgesFromTiles[startTile].bottomRight;

	// down
	int target = startTile + DOWN;
	if (top <= target && target <= bottom && !BlockedByOwnPiece(startTile, target))
		attackingTiles.push_back(target);
	
	// up
	target = startTile + UP;
	if (top <= target && target <= bottom && !BlockedByOwnPiece(startTile, target))
		attackingTiles.push_back(target);

	// left
	target = startTile + LEFT;
	if (left <= target && target <= right && startTile != left && !BlockedByOwnPiece(startTile, target))
		attackingTiles.push_back(target);

	// right
	target = startTile + RIGHT;
	if (left <= target && target <= right && startTile != right && !BlockedByOwnPiece(startTile, target))
		attackingTiles.push_back(target);

	// top left
	target = startTile + TOP_LEFT;
	if (topLeft <= target && target <= bottomRight && !TileInContainer(startTile, aFile) && !TileInContainer(startTile, eighthRank) && !BlockedByOwnPiece(startTile, target))
		attackingTiles.push_back(target);

	// top right
	target = startTile + TOP_RIGHT;
	if (topRight <= target && target <= bottomLeft && !TileInContainer(startTile, hFile) && !TileInContainer(startTile, eighthRank) && !BlockedByOwnPiece(startTile, target))
		attackingTiles.push_back(target);

	// bottom left
	target = startTile + BOT_LEFT;
	if (topRight <= target && target <= bottomLeft && !TileInContainer(startTile, aFile) && !TileInContainer(startTile, firstRank) && !BlockedByOwnPiece(startTile, target))
		attackingTiles.push_back(target);

	// bottom right
	target = startTile + BOT_RIGHT;
	if (topLeft <= target && target <= bottomRight && !TileInContainer(startTile, hFile) && !TileInContainer(startTile, firstRank) && !BlockedByOwnPiece(startTile, target))
		attackingTiles.push_back(target);

	if (pieces[startTile]->bMoved == false)
	{
		// long castle
		target = startTile - 2;
		int rookPos = target - 2;
		if (CheckCanCastle(startTile, target, rookPos, -1))
		{
			attackingTiles.push_back(target);
		}

		// short castle
		target = startTile + 2;
		rookPos = target + 1;
		if (CheckCanCastle(startTile, target, rookPos, 1))
		{
			attackingTiles.push_back(target);
		}
	}

	AddToMap(startTile, attackingTiles);
}

void Board::CalcQueenMoves(int startTile)
{
	CalcRookMoves(startTile);
	CalcBishopMoves(startTile);
}

void Board::CalcBishopMoves(int startTile)
{
	std::vector<int> attackingTiles;

	int topLeft = edgesFromTiles[startTile].topLeft;
	int topRight = edgesFromTiles[startTile].topRight;
	int bottomLeft = edgesFromTiles[startTile].bottomLeft;
	int bottomRight = edgesFromTiles[startTile].bottomRight;

	// top left
	int target = startTile + TOP_LEFT;
	while (topLeft <= target && target <= bottomRight && !BlockedByOwnPiece(startTile, target))
	{
		if (BlockedByEnemyPiece(startTile, target))
		{
			attackingTiles.push_back(target);
			break;
		}

		attackingTiles.push_back(target);
		target += TOP_LEFT;
	}
	
	// top right
	target = startTile + TOP_RIGHT;
	while (topRight <= target && target <= bottomLeft && !BlockedByOwnPiece(startTile, target))
	{
		if (BlockedByEnemyPiece(startTile, target))
		{
			attackingTiles.push_back(target);
			break;
		}

		attackingTiles.push_back(target);
		target += TOP_RIGHT;
	}
	
	// bottom left
	target = startTile + BOT_LEFT;
	while (topRight <= target && target <= bottomLeft && !BlockedByOwnPiece(startTile, target))
	{
		if (BlockedByEnemyPiece(startTile, target))
		{
			attackingTiles.push_back(target);
			break;
		}

		attackingTiles.push_back(target);
		target += BOT_LEFT;
	}

	// bottom right
	target = startTile + BOT_RIGHT;
	while (topLeft <= target && target <= bottomRight && !BlockedByOwnPiece(startTile, target))
	{
		if (BlockedByEnemyPiece(startTile, target))
		{
			attackingTiles.push_back(target);
			break;
		}

		attackingTiles.push_back(target);
		target += BOT_RIGHT;
	}
	
	AddToMap(startTile, attackingTiles);
	
}

void Board::CalcKnightMoves(int startTile)
{
	std::vector<int> attackingTiles;
	int target;

	if (TileInContainer(startTile, aFile))
	{
		// up up right
		target = startTile + UP + UP + RIGHT;
		AddNotBlocked(startTile, target, attackingTiles);

		// right right up
		target = startTile + RIGHT + RIGHT + UP;
		AddNotBlocked(startTile, target, attackingTiles);

		// right right down
		target = startTile + RIGHT + RIGHT + DOWN;
		AddNotBlocked(startTile, target, attackingTiles);

		// down down right
		target = startTile + DOWN + DOWN + RIGHT;
		AddNotBlocked(startTile, target, attackingTiles);
	}
	else if (TileInContainer(startTile, bFile))
	{
		// up up left
		target = startTile + UP + UP + LEFT;
		AddNotBlocked(startTile, target, attackingTiles);
		
		// up up right
		target = startTile + UP + UP + RIGHT;
		AddNotBlocked(startTile, target, attackingTiles);

		// right right up
		target = startTile + RIGHT + RIGHT + UP;
		AddNotBlocked(startTile, target, attackingTiles);

		// right right down
		target = startTile + RIGHT + RIGHT + DOWN;
		AddNotBlocked(startTile, target, attackingTiles);

		// down down right
		target = startTile + DOWN + DOWN + RIGHT;
		AddNotBlocked(startTile, target, attackingTiles);

		// down down left
		target = startTile + DOWN + DOWN + LEFT;
		AddNotBlocked(startTile, target, attackingTiles);
	}
	else if (TileInContainer(startTile, hFile))
	{
		// up up left
		target = startTile + UP + UP + LEFT;
		AddNotBlocked(startTile, target, attackingTiles);

		// left left up
		target = startTile + LEFT + LEFT + UP;
		AddNotBlocked(startTile, target, attackingTiles);

		// left left down
		target = startTile + LEFT + LEFT + DOWN;
		AddNotBlocked(startTile, target, attackingTiles);

		// down down left
		target = startTile + DOWN + DOWN + LEFT;
		AddNotBlocked(startTile, target, attackingTiles);
	}
	else if (TileInContainer(startTile, gFile))
	{
		// up up right
		target = startTile + UP + UP + RIGHT;
		AddNotBlocked(startTile, target, attackingTiles);

		// up up left
		target = startTile + UP + UP + LEFT;
		AddNotBlocked(startTile, target, attackingTiles);

		// left left up
		target = startTile + LEFT + LEFT + UP;
		AddNotBlocked(startTile, target, attackingTiles);

		// left left down
		target = startTile + LEFT + LEFT + DOWN;
		AddNotBlocked(startTile, target, attackingTiles);

		// down down left
		target = startTile + DOWN + DOWN + LEFT;
		AddNotBlocked(startTile, target, attackingTiles);

		// down down right
		target = startTile + DOWN + DOWN + RIGHT;
		AddNotBlocked(startTile, target, attackingTiles);
	}
	else
	{
		// up up right
		target = startTile + UP + UP + RIGHT;
		AddNotBlocked(startTile, target, attackingTiles);

		// right right up
		target = startTile + RIGHT + RIGHT + UP;
		AddNotBlocked(startTile, target, attackingTiles);

		// right right down
		target = startTile + RIGHT + RIGHT + DOWN;
		AddNotBlocked(startTile, target, attackingTiles);

		// down down right
		target = startTile + DOWN + DOWN + RIGHT;
		AddNotBlocked(startTile, target, attackingTiles);

		// up up left
		target = startTile + UP + UP + LEFT;
		AddNotBlocked(startTile, target, attackingTiles);

		// left left up
		target = startTile + LEFT + LEFT + UP;
		AddNotBlocked(startTile, target, attackingTiles);

		// left left down
		target = startTile + LEFT + LEFT + DOWN;
		AddNotBlocked(startTile, target, attackingTiles);

		// down down left
		target = startTile + DOWN + DOWN + LEFT;
		AddNotBlocked(startTile, target, attackingTiles);
	}

	AddToMap(startTile, attackingTiles);
}

void Board::CalcRookMoves(int startTile)
{
	std::vector<int> attackingTiles;
	
	int target = startTile + DOWN;

	int top = edgesFromTiles[startTile].top;
	int bottom = edgesFromTiles[startTile].bottom;

	// down
	while (top <= target && target <= bottom && !BlockedByOwnPiece(startTile, target))
	{
		if (BlockedByEnemyPiece(startTile, target))
		{
			attackingTiles.push_back(target);
			break;
		}

		attackingTiles.push_back(target);
		target += DOWN;
	}

	// up
	target = startTile + UP;
	while (top <= target && target <= bottom && !BlockedByOwnPiece(startTile, target))
	{
		if (BlockedByEnemyPiece(startTile, target))
		{
			attackingTiles.push_back(target);
			break;
		}

		attackingTiles.push_back(target);
		target += UP;
	}

	int left = edgesFromTiles[startTile].left;
	int right = edgesFromTiles[startTile].right;

	// right
	target = startTile + RIGHT;
	while (left <= target && target <= right && !BlockedByOwnPiece(startTile, target))
	{
		if (BlockedByEnemyPiece(startTile, target))
		{
			attackingTiles.push_back(target);
			break;
		}

		attackingTiles.push_back(target);
		target += RIGHT;
	}

	// left
	target = startTile + LEFT;
	while (left <= target && target <= right && !BlockedByOwnPiece(startTile, target))
	{
		if (BlockedByEnemyPiece(startTile, target))
		{
			attackingTiles.push_back(target);
			break;
		}

		attackingTiles.push_back(target);
		target += LEFT;
	}
	
	AddToMap(startTile, attackingTiles);
}

void Board::CalcPawnMoves(int startTile)
{
	// 4 cases, move forward by 1, move forward by 2 (only first move), take diagonal, take by en passant
	std::vector<int> attackingTiles;
	int teamDir = pieces[startTile]->GetTeam() == WHITE ? 1 : -1;
	int target;

	// forward by 1
	target = startTile + UP * teamDir;
	if (InMapRange(target) && !BlockedByOwnPiece(startTile, target) && !BlockedByEnemyPiece(startTile, target))
		attackingTiles.push_back(target);

	// forward by 2 if first move of this pawn
	// handle creation of en passant piece when move is confirmed
	target = startTile + (2 * UP) * teamDir;
	if (InMapRange(target) && !pieces[startTile]->bMoved && !BlockedByOwnPiece(startTile, target) && !BlockedByEnemyPiece(startTile, target) &&
		!BlockedByOwnPiece(startTile, target + DOWN * teamDir) && !BlockedByEnemyPiece(startTile, target + DOWN * teamDir))
		attackingTiles.push_back(target);

	// take on diagonal, local forward right
	// handle destruction of en passant piece when move is confirmed
	target = startTile + TOP_RIGHT * teamDir;
	if (InMapRange(target) && pieces[target] && pieces[startTile]->GetTeam() != pieces[target]->GetTeam())
		attackingTiles.push_back(target);

	// take on diagonal, local forward left
	// handle destruction of en passant piece when move is confirmed
	target = startTile + TOP_LEFT * teamDir;
	if (InMapRange(target) && pieces[target] && pieces[startTile]->GetTeam() != pieces[target]->GetTeam())
		attackingTiles.push_back(target);

	AddToMap(startTile, attackingTiles);
}

bool Board::BlockedByOwnPiece(int startTile, int target) const
{
	return pieces[target] && pieces[startTile]->GetTeam() == pieces[target]->GetTeam() && pieces[target]->GetType() != EN_PASSANT;
}

bool Board::BlockedByEnemyPiece(int startTile, int target) const
{
	return pieces[target] && pieces[startTile]->GetTeam() != pieces[target]->GetTeam() && pieces[target]->GetType() != EN_PASSANT;
}

bool Board::PieceExists(int index)
{
	if (pieces[index] == nullptr)
	{
		return false;
	}
	return true;
}

void Board::CompleteTurn()
{
	ClearEnPassant();
	CalculateMoves();
	currentTurn = currentTurn == WHITE ? BLACK : WHITE;
}

template <typename T>
bool Board::TileInContainer(int target, T container) const
{
	return std::find(container.begin(), container.end(), target) != container.end();
}

void Board::PrepEdges()
{
	firstRank = { 56, 57, 58, 59, 60, 61, 62, 63 };
	eighthRank = { 0, 1, 2, 3, 4, 5, 6, 7 };
	aFile = { 0, 8, 16, 24, 32, 40, 48, 56 };
	bFile = { 1, 9, 17, 25, 33, 41, 49, 57 };
	gFile = { 6, 14, 22, 30, 38, 46, 54, 62 };
	hFile = { 7, 15, 23, 31, 39, 47, 55, 63 };
	topLeft = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 16, 24, 32, 40, 48, 56 };
	topRight = { 0, 1, 2, 3, 4, 5, 6, 7, 15, 23, 31, 39, 47, 55, 63 };
	bottomLeft = { 0, 8, 16, 24, 32, 40, 48, 56, 57, 58, 59, 60, 61, 62, 63 };
	bottomRight = { 7, 15, 23, 31, 39, 47, 55, 56, 57, 58, 59, 60, 61, 62, 63 };
}

void Board::CalculateEdges()
{
	int temp;

	for (size_t i = 0; i < 64; i++)
	{
		temp = i % 8;

		edgesFromTiles[i].left = i - temp;
		edgesFromTiles[i].right = edgesFromTiles[i].left + 7;

		temp = i / 8;

		edgesFromTiles[i].top = i - (8 * temp);
		edgesFromTiles[i].bottom = 56 + edgesFromTiles[i].top;

		temp = i;
		while (!TileInContainer(temp, topRight))
		{
			temp -= 7;
		}
		edgesFromTiles[i].topRight = temp;

		temp = i;
		while (!TileInContainer(temp, topLeft))
		{
			temp -= 9;
		}
		edgesFromTiles[i].topLeft = temp;

		temp = i;
		while (!TileInContainer(temp, bottomLeft))
		{
			temp += 7;
		}
		edgesFromTiles[i].bottomLeft = temp;

		temp = i;
		while (!TileInContainer(temp, bottomRight))
		{
			temp += 9;
		}
		edgesFromTiles[i].bottomRight = temp;

		// printf("%i\n", i);
		// edgesFromTiles[i].Print();
	}
}

void Board::AddNotBlocked(int startTile, int target, std::vector<int>& validMoves)
{
	if (InMapRange(target) && !BlockedByOwnPiece(startTile, target))
	{
		validMoves.push_back(target);
	}
}

bool Board::CheckCanCastle(int startTile, int target, int rookPos, int dir) const
{
	return InMapRange(target) && !BlockedByOwnPiece(startTile, target) && !BlockedByOwnPiece(startTile, target + -dir) &&
		!BlockedByEnemyPiece(startTile, target) && !BlockedByEnemyPiece(startTile, target + -dir) &&
		InMapRange(rookPos) && pieces[rookPos] && pieces[rookPos]->GetTeam() == pieces[startTile]->GetTeam() &&
		pieces[rookPos]->GetType() == ROOK && !pieces[rookPos]->bMoved;
}

void Board::HandleCastling(int startTile, int endTile)
{
	// long castle
	if (startTile - 2 == endTile)
	{
		pieces[startTile - 1] = pieces[startTile - 4];
		pieces[startTile - 4] = nullptr;
		pieces[startTile - 1]->bMoved = true;
	}
	// short castle
	else if (startTile + 2 == endTile)
	{
		pieces[startTile + 1] = pieces[startTile - 4];
		pieces[startTile + 3] = nullptr;
		pieces[startTile + 1]->bMoved = true;
	}
}

void Board::CreateEnPassant(int startTile, int endTile)
{
	int teamDir = pieces[endTile]->GetTeam() == WHITE ? -1 : 1;
	if (startTile + 16 * teamDir == endTile)
	{
		pieces[startTile + 8 * teamDir] = new Piece(currentTurn, EN_PASSANT);
		lastEnPassantIndex = startTile + 8 * teamDir;
		enPassantOwner = pieces[endTile];
	}
}

void Board::ClearEnPassant()
{
	for (size_t i = 0; i < 64; i++)
	{
		if (pieces[i] && pieces[i]->GetType() == EN_PASSANT && i != lastEnPassantIndex)
		{
			delete pieces[i];
			pieces[i] = nullptr;
		}
	}
	lastEnPassantIndex = -1;
}

void Board::TakeByEnPassant()
{
	for (size_t i = 0; i < 64; i++)
	{
		if (pieces[i] == enPassantOwner)
		{
			delete pieces[i];
			pieces[i] = nullptr;
			enPassantOwner = nullptr;
			return;
		}
	}
}

void Board::CalculateMoves()
{
	for (size_t i = 0; i < 64; i++)
	{
		attackMapWhite[i].clear();
		attackMapBlack[i].clear();
		
		if (pieces[i] == nullptr || pieces[i]->GetType() == EN_PASSANT)
		{
			continue;
		}
		
		switch (pieces[i]->GetType())
		{
		case KING:
			CalcKingMoves(i);
			if (pieces[i]->GetTeam() == WHITE)
			{
				kingPosWhite = i;
			}
			else
			{
				kingPosBlack = i;
			}
			break;
		case QUEEN:
			CalcQueenMoves(i);
			break;
		case BISHOP:
			CalcBishopMoves(i);
			break;
		case KNIGHT:
			CalcKnightMoves(i);
			break;
		case ROOK:
			CalcRookMoves(i);
			break;
		case PAWN:
			CalcPawnMoves(i);
			break;
		}
	}

	CalculateAttacks();
	CalculateCheck();
}

void Board::CalculateAttacks()
{
	attackSetWhite.clear();
	attackSetBlack.clear();

	for (size_t tile = 0; tile < 64; tile++)
	{
		if (attackMapWhite[tile].empty())
			continue;

		// if pawn, only insert attacking, diagonal moves
		if (pieces[tile]->GetType() == PAWN)
		{
			for (int i : attackMapWhite[tile])
			{
				if (i != tile - 8 && i != tile - 16)
				{
					attackSetWhite.insert(i);
				}
			}
			continue;
		}

		for (int i : attackMapWhite[tile])
		{
			attackSetWhite.insert(i);
		}
	}

	for (size_t tile = 0; tile < 64; tile++)
	{
		if (attackMapWhite[tile].empty())
			continue;

		// if pawn, only insert attacking, diagonal moves
		if (pieces[tile]->GetType() == PAWN)
		{
			for (int i : attackMapWhite[tile])
			{
				if (i != tile - 8 && i != tile - 16)
				{
					attackSetWhite.insert(i);
				}
			}
			continue;
		}

		for (int i : attackMapWhite[tile])
		{
			attackSetWhite.insert(i);
		}
	}
}

void Board::AddToMap(int startTile, std::vector<int> validMoves)
{
	if (validMoves.empty())
	{
		return;
	}

	if (pieces[startTile]->GetTeam() == WHITE)
	{
		for (int i : validMoves)
		{
			attackMapWhite[startTile].push_back(i);
		}
	}
	else
	{
		for (int i : validMoves)
		{
			attackMapBlack[startTile].push_back(i);
		}
	}
}

void Board::CalculateCheck()
{
	if (TileInContainer(kingPosBlack, attackSetWhite))
	{
		bInCheckBlack = true;
		CalcCheckVision(WHITE);
		printf("Black in check!\n");
	}
	else
	{
		bInCheckBlack = false;
	}

	if (TileInContainer(kingPosWhite, attackSetBlack))
	{
		bInCheckWhite = true;
		CalcCheckVision(BLACK);
		printf("White in check!\n");

	}
	else
	{
		bInCheckWhite = false;
	}
}

void Board::CalcCheckVision(PieceTeam team)
{
	std::vector<CheckingPiece*> checkingPieces;
	int kingPos = team == WHITE ? kingPosBlack : kingPosWhite;

	for (size_t i = 0; i < 64; i++)
	{
		if (pieces[i] == nullptr)
		{
			continue;
		}

		if (TileInContainer(kingPos, team == WHITE ? attackMapWhite[i] : attackMapBlack[i]))
		{
			CheckingPiece* checkingPiece = new CheckingPiece();
			checkingPiece->tile = i;
			checkingPiece->pieceType = pieces[i]->GetType();
			checkingPiece->CalculateDir();
			checkingPieces.push_back(checkingPiece);
		}
	}

	if (team == WHITE)
	{
		checkingPiecesWhite.clear();
		checkingPiecesWhite = checkingPieces;
	}
	else
	{
		checkingPiecesBlack.clear();
		checkingPiecesBlack = checkingPieces;
	}
}

void Board::CheckingPiece::CalculateDir()
{
	printf("Remember to calculate dir\n");
}

void Board::GameOver(int winningTeam)
{
	winner = winningTeam;
	bGameOver = true;
}

void Board::SetupBoardFromFEN(std::string fen)
{
	int index = 0;
	PieceTeam team;

	for (char c : fen)
	{
		// if letter, place piece
		if (isalpha(c))
		{
			if (islower(c))
			{
				team = BLACK;
			}
			else
			{
				team = WHITE;
			}

			switch (tolower(c))
			{
				case 'r':
					pieces[index] = new Piece(team, ROOK);
					index++;
					continue;
				case 'n':
					pieces[index] = new Piece(team, KNIGHT);
					index++;
					continue;
				case 'b':
					pieces[index] = new Piece(team, BISHOP);
					index++;
					continue;
				case 'q':
					pieces[index] = new Piece(team, QUEEN);
					index++;
					continue;
				case 'k':
					pieces[index] = new Piece(team, KING);
					index++;
					continue;
				case 'p':
					pieces[index] = new Piece(team, PAWN);
					index++;
					continue;
			}
		}

		// if number, traverse board
		if (isdigit(c))
		{
			int moveDistance = c - '0';
			index += moveDistance;
		}
	}
}
