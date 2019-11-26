#pragma once

#include "glad/glad.h"
#include <cstdlib>
#include <vector>

GLbyte* readTGABits(const char* szFileName, GLint* iWidth, GLint* iHeight, GLint* iComponents, GLenum* eFormat, GLbyte* pData = nullptr);

// Define targa header. This is only used locally.
#pragma pack(1)
typedef struct
{
	GLbyte	identsize;              // Size of ID field that follows header (0)
	GLbyte	colorMapType;           // 0 = None, 1 = paletted
	GLbyte	imageType;              // 0 = none, 1 = indexed, 2 = rgb, 3 = grey, +8=rle
	unsigned short	colorMapStart;          // First colour map entry
	unsigned short	colorMapLength;         // Number of colors
	unsigned char 	colorMapBits;   // bits per palette entry
	unsigned short	xstart;                 // image x origin
	unsigned short	ystart;                 // image y origin
	unsigned short	width;                  // width in pixels
	unsigned short	height;                 // height in pixels
	GLbyte	bits;                   // bits per pixel (8 16, 24, 32)
	GLbyte	descriptor;             // image descriptor
} TGAHEADER;
#pragma pack(8)

class Texture
{
private:
	GLuint textureIndex;
public:
	Texture();
	Texture(const char* filename);
	~Texture();
	void load(const char* fileName);
	void loadFromPixels(std::vector<GLubyte> pixels, int width, int height);	//4 elements in array define a pixel in rgba format
	void use();
};