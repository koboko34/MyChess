#pragma once

#include <GLFW/glfw3.h>

class Window
{
public:	
	Window();
	~Window();

	bool Initialise(int width, int height, bool bFullscreen);

	GLFWwindow* GetWindow() const { return mainWindow; }

	double x, y;
	bool bIsPressed;

private:
	GLFWwindow* mainWindow;

	int windowWidth, windowHeight;
	int bufferWidth, bufferHeight;

	bool keys[1024];

	void CreateCallbacks();
	static void HandleKeys(GLFWwindow* window, int key, int code, int action, int mode);
	static void HandleMouseClicks(GLFWwindow* window, int button, int action, int mods);
};

