
#include <stdio.h>//DEVICEDRV
#include "common.h"
#include "input_to_channel.h"
#include "ti_media_std.h"
//#include "adv7441_priv.h"
#include <mcfw/src_linux/devices/inc/device.h>
#include <mcfw/src_linux/devices/inc/device_videoDecoder.h>
#include <mcfw/src_linux/devices/adv7441/inc/adv7441.h>
#include <mcfw/src_linux/devices/adv7441/src/adv7441_priv.h>
#include <mcfw/src_linux/devices/adv7442/inc/adv7442.h>
#include <mcfw/src_linux/devices/adv7442/src/adv7442_priv.h>

#include <mcfw/src_linux/devices/gs2971/src/gs2971_priv.h>
#include <mcfw/src_linux/devices/gs2971/inc/gs2971.h>
//#include <mcfw/src_linux/devices/gs2972/src/gs2972_priv.h>
//#include <mcfw/src_linux/devices/gs2972/inc/gs2972.h>

#include "app_signal.h"
#include "reach.h"
#include "rwini.h"





typedef struct _Signal_Table_ {
	Vps_Adv7441_hpv_vps signal_info;
	unsigned short res;
	unsigned short digital_val;
	unsigned short analog_val;
} Signal_Table;

HVTable g_HV_table;

static Signal_Table s_signal_table[DEVICE_STD_REACH_LAST + 1] = {
	{{"480I",		263, 60, 1587, 720, 240}, DEVICE_STD_NTSC, 1, 1}, // 0-480ix60
	{{"576I",		313, 50, 1600, 720, 288}, DEVICE_STD_PAL, 1, 1}, // 1-576ix50
	{{"480I",		263, 60, 1587, 720, 240}, DEVICE_STD_480I, 1, 1}, // 2-480ix60
	{{"576I",		313, 50, 1600, 720, 288}, DEVICE_STD_576I, 1, 1}, // 3-576ix50
	{{"Revs",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_CIF, 1, 1}, // FVID2_STD_CIF, /**< Interlaced, 360x120 per field NTSC, 360x144 per field PAL. */
	{{"Revs",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_HALF_D1, 1, 1}, // FVID2_STD_HALF_D1, /**< Interlaced, 360x240 per field NTSC, 360x288 per field PAL. */
	{{"Revs",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_D1, 1, 1}, // FVID2_STD_D1, /**< Interlaced, 720x240 per field NTSC, 720x288 per field PAL. */
	{{"480P",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_480P, 1, 1}, // 7-480px60
	{{"576P",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_576P, 1, 1}, // 8-576px50

	{{"720P60",	750, 60, 1111, 1280, 720}, DEVICE_STD_720P_60, 1, 1}, // 9-1280x720x60
	{{"720P50",	750, 50, 1333, 1280, 720}, DEVICE_STD_720P_50, 1, 1}, // 10-1280x720x50
	{{"1080I60",	563, 60, 1481, 1920, 540}, DEVICE_STD_1080I_60, 1, 1}, // 11-1920x1080x60i
	{{"1080I50",	562, 50, 1777, 1920, 540}, DEVICE_STD_1080I_50, 1, 1}, // 12-1920x1080x50i
	{{"1080P60",	1125, 60, 740, 1920, 1080}, DEVICE_STD_1080P_60, 1, 1}, // 13-1920x1080x60p
	{{"1080P50",	1125, 50, 888, 1920, 1080}, DEVICE_STD_1080P_50, 1, 1}, // 14-1920x1080x50p
	{{"1080P25",	1125, 25, 1777, 1920, 1080}, DEVICE_STD_1080P_24, 1, 1}, // 15-1920x1080x25p
	{{"1080P30",	1125, 30, 1481, 1920, 1080}, DEVICE_STD_1080P_30, 1, 1}, // 16-1920x1080x30p

	{{"640x480@60",	525, 60, 1588, 640, 480}, DEVICE_STD_VGA_640X480X60, 1, 1}, // 17-640x480x60
	{{"640x480@72",	520, 72, 1320, 640, 480}, DEVICE_STD_VGA_640X480X72, 1, 1}, // 18-640x480x72
	{{"640x480@75",	500, 75, 1333, 640, 480}, DEVICE_STD_VGA_640X480X75, 1, 1}, // 19-640x480x75
	{{"640x480@85",	509, 85, 1155, 640, 480}, DEVICE_STD_VGA_640X480X85, 1, 1}, // 20-640x480x85
	{{"800x600@60",	628, 60, 1320, 800, 600}, DEVICE_STD_VGA_800X600X60, 1, 1}, // 21-800x600x60
	{{"800x600@72",	666, 72, 1040, 800, 600}, DEVICE_STD_VGA_800X600X72, 1, 1}, // 22-800x600x72
	{{"800x600@75",	625, 75, 1066, 800, 600}, DEVICE_STD_VGA_800X600X75, 1, 1}, // 23-800x600x75
	{{"800x600@85",	631, 85, 931, 800, 600}, DEVICE_STD_VGA_800X600X85, 1, 1}, // 24-800x600x85
	{{"1024x768@60",	806, 60, 1033, 1024, 768}, DEVICE_STD_VGA_1024X768X60, 1, 1}, // 25-1024x768x60
	{{"1024x768@70",	806, 70, 885, 1024, 768}, DEVICE_STD_VGA_1024X768X70, 1, 1}, // 26-1024x768x70
	{{"1024x768@75",	800, 75, 833, 1024, 768}, DEVICE_STD_VGA_1024X768X75, 1, 1}, // 27-1024x768x75
	{{"1024x768@85",	808, 85, 728, 1024, 768}, DEVICE_STD_VGA_1024X768X85, 1, 1}, // 28-1024x768x85
	{{"1280x720@60",	750, 60, 1111, 1280, 720}, DEVICE_STD_VGA_1280X720X60, 1, 1}, // 29-1280x720x60
	{{"1280x768@60",	798, 60, 1054, 1280, 768}, DEVICE_STD_VGA_1280X768X60, 1, 1}, // 30-1280x768x60
	{{"1280x768@75",	0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_VGA_1280X768X75, 1, 1}, // 31-1280x768x75
	{{"1280x768@85",	0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_VGA_1280X768X85, 1, 1}, // 32-1280x768x85
	{{"1280x800@60",	828, 60, 1006, 1280, 800}, DEVICE_STD_VGA_1280X800X60, 1, 1}, // 33-1280x800x60
	{{"1280x960@60",	1000, 60, 833, 1280, 960}, DEVICE_STD_VGA_1280X960X60, 1, 1}, // 34-1280x960x60
	{{"1280x1024@60",	1066, 60, 781, 1280, 1024}, DEVICE_STD_VGA_1280X1024X60, 1, 1}, // 35-1280x1024x60
	{{"1280x1024@75",	1066, 75, 625, 1280, 1024}, DEVICE_STD_VGA_1280X1024X75, 1, 1}, // 36-1280x1024x75
	{{"1280x1024@85",	0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_VGA_1280X1024X85, 1, 1}, // 37-1280x1024x85
	{{"1366x768@60",	795, 60, 1047, 1366, 768}, DEVICE_STD_VGA_1366X768X60, 1, 1}, // 38-1366x768x60
	{{"1440x900@60",	934, 60, 901, 1440, 900}, DEVICE_STD_VGA_1440X900X60, 1, 1}, // 39-1440x900x60
	{{"1400x1050@60",	1089, 60, 765, 1400, 1050}, DEVICE_STD_VGA_1400X1050X60, 1, 1}, // 40-1400x1050x60
	{{"1400x1050@75",	0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_VGA_1400X1050X75, 1, 1}, // 41-1400x1050x75
	{{"1600x1200@60",	1250, 60, 666, 1600, 1200}, DEVICE_STD_VGA_1600X1200X60, 1, 1}, // 42-1600x1200x60
	{{"1920x1080@60_DMT", 1125, 60, 740, 1920, 1080}, DEVICE_STD_VGA_1920X1080X60_DMT, 1, 1}, // 43-1920x1080x60-DMT
	{{"1920x1080@60_GTF", 1125, 60, 740, 1920, 1080}, DEVICE_STD_VGA_1920X1080X60_GTF, 1, 1}, // 44-1920x1080x60-GTF
	{{"1920x1200@60",	1244, 60, 675, 1920, 1200}, DEVICE_STD_VGA_1920X1200X60, 1, 1}, // 45-1920x1200x60
	{{"2560x1440@60",	1481, 60, 0xFFFF, 2560, 1440}, DEVICE_STD_VGA_2560X1440X60, 1, 1}, // 46-2560x1440x60

	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_2CH_D1, 1, 1}, // FVID2_STD_MUX_2CH_D1,/**< Interlaced, 2Ch D1, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_2CH_HALF_D1, 1, 1}, // FVID2_STD_MUX_2CH_HALF_D1, /**< Interlaced, 2ch half D1, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_2CH_CIF, 1, 1}, // FVID2_STD_MUX_2CH_CIF, /**< Interlaced, 2ch CIF, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_4CH_D1, 1, 1}, // FVID2_STD_MUX_4CH_D1, /**< Interlaced, 4Ch D1, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_4CH_CIF, 1, 1}, // FVID2_STD_MUX_4CH_CIF, /**< Interlaced, 4Ch CIF, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_4CH_HALF_D1, 1, 1}, // FVID2_STD_MUX_4CH_HALF_D1, /**< Interlaced, 4Ch Half-D1, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_8CH_CIF, 1, 1}, // FVID2_STD_MUX_8CH_CIF, /**< Interlaced, 8Ch CIF, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_8CH_HALF_D1, 1, 1}, // FVID2_STD_MUX_8CH_HALF_D1, /**< Interlaced, 8Ch Half-D1, NTSC or PAL. */

	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_AUTO_DETECT, 1, 1}, // FVID2_STD_AUTO_DETECT, /**< Auto-detect standard. Used in capture mode. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_CUSTOM, 1, 1}, // FVID2_STD_CUSTOM, /**< Custom standard used when connecting to external LCD etc...
	//The video timing is provided by the application.
	//Used in display mode. */

	{{"Max", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_REACH_LAST, 0, 0} // FVID2_STD_MAX
};

typedef struct _HV_TABEL_ {
	char name[48];
	unsigned int h_pos;
	unsigned int width;
	unsigned int hight;
	unsigned int v_pos;
	int id;
} HV_Refer_Tabel;

//模拟+分量 参照表
static const HV_Refer_Tabel const hv_refer_dvi_table[DEVICE_STD_REACH_LAST] = {
	{"480I",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_NTSC},  			// 0-480ix60
	{"576I",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_PAL},  			// 1-576ix50
	{"480I",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_480I},  			// 2-480ix60
	{"576I",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_576I},  			// 3-576ix50
	{"Revs",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_CIF},				// FVID2_STD_CIF, /**< Interlaced, 360x120 per field NTSC, 360x144 per field PAL. */
	{"Revs",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_HALF_D1},  			// FVID2_STD_HALF_D1, /**< Interlaced, 360x240 per field NTSC, 360x288 per field PAL. */
	{"Revs",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_D1},  			// FVID2_STD_D1, /**< Interlaced, 720x240 per field NTSC, 720x288 per field PAL. */
	{"480P",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_480P},  			// 7-480px60
	{"576P",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_576P},				// 8-576px50
	//YPbPr
	{"720P60",	255, 1541, 748, 28},				// 9-1280x720x60
	{"720P50",	251, 1537, 746, 26},  			// 10-1280x720x50
	{"1080I60",	185, 2111, 560, 20},				// 11-1920x1080x60i
	{"1080I50",	186, 2112, 562, 22},				// 12-1920x1080x50i
	{"1080P60",	180, 2106, 1120, 40},				// 13-1920x1080x60p
	{"1080P50",	180, 2106, 1120, 40},				// 14-1920x1080x50p
	{"1080P25",	200, 2126, 1122, 42},				// 15-1920x1080x25p
	{"1080P30",	236, 2162, 1120, 42},				// 16-1920x1080x30p

	{"640x480@60",	135, 781, 525, 45, DEVICE_STD_VGA_640X480X60},				// 17 -640x480x60
	{"640x480@72",	160, 806, 508, 28, DEVICE_STD_VGA_640X480X72},				// 18 -640x480x72
	{"640x480@75",	164, 810, 502, 22, DEVICE_STD_VGA_640X480X75},				// 19 -640x480x75
	{"640x480@85",	126, 772, 500, 20, DEVICE_STD_VGA_640X480X85},				// 20 -640x480x85
	{"800x600@60",	202, 1008, 628, 28, DEVICE_STD_VGA_800X600X60}, 				// 21 -800x600x60
	{"800x600@72",	178, 984, 626, 26, DEVICE_STD_VGA_800X600X72},	 			// 22 -800x600x72
	{"800x600@75",	225, 1031, 625, 25, DEVICE_STD_VGA_800X600X75},	 			// 23 -800x600x75
	{"800x600@85",	200, 1006, 631, 31, DEVICE_STD_VGA_800X600X85},	 			// 24 -800x600x85
	{"1024x768@60",	280, 1310, 806, 38, DEVICE_STD_VGA_1024X768X60},				 // 25 -1024x768x60
	{"1024x768@70",	264, 1294, 806, 38, DEVICE_STD_VGA_1024X768X70},				 // 26 -1024x768x70
	{"1024x768@75",	260, 1290, 800, 32, DEVICE_STD_VGA_1024X768X75},				 // 27-1024x768x75
	{"1024x768@85",	292, 1322, 808, 40, DEVICE_STD_VGA_1024X768X85},		 		// 28-1024x768x85
	{"1280x720@60",	320, 1606, 750, 30, DEVICE_STD_VGA_1280X720X60},				 // 29-1280x720x60
	{"1280x768@60",	320, 1606, 798, 30, DEVICE_STD_VGA_1280X768X60},				 // 30-1280x768x60
	{"1280x768@75",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_1280X768X75},			 // 31-1280x768x75
	{"1280x768@85",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_1280X768X85},	 		// 32-1280x768x85
	{"1280x800@60",	330, 1616, 830, 30, DEVICE_STD_VGA_1280X800X60},	 			// 33-1280x800x60
	{"1280x960@60",	340, 1626, 990, 30, DEVICE_STD_VGA_1280X960X60},	 			// 34-1280x960x60

	{"1280x1024@60",	350, 1636, 1066, 42, DEVICE_STD_VGA_1280X1024X60},	 		// 35-1280x1024x60
	{"1280x1024@75",	376, 1662, 1066, 42, DEVICE_STD_VGA_1280X1024X75},	 		// 36-1280x1024x75
	{"1280x1024@85",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_1280X1024X85},	 		// 37-1280x1024x85
	{"1366x768@60",	266, 1638, 788, 20, DEVICE_STD_VGA_1366X768X60},	 		   // 38-1366x768x60
	{"1440x900@60",	350, 1796, 930, 30, DEVICE_STD_VGA_1440X900X60},	 			// 39-1440x900x60
	{"1400x1050@60",	359, 1765, 1087, 37, DEVICE_STD_VGA_1400X1050X60},	 		// 40-1400x1050x60
	{"1400x1050@75",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_1400X1050X75},	 		// 41-1400x1050x75
	{"1600x1200@60",	480, 2086, 1250, 50, DEVICE_STD_VGA_1600X1200X60},  			 // 42-1600x1200x60
	{"1920x1080@60_DMT", 300, 2326, 1100, 20, DEVICE_STD_VGA_1920X1080X60_DMT},		// 43-1920x1080x60-DMT
	{"1920x1080@60_GTF", 100, 2026, 1090, 10, DEVICE_STD_VGA_1920X1080X60_GTF},		// 44-1920x1080x60-GTF
	{"1920x1200@60",	536, 2462, 1242, 38, DEVICE_STD_VGA_1920X1200X60},  			 // 45-1920x1200x60  XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	{"2560x1440@60",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_2560X1440X60},			// 46-2560x1440x60

	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_2CH_D1}, // FVID2_STD_MUX_2CH_D1,/**< Interlaced, 2Ch D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_2CH_HALF_D1}, // FVID2_STD_MUX_2CH_HALF_D1, /**< Interlaced, 2ch half D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_2CH_CIF}, // FVID2_STD_MUX_2CH_CIF, /**< Interlaced, 2ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_4CH_D1}, // FVID2_STD_MUX_4CH_D1, /**< Interlaced, 4Ch D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_4CH_CIF}, // FVID2_STD_MUX_4CH_CIF, /**< Interlaced, 4Ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_4CH_HALF_D1}, // FVID2_STD_MUX_4CH_HALF_D1, /**< Interlaced, 4Ch Half-D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_8CH_CIF}, // FVID2_STD_MUX_8CH_CIF, /**< Interlaced, 8Ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_8CH_HALF_D1}, // FVID2_STD_MUX_8CH_HALF_D1, /**< Interlaced, 8Ch Half-D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_AUTO_DETECT}, // FVID2_STD_AUTO_DETECT, /**< Auto-detect standard. Used in capture mode. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_CUSTOM}, // FVID2_STD_CUSTOM, /**< Custom standard used when connecting to external LCD etc...
	//The video timing is provided by the application.
	//Used in display mode. */
	{"Max", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_REACH_LAST}, // FVID2_STD_MAX
};


static int g_fpga_version = 0;

int app_get_fpga_version(void)
{
	return g_fpga_version;
}
int app_init_fpga_version(void)
{
	Device_Adv7442Obj pObj;
	int cmd = IOCTL_DEVICE_VIDEO_GET_FPGA_VERSION;
	g_fpga_version = Device_adv7442Control((Device_Adv7442Handle)&pObj,cmd, NULL, NULL);

	return g_fpga_version;
}

char kernel_version[16] = {0};
int init_kernel_version(void)
{
	if(gEduKit->gpiofd> 0) {
		ioctl(gEduKit->gpiofd, 0x11111111, kernel_version);
		printf("[init_kernel_version] kernel_version:[%s]\n", kernel_version);
	} else {
		sprintf(kernel_version,"0000");
	}

}

char *get_kernel_version(void)
{
	return kernel_version;
}

int write_input_info(int input1_mode, int input2_mode)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	int 			rst = -1;
	char    		title[24];
	const char config_file[64] = INPUT_CONFIG;
	sprintf(title, "input");



	sprintf(temp, "%d", input1_mode);
	ret =  ConfigSetKey((char *)config_file, title, "Vp0", temp);

	if(ret != 0) {
		fprintf(stderr, "Failed to Get input \n");
		goto EXIT;
	}

	sprintf(temp, "%d", input2_mode);
	ret =  ConfigSetKey((char *)config_file, title, "Vp1", temp);

	if(ret != 0) {
		fprintf(stderr, "Failed to Get input \n");
		goto EXIT;
	}

	rst = 1;
	fprintf(stderr, "write finished!\n");
EXIT:
	return rst;
}

int read_input_info(int *input1_mode, int *input2_mode)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	int 			rst = -1;
	const char config_file[64] = INPUT_CONFIG;
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char    title[24];
	sprintf(title, "input");

	ret =  ConfigGetKey((char *)config_file, title, "Vp0", temp);

	if(ret != 0) {
		fprintf(stderr, "Failed to Get input \n");
		goto EXIT;
	}

	*input1_mode = atoi(temp);

	ret =  ConfigGetKey((char *)config_file, title, "Vp1", temp);

	if(ret != 0) {
		fprintf(stderr, "Failed to Get input \n");
		goto EXIT;
	}

	*input2_mode = atoi(temp);
	rst = 1;

	fprintf(stderr, "read finished!\n-----g_input1_have_signal=%d,g_input2_have_signal=%d\n",
	        *input1_mode, *input2_mode);
EXIT:
	return rst;
}



/*read Hportch Vportch Table*/
int read_HV_table(const HVTable *pp, int digital)
{
	int ret = 0, val = 0;
	char temp[512], config_file[512];
	HV *pHV = NULL;
	int i = 0;

	if(digital) {
		pHV = (HV *)pp->digital;
		strcpy(config_file, HV_TABLE_D_NAME);
	} else {
		pHV = (HV *)pp->analog;
		strcpy(config_file, HV_TABLE_A_NAME);
	}

	for(i = 0; i < DEVICE_STD_REACH_LAST; i++) {
		ret = ConfigGetKey(config_file, s_signal_table[i].signal_info.name, "hporch", temp); // 0

		if(ret == 0) {
			val = atoi(temp);

			if(val > 0) {
				pHV[i].hporch = val;
				printf("[%s] h[%d]", s_signal_table[i].signal_info.name, val);
			}
		}

		ret = ConfigGetKey(config_file, s_signal_table[i].signal_info.name, "vporch", temp);

		if(ret == 0) {
			val = atoi(temp);

			if(val > 0) {
				pHV[i].vporch = val;
				printf("[%s] v[%d]", s_signal_table[i].signal_info.name, val);
			}
		}
	}

	return 0;
}

int get_HV_table(HVTable *hv_table)
{
	if(hv_table == NULL) {
		fprintf(stderr, "WRANG:hv_table is NULL!");
		return -1;
	}

	memcpy(hv_table, &g_HV_table, sizeof(HVTable));
	return 0;
}


int set_HV_table(HVTable *hv_table)
{
	if(hv_table == NULL) {
		fprintf(stderr, "WRANG:hv_table is NULL!");
		return -1;
	}

	memcpy(&g_HV_table, hv_table, sizeof(HVTable));
	return 0;
}


/*save HV  Table*/
int write_HV_table(HVTable *pNew, int digital)
{
	char  temp[512], config_file[512];
	HV *pHV = NULL, *pNewHV = NULL;
	int ret;
	int i = 0;

	if(digital == 1) {
		pHV = g_HV_table.digital;
		pNewHV = pNew->digital;
		strcpy(config_file, HV_TABLE_D_NAME);
	} else if(digital == 0) {
		pHV = g_HV_table.analog;
		pNewHV = pNew->analog;
		strcpy(config_file, HV_TABLE_A_NAME);
	}

	for(i = 0; i < DEVICE_STD_REACH_LAST; i++) {
		if(pNewHV[i].hporch != pHV[i].hporch) {
			pHV[i].hporch = pNewHV[i].hporch;
			sprintf(temp, "%d", pHV[i].hporch);
			ret = ConfigSetKey(config_file, s_signal_table[i].signal_info.name, "hporch", temp); // 0

			if(ret != 0) {
				fprintf(stderr, "%s!!\n", s_signal_table[i].signal_info.name);
				return -1;
			}
		}


		if(pNewHV[i].vporch != pHV[i].vporch) {
			pHV[i].vporch = pNewHV[i].vporch;
			sprintf(temp, "%d", pHV[i].vporch);
			ret = ConfigSetKey(config_file, s_signal_table[i].signal_info.name, "vporch", temp);

			if(ret != 0) {
				fprintf(stderr, "%s!!\n", s_signal_table[i].signal_info.name);
				return -1;
			}
		}
	}

	set_HV_table(pNew);
	return 0;

}


/*compare beyond HV table*/
int set_signal_HV(int mode, int digital, short hporch, short vporch, HVTable *pp)
{
	if(mode > DEVICE_STD_REACH_LAST) {
		return -1;
	}

	if(digital) {
		pp->digital[mode].hporch = hporch;
		pp->digital[mode].vporch = vporch;
	} else {
		pp->analog[mode].hporch = hporch;
		pp->analog[mode].vporch = vporch;
	}

	return 0;
}


int set_mp_mode(int mode)
{

	return 0;
}

void init_HV_table(void)
{
	memset(&g_HV_table, 0, sizeof(HVTable));
	//read_HV_table(&g_HV_table, 1);
	read_HV_table(&g_HV_table, 0);
}


int get_signal_default_hv(int mode_id, int *h, int *v)
{
	int i   = 0;
	int num = sizeof(s_signal_table) / sizeof(s_signal_table[0]);

	if(h == NULL || v == NULL) {
		return -1;
	}

	for(i = 0; i < num; i++) {
		if(mode_id == s_signal_table[i].res) {
			*h = s_signal_table[i].signal_info.hpv;
			*v = s_signal_table[i].signal_info.lps;
			break;
		}
	}

	if(i < num) {
		return i;
	}

	return -1;
}

int get_signal_analog_base_hv(int mode_id, int *h, int *v)
{
	int i   = 0;
	int num = sizeof(hv_refer_dvi_table) / sizeof(hv_refer_dvi_table[0]);

	if(h == NULL || v == NULL) {
		return -1;
	}

	for(i = 0; i < num; i++) {
		if(mode_id == hv_refer_dvi_table[i].id) {
			*h = hv_refer_dvi_table[i].h_pos;
			*v = hv_refer_dvi_table[i].v_pos;
			break;
		}
	}

	if(i < num) {
		return i;
	}

	return -1;
}

char *get_signal_analog_name(int mode_id)
{
	int i   = 0;
	int num = sizeof(hv_refer_dvi_table) / sizeof(hv_refer_dvi_table[0]);

	for(i = 0; i < num; i++) {
		if(mode_id == hv_refer_dvi_table[i].id) {
			return hv_refer_dvi_table[i].name;
		}
	}

	return NULL;
}

