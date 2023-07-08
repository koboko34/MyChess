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

const int WIDTH = 1366; const int HEIGHT = 768;

Window window;
Board board;
PickingTexture pickingTexture;
PickingTexture::PixelInfo pixel;

void PickPixel(double mouseX, double mouseY)
{
	pickingTexture.EnableWriting();

	glClearColor(0.0f, 0.0f, 0.0f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	board.PickingPass();

	pixel = pickingTexture.ReadPixel(mouseX, HEIGHT - mouseY - 1);
	pickingTexture.DisableWriting();
}

void PromotePiece(unsigned int objectId)
{
	switch (objectId - 1)
	{
	case QUEEN:
		board.Promote(QUEEN);
		break;
	case ROOK:
		board.Promote(ROOK);
		break;
	case BISHOP:
		board.Promote(BISHOP);
		break;
	case KNIGHT:
		board.Promote(KNIGHT);
	default:
		break;
	}
}

void HandleBoardClick(int& clickedObjectId)
{
	if (clickedObjectId == -1 || !board.PieceExists(clickedObjectId))
	{
		clickedObjectId = pixel.objectId - 1;
	}
	else if (clickedObjectId == pixel.objectId - 1)
	{
		clickedObjectId = -1;
	}
	else
	{
		board.MovePiece(clickedObjectId, pixel.objectId - 1);
		clickedObjectId = -1;
	}
}

int main()
{
	window.Initialise(WIDTH, HEIGHT, false);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	irrklang::ISoundEngine* soundEngine = irrklang::createIrrKlangDevice();
	soundEngine->setSoundVolume(1.f);

	board.Init(WIDTH, HEIGHT, window.GetWindow(), soundEngine);

	pickingTexture.Init(WIDTH, HEIGHT);
	int clickedObjectId = -1;

	while (!glfwWindowShouldClose(window.GetWindow()))
	{
		glfwPollEvents();

		if (window.bIsPressed)
		{
			PickPixel(window.mouseX, window.mouseY);
			// pixel.Print();

			if ((int)pixel.objectId >= 100)
			{
				board.ButtonCallback(pixel.objectId);
			}
			else if (board.IsChoosingPromotion())
			{
				PromotePiece(pixel.objectId);
			}
			else if (!board.IsGameOver())
			{
				HandleBoardClick(clickedObjectId);
			}
			
			window.bIsPressed = false;
		}

		board.RenderScene(clickedObjectId);

		glfwSwapBuffers(window.GetWindow());
	}

	glfwTerminate();

	return 0;
}