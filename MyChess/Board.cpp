#include "Board.h"

Board::Board()
{
	VAO, EBO, VBO = 0;
}

Board::~Board()
{
}

void Board::Init(unsigned int width, unsigned int height)
{
	// setup board
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

	glm::mat4 view = glm::mat4(1.f);
	view = glm::translate(view, glm::vec3(0.f, 0.f, -1.f));

	aspect = (float)width / (float)height;
	glm::mat4 projection = glm::ortho(0.f, aspect, 0.f, 1.f, 0.f, 1000.f);

	glUniformMatrix4fv(tileViewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(tileProjectionLocation, 1, GL_FALSE, glm::value_ptr(projection));


	// setup piece textures
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
		glGenerateMipmap(GL_TEXTURE_2D);
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

	// setup pieces using FEN string
	for (int i = 0; i < 64; i++)
	{
		pieces[i] = new Piece(WHITE, KNIGHT);
	}
}

void Board::DrawBoard()
{
	float size = 0.13f;

	for (size_t rank = 1; rank <= 8; rank++)
	{
		for (size_t file = 1; file <= 8; file++)
		{
			boardShader.UseShader();
			glBindVertexArray(VAO);
			
			if ((rank % 2 == 0) != (file % 2 == 0))
			{
				glUniform3f(tileColorLocation, whiteColor.x, whiteColor.y, whiteColor.z);
			}
			else
			{
				glUniform3f(tileColorLocation, blackColor.x, blackColor.y, blackColor.z);
			}
			
			glm::mat4 tileModel = glm::mat4(1.f);
			tileModel = glm::translate(tileModel, glm::vec3((aspect / 13.7f) * rank + 0.305f, (file * 2 - 1) / 16.f, 1.f));
			tileModel = glm::scale(tileModel, glm::vec3(size, size, 1.f));
			glUniformMatrix4fv(tileModelLocation, 1, GL_FALSE, glm::value_ptr(tileModel));
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			glBindVertexArray(0);
			
			// draw pieces
			pieceShader.UseShader();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, piecesTextureId);
			int i = (rank - 1) * 8 + file - 1;

			glm::mat4 pieceModel = glm::mat4(1.f);
			pieceModel = glm::translate(pieceModel, glm::vec3((aspect / 13.7f) * rank + 0.305f, (file * 2 - 1) / 16.f, 0.f));
			pieceModel = glm::scale(pieceModel, glm::vec3(size, size, 1.f));
			glUniformMatrix4fv(pieceModelLocation, 1, GL_FALSE, glm::value_ptr(pieceModel));

			if (pieces[i] == nullptr)
			{
				printf("pieces[i] is null!");
				continue;
			}
			pieces[i]->DrawPiece();
		}
	}
	
	glBindVertexArray(0);
}
