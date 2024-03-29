#include "Board.h"

#include <memory>

#include "Button.h"

#ifdef TESTING
#include "Timer.h"
#endif

#include "EvalBoard.h"

Board::Board()
{
	VAO, EBO, VBO = 0;
	bInMainMenu = true;
	bInGame = false;
	bTesting = false;

	bestMoveStart = -1;
	bestMoveEnd = -1;
}

Board::~Board()
{
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

	if (evalBoard)
	{
		delete evalBoard;
	}
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

	evalBoard = new EvalBoard();
	if (evalBoard == nullptr)
	{
		printf("evalBoard is null!\n");
	}
	evalBoard->Init(this, soundEngine);

	SetBoardCoords();
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
		pieces[i].Init(PieceTeam::NONE, PieceType::NONE, false);
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
		Piece* piece = new Piece();
		piece->Init(PieceTeam::WHITE, type, false);
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

			if (!IsActivePiece(objectId))
			{
				continue;
			}

			glm::mat4 pieceModel = glm::mat4(1.f);
			pieceModel = glm::translate(pieceModel, glm::vec3((aspect / 13.7f) * file + 0.305f, (rank * 2 - 1) / 16.f, 0.f));
			pieceModel = glm::scale(pieceModel, glm::vec3(tileSize, tileSize, 1.f));
			glUniformMatrix4fv(pieceModelLocation, 1, GL_FALSE, glm::value_ptr(pieceModel));

			pieces[objectId].DrawPiece();
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
	soundEngine->setSoundVolume(1.f);
	
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

#ifdef TESTING
	Button* shannonTestButton = new Button(this, (float)width / 8 * 7, 0.f, 0.8f, (float)width / 8, 0.f, 0.15f);
	shannonTestButton->SetCallback(std::bind(&EvalBoard::ShannonTestCallback, evalBoard));
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
	compTeam = PieceTeam::BLACK;
	bVsComputer = true;
	bInMainMenu = false;
	bInGame = true;
	SetupGame(false);
}

void Board::PlayBlackCallback()
{
	compTeam = PieceTeam::WHITE;
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

// ========================================== TESTING ==========================================

// ========================================== MOVE LOGIC ==========================================

bool Board::MovePiece(int startTile, int endTile)
{
	if (!IsActivePiece(startTile))
		return false;

	if (!InMapRange(startTile) || !InMapRange(endTile))
		return false;

	if (!IsCurrentTurn(startTile))
	{
#ifdef TESTING
		if (!bTesting && !bSearching)
		{
			printf("It is %s's move!\n", currentTurn == PieceTeam::WHITE ? "white" : "black");
		}
#endif
		return false;
	}

	// check piece specific move
	if (!CheckLegalMove(startTile, endTile))
	{
#ifdef TESTING
		if (!bTesting && !bSearching)
		{
			printf("Not a legal move for this piece!\n");
		}
#endif
		return false;
	}

	if (pieces[startTile].GetTeam() == pieces[endTile].GetTeam())
	{
#ifdef TESTING
		if (!bTesting && !bSearching)
		{
			printf("Cannot take your own team's piece!\n");
		}
#endif
		return false;
	}

	// check if move puts self in check by king moving into attacked square
	if (pieces[startTile].GetType() == KING && TileInContainer(endTile, pieces[startTile].GetTeam() == PieceTeam::WHITE ? attackSetBlack : attackSetWhite))
	{
#ifdef TESTING
		if (!bTesting && !bSearching)
		{
			printf("Cannot put yourself in check!\n");
		}
#endif
		return false;
	}

	// if in check
	if (currentTurn == PieceTeam::WHITE ? bInCheckWhite : bInCheckBlack == true)
	{
		if (!MoveBlocksCheck(startTile, endTile) && !(pieces[startTile].GetType() == KING && KingEscapesCheck(endTile)) && !MoveTakesCheckingPiece(endTile))
		{
#ifdef TESTING
			if (!bTesting && !bSearching)
			{
				printf("Move does not escape check!\n");
			}
#endif
			return false;
		}
	}


	// checks complete
	bool bTookPiece = pieces[endTile].GetTeam() != currentTurn && pieces[endTile].GetType() != EN_PASSANT && pieces[endTile].GetType() != NONE;

	if (startTile == secondLastMoveEnd && endTile == secondLastMoveStart)
	{
		repeatedMoveCount++;
	}
	else
	{
		repeatedMoveCount = 0;
	}

	secondLastMoveStart = lastMoveStart;
	secondLastMoveEnd = lastMoveEnd;
	lastMoveStart = startTile;
	lastMoveEnd = endTile;

	if (pieces[startTile].GetType() == PAWN && pieces[endTile].GetType() == EN_PASSANT)
	{
		// handle destruction of en passant piece owner when move is confirmed
		TakeByEnPassant();
	}

	pieces[endTile].SetPiece(pieces[startTile].GetTeam(), pieces[startTile].GetType());
	pieces[startTile].ClearPiece();
	pieces[endTile].bMoved = true;

	switch (pieces[endTile].GetType())
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
	if (bTookPiece)
		lastMoveSound = MoveSounds::CAPTURE;
	else if (bSetPromoSound)
		lastMoveSound = MoveSounds::PROMOTE;
	else if (currentTurn == PieceTeam::BLACK)
		lastMoveSound = MoveSounds::MOVE_OPP;
	else
		lastMoveSound = MoveSounds::MOVE_SELF;

	if (bChoosingPromotion && ((bVsComputer && currentTurn == compTeam) || bTesting || bSearching))
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
	return TileInContainer(endTile, currentTurn == PieceTeam::WHITE ? attackMapWhite[startTile] : attackMapBlack[startTile]);
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
			if (BlockedByEnemyPiece(startTile, target) && pieces[target].GetType() == KING && pieces[target].GetTeam() != pieces[startTile].GetTeam())
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

	PieceTeam team = pieces[startTile].GetTeam();

	int kingPos = team == PieceTeam::WHITE ? kingPosBlack : kingPosWhite;
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

	PieceTeam team = pieces[startTile].GetTeam();

	int kingPos = team == PieceTeam::WHITE ? kingPosBlack : kingPosWhite;

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

	PieceTeam team = pieces[startTile].GetTeam();
	
	int kingPos = team == PieceTeam::WHITE ? kingPosBlack : kingPosWhite;
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
	int teamDir = pieces[startTile].GetTeam() == PieceTeam::WHITE ? 1 : -1;
	int target;

	// forward by 1
	target = startTile + UP * teamDir;
	if (InMapRange(target) && !BlockedByOwnPiece(startTile, target) && !BlockedByEnemyPiece(startTile, target))
		attackingTiles.push_back(target);

	// forward by 2 if first move of this pawn
	// handle creation of en passant piece when move is confirmed
	target = startTile + (2 * UP) * teamDir;
	if (InMapRange(target) && !pieces[startTile].bMoved && !BlockedByOwnPiece(startTile, target) && !BlockedByEnemyPiece(startTile, target) &&
		!BlockedByOwnPiece(startTile, target + DOWN * teamDir) && !BlockedByEnemyPiece(startTile, target + DOWN * teamDir))
		attackingTiles.push_back(target);

	// take on diagonal, local forward right
	// handle destruction of en passant piece when move is confirmed
	target = startTile + TOP_RIGHT * teamDir;
	if (InMapRange(target) && ((pieces[startTile].GetTeam() == PieceTeam::WHITE && !TileInContainer(startTile, hFile)) || (pieces[startTile].GetTeam() == PieceTeam::BLACK && !TileInContainer(startTile, aFile))))
	{
		if (IsActivePiece(target))
		{
			if (pieces[startTile].GetTeam() != pieces[target].GetTeam())
			{
				attackingTiles.push_back(target);
				if (pieces[target].GetType() == KING)
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
	if (InMapRange(target) && ((pieces[startTile].GetTeam() == PieceTeam::WHITE && !TileInContainer(startTile, aFile)) || (pieces[startTile].GetTeam() == PieceTeam::BLACK && !TileInContainer(startTile, hFile))))
	{
		if (IsActivePiece(target))
		{
			if (pieces[startTile].GetTeam() != pieces[target].GetTeam())
			{
				attackingTiles.push_back(target);
				if (pieces[target].GetType() == KING)
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
	return InMapRange(target) && pieces[startTile].GetTeam() == pieces[target].GetTeam() && pieces[target].GetType() != EN_PASSANT;
}

bool Board::BlockedByEnemyPiece(int startTile, int target) const
{
	return IsActivePiece(target) && pieces[startTile].GetTeam() != pieces[target].GetTeam() && pieces[target].GetType() != EN_PASSANT;
}

void Board::CompleteTurn()
{	
	ClearEnPassant();
	ClearPinnedPieces();

	if (currentTurn == PieceTeam::WHITE)
	{
		bInCheckWhite = false;
	}
	else
	{
		bInCheckBlack = false;
	}

	currentTurn = currentTurn == PieceTeam::WHITE ? PieceTeam::BLACK : PieceTeam::WHITE;

	if (bSearching && bSearchEnd)
	{
		return;
	}

	CalculateMoves();

	if (bGameOver)
	{
		return;
	}

	if (bVsComputer && currentTurn == compTeam && !bTesting && !bSearching)
	{
		std::thread([this] {this->PlayCompMove(); }).detach();
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
	if (pieces[kingPosWhite].bMoved == false)
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

	if (pieces[kingPosBlack].bMoved == false)
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
	PieceTeam team = pieces[startTile].GetTeam();
	
	return InMapRange(target) && !IsActivePiece(target) && !IsActivePiece(target - dir) &&
		InMapRange(rookPos) && pieces[rookPos].GetTeam() == pieces[startTile].GetTeam() &&
		pieces[rookPos].GetType() == ROOK && !pieces[rookPos].bMoved &&
		!TileInContainer(target, team == PieceTeam::WHITE ? attackSetBlack : attackSetWhite) && !TileInContainer(target - dir, team == PieceTeam::WHITE ? attackSetBlack : attackSetWhite);
}

void Board::HandleCastling(int startTile, int endTile)
{
	PieceTeam kingTeam = pieces[endTile].GetTeam();
	
	// long castle
	if (startTile - 2 == endTile)
	{
		pieces[startTile - 1].SetPiece(kingTeam, ROOK);
		pieces[startTile - 4].ClearPiece();
		pieces[startTile - 1].bMoved = true;
	}
	// short castle
	else if (startTile + 2 == endTile)
	{
		pieces[startTile + 1].SetPiece(kingTeam, ROOK);
		pieces[startTile + 3].ClearPiece();
		pieces[startTile + 1].bMoved = true;
	}
}

// ========================================== EN PASSANT ==========================================

void Board::CreateEnPassant(int startTile, int endTile)
{
	int teamDir = pieces[endTile].GetTeam() == PieceTeam::WHITE ? -1 : 1;
	if (startTile + 16 * teamDir == endTile)
	{
		pieces[startTile + 8 * teamDir].SetPiece(currentTurn, EN_PASSANT);
		lastEnPassantIndex = startTile + 8 * teamDir;
		enPassantOwner = endTile;
	}
}

void Board::ClearEnPassant()
{
	for (size_t i = 0; i < 64; i++)
	{
		if (pieces[i].GetType() == EN_PASSANT && i != lastEnPassantIndex)
		{
			pieces[i].ClearPiece();
		}
	}
	lastEnPassantIndex = -1;
}

void Board::TakeByEnPassant()
{
	pieces[enPassantOwner].ClearPiece();
}

void Board::HandlePromotion(int endTile)
{
	if ((currentTurn == PieceTeam::WHITE && TileInContainer(endTile, eighthRank)) || (currentTurn == PieceTeam::BLACK && TileInContainer(endTile, firstRank)))
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
	FindKings();

	for (size_t i = 0; i < 64; i++)
	{
		attackMapWhite[i].clear();
		attackMapBlack[i].clear();

		if (!IsActivePiece(i) || pieces[i].GetType() == EN_PASSANT)
		{
			continue;
		}

		switch (pieces[i].GetType())
		{
		case KING:
			CalcKingMoves(i);
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

	if (repeatedMoveCount >= 6)
	{
		GameOver(PieceTeam::NONE);
		return;
	}

	if (bInGame && !bSearching && !bTesting && !bGameOver)
	{
		HandleEval();
	}
}

void Board::CalculateAttacks()
{
	for (size_t tile = 0; tile < 64; tile++)
	{
		if (attackMapWhite[tile].empty())
			continue;

		// if pawn, only insert attacking, diagonal moves
		if (pieces[tile].GetType() == PAWN)
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
		if (pieces[tile].GetType() == PAWN)
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

	if (pieces[startTile].GetTeam() == PieceTeam::WHITE)
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
	if (pieces[startTile].GetTeam() == PieceTeam::WHITE)
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
		GameOver(PieceTeam::NONE);
		return;
	}
	
	int kingPos = currentTurn == PieceTeam::WHITE ? kingPosWhite : kingPosBlack;

	if (currentTurn == PieceTeam::WHITE)
	{
		if (TileInContainer(kingPos, attackSetBlack))
		{
			bInCheckWhite = true;
			int moveCount = CalcValidCheckMoves();

#ifdef TESTING
			if (!bTesting && !bSearching)
			{
				printf("White in check!\n");
				printf("%i moves possible!\n", moveCount);
			}
#endif

			if (moveCount <= 0)
			{
				GameOver(PieceTeam::BLACK);
				return;
			}

			if (!bTesting && !bSearching)
			{
				soundEngine->play2D("sounds/move-check.mp3");
			}
		}
		else
		{
			bInCheckWhite = false;

			if (!bTesting && !bSearching)
			{
				PlayMoveSound();
			}
		}
	}
	else
	{
		if (TileInContainer(kingPos, attackSetWhite))
		{
			bInCheckBlack = true;
			int moveCount = CalcValidCheckMoves();

#ifdef TESTING
			if (!bTesting && !bSearching)
			{
				printf("Black in check!\n");
				printf("%i moves possible!\n", moveCount);
			}
#endif

			if (moveCount <= 0)
			{
				GameOver(PieceTeam::WHITE);
				return;
			}

			if (!bTesting && !bSearching)
			{
				soundEngine->play2D("sounds/move-check.mp3");
			}
		}
		else
		{
			bInCheckBlack = false;
			if (!bTesting && !bSearching)
			{
				PlayMoveSound();
			}
		}
	}

	if (bSetPromoSound)
	{
		bSetPromoSound = false;
	}
}

void Board::ClearMoves(PieceTeam team)
{
	if (team == PieceTeam::WHITE)
	{
		for (std::vector<int>& attacks : attackMapWhite)
		{
			attacks.clear();
		}
	}
	else if (team == PieceTeam::BLACK)
	{
		for (std::vector<int>& attacks : attackMapBlack)
		{
			attacks.clear();
		}
	}
}

bool Board::KingEscapesCheck(int endTile)
{
	if (currentTurn == PieceTeam::WHITE)
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

#ifdef TESTING
	if (!bTesting && !bSearching)
	{
		printf("Move count after CanKingEscape(): %i\n", moveCount);
	}
#endif
	
	return moveCount > 0;
}

bool Board::MoveBlocksCheck(int startTile, int endTile)
{
	int size = currentTurn == PieceTeam::WHITE ? checkingPiecesBlack.size() : checkingPiecesWhite.size();

	if (size == 1)
	{
		return pieces[startTile].GetType() != KING && TileInContainer(endTile, currentTurn == PieceTeam::WHITE ? checkingPiecesBlack[0].lineOfSight : checkingPiecesWhite[0].lineOfSight);
	}
	return false;
}

bool Board::CanBlockCheck(int kingPos, int& moveCount)
{
	bool bCanBlockCheck = false;

	std::set<int> checkingTiles;
	if (currentTurn == PieceTeam::WHITE)
	{
		for (CheckingPiece piece : checkingPiecesBlack)
		{
			std::copy(piece.lineOfSight.begin(), piece.lineOfSight.end(), std::inserter(checkingTiles, checkingTiles.end()));
		}
	}
	else
	{
		for (CheckingPiece piece : checkingPiecesWhite)
		{
			std::copy(piece.lineOfSight.begin(), piece.lineOfSight.end(), std::inserter(checkingTiles, checkingTiles.end()));
		}
	}

	if (currentTurn == PieceTeam::WHITE)
	{
		for (size_t i = 0; i < 64; i++)
		{
			for (int tile : attackMapWhite[i])
			{
				if (TileInContainer(tile, checkingTiles) && IsActivePiece(i) && pieces[i].GetType() != KING)
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
				if (TileInContainer(tile, checkingTiles) && IsActivePiece(i) && pieces[i].GetType() != KING)
				{
					bCanBlockCheck = true;
					moveCount++;
					validCheckMoves[i].push_back(tile);
				}
			}
		}
	}

#ifdef TESTING
	if (!bTesting && !bSearching)
	{
		printf("Move count after CanBlockCheck(): %i\n", moveCount);
	}
#endif

	return bCanBlockCheck;
}

bool Board::MoveTakesCheckingPiece(int endTile) const
{
	const std::vector<CheckingPiece>& checkingPieces = currentTurn == PieceTeam::WHITE ? checkingPiecesBlack : checkingPiecesWhite;
	
	if (checkingPieces.size() == 1 && endTile == checkingPieces[0].tile)
	{
		return true;
	}
	return false;
}

void Board::AddCheckingPiece(int startTile, const std::vector<int>& checkLOS)
{
	if (pieces[startTile].GetTeam() == PieceTeam::WHITE)
	{
		checkingPiecesWhite.emplace(checkingPiecesWhite.begin());
		checkingPiecesWhite[0].tile = startTile;
		checkingPiecesWhite[0].pieceType = pieces[startTile].GetType();
		checkingPiecesWhite[0].lineOfSight = checkLOS;
	}
	else
	{
		checkingPiecesBlack.emplace(checkingPiecesBlack.begin());
		checkingPiecesBlack[0].tile = startTile;
		checkingPiecesBlack[0].pieceType = pieces[startTile].GetType();
		checkingPiecesBlack[0].lineOfSight = checkLOS;
	}
}

void Board::AddProtectedPieceToSet(int target)
{
	if (pieces[target].GetTeam() == PieceTeam::WHITE)
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
	checkingPiecesWhite.clear();
	checkingPiecesBlack.clear();
}

bool Board::CanTakeCheckingPiece(int kingPos, int& moveCount)
{
	// exclude taking by king as this is calculated in CanKingEscape()

	bool bCanTakeCheckingPiece = false;
	int checkPiecePos = -1;
	if (currentTurn == PieceTeam::WHITE && checkingPiecesBlack.size() > 0)
	{
		checkPiecePos = checkingPiecesBlack[0].tile;
	}
	else if (currentTurn == PieceTeam::BLACK && checkingPiecesWhite.size() > 0)
	{
		checkPiecePos = checkingPiecesWhite[0].tile;
	}

	// find moves where own piece attacks the checking piece
	if (currentTurn == PieceTeam::WHITE)
	{
		for (size_t i = 0; i < 64; i++)
		{
			if (TileInContainer(checkPiecePos, attackMapWhite[i]) && IsActivePiece(i) && pieces[i].GetType() != KING)
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
			if (TileInContainer(checkPiecePos, attackMapBlack[i]) && IsActivePiece(i) && pieces[i].GetType() != KING)
			{
				bCanTakeCheckingPiece = true;
				moveCount++;
				validCheckMoves[i].push_back(checkPiecePos);
			}
		}
	}

#ifdef TESTING
	if (!bTesting && !bSearching)
	{
		printf("Move count after CanTakeCheckingPiece(): %i\n", moveCount);
	}
#endif

	return bCanTakeCheckingPiece;
}

int Board::CalcValidCheckMoves()
{	
	ClearValidCheckMoves();

	int kingPos = currentTurn == PieceTeam::WHITE ? kingPosWhite : kingPosBlack;
	int moveCount = 0;
	
	CanKingEscape(kingPos, moveCount);

	if (currentTurn == PieceTeam::WHITE && checkingPiecesBlack.size() == 1)
	{
		CanBlockCheck(kingPos, moveCount);
		CanTakeCheckingPiece(kingPos, moveCount);
	}
	else if (currentTurn == PieceTeam::BLACK && checkingPiecesWhite.size() == 1)
	{
		CanBlockCheck(kingPos, moveCount);
		CanTakeCheckingPiece(kingPos, moveCount);
	}

	if (moveCount > 0)
	{
		if (currentTurn == PieceTeam::WHITE)
		{
			ClearMoves(PieceTeam::WHITE);
			std::copy(std::begin(validCheckMoves), std::end(validCheckMoves), std::begin(attackMapWhite));
		}
		else
		{
			ClearMoves(PieceTeam::BLACK);
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

	if (pieces[startTile].GetTeam() == PieceTeam::WHITE)
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
	if (currentTurn == PieceTeam::WHITE)
	{
		for (PinnedPiece* piece : pinnedPiecesWhite)
		{
			for (int i = attackMapWhite[piece->tile].size() - 1; i >= 0; i--)
			{
				if (!TileInContainer(attackMapWhite[piece->tile][i], piece->lineOfSight))
				{
					attackMapWhite[piece->tile].erase(std::remove(attackMapWhite[piece->tile].begin(), attackMapWhite[piece->tile].end(), attackMapWhite[piece->tile][i]), attackMapWhite[piece->tile].end());
				}
			}
		}
	}
	else
	{
		for (PinnedPiece* piece : pinnedPiecesBlack)
		{
			for (int i = attackMapBlack[piece->tile].size() - 1; i >= 0; i--)
			{
				if (!TileInContainer(attackMapBlack[piece->tile][i], piece->lineOfSight))
				{
					attackMapBlack[piece->tile].erase(std::remove(attackMapBlack[piece->tile].begin(), attackMapBlack[piece->tile].end(), attackMapBlack[piece->tile][i]), attackMapBlack[piece->tile].end());
				}
			}
		}
	}
}

void Board::SetupGame(bool bTest)
{
	currentTurn = PieceTeam::WHITE;
	lastEnPassantIndex = -1;
	enPassantOwner = -1;
	bChoosingPromotion = false;
	pieceToPromote = -1;
	lastMoveStart = -1;
	lastMoveEnd = -1;
	secondLastMoveStart = -1;
	secondLastMoveEnd = -1;
	repeatedMoveCount = 0;

	bInCheckBlack = false;
	bInCheckWhite = false;
	
	bGameOver = false;
	winner = PieceTeam::NONE;

	bTesting = bTest;

	if (!bTest)
	{
		lastMoveSound = MoveSounds::NONE;
		bSetPromoSound = false;
		ClearButtons();
	}

	SetupBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
	FindKings();
	CalculateMoves();
}

void Board::CapturePieceMovedState(std::vector<PieceMovedState>& pieceMovedStates)
{
	for (size_t i = 0; i < 64; i++)
	{
		if (!IsActivePiece(i))
			continue;

		if (pieces[i].GetType() == PAWN || pieces[i].GetType() == ROOK || pieces[i].GetType() == KING)
		{
			PieceMovedState state = PieceMovedState(i, pieces[i].bMoved);
			pieceMovedStates.push_back(state);
		}
	}
}

void Board::RecoverPieceMovedState(const std::vector<PieceMovedState>& pieceMovedStates)
{
	for (const PieceMovedState& boardState : pieceMovedStates)
	{
		pieces[boardState.tile].bMoved = boardState.bMoved;
	}
}

void Board::HandleEval()
{
	evalBoard->StopEval();
	evalBoard->SetFEN(BoardToFEN());

	std::vector<PieceMovedState> pieceMovedStates;
	CapturePieceMovedState(pieceMovedStates);
	evalBoard->SetMovedStates(pieceMovedStates);

	evalBoard->StartEval(DEPTH);
}

void Board::PlayCompMove()
{
	using namespace std::literals::chrono_literals;
	
	std::this_thread::sleep_for(1s);
	evalBoard->StopEval();

	while (evalBoard->IsSearching())
	{
		std::this_thread::sleep_for(500ms);
	}
	
	if (!MovePiece(evalBoard->bestMoveStart, evalBoard->bestMoveEnd))
	{
		printf("Computer cannot make optimal move from search!\n");
	}
}

void Board::PlayCompMoveRandom()
{
	std::random_device rd;
	std::uniform_int_distribution<int> tiles(0, 63);
	
	while (true)
	{
		int startTile = tiles(rd);
		int numOfMoves = compTeam == PieceTeam::WHITE ? attackMapWhite[startTile].size() : attackMapBlack[startTile].size();
		if (numOfMoves == 0)
		{
			continue;
		}

		std::uniform_int_distribution<int> moves(0, numOfMoves - 1);
		int endTile = compTeam == PieceTeam::WHITE ? attackMapWhite[startTile][moves(rd)] : attackMapBlack[startTile][moves(rd)];
		if (MovePiece(startTile, endTile))
		{
			return;
		}
	}
}

bool Board::CheckStalemate()
{
	if (currentTurn == PieceTeam::WHITE)
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

void Board::GameOver(PieceTeam winningTeam)
{
	winner = winningTeam;
	bGameOver = true;

	if (!bTesting && !bSearching)
	{
		soundEngine->stopAllSounds();
		soundEngine->play2D("sounds/game-end.mp3");
		ClearButtons();
		Button* continueButton = new Button(this, (float)width / 8 * 7, 0.038f, 0.2f, (float)width / 8, 0.12f, 0.15f);
		continueButton->SetCallback(std::bind(&Board::ContinueCallback, this));
		continueButton->bUseTexture = true;
		continueButton->SetTexture("textures/buttons/continue.png");
		buttons.push_back(continueButton);

		ShowWinnerMessage();

		if (winner == PieceTeam::NONE)
		{
			printf("Stalemate!\n");
			return;
		}

		printf("%s wins!\n", winner == PieceTeam::WHITE ? "White" : "Black");
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
		if (pieces[i].GetType() == KING)
		{
			SetKingPos(i);
			kingsSet++;
		}
	}
}

void Board::SetKingPos(int target)
{
	if (pieces[target].GetTeam() == PieceTeam::WHITE)
	{
		kingPosWhite = target;
	}
	else
	{
		kingPosBlack = target;
	}
}

void Board::SetBestMoves(const std::vector<Move>& bestMoves)
{
	if (bestMoves.size() == 1)
	{
		bestMoveStart = bestMoves[0].startTile;
		bestMoveEnd = bestMoves[0].endTile;
		return;
	}
	
	std::random_device rd;
	std::uniform_int_distribution<int> moveList(0, bestMoves.size() - 1);

	int move = moveList(rd);
	bestMoveStart = bestMoves[move].startTile;
	bestMoveEnd = bestMoves[move].endTile;
}

void Board::PlayMoveSound()
{
	switch (lastMoveSound)
	{
	case MoveSounds::MOVE_SELF:
		soundEngine->play2D("sounds/move-self.mp3");
		break;
	case MoveSounds::MOVE_OPP:
		soundEngine->play2D("sounds/move-opponent.mp3");
		break;
	case MoveSounds::CAPTURE:
		soundEngine->play2D("sounds/capture.mp3");
		break;
	case MoveSounds::CHECK:
		soundEngine->play2D("sounds/move-check.mp3");
		break;
	case MoveSounds::CASTLE:
		soundEngine->play2D("sounds/castle.mp3");
		break;
	case MoveSounds::CHECKMATE:
		soundEngine->play2D("sounds/game-end.mp3");
		break;
	case MoveSounds::PROMOTE:
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

int Board::CalcWhiteValue() const
{
	int teamVal = 0;

	for (size_t i = 0; i < 64; i++)
	{
		if (pieces[i].GetTeam() == PieceTeam::WHITE && pieces[i].GetType() != EN_PASSANT)
		{
			teamVal += pieces[i].GetValue();
		}
	}

	return teamVal;
}

int Board::CalcBlackValue() const
{
	int teamVal = 0;

	for (size_t i = 0; i < 64; i++)
	{
		if (pieces[i].GetTeam() == PieceTeam::BLACK && pieces[i].GetType() != EN_PASSANT)
		{
			teamVal += pieces[i].GetValue();
		}
	}

	return teamVal;
}

void Board::Promote(PieceType pieceType)
{
#ifdef TESTING
	if (!bTesting && !bSearching)
	{
		printf("Promoting a piece...\n");
	}
#endif
	pieces[pieceToPromote].SetPiece(currentTurn, pieceType);
	bChoosingPromotion = false;
	CompleteTurn();
}

void Board::ShowWinnerMessage()
{
	if (winner == PieceTeam::WHITE)
	{
		Button* winText = new Button(this, (float)width / 8 * 7, 0.038f, 0.7f, (float)width / 8, 0.12f, 0.15f);
		winText->SetCallback(std::bind(&Board::EmptyFunction, this));
		winText->bFillButton = false;
		winText->bUseTexture = true;
		winText->SetTexture("textures/buttons/white_wins.png");
		buttons.push_back(winText);
	}
	else if (winner == PieceTeam::BLACK)
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

void Board::SetupBoardFromFEN(const std::string& fen)
{
	for (size_t i = 0; i < 64; i++)
	{
		if (!IsActivePiece(i))
			continue;

		pieces[i].ClearPiece();
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
				team = PieceTeam::BLACK;
			}
			else
			{
				team = PieceTeam::WHITE;
			}

			switch (tolower(c))
			{
				case 'r':
					pieces[index].SetPiece(team, ROOK);
					pieces[index].bMoved = false;
					index++;
					continue;
				case 'n':
					pieces[index].SetPiece(team, KNIGHT);
					pieces[index].bMoved = false;
					index++;
					continue;
				case 'b':
					pieces[index].SetPiece(team, BISHOP);
					pieces[index].bMoved = false;
					index++;
					continue;
				case 'q':
					pieces[index].SetPiece(team, QUEEN);
					pieces[index].bMoved = false;
					index++;
					continue;
				case 'k':
					pieces[index].SetPiece(team, KING);
					pieces[index].bMoved = false;
					index++;
					continue;
				case 'p':
					pieces[index].SetPiece(team, PAWN);
					pieces[index].bMoved = false;
					index++;
					continue;
				case 'e':
					pieces[index].SetPiece(team, EN_PASSANT);
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

	if (InMapRange(lastEnPassantIndex) && IsActivePiece(lastEnPassantIndex))
	{
		if (pieces[lastEnPassantIndex].GetTeam() == PieceTeam::WHITE)
		{
			enPassantOwner = lastEnPassantIndex - 8;
		}
		else if (pieces[lastEnPassantIndex].GetTeam() == PieceTeam::BLACK)
		{
			enPassantOwner = lastEnPassantIndex + 8;
		}
	}
	lastEnPassantIndex = -1;
}

std::string Board::BoardToFEN()
{
	std::string fen;
	std::string fenChar = "";
	int length = 0;
	int spaces = 0;

	for (size_t i = 0; i < 64; i++)
	{
		if ((length + spaces) % 8 == 0 && (length + spaces) != 0)
		{
			fen.append("/");
			spaces = 0;
		}

		if (pieces[i].GetTeam() == PieceTeam::NONE || pieces[i].GetType() == PieceType::NONE)
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

		switch (pieces[i].GetType())
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

		if (pieces[i].GetTeam() == PieceTeam::WHITE)
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

	return fen;
}

void Board::SetBoardCoords()
{
	char file = 'a';
	int tile = 0;
	std::string coord = "";
	for (int i = 8; i > 0; i--)
	{
		for (int j = 0; j < 8; j++)
		{
			coord = file + j;
			std::string s = std::to_string(i);
			coord += s;
			boardCoords[tile] = coord;
			tile++;
		}
	}
}

std::string Board::ToBoard(const int tile) const
{
	return boardCoords[tile];
}

bool Board::ShouldHighlightSelectedObject(int selectedObjectId, int objectId)
{
	return (InMapRange(selectedObjectId) && ((selectedObjectId == objectId && IsActivePiece(objectId)) ||
		TileInContainer(objectId, currentTurn == PieceTeam::WHITE ? attackMapWhite[selectedObjectId] : attackMapBlack[selectedObjectId])));
}

bool Board::ShouldHighlightLastMove(int objectId)
{
	return (InMapRange(lastMoveStart) && lastMoveStart == objectId) || (InMapRange(lastMoveEnd) && lastMoveEnd == objectId);
}
