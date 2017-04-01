// taken from https://cggame.googlecode.com/svn-history/r14/trunk/tgalib.cpp
// which seems to be based on free-to-use http://www.lighthouse3d.com/opengl/terrain/index.php3?tgasource

#ifndef __glt__tgalib__
#define __glt__tgalib__

#define TGALIB

typedef struct {
	int status;
	unsigned char type, pixelDepth;
	short int width, height;
	unsigned char *imageData;
}tgaInfo;

enum TGA_TYPE {
	TGA_ERROR_FILE_OPEN,
	TGA_ERROR_READING_FILE,
	TGA_ERROR_INDEXED_COLOR,
	TGA_ERROR_COMPRESSED_FILE,
	TGA_ERROR_MEMORY,
	TGA_OK };

// load the image header fields. We only keep those that matter!
void tgaLoadHeader(FILE *file, tgaInfo *info);

// this is the function to call when we want to load
// an image
tgaInfo * tgaLoad(const char *filename);

// converts RGB to greyscale
void tgaRGBtoGreyscale(tgaInfo *info);

// takes a screen shot and saves it to a TGA image
int tgaGrabScreenSeries(char *filename, int xmin,int ymin, int xmax, int ymax);

// saves an array of pixels as a TGA image
int tgaSave(	char 		*filename,
            short int	width,
            short int	height,
            unsigned char	pixelDepth,
            unsigned char	*imageData);

// saves a series of files with names "filenameX.tga"
int tgaSaveSeries(char		*filename,
                  short int		width,
                  short int		height,
                  unsigned char	pixelDepth,
                  unsigned char	*imageData);

// releases the memory used for the image
void tgaDestroy(tgaInfo *info);

#endif /* defined(__glt__tgalib__) */
