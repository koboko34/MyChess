#pragma once

#include <GL/glew.h>

class PickingTexture
{
public:
	PickingTexture();
	~PickingTexture();

	void Init(int width, int height);

	void EnableWriting();
	void DisableWriting();

	struct PixelInfo
	{
		GLuint objectId = 0;
		GLuint drawId = 0;
		GLuint primId = 0;
	};

	// PixelInfo ReadPixel(unsigned int x, unsigned int y);

private:
	GLuint FBO;
	GLuint pickingTextureId, depthTextureId;

};

