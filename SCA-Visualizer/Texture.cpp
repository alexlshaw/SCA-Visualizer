#include "Texture.h"

Texture::Texture() 
{
	textureIndex = -1;
}

Texture::Texture(const char* fileName)
{
	load(fileName);
}

Texture::~Texture()
{
	glDeleteTextures(1, &textureIndex);
}

void Texture::load(const char* fileName)
{
	GLbyte* pBits;
	int nWidth, nHeight, nComponents;
	GLenum eFormat;

	pBits = readTGABits(fileName, &nWidth, &nHeight, &nComponents, &eFormat);
	if (pBits == nullptr)
	{
		return;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &textureIndex);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);
	free(pBits);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
}

void Texture::use()
{
	glBindTexture(GL_TEXTURE_2D, textureIndex);
}

void Texture::loadFromPixels(std::vector<GLubyte> pixels, int width, int height)
{
	glGenTextures(1, &textureIndex);
	glBindTexture(GL_TEXTURE_2D, textureIndex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)&pixels[0]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}


////////////////////////////////////////////////////////////////////
// Allocate memory and load targa bits. Returns pointer to new buffer,
// height, and width of texture, and the OpenGL format of data.
// Call free() on buffer when finished!
// This only works on pretty vanilla targas... 8, 24, or 32 bit color
// only, no palettes, no RLE encoding.
// This function also takes an optional final parameter to preallocated 
// storage for loading in the image data.
GLbyte* readTGABits(const char* szFileName, GLint* iWidth, GLint* iHeight, GLint* iComponents, GLenum* eFormat, GLbyte* pData)
{
	FILE* pFile;			// File pointer
	TGAHEADER tgaHeader;		// TGA file header
	unsigned long lImageSize;		// Size in bytes of image
	short sDepth;			// Pixel depth;
	GLbyte* pBits = nullptr;          // Pointer to bits

	// Default/Failed values
	*iWidth = 0;
	*iHeight = 0;
	*eFormat = GL_RGB;
	*iComponents = GL_RGB;

	// Attempt to open the file
	fopen_s(&pFile, szFileName, "rb");
	if (pFile == nullptr)
	{
		return nullptr;
	}
	// Read in header (binary)
	fread(&tgaHeader, 18/* sizeof(TGAHEADER)*/, 1, pFile);

	// Get width, height, and depth of texture
	*iWidth = tgaHeader.width;
	*iHeight = tgaHeader.height;
	sDepth = tgaHeader.bits / 8;

	// Put some validity checks here. Very simply, I only understand
	// or care about 8, 24, or 32 bit targa's.
	if (tgaHeader.bits != 8 && tgaHeader.bits != 24 && tgaHeader.bits != 32)
	{
		return nullptr;
	}

	// Calculate size of image buffer
	lImageSize = tgaHeader.width * tgaHeader.height * sDepth;

	// Allocate memory and check for success
	if (pData == nullptr)
	{
		pBits = (GLbyte*)malloc(lImageSize * sizeof(GLbyte));
	}
	else
	{
		pBits = pData;
	}
	// Read in the bits
	// Check for read error. This should catch RLE or other 
	// weird formats that I don't want to recognize
	if (pBits != nullptr && fread(pBits, lImageSize, 1, pFile) != 1)
	{
		if (pBits != nullptr)
		{
			free(pBits);
		}
		return nullptr;
	}

	// Set OpenGL format expected
	switch (sDepth)
	{
	case 4:
		*eFormat = GL_BGRA;
		*iComponents = GL_BGRA;// GL_RGBA;
		break;
	case 1:
		*eFormat = GL_RED;		//Changed from GL_LUMINANCE due to removal in gl 3.1
		*iComponents = GL_RED;
		break;
	default:        // RGB
		break;
	}

	// Done with File
	fclose(pFile);

	// Return pointer to image data
	return pBits;
}