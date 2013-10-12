#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "osd.h"


int write_yuv4xx_file(unsigned char *buf, int buf_len, char *filename)
{
	FILE *fd = fopen(filename, "wb+");

	if(!fd) {
		printf("open output.yuv failed!\n");
		return -1;
	}

	fwrite(buf, 1, buf_len, fd);
	printf("write--- %d ---btye!\n", buf_len);
	fclose(fd);
	return 0;
}


void yuv422Y16_2_yuv422yuyv(unsigned char *yuyv, unsigned char *y16, int width, int height)
{
	int x = 0, i = 0;
	unsigned char *Ubuf = y16 + width * height;
	unsigned char *Vbuf = y16 + width * height + width * height / 2;

	for(x = 0; x < height * width; x++, i += 2) {
		yuyv[i] = y16[x];

	}

	i = 1;

	for(x = 0; x < height * width / 2; x++, i += 4) {
		yuyv[i] = Ubuf[x];
	}

	i = 3;

	for(x = 0; x < height * width / 2; x++, i += 4) {
		yuyv[i] = Vbuf[x];
	}
}




//Convert from RGB to YUV 4:2:2
int rgb_2_yuv422(unsigned char *out, unsigned char **row_pointers, int height, int width)
{

	int i, j;
	int R, G, B;
	int Y, U, V;
	unsigned char *test_buff = (unsigned char *)malloc(width * height * 3);

	unsigned char *YBuffer = (unsigned char *) malloc(width * height);
	unsigned char *VBuffer = (unsigned char *) malloc(width * height / 2);
	unsigned char *UBuffer = (unsigned char *) malloc(width * height / 2);

	//Define some working variables and arrays
	//Define buffers for filtering (width+2 to allow filtering edges)
	unsigned char *ULine = (unsigned char *) malloc(width + 2) + 1;
	unsigned char *VLine = (unsigned char *) malloc(width + 2) + 1;
	ULine[-1] = ULine[width] = 128;
	VLine[-1] = VLine[width] = 128;

	//fread(RGBBuffer, sizeof(char), LOGO_HEIGHT*LOGO_WIDTH*3, ifp);
	for(i = 0; i < height; i++) {
		memcpy((test_buff + i * width * 3), row_pointers[i], width * 3);
	}

	for(i = 0; i < height; ++i) {
		int RGBIndex = 3 * width * i;
		int YIndex   = width * i;
		int UVIndex  = width * i / 2;

		for(j = 0; j < width; ++j) {
			R = test_buff[RGBIndex++];
			G = test_buff[RGBIndex++];
			B = test_buff[RGBIndex++];

			//Convert RGB to YUV
			Y = (unsigned char)(0.299 * R + 0.587 * G +  0.114 * B);
			V = (unsigned char)(0.5 * R -  0.4187 * G - 0.0813 * B + 128);
			U = (unsigned char)(-0.1687 * R -  0.3313 * G +  0.5 * B + 128);

			//Clip Y ready for output & copy UV ready for filtering

			YBuffer[YIndex++] = (unsigned char)((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
			VLine[j] = V;
			ULine[j] = U;
		}

		for(j = 0; j < width; j += 2) {
			//Filter line
			V = ((VLine[j - 1] + 2 * VLine[j] + VLine[j + 1] + 2) >> 2);
			U = ((ULine[j - 1] + 2 * ULine[j] + ULine[j + 1] + 2) >> 2);

			//Clip and copy UV to output buffer
			VBuffer[UVIndex] = (unsigned char)((V < 0) ? 0 : ((V > 255) ? 255 : V));
			UBuffer[UVIndex++] = (unsigned char)((U < 0) ? 0 : ((U > 255) ? 255 : U));

		}
	}

	memcpy(out, YBuffer, width * height);
	memcpy(out + width * height, UBuffer, width * height / 2);
	memcpy(out + width * height + width * height / 2, VBuffer, width * height / 2);

	free(YBuffer);
	free(VBuffer);
	free(UBuffer);

	free(ULine - 1);
	free(VLine - 1);
	free(test_buff);

	return 3;
}

void yuv422_2_yuv420(unsigned char *yuv420, unsigned char *yuv422, int width, int height)
{

	int UV_width_422 = width >> 1; //422色度信号U和V宽度
	int UV_height_422 = height; //422色度信号U和V高度

	int UV_width_420 = width >> 1; //420色度信号U和V宽度
	//	int UV_height_420 = height >> 1; //420色度信号U和V高度

	int Ylen = height * width;

	unsigned char *p_Y420 = yuv420 ;
	unsigned char *p_U420 = p_Y420 + Ylen ;
	unsigned char *p_V420 = p_U420 + Ylen / 4 ;

	unsigned char *p_Y422 = yuv422 ;
	unsigned char *p_U422 = p_Y422 + Ylen ;
	unsigned char *p_V422 = p_U422 + Ylen / 2 ;

	//亮度信号Y复制
	memcpy(yuv420, yuv422, Ylen);

	//U、V取平均值
	int k = 0;
	int i, j;
	unsigned char u1, u2 , v1, v2 ;

	for(i = 0 ; i < UV_height_422 ; i += 2) {
		for(j = 0 ; j < UV_width_422 ; j++) {
			u1 = *(p_U422 + i * UV_width_422 + j) ;
			u2 = *(p_U422 + (i + 1) * UV_width_422 + j) ;

			v1 = *(p_V422 + i * UV_width_422 + j) ;
			v2 = *(p_V422 + (i + 1) * UV_width_422 + j) ;

			//色度信号U复制
			*(p_U420 + k * UV_width_420 + j) = ((u1 + u2) >> 1) ;

			//色度信号V复制
			*(p_V420 + k * UV_width_420 + j) = ((v1 + v2) >> 1) ;

		}

		k++ ;
	}
}


void yuv420p_2_yuv420sp(unsigned char *yuv420sp, unsigned char *yuv420p, int width, int height)
{
	int i = 0, u = 0, v = 0, x = 0;
	unsigned char *uv_interlace;
	int Ylen = height * width;

	memcpy(yuv420sp, yuv420p, Ylen);

	uv_interlace = yuv420sp + width * height;

#if 1

	for(i = 0; i < width * height / 2; i++) {
		if(x == 0) {
			x = 1;
			uv_interlace[i] = (yuv420p[width * height + u]);
			u++;
		} else {
			x = 0;
			uv_interlace[i] = (yuv420p[width * height + width * height / 4 + v]);
			v++;
		}
	}

#endif
}



void write_yuv420sp_file(unsigned char *yuv420sp, int width, int height)
{
	FILE *fd = fopen("yuv420sp.yuv", "wb+");

	if(!fd) {
		printf("fopen failed!\n");
		return ;
	}

	int i = 0;
	int pixels = width * height;
	unsigned char *ptr = yuv420sp;

	fwrite(ptr, 1, pixels, fd);

	ptr = ptr + pixels;

	// Write Cb
	for(i = 0; i < (pixels / 2); i += 2) {
		fwrite((ptr + i), 1, sizeof(unsigned char), fd);
	}

	// Write Cr
	for(i = 1; i < (pixels / 2); i += 2) {
		fwrite((ptr + i), 1, sizeof(unsigned char), fd);
	}

	fclose(fd);
}



