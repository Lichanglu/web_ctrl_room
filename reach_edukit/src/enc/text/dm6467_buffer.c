#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ti_media_std.h"

#include "dm6467_struct.h"


Int ColorSpace_getBpp(ColorSpace_Type colorSpace)
{
	Int bpp;

	switch(colorSpace) {
		case ColorSpace_RGB888:
		case ColorSpace_YUV444P:
			bpp = 24;
			break;

		case ColorSpace_UYVY:
			bpp = 16;
			break;

		case ColorSpace_GRAY:
		case ColorSpace_YUV422P:
		case ColorSpace_YUV420P:
		case ColorSpace_YUV420PSEMI:
		case ColorSpace_YUV422PSEMI:
			bpp = 8;
			break;

		case ColorSpace_2BIT:
			bpp = 2;
			break;

		default:
			printf("Unknown color space format (%d)\n", colorSpace);
			return -1;
	}

	return bpp;
}


Buffer_Handle Buffer_create(Int32 size, BufferGfx_Attrs *attrs)
{
	Buffer_Handle tmp = NULL;
	char *p = NULL;
	tmp = (Buffer_strcut *)malloc(sizeof(Buffer_strcut));
	p = (char *)malloc(size * sizeof(char));

	if(tmp == NULL || p == NULL) {
		printf("ERROR!!!!!!!!malloc buffer create error!!!!!!!!!!!!!!!\n");
		return NULL;
	}

	memset(tmp, 0, sizeof(Buffer_strcut));
	tmp->bufsize = size;
	tmp->colortype = attrs->colorSpace;
	tmp->dim = attrs->dim;

	tmp->buf = p;

	printf("buff create=%p\n", tmp);
	return tmp;

}
char *Buffer_getUserPtr(Buffer_Handle hBuf)
{
	if(hBuf != NULL) {
		return hBuf->buf;
	}

	return NULL;
}
int Buffer_getSize(Buffer_Handle hBuf)
{
	if(hBuf != NULL) {
		return hBuf->bufsize;
	}

	return -1;
}
int BufferGfx_getColorSpace(Buffer_Handle hBuf)
{
	if(hBuf != NULL) {
		return hBuf->colortype;
	}

	return -1;
}

int BufferGfx_getDimensions(Buffer_Handle hBuf, BufferGfx_Dimensions *dim)
{
	if(hBuf != NULL) {
		memcpy(dim, &(hBuf->dim), sizeof(BufferGfx_Dimensions));
		return 0;
	}

	return -1;
}

int BufferGfx_setDimensions(Buffer_Handle hBuf, BufferGfx_Dimensions *dim)
{
	if(hBuf != NULL) {
		memcpy(&(hBuf->dim), dim, sizeof(BufferGfx_Dimensions));
		return 0;
	}

	return -1;
}

int write2_yuv_422(Buffer_Handle hCapBuf, int width, int height)
{

	FILE *fp;
	unsigned char *pY, *pU, *pV;
	unsigned int i;
	static int g_time = 0;
	char buf[100];

	int linlen = 640;
	int j = 0;
	sprintf(buf, "/%dx%d_%d.yuv", width, height, g_time++);
	printf("buf = %s,width=%d,height=%d\n", buf, width, height);
	//    if(time != 50)
	//       return 1;
	fp = fopen(buf, "wb");
	pY = (unsigned char *)Buffer_getUserPtr(hCapBuf);
	pU = (unsigned char *)Buffer_getUserPtr(hCapBuf) + 640 * 64;
	pV = (unsigned char *)Buffer_getUserPtr(hCapBuf) + 640 * 64 + 1;

	j = 0;

	for(i = 0; i < height; i++) {
		if(j < width) {
			fputc(*pY, fp);
			pY++;
			j++;
		}

		j = 0;
		pY = pY + (linlen - width);
	}

	for(i = 0; i < height * width / 2; i++) {
		fputc(*pU, fp);
		pU++;
		pU++;
	}

	for(i = 0; i < height * width / 2; i++) {
		fputc(*pV, fp);
		pV++;
		pV++;
	}

	fclose(fp);
	printf("write over  height=%d   width=%d   time = %d\n", height, width, g_time);
	return 1;
}


int write2_yuv_4xx(char *buff, int height, int width)
{

	FILE *fp;
	unsigned char *pY, *pU, *pV;
	unsigned int i;
	static int g_time = 0;
	char buf[100];

	sprintf(buf, "/%dx%d_%d.yuv", width, height, g_time++);
	//    if(time != 50)
	//       return 1;
	fp = fopen(buf, "wb");
	pY = (unsigned char *)buff;
	pU = (unsigned char *)buff + width * height;
	pV = (unsigned char *)buff + width * height + 1;

	for(i = 0; i < height * width; i++) {
		fputc(*pY, fp);
		pY++;
	}

	for(i = 0; i < height * width / 2 ; i++) {
		fputc(*pU, fp);
		pU++;
		pU++;
	}

	for(i = 0; i < height * width / 2; i++) {
		fputc(*pV, fp);
		pV++;
		pV++;
	}

	fclose(fp);
	printf("write over  height=%d   width=%d   time = %d\n", height, width, g_time);
	return 1;
}



