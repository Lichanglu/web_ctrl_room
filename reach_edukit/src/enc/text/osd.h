#ifndef OSD_H
#define OSD_H

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <linux/watchdog.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <iconv.h>
#include <png.h>


#include "ti_media_std.h"
#include "dm6467_struct.h"

#include "middle_control.h"

#include "Font.h"
#include "screen.h"


#include "sd_demo_osd.h"

#define HAVE_OSD_MODULE    1


#define CHN_FONT_NAME        "data/fonts/simkai.ttf"

#define MSG_ADD_TEXT    33  //加入字幕
#define TEXT_BUFTYPE_NUMTYPES  2

//#define MAX_OSD_WIDTH           (16*2*32)


#define TEXT_BITMAP_WIDTH		OSD_MAX_WIDTH    //	(16*2*32)   //(16*2 *24) //8 * 4 * 50
#define TEXT_BITMAP_HEIGHT		42  //8 * 4 * 2

#define TIME_BITMAP_WIDTH		8 * 4 * 10    //8 * 4 * 12
#define TIME_BITMAP_HEIGHT		48//8 * 4 * 2

#define	PNG_MAX_WIDTH		192
#define	PNG_MAX_HEIGHT		64


#define	PNG_DF_WIDTH		192
#define	PNG_DF_HEIGHT		64



#define TIME_XPOS		(32*2)
#define TIME_YPOS		(32*2)
#define OSD_FONT_SIZE		23
#define OSD_TRANS_VALUE   0xFF



// 2lu video +1 lu sw video
#define INPUT_TEXT_MAX_NUM     SIGNAL_INPUT_MAX


/*加字幕结构标题*/
typedef struct __RecAVTitle {
	int  x;         		//x pos
	int  y;        			//y pos
	int len;   					//Text实际长度
	char Text[128];			//text
} RecAVTitle;


#define LOGOFILE "/opt/dvr_rdk/ti816x_2.8/logo"



typedef enum __DisplayLogo_Text {
    NOdisplay = 0,
    OnlyShowText,
    OnlyShowLogo,
    BothShow,
} DisplayLogo_Text;


typedef struct _TEXTOSD_INFO_T_ {
	Buffer_Handle hBuf;
	TextInfo text;
	unsigned char *yuyv_buf ;
} TextOsd_Info;


#define LOGO_CONFIG 			".config/logo_1260.ini"
#define TEXT_CONFIG				".config/text.ini"

#if defined (__cplusplus)
extern "C" {
#endif

extern int create_TextTime_info(void);
extern int init_osd_info(void);

//int check_CHN(char *text, int size);
void show_text_info(int , const char *, int ,TextInfo* );
void hide_osd_view(int input, int type);

extern int get_logo_info(int ,LogoInfo* );
extern int set_logo_info(int ,LogoInfo* );

extern int get_text_info(int , TextInfo* );
extern int set_text_info(int , TextInfo* );

extern int check_logo_png(int input, char *filename);
extern int  add_logo_osd(int input, LogoInfo* lh);

extern int write_yuv4xx_file(unsigned char *buf,int buf_len,char* filename);
extern int rgb_2_yuv422(unsigned char * out,unsigned char * * row_pointers,int height,int width);
extern void yuv422_2_yuv420(unsigned char * yuv420,unsigned char * yuv422,int width,int height);
extern void yuv420p_2_yuv420sp(unsigned char * yuv420sp,unsigned char * yuv420p,int width,int height);
extern void write_yuv420sp_file(unsigned char * yuv420sp,int width,int height);
extern void yuv422Y16_2_yuv422yuyv( unsigned char * yuyv,unsigned char *y16, int width, int height);

extern int SD_logo_osdUpdate(int input,int osdtype,unsigned char *osdbuff,int osdlen,LogoInfo *info);
extern int SD_subtitle_osdUpdate(int input, int osdtype, unsigned char *osdbuff, int osdlen,TextInfo *info);

#if defined (__cplusplus)
}
#endif




#endif
