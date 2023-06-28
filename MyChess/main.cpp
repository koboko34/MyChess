#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include <irrKlang/irrKlang.h>

#include "CommonValues.h"

#include "board.h"
#include "Window.h"
#include "Shader.h"
#include "PickingTexture.h"

int WIDTH = 1366; int HEIGHT = 768;

void PromotePiece(unsigned int objectId, Board* board)
{
	switch (objectId - 1)
	{
	case QUEEN:
		board->Promote(QUEEN);
		break;
	case ROOK:
		board->Promote(ROOK);
		break;
	case BISHOP:
		board->Promote(BISHOP);
		break;
	case KNIGHT:
		board->Promote(KNIGHT);
	default:
		break;
	}
}

int main()
{
	Window window;
	window.Initialise(WIDTH, HEIGHT, false);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Board* board = nullptr;

	irrklang::ISoundEngine* soundEngine = irrklang::createIrrKlangDevice();
	soundEngine->setSoundVolume(1.f);

	PickingTexture pickingTexture;
	pickingTexture.Init(WIDTH, HEIGHT);
	PickingTexture::PixelInfo pixel;
	int clickedObjectId = -1;

	while (!glfwWindowShouldClose(window.GetWindow()))
	{
		glfwPollEvents();
		
		if (board == nullptr)
		{
			board = new Board();
			board->Init(WIDTH, HEIGHT, window.GetWindow(), soundEngine);
		}

		if (window.bIsPressed)
		{
			pickingTexture.EnableWriting();

			glClearColor(0.0f, 0.0f, 0.0f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			board->PickingPass();

			pixel = pickingTexture.ReadPixel(window.mouseX, HEIGHT - window.mouseY - 1);
			pickingTexture.DisableWriting();

			// pixel.Print();

			if (board->IsGameOver())
			{
				if (pixel.objectId - 1 == 0)
				{
					delete board;
					board = nullptr;
					soundEngine->play2D("sounds/notify.mp3");
					continue;
				}
			}
			else if (board->IsChoosingPromotion())
			{
				PromotePiece(pixel.objectId, board);
			}
			else
			{
				if (clickedObjectId == -1 || !board->PieceExists(clickedObjectId))
				{
					clickedObjectId = pixel.objectId - 1;
				}
				else if (clickedObjectId == pixel.objectId - 1)
				{
					clickedObjectId = -1;
				}
				else
				{
					board->MovePiece(clickedObjectId, pixel.objectId - 1);
					clickedObjectId = -1;
				}
			}
			
			window.bIsPressed = false;
		}

		board->DrawBoard(clickedObjectId);

		glfwSwapBuffers(window.GetWindow());
	}

	delete board;
	glfwTerminate();

	return 0;
}