#ifndef VIDEOBUFFER_H
#define VIDEOBUFFER_H

#include <string>
#include <cstdint>
#include "ARGBColour.h"
#include "Pixel.h"
#include "common/Point.h"

namespace gfx
{

class VideoBuffer
{
	//pixel video map, usually an unsigned int in 0x00RRGGBB format
	pixel* vid;
	int width;
	int height;

	Point clipRect1, clipRect2;

public:
	VideoBuffer(int width, int height);
	~VideoBuffer();

	void Clear();
	void ClearRect(int x, int y, int w, int h);
	void CopyBufferInto(pixel* vidPaste, int vidWidth, int vidHeight, int x, int y);
	void CopyBufferIntoWithPartialAlpha(pixel* vidPaste, int vidWidth, int vidHeight, int x, int y, int alpha);
	void CopyBufferFrom(pixel* vidFrom, int vidWidth, int vidHeight, int w, int h);

	void DrawPixel(int x, int y, int r, int g, int b, int a);
	void DrawPixel(int x, int y, ARGBColour color);
	void XorPixel(int x, int y);
	void DrawLine(int x1, int y1, int x2, int y2, int r, int g, int b, int a);
	void DrawLine(int x1, int y1, int x2, int y2, ARGBColour color);
	void XorLine(int x1, int y1, int x2, int y2);
	void DrawRect(int x, int y, int w, int h, int r, int g, int b, int a);
	void DrawRect(int x, int y, int w, int h, ARGBColour color);
	void FillRect(int x, int y, int w, int h, int r, int g, int b, int a);
	void FillRect(int x, int y, int w, int h, ARGBColour color);
	void DrawCircle(int x, int y, int rx, int ry, int r, int g, int b, int a);
	void FillCircle(int x, int y, int rx, int ry, int r, int g, int b, int a);

	int DrawChar(int x, int y, uint32_t c, int r, int g, int b, int a, bool modifiedColor = false);
	int DrawChar(int x, int y, uint32_t c, ARGBColour color, bool modifiedColor = false);
	int DrawString(int x, int y, const std::string &s, int r, int g, int b, int a);
	int DrawString(int x, int y, const std::string &s, ARGBColour color);
	static signed char CharSize(unsigned char c);
	static int CodepointSize(uint32_t codepoint);
	static Point TextSize(std::string s);

	void DrawImage(pixel *image, int x, int y, int w, int h, int a=255);

	pixel* GetVid() const { return vid; }
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	Point GetSize() const { return Point(width, height); }

	void SwapClipRect(Point & upperLeft, Point & bottomRight);
	void ResetClipRect();
};

}

#endif
