#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "Window.h"
#include "Shader.h"

int WIDTH = 1366; int HEIGHT = 768;

int main()
{
	Window window;
	window.Initialise(WIDTH, HEIGHT, false);

	GLfloat triangleVertices[] = {
		-0.5f, -0.5f, 0.f,		1.f, 0.f, 0.f,
		 0.5f, -0.5f, 0.f,		0.f, 1.f, 0.f,
		  0.f,  0.5f, 0.f,		0.f, 0.f, 1.f
	};

	GLuint triangleIndices[] = {
		0, 1, 2
	};

	float offsetPixels = 1920.f / 6.f;
	float pieceTextureOffset = offsetPixels / 1920.f;
	
	GLfloat squareVertices[] = {
		 0.5f,  0.5f, 0.f,		1.f, 0.f, 0.f,		pieceTextureOffset, 0.5f,
		 0.5f, -0.5f, 0.f,		0.f, 1.f, 0.f,		pieceTextureOffset, 0.f,
		-0.5f, -0.5f, 0.f,		0.f, 0.f, 1.f,		0.f, 0.f,
		-0.5f,  0.5f, 0.f,		1.f, 1.f, 0.f,		0.f, 0.5f
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

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLuint pieceTex;
	glGenTextures(1, &pieceTex);
	glBindTexture(GL_TEXTURE_2D, pieceTex);

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

	Shader shader;
	shader.CompileShaders("shaders/shader.vert", "shaders/shader.frag");
	shader.UseShader();

	float timeValue;
	float greenValue;

	GLint colourLocation = glGetUniformLocation(shader.GetShaderId(), "ourColor");
	GLint modelLocation = glGetUniformLocation(shader.GetShaderId(), "model");

	GLint viewLocation = glGetUniformLocation(shader.GetShaderId(), "view");
	glm::mat4 view = glm::mat4(1.f);
	view = glm::translate(view, glm::vec3(0.f, 0.f, -3.f));
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

	GLint projectionLocation = glGetUniformLocation(shader.GetShaderId(), "projection");
	float aspect = (float)WIDTH / (float)HEIGHT;
	glm::mat4 projection = glm::ortho(0.f, aspect, 0.f, 1.f, 0.f, 1000.f);
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
	
	while (!glfwWindowShouldClose(window.GetWindow()))
	{
		glfwPollEvents();
		window.HandleInputs();

		glClearColor(0.5f, 0.3f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		timeValue = glfwGetTime();
		greenValue = (sin(timeValue) / 2.f) + 0.5f;
		glUniform4f(colourLocation, 0.f, greenValue, 0.f, 1.f);

		glm::mat4 model = glm::mat4(1.f);
		model = glm::translate(model, glm::vec3(aspect / 2.f, 0.5f, 1.f));
		model = glm::scale(model, glm::vec3(0.3f, 0.3f, 1.f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

		// GL_LINE for wireframe, GL_FILL for standard
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window.GetWindow());
	}

	glfwTerminate();

	return 0;
}