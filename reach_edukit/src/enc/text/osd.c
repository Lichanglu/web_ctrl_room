
#include "reach.h"
#include "osd.h"
#include "ti_media_std.h"
#include "dm6467_struct.h"

#include "Convert.h"
#include "middle_control.h"
#include "Font.h"
#include "screen.h"
#include "sd_demo_osd.h"
#include "input_to_channel.h"
#include "capture.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


typedef struct Png_Object {
	Int        w;
	Int        h;
	Int8      *rp;
	Int8      *gp;
	Int8      *bp;
	Int        lineLength;
} Png_Object;



extern int read_logo_info(int id,  LogoInfo *logo);
extern int read_text_info(int id, TextInfo *text);


typedef struct Png_Object *Png_Handle;


static TextOsd_Info g_text_info[INPUT_TEXT_MAX_NUM];
static int g_display_time[INPUT_TEXT_MAX_NUM];
static LogoInfo gLogoinfo[INPUT_TEXT_MAX_NUM];

//static Png_Handle ghPng[INPUT_TEXT_MAX_NUM];
static pthread_mutex_t osd_mutex;
static TextOsd_Info	g_timeosd_info;
static  Font_Handle		g_chnfont 			= NULL;

static int copy_text_global(Font_Handle hFont, Char *txt, Int fontHeight, Int x, Int y,
                            Buffer_Handle hBuf);

int get_logo_info(int input, LogoInfo *logo)
{
	if(logo == NULL || (input > INPUT_TEXT_MAX_NUM || input < 0)) {
		printf("ERROR:newLogoInfo = %p,input=%d\n", logo, input);
		return -1;
	}

	// check
	pthread_mutex_lock(&osd_mutex);
	memcpy(logo, &(gLogoinfo[input]),  sizeof(LogoInfo));
	pthread_mutex_unlock(&osd_mutex);
	return 0;
}

int set_logo_info(int input, LogoInfo *newLogoInfo)
{
	if(newLogoInfo == NULL || (input > INPUT_TEXT_MAX_NUM || input < 0)) {
		printf("ERROR:newLogoInfo = %p\n", newLogoInfo);
		return -1;
	}

	// check
	pthread_mutex_lock(&osd_mutex);
	memcpy(&(gLogoinfo[input]), newLogoInfo, sizeof(LogoInfo));
	pthread_mutex_unlock(&osd_mutex);
	return 0;
}


int get_time_display(int input)
{
	if((input > INPUT_TEXT_MAX_NUM || input < 0)) {
		printf("ERROR:input = %d\n", input);
		return -1;
	}

	return g_display_time[input];
}

void set_time_display(int input, int enable)
{
	if((input > INPUT_TEXT_MAX_NUM || input < 0)) {
		printf("ERROR:input = %d\n", input);
		return;
	}

	g_display_time[input] = enable;
	return;
}


int get_text_info(int input, TextInfo *text)
{
	if(text == NULL || (input > INPUT_TEXT_MAX_NUM || input < 0)) {
		printf("ERROR:newtextInfo = %p,input=%d\n", text, input);
		return -1;
	}

	pthread_mutex_lock(&osd_mutex);
	memcpy(text, &(g_text_info[input].text),  sizeof(TextInfo));
	pthread_mutex_unlock(&osd_mutex);
	return 0;
}

int set_text_info(int input, TextInfo *newtextInfo)
{
	if(newtextInfo == NULL || (input > INPUT_TEXT_MAX_NUM || input < 0)) {
		printf("ERROR:newtextInfo = %p,input=%d\n", newtextInfo, input);
		return -1;
	}

	pthread_mutex_lock(&osd_mutex);
	memcpy(&(g_text_info[input].text), newtextInfo, sizeof(TextInfo));
	pthread_mutex_unlock(&osd_mutex);
	return 0;
}


int check_CHN(char *text, int size)
{
	int i;

	for(i = 0; i < size; i++)
		if(text[i] > 127) {
			return 1;
		}

	return 0;
}



/*¥˙¬Î◊™ªª:¥”“ª÷÷±‡¬Î◊™Œ™¡Ì“ª÷÷±‡¬Î*/
int code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;
	cd = iconv_open(to_charset, from_charset);

	if((iconv_t) - 1 == cd) {
		printf("open the char set failed,errno = %d,strerror(errno) = %s \n", errno, strerror(errno));
		return -1;
	}

	memset(outbuf, 0, outlen);

	if(iconv(cd, pin, (size_t *)&inlen, pout, (size_t *)&outlen) == (size_t) - 1) {
		printf("conver char set failed,errno = %d,strerror(errno) = %s \n", errno, strerror(errno));
		return -1;
	}

	iconv_close(cd);
	return 0;
}

/*µ√µΩœµÕ≥ ±º‰*/
static void get_sys_time(DATE_TIME_INFO *dtinfo)
{
	long ts;
	struct tm *ptm = NULL;

	ts = time(NULL);
	ptm = localtime((const time_t *)&ts);
	dtinfo->year = ptm->tm_year + 1900;
	dtinfo->month = ptm->tm_mon + 1;
	dtinfo->mday = ptm->tm_mday;
	dtinfo->hours = ptm->tm_hour;
	dtinfo->min = ptm->tm_min;
	dtinfo->sec = ptm->tm_sec;
}
void yuvuv420_2_yuyv422(unsigned char *out, unsigned char *buf, int width, int height)
{
	int i = 0, j = 0;
	int u = 0, v = 0;
	unsigned char *y_buf = buf;
	unsigned char *u_buf = buf + width * height;

	for(i = 0, j = 0; i < width * height * 2; j++) {
		out[i] = y_buf[j];
		i++;
		j++;
		out[i] = u_buf[u];
		i++;
		out[i] = y_buf[j];
		i++;
		out[i] = u_buf[v];
		i++;

	}
}

#if 1
static void add_time_osd(void)
{
	char  text[128] = {0};
	DATE_TIME_INFO info;
	TextInfo  *time = &(g_timeosd_info.text);
	Buffer_Handle hbuf = g_timeosd_info.hBuf;
	get_sys_time(&info);
	sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d", info.year, info.month, info.mday, info.hours, info.min, info.sec);

	Screen_clear(hbuf, 0, 0, TIME_BITMAP_WIDTH, TIME_BITMAP_HEIGHT);
	copy_text_global(g_chnfont, text, OSD_FONT_SIZE, 12, 36, hbuf);

	time->xpos = 10;
	time->ypos = 10;
	time->alpha = 0x80;
	BufferGfx_Dimensions dim;
	BufferGfx_getDimensions(hbuf, &dim);

	time->width = dim.width;
	time->height = dim.height;
	yuvuv420_2_yuyv422(g_timeosd_info.yuyv_buf, (unsigned char *)hbuf->buf, dim.width , dim.height);

	hbuf->bufsize = dim.width * dim.height * 2;
	memcpy(hbuf->buf, g_timeosd_info.yuyv_buf, hbuf->bufsize);
	SD_subtitle_osdUpdate(0, WINDOW_TIME_OSD, (unsigned char *)hbuf->buf, hbuf->bufsize, time);

	return ;
}
#endif
#if 0
static void add_time_osd0(void)
{
	int i = 0;
	int time_diplay = 0;
	char  text[128] = {0};
	unsigned char *yuyv_buf ;

	DATE_TIME_INFO info;
	get_sys_time(&info);
	sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d", info.year, info.month, info.mday, info.hours, info.min, info.sec);

	Screen_clear(g_timeosd_info.hBuf, 0, 0, TIME_BITMAP_WIDTH, TIME_BITMAP_HEIGHT);
	copy_text_global(g_chnfont, text, OSD_FONT_SIZE, 40, 40, g_timeosd_info.hBuf);

	if(1) {
		BufferGfx_Dimensions dim;
		BufferGfx_getDimensions(hbuf, &dim);
		yuyv_buf = (unsigned char *)malloc(dim.width * dim.height * 2);

		if(yuyv_buf == NULL) {
			printf("yuyv_buf is NULL\n");
			return ;
		}

		time->width = dim.width;
		time->height = dim.height;
		yuvuv420_2_yuyv422(yuyv_buf, hbuf->buf, dim.width , dim.height);

		hbuf->bufsize = dim.width * dim.height * 2;
		memcpy(hbuf->buf, yuyv_buf, hbuf->bufsize);
		SD_subtitle_osdUpdate(input, WINDOW_TIME_OSD, hbuf->buf, hbuf->bufsize, time);

		if(yuyv_buf) {
			free(yuyv_buf);
		}

		yuyv_buf = NULL;
		return ;
	}

	g_timeosd_info.text.xpos = 10;
	g_timeosd_info.text.ypos = 10;


	g_timeosd_info.text.alpha = g_text_info[i].text.alpha;
	SD_subtitle_osdUpdate(0, WINDOW_TIME_OSD, g_timeosd_info.hBuf->buf, g_timeosd_info.hBuf->bufsize, &(g_timeosd_info.text));

}

return ;
}
#endif

/*ÃÌº” ±º‰◊÷ƒªœﬂ≥Ã*/
static void time_osd_thread(void *pParam)
{
	int i = 0;
	int need_show = 0;

	while(1) {
		need_show = 0;


		for(i = 0; i < 1; i++) {
			if(g_display_time[i] != 0) {
				need_show = 1;
				break;
			}
		}


		if(need_show == 1) {
			add_time_osd();
		}

		sleep(1);
	}
}


void show_text_info(int input, const char *text_buff, int len, TextInfo *text)
{

	BufferGfx_Dimensions dim;
	Buffer_Handle hbuf = g_text_info[input].hBuf;
	printf("input=%d\n", input);

	Screen_clear(hbuf, 0, 0, TEXT_BITMAP_WIDTH, TEXT_BITMAP_HEIGHT);
	copy_text_global(g_chnfont, (char *)text_buff, OSD_FONT_SIZE, 2, 32, hbuf);
	BufferGfx_getDimensions(hbuf, &dim);

	unsigned char *yuyv_buf = (unsigned char *)malloc(dim.width * dim.height * 2);

	if(yuyv_buf == NULL) {
		printf("yuyv_buf is NULL\n");
		return ;
	}

	yuvuv420_2_yuyv422(yuyv_buf, (unsigned char *)hbuf->buf, dim.width, dim.height);

	if(len != 0) {
		text->width  =  16 * len;

		if(text->width > TEXT_BITMAP_WIDTH) {
			text->width  = TEXT_BITMAP_WIDTH;
		}
	}

	text->height = dim.height;
	hbuf->bufsize = dim.width * dim.height * 2;
	memcpy(hbuf->buf, yuyv_buf, hbuf->bufsize);
	SD_subtitle_osdUpdate(input, WINDOW_TEXT_OSD, (unsigned char *)hbuf->buf, hbuf->bufsize , text);

	if(yuyv_buf) {
		free(yuyv_buf);
	}

	yuyv_buf = NULL;



}

void hide_osd_view(int input, int type)
{
	LogoInfo logo ;
	TextInfo text;
	int ret = 0;
	ret = get_text_info(input, &text);

	if(ret < 0) {
		printf("get text failed!\n");
		return ;
	}

	ret = get_logo_info(input, &logo);

	if(ret < 0) {
		printf("get logo failed!\n");
		return ;
	}

	if(type == WINDOW_TEXT_OSD) {
		printf("--HIDE   text bufffer!\n");
		SD_subtitle_osdUpdate(input, WINDOW_TEXT_OSD, NULL, 0, &(text));
	} else if(type == WINDOW_TIME_OSD) {
		printf("--HIDE   Time bufffer!\n");
		SD_subtitle_osdUpdate(input, WINDOW_TIME_OSD, NULL, 0, &(g_timeosd_info.text));
	} else if(type == WINDOW_LOGO_OSD) {
		printf("--HIDE   Logo bufffer!\n");
		logo.alpha = 0;
		SD_logo_osdUpdate(input, WINDOW_LOGO_OSD, NULL, 0, &(logo));
	}
}


static int copy_text_global(Font_Handle hFont, Char *txt, Int fontHeight, Int x, Int y,
                            Buffer_Handle hBuf)
{
	BufferGfx_Dimensions dim;
	FT_Error error;
	Int index;
	Int width, height;
	Int i, j, p, q, x_max, y_max, sp;
	UInt32 srcValue, dstValue;
	UInt8 *src;
	FT_Vector pen;
	FT_Face face = (FT_Face) hFont->face;
	Convert_Mode cMode;
	Int inc, start;
	UInt8 *ptr = NULL, *rowPtr;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);

	UInt32 TestCharSet = 0;
	UInt32 nTestFF = 0;
	UInt32 nTestDD = 0;
	wchar_t *wcTxt = 0;

	//int *wcTxt = 0;
	memcpy((Char *)&TestCharSet, txt, 4);
	nTestFF = TestCharSet & 0xFFFF0000;
	nTestDD = TestCharSet & 0x0000FFFF;

	if(nTestFF == 0 && nTestDD > 0) {
		memcpy((Char *)&TestCharSet, txt + 4, 4);
		nTestFF = TestCharSet & 0xFFFF0000;
		nTestDD = TestCharSet & 0x0000FFFF;

		if(nTestFF == 0 && nTestDD > 0) {
			wcTxt = (wchar_t *)txt + 1;
		}
	}

	BufferGfx_getDimensions(hBuf, &dim);
	//face->glyph->bitmap.num_grays =100;
	FT_Set_Char_Size(face,
	                 fontHeight << 6,
	                 fontHeight << 6,
	                 100,
	                 100);

	Convert_init();

	//	printf("copy_text_global() =%d, not supporting image color format=%p=%d\n", sizeof(wchar_t),hBuf, BufferGfx_getColorSpace(hBuf));



	if(BufferGfx_getColorSpace(hBuf) == ColorSpace_2BIT) {
		cMode = Convert_Mode_RGB888_2BIT;
		start = 65;
		inc =  80;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
		cMode = Convert_Mode_RGB888_YUV422SEMI;
		start = 80;
		inc = 80;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
		cMode = Convert_Mode_RGB888_RGB565;
		start = 0;
		inc = 0;
	} else {
		//printf("copy_text_global() not supporting image color format=%d\n", BufferGfx_getColorSpace(hBuf));
		return SW_EFAIL;
	}

	pen.x = x << 6;
	pen.y = (dim.height - y + start) << 6;
	//	pen.x = 0;


	while(wcTxt ? *wcTxt : *txt) {
		FT_Set_Transform(face, NULL, &pen);

		index = FT_Get_Char_Index(face, wcTxt ? *wcTxt : *txt);

		error = FT_Load_Glyph(face, index, 0);

		if(error) {
			printf("Failed to load glyph for character %c\n", wcTxt ? *wcTxt : *txt);
			break;
		}

		if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

			if(error) {
				printf("Failed to render glyph for character %c\n", wcTxt ? *wcTxt : *txt);
				break;
			}
		}

		width = face->glyph->bitmap.width;
		height = face->glyph->bitmap.rows;

		x = face->glyph->bitmap_left;
		y = dim.height + inc - face->glyph->bitmap_top;
		x_max = x + width;
		y_max = y + height;


		src = face->glyph->bitmap.buffer;

		//printf("ptr=%p=width=%d,height=%d\n", ptr, width, height);
		ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;

		for(j = y, q = 0; j < y_max; j++, q++) {
			rowPtr = ptr + Screen_getInc(colorSpace, x);

			for(i = x, p = 0; i < x_max; i++, p++) {

				if(i >= dim.width || j >= dim.height) {
					continue;
				}

				sp = q * width + p;
				//if(src[sp]!=0)
				//src[sp]=~src[sp];
				//if(src[sp] ==255)
				//src[sp]=src[sp] &30;
				//printf("src[sp]=%d\n",src[sp]);
				srcValue = src[sp] << 16 | src[sp] << 8 | src[sp];
				dstValue = Convert_execute(cMode, srcValue, i & 1);

				Screen_setPixelPtr(rowPtr, bufSize, i, dstValue,
				                   TRUE, colorSpace);
				rowPtr += Screen_getStep(colorSpace, i);
			}

			ptr += dim.lineLength;
		}

		//	wcTxt ? wcTxt++ : txt++;
		// txt++;

		//wcTxt ? wcTxt++ : txt++;
		if(wcTxt) {
			wcTxt++;
		} else {
			txt++;
		}

		// txt++;
		pen.x += face->glyph->advance.x;
		pen.y += face->glyph->advance.y;

	}

	return SW_EOK;
}

#if 0
static Int text_string_len(Font_Handle hFont, Char *txt, Int fontHeight,
                           Int *stringWidth, Int *stringHeight)
{
	Int index;
	Int width = 0;
	Int height = 0;
	FT_Vector pen;
	FT_Face face = (FT_Face) hFont->face;
	UInt32 TestCharSet = 0;
	UInt32 nTestFF = 0;
	UInt32 nTestDD = 0;
	wchar_t *wcTxt = 0;
	//int *wcTxt = 0;
	FT_Set_Char_Size(face,
	                 fontHeight << 6,
	                 fontHeight << 6,
	                 100,
	                 100);

	pen.x = 0;
	pen.y = 0;
	memcpy((Char *)&TestCharSet, txt, 4);
	nTestFF = TestCharSet & 0xFFFF0000;
	nTestDD = TestCharSet & 0x0000FFFF;

	if(nTestFF == 0 && nTestDD > 0) {
		memcpy((Char *)&TestCharSet, txt + 4, 4);
		nTestFF = TestCharSet & 0xFFFF0000;
		nTestDD = TestCharSet & 0x0000FFFF;

		if(nTestFF == 0 && nTestDD > 0) {
			wcTxt = (wchar_t *)txt;
		}
	}

	while(wcTxt ? *wcTxt : *txt) {
		FT_Set_Transform(face, NULL, &pen);
		index = FT_Get_Char_Index(face, wcTxt ? *wcTxt : *txt);

		if(FT_Load_Glyph(face, index, 0)) {
			printf("Failed to load glyph for character %c\n", wcTxt ? *wcTxt : *txt);
			break;
		}

		if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			if(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
				printf("Failed to render glyph for character %c\n", wcTxt ? *wcTxt : *txt);
				break;
			}
		}

		width += face->glyph->bitmap.width + 1;

		if(face->glyph->bitmap.rows > height) {
			height = face->glyph->bitmap.rows;
		}

		pen.x += face->glyph->advance.x;
		pen.y += face->glyph->advance.y;
		(wcTxt != NULL) ? wcTxt++ : txt++;
	}

	*stringWidth = width;
	*stringHeight = height;

	return SW_EOK;
}
#endif
//============

static int Png_checkHead(unsigned char *head, int length)
{
	//int ret = 0;
	int i;
	unsigned char fixhead[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

	if(length < 33) {
		return 1;
	}

	for(i = 0; i < 8; i++) {

		if(fixhead[i] != head[i]) {
			return 1;
		}
	}

	if(head[25] == 3) {
		return 1;
	}

	return 0;

}
int rgb_2_yuyv422(unsigned char *out, unsigned char **row_pointers, int height, int width)
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

	for(i = 0, j = 0, U = 0, V = 0; i < width * height * 2 && j < width * height ; j++) {

		out[i] = YBuffer[j];
		i++;

		if(j % 2 == 0) {
			out[i] = UBuffer[U];
			U++;
		} else {
			out[i] = VBuffer[V];
			V++;
		}

		i++;
	}

	free(YBuffer);
	free(VBuffer);
	free(UBuffer);

	free(ULine - 1);
	free(VLine - 1);
	free(test_buff);

	return 3;
}



static int Png_create(int input, char *fileName, int show, LogoInfo *logo)
{
	//	Png_Handle hPng;
	unsigned char *row_pointers[720]; // TODO large enough?
	unsigned char head[33]; //pngÕ∑
	//    Int r, g, b;
	Int  y;//Int x;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	Int bit_depth, color_type, interlace_type;
	FILE *infile;
	int len = 0;
	int ret = 0;

	unsigned char *yuv422_buf ;
	unsigned char  *yuv420_buf;
	unsigned char  *yuv420sp_buf;
	/*
		hPng = calloc(1, sizeof(Png_Object));

		if(hPng == NULL) {
			printf("Failed to allocate space for Png Object\n");
			return NULL;
		}
	*/
	printf("fileName=%s\n", fileName);

	Convert_init();
	infile = fopen(fileName, "rb");

	if(infile == NULL) {
		printf("Failed to open image file [%s]\n", fileName);
		//	free(hPng);
		return -1;
	}

	if(fread(head, 1, sizeof(head), infile) < sizeof(head)) {
		printf("Failed fread png head=%d\n", sizeof(head));
		fclose(infile);
		//free(hPng);
		return -1;
	}

	fseek(infile, 0L, SEEK_SET);

	if(Png_checkHead(head, sizeof(head))) {
		printf("Png head error\n");
		fclose(infile);
		//free(hPng);
		return -1;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if(png_ptr == NULL) {
		printf("Failed to create read struct\n");
		fclose(infile);
		//free(hPng);
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);

	if(info_ptr == NULL) {
		printf("Failed to create info struct\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		fclose(infile);
		//free(hPng);
		return -1;
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		printf("Failed png_jmpbuf\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		fclose(infile);
		//free(hPng);
		return -1;
	}

	png_init_io(png_ptr, infile);

	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	             &interlace_type, int_p_NULL, int_p_NULL);

	png_set_strip_16(png_ptr);
	png_set_strip_alpha(png_ptr);
	png_set_packing(png_ptr);
	png_set_packswap(png_ptr);

	if(color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png_ptr);
	}

	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_gray_1_2_4_to_8(png_ptr);
	}

	for(y = 0; y < height; y++) {
		row_pointers[y] = png_malloc(png_ptr,
		                             png_get_rowbytes(png_ptr, info_ptr));
	}

	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, info_ptr);

	len = width * height;

	if(width > PNG_MAX_WIDTH || height > PNG_MAX_HEIGHT) {
		printf("png Too big! width = %d, height = %d\n", (int)width, (int)height);
		ret = -4;
		goto DELETE_PNG;
	}

	if(show == 1) {
		printf("higth=%d,width=%d\n", (int)width, (int)height);

		yuv422_buf = (unsigned char *)malloc(len * 2);

		if(yuv422_buf == NULL) {
			printf("Error =%d\n", len * 2);
			return -1;
		}

		printf("yuv422_buf..\n");

		if(1) {
			rgb_2_yuyv422(yuv422_buf, row_pointers, height, width);

			int png_buf_len = width * height * 2;

			logo->height = height;
			logo->width = width;

			ret = SD_logo_osdUpdate(input, 2, yuv422_buf, png_buf_len, logo);

			if(yuv422_buf) {
				free(yuv422_buf);
			}

			yuv422_buf = NULL;
			goto DELETE_PNG;

		} else {
			rgb_2_yuv422(yuv422_buf, row_pointers, height , width);
		}

		//write_yuv4xx_file(yuv422_buf, height* width*2,"422.yuv");

		printf("yuv420_buf malloc---\n");
		yuv420_buf = (unsigned char *)malloc(len * 2);

		if(!yuv420_buf) {
			printf("yuv420_buf:malloc failed!\n");
			return -1;
		}

		printf("yuv422_2_yuv420----\n");
		yuv422_2_yuv420(yuv420_buf, yuv422_buf, width, height);
		//	write_yuv4xx_file(yuv420_buf, height* width*3/2,"420.yuv");

		printf("yuv420sp_buf malloc \n");
		yuv420sp_buf = (unsigned char *)malloc(len * 2);

		if(!yuv420sp_buf) {
			printf("yuv420sp_buf:malloc failed!\n");
			return -1;
		}

		printf("yuv420p_2_yuv420sp----\n");
		yuv420p_2_yuv420sp(yuv420sp_buf, yuv420_buf, width, height);
		//		write_yuv420sp_file(yuv420sp_buf, width, height);

		int png_buf_len = len * 3 / 2;

		logo->height = height;
		logo->width = width;

		ret = SD_logo_osdUpdate(input, 2, yuv420sp_buf, png_buf_len, logo);

		printf("....free yuv422_buf=%p buff!\n", yuv422_buf);

		if(yuv422_buf) {
			free(yuv422_buf);
		}

		printf("....free yuv420_buf=%p buff!\n", yuv420_buf);

		if(yuv420_buf) {
			free(yuv420_buf);
		}

		printf("....free yuv420sp_buf=%p buff!\n", yuv420sp_buf);

		if(yuv420sp_buf) {
			free(yuv420sp_buf);
		}

		printf("....free yuv buff is NULL!\n");
		yuv422_buf = NULL;
		yuv420_buf = NULL;
		yuv420sp_buf = NULL;
	}

DELETE_PNG:

	for(y = 0; y < height; y++) {
		png_free(png_ptr, row_pointers[y]);
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	fclose(infile);
	unlink(fileName);

	printf("Png create success\n");
	return ret;
}



#if 0
static Void Png_show(Png_Handle hPng, Int x, Int y, Buffer_Handle hBuf)
{
	Int   minWidth;
	Int   minHeight;
	Int32 srcValue, dstValue;
	Int   i, j, pos;
	Convert_Mode cMode;
	UInt8 *ptr, *rowPtr;
	BufferGfx_Dimensions dim;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);

	BufferGfx_getDimensions(hBuf, &dim);
	ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;
	ptr += Screen_getInc(colorSpace, x);

	if(BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
		cMode = Convert_Mode_RGB888_YUV422SEMI;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
		cMode = Convert_Mode_RGB888_RGB565;
	} else {
		printf("Png_show() not supporting image color format\n");
		return;
	}

	minWidth = dim.width < hPng->w ? dim.width : hPng->w;
	minHeight = dim.height < hPng->h ? dim.height : hPng->h;

	for(j = 0; j < minHeight; j++) {
		rowPtr = ptr;

		for(i = 0; i < minWidth; i++) {
			pos = j * hPng->w + i;
			srcValue = ((hPng->rp[pos] & 0xff) << 16) |
			           ((hPng->gp[pos] & 0xff) << 8) |
			           (hPng->bp[pos] & 0xff);
			dstValue = Convert_execute(cMode, srcValue, (i + x) & 1);
			Screen_setPixelPtr(rowPtr, bufSize, i + x, dstValue,
			                   FALSE, colorSpace);
			rowPtr += Screen_getStep(colorSpace, i + x);
		}

		ptr += dim.lineLength;
	}
}

static Void Png_showFilt(Png_Handle hPng, Int x, Int y, Buffer_Handle hBuf, int Filt)
{
	Int   minWidth;
	Int   minHeight;
	Int32 srcValue, dstValue;
	Int   i, j, pos;
	Convert_Mode cMode;
	UInt8 *ptr, *rowPtr;
	BufferGfx_Dimensions dim;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);

	BufferGfx_getDimensions(hBuf, &dim);
	ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;
	ptr += Screen_getInc(colorSpace, x);

	if(BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
		cMode = Convert_Mode_RGB888_YUV422SEMI;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
		cMode = Convert_Mode_RGB888_RGB565;
	} else {
		printf("Png_show() not supporting image color format\n");
		return;
	}

	minWidth = dim.width < hPng->w ? dim.width : hPng->w;
	minHeight = dim.height < hPng->h ? dim.height : hPng->h;

	for(j = 0; j < minHeight; j++) {
		rowPtr = ptr;

		for(i = 0; i < minWidth; i++) {
			pos = j * hPng->w + i;
			srcValue = ((hPng->rp[pos] & 0xff) << 16) |
			           ((hPng->gp[pos] & 0xff) << 8) |
			           (hPng->bp[pos] & 0xff);

			if(srcValue == Filt) {
				rowPtr += Screen_getStep(colorSpace, i + x);
				continue;
			}

			dstValue = Convert_execute(cMode, srcValue, (i + x) & 1);
			Screen_setPixelPtr(rowPtr, bufSize, i + x, dstValue,
			                   FALSE, colorSpace);
			rowPtr += Screen_getStep(colorSpace, i + x);
		}

		ptr += dim.lineLength;
	}
}

static void Png_getWH(Png_Handle hPng, int *w, int *h)
{
	*w = hPng->w;
	*h = hPng->h;
	return;
}

static Void Png_showTransparency(Png_Handle hPng, Int x, Int y, Buffer_Handle hBuf, int isThrough, int Filt, int bl)
{
	Int   minWidth;
	Int   minHeight;
	Int32 srcValue, dstValue;
	Int   i, j, pos;
	Convert_Mode cMode;
	UInt8 *ptr, *rowPtr;
	UInt8 by, bcbcr, fy, fcbcr, ry, rcbcr;
	by = bcbcr = fy = fcbcr = ry = rcbcr = 0;
	float alpha = (float)bl / (float)100;
	BufferGfx_Dimensions dim;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);

	BufferGfx_getDimensions(hBuf, &dim);
	ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;
	ptr += Screen_getInc(colorSpace, x);

	if(BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
		cMode = Convert_Mode_RGB888_YUV422SEMI;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
		cMode = Convert_Mode_RGB888_RGB565;
	} else {
		printf("Png_show() not supporting image color format\n");
		return;
	}

	minWidth = dim.width < hPng->w ? dim.width : hPng->w;
	minHeight = dim.height < hPng->h ? dim.height : hPng->h;

	for(j = 0; j < minHeight; j++) {
		rowPtr = ptr;

		for(i = 0; i < minWidth; i++) {
			pos = j * hPng->w + i;
			srcValue = ((hPng->rp[pos] & 0xff) << 16) |
			           ((hPng->gp[pos] & 0xff) << 8) |
			           (hPng->bp[pos] & 0xff);

			if((srcValue == Filt) && (isThrough == 1)) {
				rowPtr += Screen_getStep(colorSpace, i + x);
				continue;
			}

			dstValue = Convert_execute(cMode, srcValue, (i + x) & 1);
			fy = (dstValue >> 8) & 0xff;
			fcbcr = dstValue & 0xff;
			Screen_getPixelPtr(rowPtr, bufSize, i + x, &by, &bcbcr, FALSE, colorSpace);
			ry = (fy - 16) * (1 - alpha) + (by - 16) * (alpha) + 16;
			rcbcr = (fcbcr - 128) * (1 - alpha) + (bcbcr - 128) * (alpha) + 128; //¬ªÏªè
			dstValue = ry;
			dstValue <<= 8;
			dstValue |= rcbcr;
			Screen_setPixelPtr(rowPtr, bufSize, i + x, dstValue, FALSE, colorSpace);
			rowPtr += Screen_getStep(colorSpace, i + x);
		}

		ptr += dim.lineLength;
	}
}



static Int Png_delete(Png_Handle hPng)
{
	if(hPng) {
		if(hPng->rp) {
			free(hPng->rp);
		}

		if(hPng->gp) {
			free(hPng->gp);
		}

		if(hPng->bp) {
			free(hPng->bp);
		}

		free(hPng);
	}

	return SW_EOK;
}

#endif

int get_text_size(char *Data)
{
	int n_Len = strlen(Data);
	unsigned char *lpData = (unsigned char *)Data;
	int i = 0;
	int Sum = 0;

	for(i = 0; /**pStr != '\0'*/ i < n_Len ;) {
		if(*lpData <= 127) {
			i++;
			lpData++;
		} else {
			i += 2;
			lpData += 2;
		}

		Sum++;
	}

	return i;
}


static int check_text_pos(int input, TextInfo *text, int text_width)
{
	int new_xpos = 0, new_ypos = 0;
	int width = 0 , height = 0;

	width = 1920;
	height = 1080;
	printf("width*height:%dX%d,text_width=%d\n", width, height, text_width);

	if(text->xpos + text_width > width) {
		new_xpos = width - text_width;
		printf(">width,new_xpos=%d\n", new_xpos);
	} else {
		new_xpos = text->xpos;
		printf("<width,new_xpos=%d\n", new_xpos);
	}

	text->xpos = new_xpos;

	if(height == 540) {
		height = height * 2;
	}

	if(text->ypos + TEXT_BITMAP_HEIGHT + 1 > height) {
		new_ypos = height - TEXT_BITMAP_HEIGHT;
		printf(">height,new_ypos=%d\n", new_ypos);
	} else {
		new_ypos =  text->ypos;
		printf("<width,new_ypos=%d\n", new_ypos);
	}

	text->ypos = new_ypos;

	return 0;
}


//error code -4 invaild file -3 size to big
int check_logo_png(int input, char *filename)
{
	int error_code = 0;
	//	int w = 0;
	//	int h = 0;
	LogoInfo logo;
	int ret = 0;
	ret = Png_create(input, filename, 0, &logo);

	if(ret < 0) {
		printf("Png_create failed!\n");
		error_code = -4;
	}

#if 0

	if(temp) {
		Png_getWH(&temp, &w, &h);

		if(h > PNG_MAX_WIDTH || w > PNG_MAX_HEIGHT) {
			printf("update PNG too big!\n");
			error_code = -3;
		}
	}

	if(temp != NULL) {
		Png_delete(&temp);
	}

#endif
	return error_code;
}

void print_xx(char *buf, int len)
{
	int i = 0;

	printf(">>>>>>>>>>>>");

	for(i = 0; i < len; i++) {
		if(i % 4 == 0) {
			printf("\n");
		}

		printf("[%x]", buf[i]);
	}

	printf("\n>>>>>>>>>>>>\n");
}

/*ÃÌº”◊÷ƒª*/
int add_text_info(int input, TextInfo *info)
{
	int charlen = strlen(info->msgtext);
	//	int templen = 0;
	int time_displaye = 0;
	int display_flag = 0;

	if(charlen != 0 && info->enable != 0) {
		display_flag = 1;
	} else  {
		display_flag = 0;
	}

	printf("x:%d, y:%d, enable:%d, showtime:%d, text=%s\n",
	       info->xpos, info->ypos, info->enable, info->showtime, info->msgtext);

	if(display_flag != 0) {
		int size_len = get_text_size(info->msgtext);

		if(check_CHN(info->msgtext, charlen)) {
			char temp[512] = {0};
			char temp_2[512] = {0};

			printf("check_CHN is CHN!\n");
			printf("text len  = %d\n", size_len);

			code_convert("gb2312", "UTF-32LE", info->msgtext, charlen, temp, charlen * 4);

			if(temp[0] != 0xFF && temp[1] != 0xFE) {
				temp_2[0] = 0xFF;
				temp_2[1] = 0xFE;
				temp_2[2] = 0x00;
				temp_2[3] = 0x00;

				memcpy(temp_2 + 4, temp, 512 - 4);
			} else {
				memcpy(temp_2, temp, 512);
			}

			//print_xx(temp_2, 160);

			if(info->postype == BOTTOM_RIGHT || info->postype == TOP_RIGHT
			   || info->postype == BOTTOM_LEFT) {
				check_text_pos(input, info, size_len * 16);
			}

			//			set_text_info( input, info);
			show_text_info(input, temp_2, size_len, info); //charlen);

		} else {
			if(info->postype == BOTTOM_RIGHT || info->postype == TOP_RIGHT) {
				check_text_pos(input, info, charlen * 16);
			}

			//			set_text_info( input, info);
			show_text_info(input, info->msgtext, charlen, info);
			printf("text len = %d\n", charlen);
		}

	} else {
		hide_osd_view(input, WINDOW_TEXT_OSD);
	}


	if(info->showtime) {//&& text->enable != 0) {
		printf("display the time \n");
		set_time_display(input, 1);
	} else 	{
		printf("Cancel Display The Time!!!\n");
		set_time_display(input, 0);
		//		hide_osd_view(id, IS_TIME_OSD);
	}

	time_displaye = get_time_display(input);

	if(time_displaye == 0) {
		hide_osd_view(input, WINDOW_TIME_OSD);
	}

	printf(" return !\n");

	return 0;
}




int  add_logo_osd(int input, LogoInfo *logo)
{
	int w, h;
	int ret = 0;
	char filename[256] = {0};
	//	struct Png_Object hPng;
	pthread_mutex_lock(&osd_mutex);

	w = h = 0;
	/*
		if(hPng != NULL) {
			Png_delete(hPng);
		}
	*/
	sprintf(filename, "%s", logo->filename);

	ret  = Png_create(input, filename, logo->enable, logo);
#if 0

	if(ret < 0) {
		Png_getWH(&hPng, &w, &h);

		if(h > PNG_MAX_WIDTH || w > PNG_MAX_HEIGHT) {
			Png_delete(&hPng);
		}
	}

	printf("ghPng = %p,w=%d,h=%d\n", hPng, w, h);

	if(hPng == NULL) {
		logo->errcode = 1;
		logo->enable = 0;
		//	logo->show = 0;
	}

	if(hPng) {
		logo->width = w;
		logo->height = h;
	}

	Png_delete(hPng);
	hPng = NULL;
#endif
	pthread_mutex_unlock(&osd_mutex);
	return ret;
}


int update_osd_view(int VP)
{
	int input = SIGNAL_INPUT_1, high = HIGH_STREAM;
	channel_get_input_info(VP, &input, &high);
	TextInfo text;
	LogoInfo logo ;
	int ret = 0;

	printf("update input=%d\n", input);

	ret = get_text_info(input, &text);

	if(ret < 0) {
		printf("get text failed!\n");
		return -1;
	}

	ret = get_logo_info(input, &logo);

	if(ret < 0) {
		printf("get logo failed!\n");
		return -1;
	}

	printf("input=%d\n", input);

	if(text.enable) {
		add_text_info(input, &text);
	}

	if(logo.enable) {
		add_logo_osd(input, &logo);
	}

	return 0;
}


int create_TextTime_info(void)
{
	Buffer_Handle hbuf;
	Int                 bufSize;
	BufferGfx_Attrs     gfxAttrs  ;
	pthread_t		drawtimethread ;
	int i = 0;

	memset(&gfxAttrs, 0, sizeof(BufferGfx_Attrs));
	gfxAttrs.colorSpace = ColorSpace_NOTSET;

	g_chnfont = Font_create(CHN_FONT_NAME);

	if(g_chnfont == NULL) {
		printf("Failed to create font %s\n", CHN_FONT_NAME);
		return -1;
	}

	/*¥¥Ω®textµƒBuffer*/
	gfxAttrs.dim.width      = TEXT_BITMAP_WIDTH;
	gfxAttrs.dim.height     = TEXT_BITMAP_HEIGHT;
	gfxAttrs.dim.x 					= 0 ;
	gfxAttrs.dim.y 					= 0;
	gfxAttrs.dim.lineLength = ColorSpace_getBpp(ColorSpace_YUV422PSEMI) * TEXT_BITMAP_WIDTH / 8;
	gfxAttrs.colorSpace     = ColorSpace_YUV422PSEMI;
	bufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height * 2;
	printf("bufsize =%d\n", bufSize);

	for(i = 0; i < 1; i++) {
		memset(&g_text_info[i], 0, sizeof(TextOsd_Info));
		hbuf = Buffer_create(bufSize, &gfxAttrs);

		if(hbuf == NULL) {
			printf("Failed to allocate bitmap buffer of size %d\n", bufSize);
			return -1;
		}

		g_display_time[i] = 0;

		g_text_info[i].hBuf = hbuf;
		g_text_info[i].text.xpos = 0;
		g_text_info[i].text.ypos = 0;
		g_text_info[i].text.width = TEXT_BITMAP_WIDTH;
		g_text_info[i].text.height = TEXT_BITMAP_HEIGHT;
		//	g_text_info[i].text.show = 0;

		Screen_clear(hbuf, 0, 0, TEXT_BITMAP_WIDTH, TEXT_BITMAP_HEIGHT);
		hbuf = NULL;
	}

	/*¥¥Ω®timerµƒBuffer*/
	gfxAttrs.dim.width      = TIME_BITMAP_WIDTH;
	gfxAttrs.dim.height     = TIME_BITMAP_HEIGHT;
	gfxAttrs.dim.lineLength = ColorSpace_getBpp(ColorSpace_YUV422PSEMI) * TIME_BITMAP_WIDTH / 8;
	gfxAttrs.colorSpace     = ColorSpace_YUV422PSEMI;
	bufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height * 2;
	printf("12 bufsize =%d\n", bufSize);
	g_timeosd_info.hBuf = Buffer_create(bufSize, &gfxAttrs);

	if(g_timeosd_info.hBuf == NULL) {
		printf("Failed to allocate bitmap buffer of size %d\n", bufSize);
		return -1;
	}

	g_timeosd_info.text.xpos = TIME_XPOS;
	g_timeosd_info.text.ypos = TIME_YPOS;
	g_timeosd_info.text.width = TIME_BITMAP_WIDTH;
	g_timeosd_info.text.height = TIME_BITMAP_HEIGHT;
	//	g_timeosd_info.text.show = 0;
	Screen_clear(g_timeosd_info.hBuf, 0, 0, TIME_BITMAP_WIDTH, TIME_BITMAP_HEIGHT);

	g_timeosd_info.yuyv_buf = (unsigned char *)malloc(TIME_BITMAP_WIDTH * TIME_BITMAP_HEIGHT * 2);

	if(g_timeosd_info.yuyv_buf == NULL) {
		printf("yuyv_buf is NULL\n");
		return -1;
	}



	pthread_create(&drawtimethread, NULL, (void *)time_osd_thread, (void *)NULL);
	return 0;
}


static void init_logo_info(int input, LogoInfo *lh)
{
	//	int h, w;
#if 0
	char filename[256] = {0};

	sprintf(filename, "%s", lh->filename);
	ghPng[input] = Png_create(input, filename, 0, lh);

	if(ghPng[input]) {
		Png_getWH(ghPng[input], &w, &h);

		if(h > PNG_MAX_WIDTH || w > PNG_MAX_HEIGHT) {
			Png_delete(ghPng[input]);
			ghPng[input] = NULL;
		}
	}

	if(ghPng[input] == NULL) {
		lh->errcode = 1;       //invaild png file
		lh->enable = 0;
	}

	if(ghPng[input]) {
		lh->width = w;
		lh->height = h;
	}

#endif
	pthread_mutex_init(&osd_mutex, NULL);
	return ;
}



int init_osd_info()
{
	int i = 0;
	int ret = 0;
	TextInfo	text;
	LogoInfo    logo;

	for(i = 0; i < 1; i++) {
		memset(&text, 0, sizeof(TextInfo));
		memset(&logo, 0, sizeof(LogoInfo));
		printf("input=%d\n", i);

		ret = read_logo_info(i, &logo);

		if(ret < 0) {
			logo.alpha = 0x80;
			logo.enable = 0 ;
			memset(logo.filename, 0, sizeof(logo.filename));
		} else {
			init_logo_info(i, &logo);
		}


		if(logo.enable) {
			add_logo_osd(i, &logo);
		}

		set_logo_info(i, &logo);

		ret = read_text_info(i,  &(text));

		if(ret < 0) {
			printf("WARNING:read_text_info failed!\n");
			text.alpha = 0x80;
			text.enable = 0;
		}

		set_text_info(i, &text);

		if(text.enable) {
			add_text_info(i, &(text));
		}

		g_display_time[i] = text.showtime;
	}

	printf("init_osd_info finished!\n");
	return 0;
}

