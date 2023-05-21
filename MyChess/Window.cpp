#include <stdio.h>

#include <GL/glew.h>

#include "Window.h"

Window::Window()
{
	mainWindow = nullptr;
}

Window::~Window()
{
	if (mainWindow != nullptr)
		glfwDestroyWindow(mainWindow);

	glfwTerminate();
}

bool Window::Initialise(int width, int height, bool bFullscreen)
{
	if (!glfwInit())
	{
		printf("GLFW initialisation failed!\n");
		return false;
	}

	windowWidth = width; windowHeight = height;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);


	mainWindow = glfwCreateWindow(width, height, "My Chess", bFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);

	if (!mainWindow)
	{
		printf("Window creation failed!\n");
		glfwTerminate();
		return false;
	}

	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);
	glfwMakeContextCurrent(mainWindow);

	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		printf("GLEW initialisation failed!\n");
		glfwDestroyWindow(mainWindow);
		glfwTerminate();
		return false;
	}

	// glEnable(GL_DEPTH_TEST);

	glViewport(0, 0, bufferWidth, bufferHeight);

	glfwSetWindowUserPointer(mainWindow, this);

	return true;
}

void Window::HandleInputs()
{
	if (glfwGetKey(mainWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(mainWindow, true);
	}
}
