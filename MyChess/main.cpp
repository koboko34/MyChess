#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "CommonValues.h"

#include "Board.h"
#include "Window.h"
#include "Shader.h"
#include "PickingTexture.h"

int WIDTH = 1366; int HEIGHT = 768;

int main()
{
	Window window;
	window.Initialise(WIDTH, HEIGHT, false);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Board board;
	board.Init(WIDTH, HEIGHT);

	PickingTexture pickingTexture;
	pickingTexture.Init(WIDTH, HEIGHT);
	PickingTexture::PixelInfo pixel;
	int clickedObjectId = -1;

	while (!glfwWindowShouldClose(window.GetWindow()))
	{
		glfwPollEvents();

		if (window.bIsPressed)
		{
			
			// need depth test enabled?
			pickingTexture.EnableWriting();

			glClearColor(0.0f, 0.0f, 0.0f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (!board.IsGameOver())
			{
				board.PickingPass();

				pixel = pickingTexture.ReadPixel(window.mouseX, HEIGHT - window.mouseY - 1);
				pickingTexture.DisableWriting();

				if (board.IsChoosingPromotion())
				{
					switch (pixel.objectId - 1)
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
				else
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
			}

			window.bIsPressed = false;
		}

		board.DrawBoard(clickedObjectId);

		glfwSwapBuffers(window.GetWindow());
	}

	glfwTerminate();

	return 0;
}