#include "Board.h"

#include <charconv>
#include <chrono>

#include "Button.h"

#define TESTING 0

Board::Board()
{
	VAO, EBO, VBO = 0;
	bInMainMenu = true;
	bInGame = false;
	bTesting = false;
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

	for (PieceType type : promotionTypes)
	{
		delete promotionPieces[type];
	}
	promotionPieces.clear();
}

void Board::Init(unsigned int windowWidth, unsigned int windowHeight, GLFWwindow* window, irrklang::ISoundEngine* sEngine)
{
	soundEngine = sEngine;
	if (soundEngine == nullptr)
	{
		printf("Sound engine is null!\n");
	}
	
	this->window = window;

	glm::mat4 view = glm::mat4(1.f);
	view = glm::translate(view, glm::vec3(0.f, 0.f, -1.f));

	width = windowWidth;
	height = windowHeight;
	aspect = (float)width / (float)height;
	glm::mat4 projection = glm::ortho(0.f, aspect, 0.f, 1.f, 0.f, 100.f);

	SetupBoard(view, projection);
	SetupPieces(view, projection);
	SetupPickingShader(view, projection);
	SetupPromotionPieces();

	PrepEdges();
	CalculateEdges();
	SetupGame(false);

	ShowMenuButtons();
}

// ========================================== RENDERING ==========================================

void Board::RenderScene(int selectedObjectId)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.5f, 0.3f, 0.2f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (bInGame)
	{
		DrawBoard(selectedObjectId);
	}

	RenderButtons();

	if (bChoosingPromotion)
	{
		boardShader.UseShader();
		RenderPromotionTiles();

		pieceShader.UseShader();
		RenderPromotionPieces();
		return;
	}
}

void Board::DrawBoard(int selectedObjectId)
{
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

void Board::SetupPromotionPieces()
{
	for (PieceType type : promotionTypes)
	{
		Piece* piece = new Piece(WHITE, type);
		std::pair<int, Piece*> keyValPair(type, piece);
		promotionPieces.insert(keyValPair);
	}
}

void Board::SetupFont()
{
	// to do
}

void Board::PickingPass()
{
	pickingShader.UseShader();
	glBindVertexArray(VAO);

	if (bChoosingPromotion)
	{
		int file = -1;
		int rank = 8;
		for (PieceType type : promotionTypes)
		{
			unsigned int objectId = type;
			GLuint adjustedObjectId = objectId + 1;
			glUniform1ui(objectIdLocation, adjustedObjectId);

			glm::mat4 tileModel = glm::mat4(1.f);
			tileModel = glm::translate(tileModel, glm::vec3((aspect / 13.7f) * file + 0.311f, (rank * 2 - 1 - 1.f) / 16.f, -2.f));
			tileModel = glm::scale(tileModel, glm::vec3(tileSize + 0.05, tileSize + 0.05, 1.f));
			glUniformMatrix4fv(pickingModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			rank -= 2;
		}
		return;
	}

	if (!buttons.empty())
	{
		int i = 0;
		for (Button* button : buttons)
		{
			unsigned int objectId = 100 + i;
			button->id = objectId;
			glUniform1ui(objectIdLocation, objectId);

			glm::mat4 tileModel = glm::mat4(1.f);
			tileModel = glm::translate(tileModel, glm::vec3((aspect / width) * button->xPos + button->xPosOffset, button->yPos, -2.f));
			tileModel = glm::scale(tileModel, glm::vec3((aspect / width) * button->xSize + button->xSizeOffset, button->ySize, 1.f));
			glUniformMatrix4fv(pickingModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			i++;
		}
		return;
	}

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
			unsigned int objectId = (8 - rank) * 8 + file - 1;

			if (ShouldHighlightSelectedObject(selectedObjectId, objectId))
			{
				glUniform3f(tileColorLocation, selectColor.x, selectColor.y, selectColor.z);
			}
			else if (ShouldHighlightLastMove(objectId))
			{
				glUniform3f(tileColorLocation, lastMoveColor.x, lastMoveColor.y, lastMoveColor.z);

			}
			else if ((rank % 2 == 0) != (file % 2 == 0))
			{
				glUniform3f(tileColorLocation, whiteTileColor.x, whiteTileColor.y, whiteTileColor.z);
			}
			else
			{
				glUniform3f(tileColorLocation, blackTileColor.x, blackTileColor.y, blackTileColor.z);
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

// ========================================== BUTTONS ==========================================

void Board::RenderPromotionTiles()
{
	glBindVertexArray(VAO);
	int file = -1;

	for (size_t rank = 8; rank >= 1; rank--)
	{
		if (rank % 4 == 0)
		{
			glUniform3f(tileColorLocation, whiteTileColor.x, whiteTileColor.y, whiteTileColor.z);
		}
		else if (rank == 6 || rank == 2)
		{
			glUniform3f(tileColorLocation, blackTileColor.x, blackTileColor.y, blackTileColor.z);

		}
		else
		{
			continue;
		}

		glm::mat4 tileModel = glm::mat4(1.f);
		tileModel = glm::translate(tileModel, glm::vec3((aspect / 13.7f) * file + 0.311f, (rank * 2 - 1 - 1.f) / 16.f, -2.f));
		tileModel = glm::scale(tileModel, glm::vec3(tileSize + 0.05, tileSize + 0.05, 1.f));
		glUniformMatrix4fv(tileModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);
}

void Board::RenderPromotionPieces()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, piecesTextureId);

	int file = -1;
	int pieceType = 0;

	for (size_t rank = 8; rank >= 1; rank -= 2)
	{
		glm::mat4 pieceModel = glm::mat4(1.f);
		pieceModel = glm::translate(pieceModel, glm::vec3((aspect / 13.7f) * file + 0.311f, (rank * 2 - 1 - 1.f) / 16.f, 0.f));
		pieceModel = glm::scale(pieceModel, glm::vec3(tileSize + 0.05, tileSize + 0.05, 1.f));
		glUniformMatrix4fv(pieceModelLocation, 1, GL_FALSE, glm::value_ptr(pieceModel));

		promotionPieces[promotionTypes[pieceType]]->DrawPiece();
		pieceType++;
	}

	glBindVertexArray(0);
}

void Board::ClearButtons()
{
	while (buttons.size() > 0)
	{
		delete buttons[0];
	}
}

void Board::RenderButton(Button* button)
{
	if (button->bFillButton)
	{
		boardShader.UseShader();

		glBindVertexArray(VAO);

		glUniform3f(tileColorLocation, button->color.x / 2, button->color.y / 2, button->color.z / 2);

		glm::mat4 tileModel = glm::mat4(1.f);
		tileModel = glm::translate(tileModel, glm::vec3((aspect / width) * button->xPos + button->xPosOffset, button->yPos, -3.f));
		tileModel = glm::scale(tileModel, glm::vec3((aspect / width) * button->xSize + button->xSizeOffset, button->ySize, 1.f));
		glUniformMatrix4fv(tileModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glUniform3f(tileColorLocation, button->color.x, button->color.y, button->color.z);

		tileModel = glm::mat4(1.f);
		tileModel = glm::translate(tileModel, glm::vec3((aspect / width) * button->xPos + button->xPosOffset, button->yPos, -2.f));
		tileModel = glm::scale(tileModel, glm::vec3((aspect / width) * button->xSize - 0.02f + button->xSizeOffset, button->ySize - 0.02f, 1.f));
		glUniformMatrix4fv(tileModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
	}

	if (!button->bUseTexture)
	{
		return;
	}

	pieceShader.UseShader();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, button->textureId);

	glm::mat4 tileModel = glm::mat4(1.f);
	tileModel = glm::translate(tileModel, glm::vec3((aspect / width) * button->xPos + button->xPosOffset, button->yPos, -1.f));
	tileModel = glm::scale(tileModel, glm::vec3((aspect / width) * button->xSize + button->xSizeOffset, button->ySize, 1.f));
	glUniformMatrix4fv(pieceModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

	glBindVertexArray(button->VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Board::RenderButtons()
{
	if (!buttons.empty())
	{
		for (Button* button : buttons)
		{
			RenderButton(button);
		}
	}
}

void Board::RenderContinueButton()
{
	glBindVertexArray(VAO);

	glUniform3f(tileColorLocation, 0.1f, 0.1f, 0.1f);

	glm::mat4 tileModel = glm::mat4(1.f);
	tileModel = glm::translate(tileModel, glm::vec3((aspect / 13.7f) * 10 + 0.296f, (2 * 2 - 2) / 16.f, -2.f));
	tileModel = glm::scale(tileModel, glm::vec3(buttonWidth, buttonHeight, 1.f));
	glUniformMatrix4fv(tileModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}

void Board::ShowMenuButtons()
{
	ClearButtons();

	Button* title = new Button(this, (float)width / 4 * 2, 0.f, 0.8f, (float)width / 2, 0.f, 0.4f);
	title->SetCallback(std::bind(&Board::EmptyFunction, this));
	title->bFillButton = false;
	title->bUseTexture = true;
	title->SetTexture("textures/buttons/title.png");
	buttons.push_back(title);
	
	Button* singleplayerButton = new Button(this, (float)width / 4, 0.1f, 0.5f, (float)width / 4, 0.f, 0.2f);
	singleplayerButton->SetCallback(std::bind(&Board::PlaySingleplayerCallback, this));
	singleplayerButton->bUseTexture = true;
	singleplayerButton->SetTexture("textures/buttons/1p.png");
	buttons.push_back(singleplayerButton);

	Button* multiplayerButton = new Button(this, (float)width / 4 * 3, -0.1f, 0.5f, (float)width / 4, 0.f, 0.2f);
	multiplayerButton->SetCallback(std::bind(&Board::PlayMultiplayerCallback, this));
	multiplayerButton->bUseTexture = true;
	multiplayerButton->SetTexture("textures/buttons/2p.png");
	buttons.push_back(multiplayerButton);

	Button* quitButton = new Button(this, (float)width / 4 * 2, 0.f, 0.2f, (float)width / 4, 0.f, 0.2f);
	quitButton->SetCallback(std::bind(&Board::QuitGameCallback, this));
	quitButton->bUseTexture = true;
	quitButton->SetTexture("textures/buttons/quit.png");
	buttons.push_back(quitButton);

#if TESTING == 1
	Button* shannonTestButton = new Button(this, (float)width / 8 * 7, 0.f, 0.8f, (float)width / 8, 0.f, 0.15f);
	shannonTestButton->SetCallback(std::bind(&Board::ShannonTestCallback, this));
	buttons.push_back(shannonTestButton);
#endif

}

void Board::PlaySingleplayerCallback()
{
	ClearButtons();
	
	Button* playWhiteButton = new Button(this, (float)width / 4, 0.1f, 0.6f, (float)width / 4, 0.f, 0.2f);
	playWhiteButton->SetColor(1.f, 1.f, 1.f);
	playWhiteButton->SetCallback(std::bind(&Board::PlayWhiteCallback, this));
	playWhiteButton->bUseTexture = true;
	playWhiteButton->SetTexture("textures/buttons/white.png");
	buttons.push_back(playWhiteButton);

	Button* playBlackButton = new Button(this, (float)width / 4 * 3, -0.1f, 0.6f, (float)width / 4, 0.f, 0.2f);
	playBlackButton->SetColor(0.f, 0.f, 0.f);
	playBlackButton->SetCallback(std::bind(&Board::PlayBlackCallback, this));
	playBlackButton->bUseTexture = true;
	playBlackButton->SetTexture("textures/buttons/black.png");
	buttons.push_back(playBlackButton);

	Button* backButton = new Button(this, (float)width / 4 * 2, 0.f, 0.3f, (float)width / 4, 0.f, 0.2f);
	backButton->SetCallback(std::bind(&Board::ShowMenuButtons, this));
	backButton->bUseTexture = true;
	backButton->SetTexture("textures/buttons/back.png");
	buttons.push_back(backButton);
}

void Board::PlayWhiteCallback()
{
	compTeam = BLACK;
	bVsComputer = true;
	bInMainMenu = false;
	bInGame = true;
	SetupGame(false);
}

void Board::PlayBlackCallback()
{
	compTeam = WHITE;
	bVsComputer = true;
	bInMainMenu = false;
	bInGame = true;
	SetupGame(false);
	PlayCompMove();
}

void Board::PlayMultiplayerCallback()
{
	bVsComputer = false;
	bInMainMenu = false;
	bInGame = true;
	SetupGame(false);
}

void Board::QuitGameCallback()
{
	glfwSetWindowShouldClose(window, true);
}

void Board::ContinueCallback()
{
	bInMainMenu = true;
	bInGame = false;
	ShowMenuButtons();
}

void Board::EmptyFunction()
{
	soundEngine->stopAllSounds();
	return;
}

void Board::ButtonCallback(int id)
{
	for (Button* button : buttons)
	{
		if (button->id == id)
		{
			soundEngine->play2D("sounds/notify.mp3");
			button->callback();
			return;
		}
	}
}

void Board::ShannonTestCallback()
{
	const int depth = 4;
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

// ========================================== TESTING ==========================================

int Board::ShannonTest(int ply, const int depth)
{
	int moveCount = 0;

	if (ply > depth || bGameOver)
	{
		return 1;
	}

	// save current board state (locations and whether piece moved for castling etc)
	std::string fen = BoardToFEN();
	std::vector<BoardState> boardState;
	CaptureBoardState(boardState);
	// capture pinned pieces
	int lastEnPassant = lastEnPassantIndex;
	bool bLocalCheckWhite = bInCheckWhite;
	bool bLocalCheckBlack = bInCheckBlack;
	PieceTeam turn = currentTurn;

	std::vector<int> attackMap[64];
	if (turn == WHITE)
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
		
		if (pieces[startTile] == nullptr || pieces[startTile]->GetTeam() != turn || attackMap[startTile].empty())
			continue;

		for (int move : attackMap[startTile])
		{
			if (TileInContainer(move, movesFound))
			{
				printf("MOVE ALREADY IN CONTAINER! FOUND MORE THAN ONCE!\n");
			}
			movesFound.push_back(move);
			
			// since CompleteTurn() wipes attack maps, copy the relevant attack map entry so that checks can be carried out
			turn == WHITE ? attackMapWhite[startTile].clear() : attackMapBlack[startTile].clear();
			for (int tileMove : attackMap[startTile])
			{
				turn == WHITE ? attackMapWhite[startTile].push_back(tileMove) : attackMapBlack[startTile].push_back(tileMove);
			}

			// play move
			MovePiece(startTile, move);

			// calc deeper moves with recursion and add
			moveCount += ShannonTest(ply + 1, depth);

			// undo move by restoring board state
			lastEnPassantIndex = lastEnPassant;
			SetupBoardFromFEN(fen);
			RecoverBoardState(boardState);
			currentTurn = turn;
			bGameOver = false;
			bInCheckWhite = bLocalCheckWhite;
			bInCheckBlack = bLocalCheckWhite;
		}
	}

	return moveCount;
}

std::string Board::BoardToFEN()
{
	std::string fen;
	std::string fenChar = "";
	int length = 0;
	int spaces = 0;

	for (Piece* piece : pieces)
	{
		// printf("%s\n", fen.c_str());
		
		if ((length + spaces) % 8 == 0 && (length + spaces) != 0)
		{
			fen.append("/");
			spaces = 0;
		}

		if (piece == nullptr)
		{
			spaces++;
			if ((length + spaces) % 8 == 0 && (length + spaces) != 0)
			{
				fenChar = std::to_string(spaces);
				fen.append(fenChar);
				length += spaces;
				spaces = 0;
			}
			continue;
		}

		if (spaces != 0)
		{
			fenChar = std::to_string(spaces);
			fen.append(fenChar);
			length += spaces;
			spaces = 0;
		}

		if ((length + spaces) % 8 == 0 && (length + spaces) != 0 && fen.back() != '/')
		{
			fen.append("/");
			spaces = 0;
		}

		switch (piece->GetType())
		{
		case KING:
			fenChar = "k";
			break;
		case QUEEN:
			fenChar = "q";
			break;
		case BISHOP:
			fenChar = "b";
			break;
		case KNIGHT:
			fenChar = "n";
			break;
		case ROOK:
			fenChar = "r";
			break;
		case PAWN:
			fenChar = "p";
			break;
		case EN_PASSANT:
			fenChar = "e";
			break;
		}

		if (piece->GetTeam() == WHITE)
		{
			for (char c : fenChar)
			{
				fenChar = std::string(1, (char)toupper(c));
				break;
			}
		}

		fen.append(fenChar);
		length++;
	}

	// printf("Done!\n");
	// printf("%s\n", fen.c_str());

	return fen;
}

void Board::RecoverBoardState(std::vector<BoardState> boardStates)
{
	for (BoardState boardState : boardStates)
	{
		pieces[boardState.tile]->bMoved = boardState.bMoved;
	}
}

void Board::CaptureBoardState(std::vector<BoardState>& boardState)
{
	for (size_t i = 0; i < 64; i++)
	{
		if (pieces[i] == nullptr)
			continue;

		if (pieces[i]->GetType() == PAWN || pieces[i]->GetType() == ROOK || pieces[i]->GetType() == KING)
		{
			BoardState state = BoardState(i, pieces[i]->bMoved);
			boardState.push_back(state);
		}
	}
}

// ========================================== MOVE LOGIC ==========================================

bool Board::MovePiece(int startTile, int endTile)
{
	if (pieces[startTile] == nullptr)
		return false;

	if (!InMapRange(startTile) || !InMapRange(endTile))
		return false;

	if (!IsCurrentTurn(startTile))
	{
#if TESTING != 1
		printf("It is %s's move!\n", currentTurn == WHITE ? "white" : "black");
#endif
		return false;
	}

	if (pieces[endTile] && pieces[startTile]->GetTeam() == pieces[endTile]->GetTeam())
	{
#if TESTING != 1
		printf("Cannot take your own team's piece!\n");
#endif
		return false;
	}

	// check piece specific move
	if (!CheckLegalMove(startTile, endTile))
	{
#if TESTING != 1
		printf("Not a legal move for this piece!\n");
#endif
		return false;
	}

	// check if move puts self in check by king moving into attacked square
	if (pieces[startTile]->GetType() == KING && TileInContainer(endTile, pieces[startTile]->GetTeam() == WHITE ? attackSetBlack : attackSetWhite))
	{
#if TESTING != 1
		printf("Cannot put yourself in check!\n");
#endif
		return false;
	}

	// if in check
	if (currentTurn == WHITE ? bInCheckWhite : bInCheckBlack == true)
	{
		if (!MoveBlocksCheck(startTile, endTile) && !(pieces[startTile]->GetType() == KING && KingEscapesCheck(endTile)) && !MoveTakesCheckingPiece(endTile))
		{
#if TESTING != 1
			printf("Move does not escape check!\n");
#endif
			return false;
		}
	}


	// checks complete
	bool tookPiece = pieces[endTile] && pieces[endTile]->GetTeam() != currentTurn;

	lastMoveStart = startTile;
	lastMoveEnd = endTile;

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
		HandlePromotion(endTile);
		break;
	case KING:
		HandleCastling(startTile, endTile);
		break;
	default:
		break;
	}

	// set relevant sound, played after checking if check or checkmate in CalculateCheck()
	if (tookPiece)
		lastMoveSound = CAPTURE;
	else if (bSetPromoSound)
		lastMoveSound = PROMOTE;
	else if (currentTurn == BLACK)
		lastMoveSound = MOVE_OPP;
	else
		lastMoveSound = MOVE_SELF;

	if (bChoosingPromotion && ((bVsComputer && currentTurn == compTeam) || bTesting))
	{
		Promote(QUEEN);
	}
	else if (!bChoosingPromotion)
	{
		CompleteTurn();
	}

	return true;
}

bool Board::CheckLegalMove(int startTile, int endTile)
{	
	return TileInContainer(endTile, currentTurn == WHITE ? attackMapWhite[startTile] : attackMapBlack[startTile]);
}

void Board::CalcSlidingMovesOneDir(int startTile, int dir, int min, int max, int kingPos, bool& foundKing, std::vector<int>& checkLOS, std::vector<int>& attackingTiles)
{
	int firstFriendlyPiece = -1;
	int target = startTile + dir;
	int pinnedPieceTile;
	bool blockedByNonKing = false;
	std::vector<int> pinLOS;

	while (min <= target && target <= max)
	{
		if (blockedByNonKing)
		{
			if (BlockedByEnemyPiece(startTile, target) && pieces[target]->GetType() == KING && pieces[target]->GetTeam() != pieces[startTile]->GetTeam())
			{
				pinLOS.push_back(startTile);
				AddPinnedPiece(pinnedPieceTile, pinLOS);
				return;
			}

			if (BlockedByOwnPiece(startTile, target) || BlockedByEnemyPiece(startTile, target))
			{
				return;
			}

			pinLOS.push_back(target);
			target += dir;
			continue;
		}
		
		if (BlockedByEnemyPiece(startTile, target))
		{
			if (foundKing)
			{
				return;
			}

			checkLOS.push_back(target);
			if (target == kingPos)
			{
				foundKing = true;
				attackingTiles.insert(attackingTiles.end(), checkLOS.begin(), checkLOS.end());
				target += dir;
				continue;
			}

			blockedByNonKing = true;
			pinnedPieceTile = target;
			pinLOS = checkLOS;
			target += dir;
			continue;
		}

		if (BlockedByOwnPiece(startTile, target) && firstFriendlyPiece == -1 && !blockedByNonKing)
		{
			firstFriendlyPiece = target;
			AddProtectedPieceToSet(firstFriendlyPiece);
			return;
		}

		if (foundKing)
		{
			kingXRay.insert(target);
		}
		else
		{
			checkLOS.push_back(target);
		}

		target += dir;
	}
}

void Board::CalcKnightMovesOneDir(int startTile, int dir, int kingPos, std::vector<int>& checkLOS, std::vector<int>& attackingTiles)
{
	int target = startTile + dir;
	if (BlockedByOwnPiece(startTile, target))
	{
			AddProtectedPieceToSet(target);
			return;
	}

	AddNotBlocked(startTile, target, attackingTiles);
	if (target == kingPos)
	{
		checkLOS.push_back(target);
		AddCheckingPiece(startTile, checkLOS);
		checkLOS.clear();
	}
}

void Board::HandleFoundMoves(int startTile, bool& foundKing, std::vector<int>& checkLOS, std::vector<int>& attackingTiles)
{
	if (!foundKing)
	{
		attackingTiles.insert(attackingTiles.end(), checkLOS.begin(), checkLOS.end());
	}
	else
	{
		AddCheckingPiece(startTile, checkLOS);
		foundKing = false;
	}
	checkLOS.clear();
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
	if (top <= target && target <= bottom)
	{		
		if (BlockedByOwnPiece(startTile, target))
		{
			AddProtectedPieceToSet(target);
		}
		else
		{
			attackingTiles.push_back(target);
		}
	}
	
	// up
	target = startTile + UP;
	if (top <= target && target <= bottom)
	{
		if (BlockedByOwnPiece(startTile, target))
		{
			AddProtectedPieceToSet(target);
		}
		else
		{
			attackingTiles.push_back(target);
		}
	}

	// left
	target = startTile + LEFT;
	if (left <= target && target <= right && startTile != left)
	{
		if (BlockedByOwnPiece(startTile, target))
		{
			AddProtectedPieceToSet(target);
		}
		else
		{
			attackingTiles.push_back(target);
		}
	}

	// right
	target = startTile + RIGHT;
	if (left <= target && target <= right && startTile != right)
	{
		if (BlockedByOwnPiece(startTile, target))
		{
			AddProtectedPieceToSet(target);
		}
		else
		{
			attackingTiles.push_back(target);
		}
	}

	// top left
	target = startTile + TOP_LEFT;
	if (topLeft <= target && target <= bottomRight && !TileInContainer(startTile, aFile) && !TileInContainer(startTile, eighthRank))
	{
		if (BlockedByOwnPiece(startTile, target))
		{
			AddProtectedPieceToSet(target);
		}
		else
		{
			attackingTiles.push_back(target);
		}
	}

	// top right
	target = startTile + TOP_RIGHT;
	if (topRight <= target && target <= bottomLeft && !TileInContainer(startTile, hFile) && !TileInContainer(startTile, eighthRank))
	{
		if (BlockedByOwnPiece(startTile, target))
		{
			AddProtectedPieceToSet(target);
		}
		else
		{
			attackingTiles.push_back(target);
		}
	}

	// bottom left
	target = startTile + BOT_LEFT;
	if (topRight <= target && target <= bottomLeft && !TileInContainer(startTile, aFile) && !TileInContainer(startTile, firstRank))
	{
		if (BlockedByOwnPiece(startTile, target))
		{
			AddProtectedPieceToSet(target);
		}
		else
		{
			attackingTiles.push_back(target);
		}
	}

	// bottom right
	target = startTile + BOT_RIGHT;
	if (topLeft <= target && target <= bottomRight && !TileInContainer(startTile, hFile) && !TileInContainer(startTile, firstRank))
	{
		if (BlockedByOwnPiece(startTile, target))
		{
			AddProtectedPieceToSet(target);
		}
		else
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
	std::vector<int> checkLOS;

	PieceTeam team = pieces[startTile]->GetTeam();

	int kingPos = team == WHITE ? kingPosBlack : kingPosWhite;
	bool foundKing = false;

	int topLeft = edgesFromTiles[startTile].topLeft;
	int topRight = edgesFromTiles[startTile].topRight;
	int bottomLeft = edgesFromTiles[startTile].bottomLeft;
	int bottomRight = edgesFromTiles[startTile].bottomRight;

	// top left
	CalcSlidingMovesOneDir(startTile, TOP_LEFT, topLeft, bottomRight, kingPos, foundKing, checkLOS, attackingTiles);
	HandleFoundMoves(startTile, foundKing, checkLOS, attackingTiles);

	// top right
	CalcSlidingMovesOneDir(startTile, TOP_RIGHT, topRight, bottomLeft, kingPos, foundKing, checkLOS, attackingTiles);
	HandleFoundMoves(startTile, foundKing, checkLOS, attackingTiles);

	// bottom left
	CalcSlidingMovesOneDir(startTile, BOT_LEFT, topRight, bottomLeft, kingPos, foundKing, checkLOS, attackingTiles);
	HandleFoundMoves(startTile, foundKing, checkLOS, attackingTiles);

	// bottom right
	CalcSlidingMovesOneDir(startTile, BOT_RIGHT, topLeft, bottomRight, kingPos, foundKing, checkLOS, attackingTiles);
	HandleFoundMoves(startTile, foundKing, checkLOS, attackingTiles);
	
	AddToMap(startTile, attackingTiles);
	
}

void Board::CalcKnightMoves(int startTile)
{
	std::vector<int> attackingTiles;
	std::vector<int> checkLOS;

	PieceTeam team = pieces[startTile]->GetTeam();

	int kingPos = team == WHITE ? kingPosBlack : kingPosWhite;

	if (TileInContainer(startTile, aFile))
	{
		CalcKnightMovesOneDir(startTile, UP + UP + RIGHT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, RIGHT + RIGHT + UP, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, RIGHT + RIGHT + DOWN, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, DOWN + DOWN + RIGHT, kingPos, checkLOS, attackingTiles);
	}
	else if (TileInContainer(startTile, bFile))
	{
		CalcKnightMovesOneDir(startTile, UP + UP + LEFT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, UP + UP + RIGHT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, RIGHT + RIGHT + UP, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, RIGHT + RIGHT + DOWN, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, DOWN + DOWN + RIGHT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, DOWN + DOWN + LEFT, kingPos, checkLOS, attackingTiles);
	}
	else if (TileInContainer(startTile, hFile))
	{
		CalcKnightMovesOneDir(startTile, UP + UP + LEFT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, LEFT + LEFT + UP, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, LEFT + LEFT + DOWN, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, DOWN + DOWN + LEFT, kingPos, checkLOS, attackingTiles);
	}
	else if (TileInContainer(startTile, gFile))
	{
		CalcKnightMovesOneDir(startTile, UP + UP + LEFT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, UP + UP + RIGHT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, LEFT + LEFT + UP, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, LEFT + LEFT + DOWN, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, DOWN + DOWN + RIGHT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, DOWN + DOWN + LEFT, kingPos, checkLOS, attackingTiles);
	}
	else
	{
		CalcKnightMovesOneDir(startTile, UP + UP + RIGHT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, RIGHT + RIGHT + UP, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, RIGHT + RIGHT + DOWN, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, DOWN + DOWN + RIGHT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, DOWN + DOWN + LEFT, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, LEFT + LEFT + DOWN, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, LEFT + LEFT + UP, kingPos, checkLOS, attackingTiles);
		CalcKnightMovesOneDir(startTile, UP + UP + LEFT, kingPos, checkLOS, attackingTiles);
	}

	AddToMap(startTile, attackingTiles);
}

void Board::CalcRookMoves(int startTile)
{
	std::vector<int> attackingTiles;
	std::vector<int> checkLOS;

	PieceTeam team = pieces[startTile]->GetTeam();
	
	int kingPos = team == WHITE ? kingPosBlack : kingPosWhite;
	bool foundKing = false;

	int top = edgesFromTiles[startTile].top;
	int bottom = edgesFromTiles[startTile].bottom;
	int left = edgesFromTiles[startTile].left;
	int right = edgesFromTiles[startTile].right;

	// down
	CalcSlidingMovesOneDir(startTile, DOWN, top, bottom, kingPos, foundKing, checkLOS, attackingTiles);
	HandleFoundMoves(startTile, foundKing, checkLOS, attackingTiles);

	// up
	CalcSlidingMovesOneDir(startTile, UP, top, bottom, kingPos, foundKing, checkLOS, attackingTiles);
	HandleFoundMoves(startTile, foundKing, checkLOS, attackingTiles);

	// right
	CalcSlidingMovesOneDir(startTile, RIGHT, left, right, kingPos, foundKing, checkLOS, attackingTiles);
	HandleFoundMoves(startTile, foundKing, checkLOS, attackingTiles);

	// left
	CalcSlidingMovesOneDir(startTile, LEFT, left, right, kingPos, foundKing, checkLOS, attackingTiles);
	HandleFoundMoves(startTile, foundKing, checkLOS, attackingTiles);
	
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
	if (InMapRange(target) && ((pieces[startTile]->GetTeam() == WHITE && !TileInContainer(startTile, hFile)) || (pieces[startTile]->GetTeam() == BLACK && !TileInContainer(startTile, aFile))))
	{
		if (pieces[target])
		{
			if (pieces[startTile]->GetTeam() != pieces[target]->GetTeam())
			{
				attackingTiles.push_back(target);
				if (pieces[target]->GetType() == KING)
				{
					std::vector<int> checkLOS;
					checkLOS.push_back(target);
					AddCheckingPiece(startTile, checkLOS);
				}
			}
			else
			{
				AddProtectedPieceToSet(target);
			}
		}
		else
		{
			AddToAttackSet(startTile, target);
		}
	}

	// take on diagonal, local forward left
	// handle destruction of en passant piece when move is confirmed
	target = startTile + TOP_LEFT * teamDir;
	if (InMapRange(target) && ((pieces[startTile]->GetTeam() == WHITE && !TileInContainer(startTile, aFile)) || (pieces[startTile]->GetTeam() == BLACK && !TileInContainer(startTile, hFile))))
	{
		if (pieces[target])
		{
			if (pieces[startTile]->GetTeam() != pieces[target]->GetTeam())
			{
				attackingTiles.push_back(target);
				if (pieces[target]->GetType() == KING)
				{
					std::vector<int> checkLOS;
					checkLOS.push_back(target);
					AddCheckingPiece(startTile, checkLOS);
				}
			}
			else
			{
				AddProtectedPieceToSet(target);
			}
		}
		else
		{
			AddToAttackSet(startTile, target);
		}
	}

	AddToMap(startTile, attackingTiles);
}

bool Board::BlockedByOwnPiece(int startTile, int target) const
{
	return InMapRange(target) && pieces[target] && pieces[startTile]->GetTeam() == pieces[target]->GetTeam() && pieces[target]->GetType() != EN_PASSANT;
}

bool Board::BlockedByEnemyPiece(int startTile, int target) const
{
	return pieces[target] && pieces[startTile]->GetTeam() != pieces[target]->GetTeam() && pieces[target]->GetType() != EN_PASSANT;
}

void Board::CompleteTurn()
{
	ClearEnPassant();
	ClearPinnedPieces();

	if (currentTurn == WHITE)
	{
		bInCheckWhite = false;
	}
	else
	{
		bInCheckBlack = false;
	}

	currentTurn = currentTurn == WHITE ? BLACK : WHITE;
	CalculateMoves();

	if (bGameOver)
	{
		return;
	}

	if (bVsComputer && currentTurn == compTeam)
	{
		PlayCompMove();
	}
}

void Board::AddNotBlocked(int startTile, int target, std::vector<int>& validMoves)
{
	if (InMapRange(target) && !BlockedByOwnPiece(startTile, target))
	{
		validMoves.push_back(target);
	}
}

// ========================================== CASTLING ==========================================

void Board::CalculateCastling()
{
	if (pieces[kingPosWhite]->bMoved == false)
	{
		// long castle
		int target = kingPosWhite - 2;
		int rookPos = target - 2;
		if (CheckCanCastle(kingPosWhite, target, rookPos, -1))
		{
			attackMapWhite[kingPosWhite].push_back(target);
		}

		// short castle
		target = kingPosWhite + 2;
		rookPos = target + 1;
		if (CheckCanCastle(kingPosWhite, target, rookPos, 1))
		{
			attackMapWhite[kingPosWhite].push_back(target);
		}
	}

	if (pieces[kingPosBlack]->bMoved == false)
	{
		// long castle
		int target = kingPosBlack - 2;
		int rookPos = target - 2;
		if (CheckCanCastle(kingPosBlack, target, rookPos, -1))
		{
			attackMapBlack[kingPosBlack].push_back(target);
		}

		// short castle
		target = kingPosBlack + 2;
		rookPos = target + 1;
		if (CheckCanCastle(kingPosBlack, target, rookPos, 1))
		{
			attackMapBlack[kingPosBlack].push_back(target);
		}
	}
}

bool Board::CheckCanCastle(int startTile, int target, int rookPos, int dir) const
{
	int team = pieces[startTile]->GetTeam();
	
	return InMapRange(target) && pieces[target] == nullptr && pieces[target - dir] == nullptr &&
		InMapRange(rookPos) && pieces[rookPos] && pieces[rookPos]->GetTeam() == pieces[startTile]->GetTeam() &&
		pieces[rookPos]->GetType() == ROOK && !pieces[rookPos]->bMoved &&
		!TileInContainer(target, team == WHITE ? attackSetBlack : attackSetWhite) && !TileInContainer(target - dir, team == WHITE ? attackSetBlack : attackSetWhite);
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
		pieces[startTile + 1] = pieces[startTile + 3];
		pieces[startTile + 3] = nullptr;
		pieces[startTile + 1]->bMoved = true;
	}
}

// ========================================== EN PASSANT ==========================================

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

void Board::HandlePromotion(int endTile)
{
	if ((currentTurn == WHITE && TileInContainer(endTile, eighthRank)) || (currentTurn == BLACK && TileInContainer(endTile, firstRank)))
	{
		bChoosingPromotion = true;
		bSetPromoSound = true;
		pieceToPromote = endTile;
	}
}

// ========================================== CHECK & CHECKMATE ==========================================

void Board::CalculateMoves()
{
	attackSetWhite.clear();
	attackSetBlack.clear();
	ClearCheckingPieces();
	kingXRay.clear();

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
			SetKingPos(i);
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
	HandlePinnedPieces();
	CalculateCastling();
	CalculateCheck();
}

void Board::CalculateAttacks()
{
	for (size_t tile = 0; tile < 64; tile++)
	{
		if (attackMapWhite[tile].empty())
			continue;

		// if pawn, only insert attacking, diagonal moves
		if (pieces[tile]->GetType() == PAWN)
		{
			for (int i : attackMapWhite[tile])
			{
				if (i != tile - 8 && i != tile - 16 && i != tile + 8 && i != tile + 16)
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
		if (attackMapBlack[tile].empty())
			continue;

		// if pawn, only insert attacking, diagonal moves
		if (pieces[tile]->GetType() == PAWN)
		{
			for (int i : attackMapBlack[tile])
			{
				if (i != tile - 8 && i != tile - 16 && i != tile + 8 && i != tile + 16)
				{
					attackSetBlack.insert(i);
				}
			}
			continue;
		}

		for (int i : attackMapBlack[tile])
		{
			attackSetBlack.insert(i);
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

void Board::AddToAttackSet(int startTile, int target)
{
	if (pieces[startTile]->GetTeam() == WHITE)
	{
		attackSetWhite.insert(target);
	}
	else
	{
		attackSetBlack.insert(target);
	}
}

void Board::CalculateCheck()
{
	if (CheckStalemate())
	{
		GameOver(NONE);
		return;
	}
	
	int kingPos = currentTurn == WHITE ? kingPosWhite : kingPosBlack;

	if (currentTurn == WHITE)
	{
		if (TileInContainer(kingPos, attackSetBlack))
		{
			bInCheckWhite = true;
#if TESTING != 1
			printf("White in check!\n");
#endif
			int moveCount = CalcValidCheckMoves();

#if TESTING != 1
			printf("%i moves possible!\n", moveCount);
#endif

			if (moveCount <= 0)
			{
				GameOver(BLACK);
				return;
			}

			soundEngine->play2D("sounds/move-check.mp3");

			ClearMoves(WHITE);
			SetCheckMoves();
		}
		else
		{
			bInCheckWhite = false;
			PlayMoveSound();
		}
	}
	else
	{
		if (TileInContainer(kingPos, attackSetWhite))
		{
			bInCheckBlack = true;
#if TESTING != 1
			printf("Black in check!\n");
#endif
			int moveCount = CalcValidCheckMoves();
#if TESTING != 1
			printf("%i moves possible!\n", moveCount);
#endif

			if (moveCount <= 0)
			{
				GameOver(WHITE);
				return;
			}

			soundEngine->play2D("sounds/move-check.mp3");
			ClearMoves(BLACK);
			SetCheckMoves();
		}
		else
		{
			bInCheckBlack = false;
			PlayMoveSound();
		}
	}

	if (bSetPromoSound)
	{
		bSetPromoSound = false;
	}
}

void Board::ClearMoves(int team)
{
	if (team == WHITE)
	{
		for (std::vector<int>& attacks : attackMapWhite)
		{
			attacks.clear();
		}
	}
	else if (team == BLACK)
	{
		for (std::vector<int>& attacks : attackMapBlack)
		{
			attacks.clear();
		}
	}
}

void Board::SetCheckMoves()
{
	if (currentTurn == WHITE)
	{
		ClearMoves(WHITE);
		std::copy(std::begin(validCheckMoves), std::end(validCheckMoves), std::begin(attackMapWhite));
	}
	else
	{
		ClearMoves(BLACK);
		std::copy(std::begin(validCheckMoves), std::end(validCheckMoves), std::begin(attackMapBlack));
	}
}

bool Board::KingEscapesCheck(int endTile)
{
	if (currentTurn == WHITE)
	{
		return !attackSetBlack.count(endTile) && !kingXRay.count(endTile);
	}
	else
	{
		return !attackSetWhite.count(endTile) && !kingXRay.count(endTile);
	}
	
}

bool Board::CanKingEscape(int startTile, int& moveCount)
{
	int top = edgesFromTiles[startTile].top;
	int bottom = edgesFromTiles[startTile].bottom;
	int left = edgesFromTiles[startTile].left;
	int right = edgesFromTiles[startTile].right;
	int topLeft = edgesFromTiles[startTile].topLeft;
	int topRight = edgesFromTiles[startTile].topRight;
	int bottomLeft = edgesFromTiles[startTile].bottomLeft;
	int bottomRight = edgesFromTiles[startTile].bottomRight;

	int target = startTile + DOWN;
	if (top <= target && target <= bottom && !BlockedByOwnPiece(startTile, target) && KingEscapesCheck(target))
	{
		validCheckMoves[startTile].push_back(target);
		moveCount++;
	}

	target = startTile + UP;
	if (top <= target && target <= bottom && !BlockedByOwnPiece(startTile, target) && KingEscapesCheck(target))
	{
		validCheckMoves[startTile].push_back(target);
		moveCount++;
	}

	target = startTile + LEFT;
	if (left <= target && target <= right && startTile != left && !BlockedByOwnPiece(startTile, target) && KingEscapesCheck(target))
	{
		validCheckMoves[startTile].push_back(target);
		moveCount++;
	}

	target = startTile + RIGHT;
	if (left <= target && target <= right && startTile != right && !BlockedByOwnPiece(startTile, target) && KingEscapesCheck(target))
	{
		validCheckMoves[startTile].push_back(target);
		moveCount++;
	}

	target = startTile + TOP_LEFT;
	if (topLeft <= target && target <= bottomRight && !TileInContainer(startTile, aFile) && !TileInContainer(startTile, eighthRank) &&
		!BlockedByOwnPiece(startTile, target) && KingEscapesCheck(target))
	{
		validCheckMoves[startTile].push_back(target);
		moveCount++;
	}

	target = startTile + TOP_RIGHT;
	if (topRight <= target && target <= bottomLeft && !TileInContainer(startTile, hFile) && !TileInContainer(startTile, eighthRank) &&
		!BlockedByOwnPiece(startTile, target) && KingEscapesCheck(target))
	{
		validCheckMoves[startTile].push_back(target);
		moveCount++;
	}

	target = startTile + BOT_LEFT;
	if (topRight <= target && target <= bottomLeft && !TileInContainer(startTile, aFile) && !TileInContainer(startTile, firstRank) &&
		!BlockedByOwnPiece(startTile, target) && KingEscapesCheck(target))
	{
		validCheckMoves[startTile].push_back(target);
		moveCount++;
	}

	target = startTile + BOT_RIGHT;
	if (topLeft <= target && target <= bottomRight && !TileInContainer(startTile, hFile) && !TileInContainer(startTile, firstRank) &&
		!BlockedByOwnPiece(startTile, target) && KingEscapesCheck(target))
	{
		validCheckMoves[startTile].push_back(target);
		moveCount++;
	}

#if TESTING != 1
	printf("Move count after CanKingEscape(): %i\n", moveCount);
#endif
	
	return moveCount > 0;
}

bool Board::MoveBlocksCheck(int startTile, int endTile)
{
	int size = currentTurn == WHITE ? checkingPiecesBlack.size() : checkingPiecesWhite.size();

	if (size == 1)
	{
		return pieces[startTile]->GetType() != KING && TileInContainer(endTile, currentTurn == WHITE ? checkingPiecesBlack[0]->lineOfSight : checkingPiecesWhite[0]->lineOfSight);
	}
	return false;
}

bool Board::CanBlockCheck(int kingPos, int& moveCount)
{
	bool bCanBlockCheck = false;

	std::set<int> checkingTiles;
	if (currentTurn == WHITE)
	{
		for (CheckingPiece* piece : checkingPiecesBlack)
		{
			std::copy(piece->lineOfSight.begin(), piece->lineOfSight.end(), std::inserter(checkingTiles, checkingTiles.end()));
		}
	}
	else
	{
		for (CheckingPiece* piece : checkingPiecesWhite)
		{
			std::copy(piece->lineOfSight.begin(), piece->lineOfSight.end(), std::inserter(checkingTiles, checkingTiles.end()));
		}
	}

	if (currentTurn == WHITE)
	{
		for (size_t i = 0; i < 64; i++)
		{
			for (int tile : attackMapWhite[i])
			{
				if (TileInContainer(tile, checkingTiles) && pieces[i] && pieces[i]->GetType() != KING)
				{
					bCanBlockCheck = true;
					moveCount++;
					validCheckMoves[i].push_back(tile);
				}
			}
		}
	}
	else
	{
		for (size_t i = 0; i < 64; i++)
		{
			for (int tile : attackMapBlack[i])
			{
				if (TileInContainer(tile, checkingTiles) && pieces[i] && pieces[i]->GetType() != KING)
				{
					bCanBlockCheck = true;
					moveCount++;
					validCheckMoves[i].push_back(tile);
				}
			}
		}
	}

#if TESTING != 1
	printf("Move count after CanBlockCheck(): %i\n", moveCount);
#endif

	return bCanBlockCheck;
}

bool Board::MoveTakesCheckingPiece(int endTile)
{
	auto checkingPieces = currentTurn == WHITE ? checkingPiecesBlack : checkingPiecesWhite;
	
	if (checkingPieces.size() == 1 && endTile == checkingPieces[0]->tile)
	{
		return true;
	}
	return false;
}

void Board::AddCheckingPiece(int startTile, const std::vector<int>& checkLOS)
{
	CheckingPiece* checkingPiece = new CheckingPiece();
	checkingPiece->tile = startTile;
	checkingPiece->pieceType = pieces[startTile]->GetType();
	checkingPiece->lineOfSight = checkLOS;
	pieces[startTile]->GetTeam() == WHITE ? checkingPiecesWhite.push_back(checkingPiece) : checkingPiecesBlack.push_back(checkingPiece);
}

void Board::AddProtectedPieceToSet(int target)
{
	if (pieces[target]->GetTeam() == WHITE)
	{
		attackSetWhite.insert(target);
	}
	else
	{
		attackSetBlack.insert(target);
	}
}

void Board::ClearCheckingPieces()
{
	while (checkingPiecesWhite.size() != 0)
	{
		delete checkingPiecesWhite[0];
		checkingPiecesWhite.erase(checkingPiecesWhite.begin());
	}
	
	while (checkingPiecesBlack.size() != 0)
	{
		delete checkingPiecesBlack[0];
		checkingPiecesBlack.erase(checkingPiecesBlack.begin());
	}
}

bool Board::CanTakeCheckingPiece(int kingPos, int& moveCount)
{
	// exclude taking by king as this is calculated in CanKingEscape()

	bool bCanTakeCheckingPiece = false;
	int checkPiecePos = -1;
	if (currentTurn == WHITE && checkingPiecesBlack.size() > 0)
	{
		checkPiecePos = checkingPiecesBlack[0]->tile;
	}
	else if (currentTurn == BLACK && checkingPiecesWhite.size() > 0)
	{
		checkPiecePos = checkingPiecesWhite[0]->tile;
	}

	// find moves where own piece attacks the checking piece
	if (currentTurn == WHITE)
	{
		for (size_t i = 0; i < 64; i++)
		{
			if (TileInContainer(checkPiecePos, attackMapWhite[i]) && pieces[i] && pieces[i]->GetType() != KING)
			{
				bCanTakeCheckingPiece = true;
				moveCount++;
				validCheckMoves[i].push_back(checkPiecePos);
			}
		}
	}
	else
	{
		for (size_t i = 0; i < 64; i++)
		{
			if (TileInContainer(checkPiecePos, attackMapBlack[i]) && pieces[i] && pieces[i]->GetType() != KING)
			{
				bCanTakeCheckingPiece = true;
				moveCount++;
				validCheckMoves[i].push_back(checkPiecePos);
			}
		}
	}

#if TESTING != 1
	printf("Move count after CanTakeCheckingPiece(): %i\n", moveCount);
#endif

	return bCanTakeCheckingPiece;
}

int Board::CalcValidCheckMoves()
{	
	ClearValidCheckMoves();

	int kingPos = currentTurn == WHITE ? kingPosWhite : kingPosBlack;
	int moveCount = 0;
	
	CanKingEscape(kingPos, moveCount);

	if (currentTurn == WHITE && checkingPiecesBlack.size() == 1)
	{
		CanBlockCheck(kingPos, moveCount);
		CanTakeCheckingPiece(kingPos, moveCount);
	}
	else if (currentTurn == BLACK && checkingPiecesWhite.size() == 1)
	{
		CanBlockCheck(kingPos, moveCount);
		CanTakeCheckingPiece(kingPos, moveCount);
	}

	// copy validCheckMoves into attackMap
	if (moveCount > 0)
	{
		if (currentTurn == WHITE)
		{
			ClearMoves(WHITE);
			std::copy(std::begin(validCheckMoves), std::end(validCheckMoves), std::begin(attackMapWhite));
		}
		else
		{
			ClearMoves(BLACK);
			std::copy(std::begin(validCheckMoves), std::end(validCheckMoves), std::begin(attackMapBlack));
		}
	}

	return moveCount;
}

void Board::ClearValidCheckMoves()
{
	for (std::vector<int>& moves : validCheckMoves)
	{
		moves.clear();
	}
}

void Board::ClearPinnedPieces()
{
	for (PinnedPiece* piece : pinnedPiecesWhite)
	{
		delete piece;
	}
	pinnedPiecesWhite.clear();

	for (PinnedPiece* piece : pinnedPiecesBlack)
	{
		delete piece;
	}
	pinnedPiecesBlack.clear();
}

void Board::AddPinnedPiece(int startTile, const std::vector<int>& pinLOS)
{
	PinnedPiece* piece = new PinnedPiece();
	piece->tile = startTile;
	piece->lineOfSight = pinLOS;

	if (pieces[startTile]->GetTeam() == WHITE)
	{
		pinnedPiecesWhite.push_back(piece);
	}
	else
	{
		pinnedPiecesBlack.push_back(piece);
	}
}

void Board::HandlePinnedPieces()
{
	if (currentTurn == WHITE)
	{
		for (PinnedPiece* piece : pinnedPiecesWhite)
		{
			for (int attack : attackMapWhite[piece->tile])
			{
				if (!TileInContainer(attack, piece->lineOfSight))
				{
					attackMapWhite[piece->tile].erase(std::remove(attackMapWhite[piece->tile].begin(), attackMapWhite[piece->tile].end(), attack), attackMapWhite[piece->tile].end());
				}
			}
		}
	}
	else
	{
		for (PinnedPiece* piece : pinnedPiecesBlack)
		{
			for (int attack : attackMapBlack[piece->tile])
			{
				if (!TileInContainer(attack, piece->lineOfSight))
				{
					attackMapBlack[piece->tile].erase(std::remove(attackMapBlack[piece->tile].begin(), attackMapBlack[piece->tile].end(), attack), attackMapBlack[piece->tile].end());
				}
			}
		}
	}
}

void Board::SetupGame(bool bTest)
{
	currentTurn = WHITE;
	lastEnPassantIndex = -1;
	enPassantOwner = nullptr;
	bChoosingPromotion = false;
	pieceToPromote = -1;
	lastMoveStart = -1;
	lastMoveEnd = -1;

	bInCheckBlack = false;
	bInCheckWhite = false;
	
	bGameOver = false;
	winner = 0;

	bTesting = bTest;

	if (!bTest)
	{
		lastMoveSound = NONE;
		bSetPromoSound = false;
		ClearButtons();
	}

	SetupBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
	FindKings();
	CalculateMoves();
}

void Board::PlayCompMove()
{
	std::random_device rd;
	std::uniform_int_distribution<int> tiles(0, 63);
	
	while (true)
	{
		int startTile = tiles(rd);
		int numOfMoves = compTeam == WHITE ? attackMapWhite[startTile].size() : attackMapBlack[startTile].size();
		if (numOfMoves == 0)
		{
			continue;
		}

		std::uniform_int_distribution<int> moves(0, numOfMoves - 1);
		int endTile = compTeam == WHITE ? attackMapWhite[startTile][moves(rd)] : attackMapBlack[startTile][moves(rd)];
		if (MovePiece(startTile, endTile))
		{
			return;
		}
	}
}

bool Board::CheckStalemate()
{
	if (currentTurn == WHITE)
	{
		std::vector<int> movesToRemove;
		for (int move : attackMapWhite[kingPosWhite])
		{
			if (TileInContainer(move, attackSetBlack))
			{
				movesToRemove.push_back(move);
			}
		}

		if (!movesToRemove.empty())
		{
			for (int move : movesToRemove)
			{
				attackMapWhite[kingPosWhite].erase(std::remove(attackMapWhite[kingPosWhite].begin(), attackMapWhite[kingPosWhite].end(), move), attackMapWhite[kingPosWhite].end());
			}
		}

		if (TileInContainer(kingPosWhite, attackSetBlack))
		{
			return false;
		}

		for (std::vector<int>& pieceMoves : attackMapWhite)
		{
			if (!pieceMoves.empty())
			{
				return false;
			}
		}
	}
	else
	{
		std::vector<int> movesToRemove;
		for (int move : attackMapBlack[kingPosBlack])
		{
			if (TileInContainer(move, attackSetWhite))
			{
				movesToRemove.push_back(move);
			}
		}

		if (!movesToRemove.empty())
		{
			for (int move : movesToRemove)
			{
				attackMapBlack[kingPosBlack].erase(std::remove(attackMapBlack[kingPosBlack].begin(), attackMapBlack[kingPosBlack].end(), move), attackMapBlack[kingPosBlack].end());

			}
		}

		if (TileInContainer(kingPosBlack, attackSetWhite))
		{
			return false;
		}

		for (std::vector<int>& pieceMoves : attackMapBlack)
		{
			if (!pieceMoves.empty())
			{
				return false;
			}
		}
	}

	return true;
}

void Board::GameOver(int winningTeam)
{
	winner = winningTeam;
	bGameOver = true;
	soundEngine->stopAllSounds();
	soundEngine->play2D("sounds/game-end.mp3");

	if (!bTesting)
	{
		ClearButtons();
		Button* continueButton = new Button(this, (float)width / 8 * 7, 0.038f, 0.2f, (float)width / 8, 0.12f, 0.15f);
		continueButton->SetCallback(std::bind(&Board::ContinueCallback, this));
		continueButton->bUseTexture = true;
		continueButton->SetTexture("textures/buttons/continue.png");
		buttons.push_back(continueButton);

		ShowWinnerMessage();

		if (winner == NONE)
		{
			printf("Stalemate!\n");
			return;
		}

		printf("%s wins!\n", winner == WHITE ? "White" : "Black");
	}
}

// ========================================== UTILITY ==========================================

template <typename T>
bool Board::TileInContainer(int target, T container) const
{
	return std::find(container.begin(), container.end(), target) != container.end();
}

void Board::FindKings()
{
	int kingsSet = 0;
	for (size_t i = 0; kingsSet < 2 && i < 64; i++)
	{
		if (pieces[i] != nullptr && pieces[i]->GetType() == KING)
		{
			SetKingPos(i);
			kingsSet++;
		}
	}
}

void Board::SetKingPos(int target)
{
	if (pieces[target]->GetTeam() == WHITE)
	{
		kingPosWhite = target;
	}
	else
	{
		kingPosBlack = target;
	}
}

void Board::PlayMoveSound()
{
	switch (lastMoveSound)
	{
	case MOVE_SELF:
		soundEngine->play2D("sounds/move-self.mp3");
		break;
	case MOVE_OPP:
		soundEngine->play2D("sounds/move-opponent.mp3");
		break;
	case CAPTURE:
		soundEngine->play2D("sounds/capture.mp3");
		break;
	case CHECK:
		soundEngine->play2D("sounds/move-check.mp3");
		break;
	case CASTLE:
		soundEngine->play2D("sounds/castle.mp3");
		break;
	case CHECKMATE:
		soundEngine->play2D("sounds/game-end.mp3");
		break;
	case PROMOTE:
		soundEngine->play2D("sounds/promote.mp3");
		break;
	default:
		break;
	}
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
	}
}

bool Board::PieceExists(int index)
{
	if (pieces[index] == nullptr)
	{
		return false;
	}
	return true;
}

void Board::Promote(PieceType pieceType)
{
	printf("Promoting a piece...\n");

	delete pieces[pieceToPromote];
	pieces[pieceToPromote] = new Piece(currentTurn, pieceType);
	bChoosingPromotion = false;
	CompleteTurn();
}

void Board::ShowWinnerMessage()
{
	if (winner == WHITE)
	{
		Button* winText = new Button(this, (float)width / 8 * 7, 0.038f, 0.7f, (float)width / 8, 0.12f, 0.15f);
		winText->SetCallback(std::bind(&Board::EmptyFunction, this));
		winText->bFillButton = false;
		winText->bUseTexture = true;
		winText->SetTexture("textures/buttons/white_wins.png");
		buttons.push_back(winText);
	}
	else if (winner == BLACK)
	{
		Button* winText = new Button(this, (float)width / 8 * 7, 0.038f, 0.7f, (float)width / 8, 0.12f, 0.15f);
		winText->SetCallback(std::bind(&Board::EmptyFunction, this));
		winText->bFillButton = false;
		winText->bUseTexture = true;
		winText->SetTexture("textures/buttons/black_wins.png");
		buttons.push_back(winText);
	}
	else
	{
		Button* winText = new Button(this, (float)width / 8 * 7, 0.038f, 0.7f, (float)width / 8, 0.12f, 0.15f);
		winText->SetCallback(std::bind(&Board::EmptyFunction, this));
		winText->bFillButton = false;
		winText->bUseTexture = true;
		winText->SetTexture("textures/buttons/stalemate.png");
		buttons.push_back(winText);
	}
}

void Board::SetupBoardFromFEN(std::string fen)
{
	for (size_t i = 0; i < 64; i++)
	{
		if (pieces[i] == nullptr)
			continue;

		delete pieces[i];
		pieces[i] = nullptr;
	}
	
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
				case 'e':
					pieces[index] = new Piece(team, EN_PASSANT);
					lastEnPassantIndex = index;
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

	if (InMapRange(lastEnPassantIndex) && pieces[lastEnPassantIndex]->GetTeam() == WHITE)
	{
		enPassantOwner = pieces[lastEnPassantIndex - 8];
	}
	else
	{
		enPassantOwner = pieces[lastEnPassantIndex + 8];
	}
}

bool Board::ShouldHighlightSelectedObject(int selectedObjectId, int objectId)
{
	return (InMapRange(selectedObjectId) && ((selectedObjectId == objectId && pieces[objectId] != nullptr) ||
		TileInContainer(objectId, currentTurn == WHITE ? attackMapWhite[selectedObjectId] : attackMapBlack[selectedObjectId])));
}

bool Board::ShouldHighlightLastMove(int objectId)
{
	return (InMapRange(lastMoveStart) && lastMoveStart == objectId) || (InMapRange(lastMoveEnd) && lastMoveEnd == objectId);
}
