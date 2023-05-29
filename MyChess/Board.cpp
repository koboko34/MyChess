#include "Board.h"

Board::Board()
{
	VAO, EBO, VBO = 0;
	currentTurn = WHITE;
	lastEnPassantIndex = -1;
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
}

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

bool Board::MovePiece(int startTile, int endTile)
{
	printf("Attempting to play a move...\n");
	
	if (pieces[startTile] == nullptr)
		return false;
	
	if (!IsCurrentTurn(startTile))
	{
		printf("It is %s's move!\n", currentTurn ? "black" : "white");
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


	// checks complete
	if (pieces[endTile])
	{
		delete pieces[endTile];
		// pieces[endTile] = nullptr;
	}
	pieces[endTile] = pieces[startTile];
	pieces[startTile] = nullptr;

	if (pieces[endTile]->GetType() == PAWN)
	{
		pieces[endTile]->bPawnMoved = true;
	}

	HandleEnPassant();
	CompleteTurn();
	return true;
}

bool Board::PieceExists(int index)
{
	if (pieces[index] == nullptr)
	{
		return false;
	}
	return true;
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
			if (selectedObjectId == objectId && pieces[objectId] != nullptr)
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

bool Board::TileInArray(int target, std::vector<int> arr) const
{
	return std::find(arr.begin(), arr.end(), target) != arr.end();
}

void Board::PrepEdges()
{
	firstRank = { 56, 57, 58, 59, 60, 61, 62, 63 };
	eighthRank = { 0, 1, 2, 3, 4, 5, 6, 7 };
	aFile = { 0, 8, 16, 24, 32, 40, 48, 56 };
	bFile = { 1, 9, 17, 25, 33, 41, 49, 57 };
	gFile = { 6, 14, 22, 30, 38, 46, 54, 62 };
	hFile ={ 7, 15, 23, 31, 39, 47, 55, 63 };
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
		while (!TileInArray(temp, topRight))
		{
			temp -= 7;
		}
		edgesFromTiles[i].topRight = temp;

		temp = i;
		while (!TileInArray(temp, topLeft))
		{
			temp -= 9;
		}
		edgesFromTiles[i].topLeft = temp;

		temp = i;
		while (!TileInArray(temp, bottomLeft))
		{
			temp += 7;
		}
		edgesFromTiles[i].bottomLeft = temp;

		temp = i;
		while (!TileInArray(temp, bottomRight))
		{
			temp += 9;
		}
		edgesFromTiles[i].bottomRight = temp;

		printf("%i\n", i);
		edgesFromTiles[i].Print();
	}
}

bool Board::CheckLegalMove(int startTile, int endTile)
{
	switch (pieces[startTile]->GetType())
	{
		case KING:
			return CheckKingMove(startTile, endTile);
		case QUEEN:
			return CheckQueenMove(startTile, endTile);
		case BISHOP:
			return CheckBishopMove(startTile, endTile);
		case KNIGHT:
			return CheckKnightMove(startTile, endTile);
		case ROOK:
			return CheckRookMove(startTile, endTile);
		case PAWN:
			return CheckPawnMove(startTile, endTile);
	}
	return false;
}

bool Board::CheckKingMove(int startTile, int endTile) const
{
	// if forward or backward
	if (endTile == startTile + 8|| endTile == startTile - 8)
	{
		return true;
	}

	// if on A file
	if (TileInArray(startTile, aFile))
	{
		// only look for +1 to prevent jump across board
		if (endTile == startTile + 1)
		{
			return true;
		}

		// only look for +9 and -7 to prevent jump across board
		if (endTile == startTile + 9 || endTile == startTile - 7)
		{
			return true;
		}
		
		return false;
	}

	// if on H file
	if (TileInArray(startTile, hFile))
	{
		// only look for -1 to prevent jumps across board
		if (endTile == startTile - 1)
		{
			return true;
		}

		// only look for +7 and -9 to prevent jumps across board
		if (endTile == startTile + 7 || endTile == startTile - 9)
		{
			return true;
		}

		return false;
	}
	
	// if away from side walls, lateral
	if (endTile == startTile + 1 || endTile == startTile - 1)
	{
		return true;
	}

	// if away from side walls, diagonal
	if (endTile == startTile + 9 || endTile == startTile - 9 ||
		endTile == startTile + 7 || endTile == startTile - 7)
	{
		return true;
	}

	return false;
}

bool Board::CheckQueenMove(int startTile, int endTile) const
{
	return CheckRookMove(startTile, endTile) || CheckBishopMove(startTile, endTile);
}

bool Board::CheckBishopMove(int startTile, int endTile) const
{
	// check using pre-calculated edges
	int target;
	if (startTile < endTile)
	{
		target = startTile;
		while (!TileInArray(target, bottomLeft))
		{
			target += 7;
			if (target == endTile)
			{
				return true;
			}
		}

		target = startTile;
		while (!TileInArray(target, bottomRight))
		{
			target += 9;
			if (target == endTile)
			{
				return true;
			}
		}
	}
	else
	{
		target = startTile; 
		while (!TileInArray(target, topLeft))
		{
			target -= 9;
			if (target == endTile)
			{
				return true;
			}
		}

		target = startTile; 
		while (!TileInArray(target, topRight))
		{
			target -= 7;
			if (target == endTile)
			{
				return true;
			}
		}
	}
	
	return false;
}

bool Board::CheckKnightMove(int startTile, int endTile) const
{
	int target;

	if (TileInArray(startTile, aFile))
	{
		// up up right
		target = startTile - 2 * 8 + 1;
		if (endTile == target)
		{
			return true;
		}

		// right right up
		target = startTile - 8 + 2;
		if (endTile == target)
		{
			return true;
		}

		// right right down
		target = startTile + 8 + 2;
		if (endTile == target)
		{
			return true;
		}

		// down down right
		target = startTile + 2 * 8 + 1;
		if (endTile == target)
		{
			return true;
		}

		return false;
	}

	if (TileInArray(startTile, bFile))
	{
		// up up left
		target = startTile - 2 * 8 - 1;
		if (endTile == target)
		{
			return true;
		}
		
		// up up right
		target = startTile - 2 * 8 + 1;
		if (endTile == target)
		{
			return true;
		}

		// right right up
		target = startTile - 8 + 2;
		if (endTile == target)
		{
			return true;
		}

		// right right down
		target = startTile + 8 + 2;
		if (endTile == target)
		{
			return true;
		}

		// down down right
		target = startTile + 2 * 8 + 1;
		if (endTile == target)
		{
			return true;
		}

		// down down left
		target = startTile + 2 * 8 - 1;
		if (endTile == target)
		{
			return true;
		}

		return false;
	}

	if (TileInArray(startTile, hFile))
	{
		// up up left
		target = startTile - 2 * 8 - 1;
		if (endTile == target)
		{
			return true;
		}

		// left left up
		target = startTile - 8 - 2;
		if (endTile == target)
		{
			return true;
		}

		// left left down
		target = startTile + 8 - 2;
		if (endTile == target)
		{
			return true;
		}

		// down down left
		target = startTile + 2 * 8 - 1;
		if (endTile == target)
		{
			return true;
		}

		return false;
	}

	if (TileInArray(startTile, gFile))
	{
		// up up right
		target = startTile - 2 * 8 + 1;
		if (endTile == target)
		{
			return true;
		}

		// up up left
		target = startTile - 2 * 8 - 1;
		if (endTile == target)
		{
			return true;
		}

		// left left up
		target = startTile - 8 - 2;
		if (endTile == target)
		{
			return true;
		}

		// left left down
		target = startTile + 8 - 2;
		if (endTile == target)
		{
			return true;
		}

		// down down left
		target = startTile + 2 * 8 - 1;
		if (endTile == target)
		{
			return true;
		}

		// down down right
		target = startTile + 2 * 8 + 1;
		if (endTile == target)
		{
			return true;
		}

		return false;
	}

	// up up right
	target = startTile - 2 * 8 + 1;
	if (endTile == target)
	{
		return true;
	}

	// right right up
	target = startTile - 8 + 2;
	if (endTile == target)
	{
		return true;
	}

	// right right down
	target = startTile + 8 + 2;
	if (endTile == target)
	{
		return true;
	}

	// down down right
	target = startTile + 2 * 8 + 1;
	if (endTile == target)
	{
		return true;
	}

	// up up left
	target = startTile - 2 * 8 - 1;
	if (endTile == target)
	{
		return true;
	}

	// left left up
	target = startTile - 8 - 2;
	if (endTile == target)
	{
		return true;
	}

	// left left down
	target = startTile + 8 - 2;
	if (endTile == target)
	{
		return true;
	}

	// down down left
	target = startTile + 2 * 8 - 1;
	if (endTile == target)
	{
		return true;
	}
	
	return false;
}

bool Board::CheckRookMove(int startTile, int endTile) const
{
	int dir = startTile < endTile ? 1 : -1;
	int target = startTile + 8 * dir;

	int top = edgesFromTiles[startTile].top;
	int bottom = edgesFromTiles[startTile].bottom;

	// forward and backwards
	while (top <= target && target <= bottom)
	{
		// if blocked by own team's piece
		if (pieces[target] && IsCurrentTurn(target))
		{
			break;
		}

		// if blocked by enemy before reaching endTile
		if (pieces[target] && !IsCurrentTurn(target) && endTile != target)
		{
			break;
		}
		
		if (endTile == target)
		{
			return true;
		}

		target += 8 * dir;
	}

	target = startTile + 1 * dir;
	int left = edgesFromTiles[startTile].left;
	int right = edgesFromTiles[startTile].right;

	// lateral movement
	while (left <= target && target <= right)
	{
		// if blocked by own team's piece
		if (pieces[target] && IsCurrentTurn(target))
		{
			break;
		}

		// if blocked by enemy before reaching endTile
		if (pieces[target] && !IsCurrentTurn(target) && endTile != target)
		{
			break;
		}

		if (endTile == target)
		{
			return true;
		}

		target += 1 * dir;
	}
	
	return false;
}

bool Board::CheckPawnMove(int startTile, int endTile)
{
	// 4 cases, move forward by 1, move forward by 2 (only first move), take diagonal, take by en passant
	
	int teamDir = pieces[startTile]->GetTeam() == WHITE ? -1 : 1;

	// forward by 1
	if (endTile == startTile + 8 * teamDir && pieces[endTile] == nullptr)
	{
		return true;
	}

	// forward by 2 if first move of this pawn
	if (endTile == startTile + 16 * teamDir && pieces[endTile] == nullptr && pieces[endTile - 8 * teamDir] == nullptr && pieces[startTile]->bPawnMoved == false)
	{
		pieces[startTile + 8 * teamDir] = new Piece(pieces[startTile]->GetTeam(), EN_PASSANT);
		lastEnPassantIndex = startTile + 8 * teamDir;
		return true;
	}

	// take on diagonal, en passant
	if ((pieces[endTile]) && (endTile == startTile + 7 * teamDir || endTile == startTile + 9 * teamDir))
	{
		// handle en passant
		if (pieces[endTile]->GetType() == EN_PASSANT)
		{
			delete pieces[endTile + 8 * -teamDir];
			pieces[endTile + 8 * -teamDir] = nullptr;
		}
		return true;
	}

	return false;
}

void Board::HandleEnPassant()
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
