#include <GLFW/glfw3.h>

#include <GL/glew.h>

#include "Window.h"

int WIDTH = 1366; int HEIGHT = 768;

int main()
{
	Window window;
	window.Initialise(WIDTH, HEIGHT, false);

	float vertices[] = {
		-0.5f, -0.5f, 0.f,
		 0.5f, -0.5f, 0.f,
		  0.f,  0.5f, 0.f
	};

	GLuint VBO;
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	while (!glfwWindowShouldClose(window.GetWindow()))
	{
		glfwPollEvents();
		window.HandleInputs();

		glClearColor(0.5f, 0.3f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwSwapBuffers(window.GetWindow());
	}

	glfwTerminate();

	return 0;
}