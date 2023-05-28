#include <stdio.h>

#include <GL/glew.h>

#include "Window.h"

Window::Window()
{
	mainWindow = nullptr;
	mouseX = 0.f;
	mouseY = 0.f;
	bIsPressed = false;
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

	CreateCallbacks();
	glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	glEnable(GL_DEPTH_TEST);

	glViewport(0, 0, bufferWidth, bufferHeight);

	glfwSetWindowUserPointer(mainWindow, this);

	return true;
}

void Window::CreateCallbacks()
{
	glfwSetKeyCallback(mainWindow, HandleKeys);
	glfwSetMouseButtonCallback(mainWindow, HandleMouseClicks);
}

void Window::HandleKeys(GLFWwindow* window, int key, int code, int action, int mode)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->keys[key] = true;
		}
		if (action == GLFW_RELEASE)
		{
			theWindow->keys[key] = false;
		}
	}
}

void Window::HandleMouseClicks(GLFWwindow* window, int button, int action, int mods)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &theWindow->mouseX, &theWindow->mouseY);
		theWindow->bIsPressed = true;
		// printf("x: %f, y: %f\n", theWindow->mouseX, theWindow->mouseY);
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		theWindow->bIsPressed = false;
	}
}
