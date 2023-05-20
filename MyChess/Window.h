#pragma once

#include <GLFW/glfw3.h>

class Window
{
public:	
	Window();
	~Window();

	bool Initialise(int width, int height, bool bFullscreen);

	void HandleInputs();

	GLFWwindow* GetWindow() const { return mainWindow; }

private:
	GLFWwindow* mainWindow;

	int windowWidth, windowHeight;
	int bufferWidth, bufferHeight;

};

