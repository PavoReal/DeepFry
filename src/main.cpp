#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#define UNUSED(a) (void) a

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef unsigned int uint;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

int
main(int argc, char **argv)
{
	if (argc <= 1)
	{
		puts("I need a filename to deep fry.");

		return 0;
	}

	for (int photoIndex = 1; photoIndex <= argc; ++photoIndex)
	{
		char *fileName = argv[photoIndex];

		int width;
		int height;
		int bytePerPixel;

		unsigned char *data = stbi_load(fileName, &width, &height, &bytePerPixel, 0);

		printf("%s:\n\tWidth: %d, Height: %d\n\tBytes per pixel: %d\n", fileName, width, height, bytePerPixel);
		puts("\n");

		char charBuffer[512];
		sprintf(charBuffer, "deepfry_%s", fileName);

		u8 *newData = (u8*) malloc(width * height * bytePerPixel);

		assert(newData);

		u8 *srcIndex  = data;
		u8 *destIndex = newData;

		double avgLum   = 69;
		double lumCount = 1;
		for (u64 i = 0; i < (width * height); ++i)
		{
			u8 red   = *srcIndex++;
			u8 green = *srcIndex++;
			u8 blue  = *srcIndex++;

			double lum = (red * 0.2126) * (green * 0.7152) * (blue * 0.0722);
			avgLum   += lum;
			lumCount += 1;

			double a        = avgLum / lumCount;
			double contrast = (a - lum) / a;

			*destIndex++ = (u8) (((red + blue) / 2) * contrast); // Red
			*destIndex++ = (u8) ((green + red)      * contrast);   // Green
			*destIndex++ = (u8) ((blue + green)     * contrast);  // Blue
		}

		stbi_write_png(charBuffer, width, height, bytePerPixel, newData, width * bytePerPixel);
		stbi_image_free(data);
		free(newData);
	}

	return 0;
}
