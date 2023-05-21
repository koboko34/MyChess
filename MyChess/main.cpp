#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "Window.h"
#include "Shader.h"

int WIDTH = 1366; int HEIGHT = 768;

int main()
{
	Window window;
	window.Initialise(WIDTH, HEIGHT, false);

	GLfloat triangleVertices[] = {
		-0.5f, -0.5f, 0.f,
		 0.5f, -0.5f, 0.f,
		  0.f,  0.5f, 0.f
	};

	GLuint triangleIndices[] = {
		0, 1, 2
	};

	GLfloat squareVertices[] = {
		 0.5f,  0.5f, 0.f,
		 0.5f, -0.5f, 0.f,
		-0.5f, -0.5f, 0.f,
		-0.5f,  0.5f, 0.f
	};

	GLuint squareIndices[] = {
		0, 1, 3,
		1, 2, 3
	};

	// to move to Mesh class
	GLuint VAO, EBO, VBO;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
	glEnableVertexAttribArray(0);

	Shader shader;
	shader.CompileShaders("shaders/shader.vert", "shaders/shader.frag");
	shader.UseShader();	
	
	while (!glfwWindowShouldClose(window.GetWindow()))
	{
		glfwPollEvents();
		window.HandleInputs();

		glClearColor(0.5f, 0.3f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// GL_LINE for wireframe, GL_FILL for standard
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window.GetWindow());
	}

	glfwTerminate();

	return 0;
}