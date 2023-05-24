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

int WIDTH = 1366; int HEIGHT = 768;

int main()
{
	Window window;
	window.Initialise(WIDTH, HEIGHT, false);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Board board;
	board.Init(WIDTH, HEIGHT);
	
	while (!glfwWindowShouldClose(window.GetWindow()))
	{
		glfwPollEvents();
		window.HandleInputs();

		glClearColor(0.5f, 0.3f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		board.DrawBoard();

		glfwSwapBuffers(window.GetWindow());
	}

	glfwTerminate();

	return 0;
}