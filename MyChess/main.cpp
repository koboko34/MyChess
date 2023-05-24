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

		glClearColor(0.5f, 0.3f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (window.bIsPressed)
		{
			// need depth test enabled?
			pickingTexture.EnableWriting();
			board.PickingPass();
			pickingTexture.DisableWriting();

			pixel = pickingTexture.ReadPixel(window.mouseX, HEIGHT - window.mouseY - 1);
			clickedObjectId = pixel.objectId - 1;
			printf("clickedObjectId = %i\n", clickedObjectId);

			window.bIsPressed = false;
		}

		board.DrawBoard(clickedObjectId);

		glfwSwapBuffers(window.GetWindow());
	}

	glfwTerminate();

	return 0;
}