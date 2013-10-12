/*
 * =====================================================================================
 *
 *       Filename:  display.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2012年11月1日 09时12分18秒
 *       Revision:
 *       Compiler:  gcc
 *
 *         Author:  黄海洪
 *        Company:  深圳锐取信息技术股份有限公司
 *
 * =====================================================================================
 */


#include <stdio.h>

#include "reach.h"
#include "display.h"


#if 1
Int32 hdmiDisplaySetResolution(UInt32 displayId, UInt32 resolution)
{
	dis_stop_drv(displayId);
#if 0
	Char  cmdBuf[100] = {0};
	VDIS_CMD_ARG2(cmdBuf, VDIS_TIMINGS_SETVENC, 0, 3);
	VDIS_CMD_ARG2(cmdBuf, VDIS_TIMINGS_SETVENC, 0, 0);

	VDIS_CMD_ARG2(cmdBuf, VDIS_CLKSRC_SETVENC, "dclk", 3);		//Arun

	switch(resolution) {
		case SYSTEM_STD_1080P_60:
		case SYSTEM_STD_1080P_50:
		case SYSTEM_STD_720P_60:
			VDIS_CMD_ARG1(cmdBuf, VDIS_TIMINGS_720P_60, 3);
			VDIS_CMD_ARG1(cmdBuf, VDIS_TIMINGS_720P_60, 0);
			break;

		case SYSTEM_STD_SXGA_60:
		case SYSTEM_STD_XGA_60:
			VDIS_CMD_ARG1(cmdBuf, VDIS_TIMINGS_XGA_60, 3);
			VDIS_CMD_ARG1(cmdBuf, VDIS_TIMINGS_XGA_60, 0);
			break;

		default:
			printf("\n Resolution not supported for this HDMI|HDCOMP!! \n");
			break;
	}

	/* Tie HDMI and HDCOMP from A8 side */
	VDIS_CMD_ARG1(cmdBuf, VDIS_TIMINGS_TIEDVENCS, VDIS_VENC_HDMI);

	VDIS_CMD_ARG2(cmdBuf, VDIS_TIMINGS_SETVENC, 1, 3);
	VDIS_CMD_ARG2(cmdBuf, VDIS_TIMINGS_SETVENC, 1, 0);
#endif
	dis_set_resolution(displayId, resolution);
	dis_start_drv(displayId);
	return 0;
}
#endif


