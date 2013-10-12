#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>



#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>


#include "reach.h"
#include "capture.h"
#include "select.h"
#include "common.h"
#include "input_to_channel.h"
#include "middle_control.h"
#include "sd_demo_osd.h"


#define MMAP_MEM_PAGEALIGN         (4*1024-1)

int g_vp0_writeyuv_flag = 0;
int g_vp1_writeyuv_flag = 0;
int g_mp_writeyuv_flag = 0;





/*==============================================================================
    函数:  Int32 set_resolution(Int32 VpNum, Int32 streamId, UInt32 outWidth, UInt32 outHeight)
    功能:  设置各VP口高低码率通道分辨率
    参数:  input :SIGNAL_INPUT_1,SIGNAL_INPUT_2
           streamId: 高低码流ID (LOW_STREAM,HIGH_STREAM)
		   1:如果需要直通的话，既不缩放的时候，设置outwidth,outheight等于采集宽高即可

    Created By 徐崇 2012.09.14 09:09:14

           |           out 0        |  out  1       |   num   |
           | ch0 | ch1 | ch2 | ch3  |  ch0  | ch1   |  chnum1 |
           |  0  |  2  |  1  |  3   |  3    |   3   |    2    | INPUT 2直通模式
           |  0  |  2  |  1  |  3   |  1    |   3   |    2    | INPUT 1 ,INPUT 2直通模式
           |  0  |  2  |  1  |  3   |  1    |   1   |    2    | INPUT 1直通模式
           |  0  |  2  |  1  |  3   |  x    |   x   |    0    | INPUT 1，INPUT 2 都不直通
=============== ===============================================================*/
Int32 set_resolution(Int32 input, Int32 streamId, UInt32 outWidth, UInt32 outHeight)
{
	fprintf(stderr, "001--set_resolution!\n");

	if((input >= SIGNAL_INPUT_MAX) || (streamId >= MAX_STREAM)) {
		fprintf(stderr, "002--set_resolution!\n");
		return -1;
	}

	fprintf(stderr, "003--set_resolution!\n");

	if((outWidth <= 0) || (outHeight <= 0)) {
		fprintf(stderr, "004--set_resolution!\n");
		return -1;
	}

	outWidth  = Reach_align(outWidth, 16u);
	outHeight = Reach_align(outHeight, 8u);


	fprintf(stderr, "\n\n\n");
	fprintf(stderr, "set_resolution width = %d, height = %d\n", outWidth, outHeight);
	fprintf(stderr, "\n\n\n");


	SclrLink_chDynamicSetOutRes params;

	/* 设置各通道输出分辨率  */
	params.width  = outWidth;
	params.height = outHeight;
	params.pitch[0]  = outWidth * 2;
	params.pitch[1]  = outWidth * 2;
	params.pitch[2]  = outWidth * 2;
	params.chId = input;

	sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_0, &params);
	sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_0, input, 60, 30);

	return 0;
}

#ifdef HAVE_MP_MODULE
#if 0
static Void swms_set_swms_layout2(UInt32 devId, SwMsLink_CreateParams *swMsCreateArgs, UInt32 num)
{
	SwMsLink_LayoutPrm *layoutInfo;
	SwMsLink_LayoutWinInfo *winInfo;
	UInt32 outWidth, outHeight, row, col, winId, widthAlign, heightAlign;
	UInt32 outputfps;

	getoutsize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);

	widthAlign = 8;
	heightAlign = 1;

	if(devId > 1) {
		devId = 0;
	}

	layoutInfo = &swMsCreateArgs->layoutPrm;
	outputfps = layoutInfo->outputFPS;

	memset(layoutInfo, 0, sizeof(*layoutInfo));
	layoutInfo->onlyCh2WinMapChanged = FALSE;
	layoutInfo->outputFPS = outputfps;

	layoutInfo->numWin = num;

	if(num == 1) {
		for(row = 0; row < 1; row++) {
			for(col = 0; col < 1; col++) {
				winId = row * 2 + col;
				winInfo = &layoutInfo->winInfo[winId];

				winInfo->width	= SystemUtils_align((outWidth * 2) / 4, widthAlign);
				winInfo->height = SystemUtils_align(outHeight / 2, heightAlign);
				winInfo->startX = winInfo->width * col;
				winInfo->startY = winInfo->height * row;

#if 1
				winInfo->width	= 1920;
				winInfo->height = 1080;
				winInfo->startX = 0;
				winInfo->startY = 0;
#endif

				winInfo->bypass = FALSE;
				winInfo->channelNum = devId * SYSTEM_SW_MS_MAX_WIN + winId;

			}
		}
	} else if(num == 2) {
		winInfo = &layoutInfo->winInfo[0];
		winInfo->width	= SystemUtils_align((outWidth), widthAlign);
		winInfo->height = SystemUtils_align(outHeight, heightAlign);
		winInfo->startX = 0;
		winInfo->startY = 0;
		winInfo->bypass = FALSE;
		winInfo->channelNum = 0;

		winInfo = &layoutInfo->winInfo[1];
		winInfo->width	= SystemUtils_align((outWidth) / 3, widthAlign);
		winInfo->height = SystemUtils_align(outHeight / 3, heightAlign);
		winInfo->startX = outWidth - winInfo->width;
		winInfo->startY = outHeight - winInfo->height;
		winInfo->bypass = FALSE;
		winInfo->channelNum = 1;
	}

}
#endif
#endif

static Void swms_set_swms_layout1(UInt32 devId, SwMsLink_CreateParams *swMsCreateArgs)
{
	SwMsLink_LayoutPrm *layoutInfo;
	SwMsLink_LayoutWinInfo *winInfo;
	UInt32 winId, widthAlign, heightAlign;
	UInt32 outputfps;

	widthAlign = 8;
	heightAlign = 1;

	if(devId > 1) {
		devId = 0;
	}

	layoutInfo = &swMsCreateArgs->layoutPrm;
	outputfps = layoutInfo->outputFPS;

	memset(layoutInfo, 0, sizeof(*layoutInfo));
	layoutInfo->onlyCh2WinMapChanged = FALSE;
	layoutInfo->outputFPS = outputfps;

	layoutInfo->numWin = 3;

	Int32 i = 0;

	for(i = 0; i < layoutInfo->numWin; i++) {
		UInt32 DstWidth, DstHeight, Srcwidth, Srcheight, x, y;
		Uint32 Outwidth, Outheight;
		SclrLink_CalSclrMode CalSclrMode = {0};
		winId = i;

		if(1 != GetSwmsTypeWH(swMsCreateArgs->maxOutRes, 32, i, &DstWidth, &DstHeight, &x, &y)) {
			return ;
		}

		capture_get_input_hw(i, &Srcwidth, &Srcheight);

		if((Srcwidth <= 0) || (Srcheight <= 0)) {
			Srcwidth  = 1920;
			Srcheight = 1080;

		}

		CalSclrMode.SrcWidth  = Srcwidth;
		CalSclrMode.SrcHeight = Srcheight;
		CalSclrMode.DstWidth  = DstWidth;
		CalSclrMode.DstHeight = DstHeight;
		Outwidth  = DstWidth;
		Outheight = DstHeight;
		CalculationSclrMode(CalSclrMode, &Outwidth, &Outheight);


		winInfo = &layoutInfo->winInfo[winId];
		winInfo->width  =  Outwidth;
		winInfo->height =  Outheight;
		winInfo->startX =  x + SystemUtils_align((CalSclrMode.DstWidth  - Outwidth) / 2, widthAlign);
		winInfo->startY =  y + SystemUtils_align((CalSclrMode.DstHeight - Outheight) / 2, heightAlign);
		winInfo->bypass = FALSE;


		winInfo->channelNum = winId;
	}

	gEduKit->layout.cur_layout   = 32;
}

#if 0
static Void swms_set_swms_layout3(UInt32 devId, SwMsLink_CreateParams *swMsCreateArgs)
{
	SwMsLink_LayoutPrm *layoutInfo;
	SwMsLink_LayoutWinInfo *winInfo;
	UInt32 outWidth, outHeight, row, col, winId, widthAlign, heightAlign;
	UInt32 outputfps;

	getoutsize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);

	widthAlign = 8;
	heightAlign = 1;

	if(devId > 1) {
		devId = 0;
	}

	layoutInfo = &swMsCreateArgs->layoutPrm;
	outputfps = layoutInfo->outputFPS;

	memset(layoutInfo, 0, sizeof(*layoutInfo));
	layoutInfo->onlyCh2WinMapChanged = FALSE;
	layoutInfo->outputFPS = outputfps;

	layoutInfo->numWin = 3;

	for(row = 0; row < 2; row++) {
		for(col = 0; col < 2; col++) {
			winId = row * 2 + col;
			winInfo = &layoutInfo->winInfo[winId];

			winInfo->width	= SystemUtils_align((outWidth * 2) / 4, widthAlign);
			winInfo->height = SystemUtils_align(outHeight / 2, heightAlign);
			winInfo->startX = winInfo->width * col;
			winInfo->startY = winInfo->height * row;

			//		winInfo->width	= 1920;
			//		winInfo->height = 1080;
			//		winInfo->startX = 0;
			//		winInfo->startY = 0;

			winInfo->bypass = FALSE;
			winInfo->channelNum = devId * SYSTEM_SW_MS_MAX_WIN + winId;
		}
	}

#if 0

	for(row = 0; row < 4; row++) {
		winId = 4 + row;
		winInfo = &layoutInfo->winInfo[winId];

		winInfo->width	= layoutInfo->winInfo[0].width / 2;
		winInfo->height = layoutInfo->winInfo[0].height / 2;
		winInfo->startX = layoutInfo->winInfo[0].width * 2;
		winInfo->startY = winInfo->height * row;

		winInfo->bypass = TRUE;
		winInfo->channelNum = devId * SYSTEM_SW_MS_MAX_WIN + winId;
	}

#endif
}
#endif

Int32 write_yuv(UInt32 physaddr, Uint32 framesize, int chid)
{
	int fd = 0;
	unsigned int mmap_offset = 0;
	unsigned int mmap_memaddr = 0;
	unsigned int mmap_memsize = 0;
	volatile unsigned int *mmap_pvirtaddr = NULL;

	UInt32 pvirtaddr;

	fd = open("/dev/mem", O_RDWR | O_SYNC);

	if(fd < 0) {
		printf(" ERROR: /dev/mem open failed !!!\n");
		return -1;
	}

	mmap_offset		= physaddr & MMAP_MEM_PAGEALIGN;
	mmap_memaddr	= physaddr - mmap_offset;
	mmap_memsize	= framesize + mmap_offset;

	mmap_pvirtaddr = mmap(
	                     (void *)mmap_memaddr,
	                     mmap_memsize,
	                     PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED,
	                     fd,
	                     mmap_memaddr
	                 );

	if(mmap_pvirtaddr == NULL) {
		printf(" ERROR: mmap() failed !!!\n");
		return -1;
	}

	pvirtaddr = (UInt32)((UInt32)mmap_pvirtaddr + mmap_offset);

#if 0
	static int time0 = 0;
	static int time1 = 0;
	char buf[256] = {0};

	if(chid == 0) {
		sprintf(buf, "picture_vp0_%d.yuv", time0++);
	} else {
		sprintf(buf, "picture_vp1_%d.yuv", time1++);
	}

	FILE *fp = fopen(buf, "w+");

	if(!fp) {
		munmap((void *)mmap_pvirtaddr, mmap_memsize);
		close(fd);
		return -1;
	}

	fwrite((unsigned char *)(pvirtaddr), 1, framesize, fp);
	fclose(fp);
#endif

#if 1
	unsigned char *pY, *pU, *pV;
	pY = (unsigned char *)pvirtaddr;
	pU = (unsigned char *)pvirtaddr + 1920 * 1200;
	pV = (unsigned char *)pvirtaddr + 1920 * 1200 + 1;

	unsigned int vwidth = 1920;
	unsigned int vheight = 1080;

	//	capture_get_input_hw(chid, &vwidth, &vheight);
	if(chid == 2) {
		write_yuv_420(pY, pU, pV, vwidth, vheight, chid);
	} else {
		write_yuv_420(pY, pU, pV, vwidth, vheight, chid);
	}

#endif
	munmap((void *)mmap_pvirtaddr, mmap_memsize);
	close(fd);

	return 0;
}



