#include <stdio.h>

#include <iostream>

#include "PickingTexture.h"

PickingTexture::PickingTexture()
{
	FBO = 0;
	depthTextureId, pickingTextureId = 0;
}

PickingTexture::~PickingTexture()
{
}

void PickingTexture::Init(int width, int height)
{
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glActiveTexture(GL_TEXTURE1);

	glGenTextures(1, &pickingTextureId);
	glBindTexture(GL_TEXTURE_2D, pickingTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, width, height, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTextureId, 0);

	glActiveTexture(GL_TEXTURE2);

	glGenTextures(1, &depthTextureId);
	glBindTexture(GL_TEXTURE_2D, depthTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextureId, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Framebuffer error: %s\n", status);
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickingTexture::EnableWriting()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
}

void PickingTexture::DisableWriting()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

PickingTexture::PixelInfo PickingTexture::ReadPixel(unsigned int x, unsigned int y)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	PixelInfo pixel;
	glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixel);

	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	return pixel;
}
