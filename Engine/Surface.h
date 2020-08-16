#pragma once

#include "Colors.h"
#include <string>

class Surface
{
public:
	Surface(const std::string& filename);
	Surface(int width, int height);
	Surface(const Surface& rhs);
	~Surface();
	Surface& operator=(const Surface& rhs);
	void PutPixel(int x, int y, Color c);
	Color GetPixel(int x, int y)const;
	int width;
	//void center();
	float angle = 0.0f;
	float shearX = 0.0f;
	float shearY = 0.0f;
	float sizeX = 1.0f;
	float sizeY = 1.0f;
	float TransY = 0.0f;
	float TransX = 0.0f;
	int height;
private:
	Color* pPixels = nullptr;
};