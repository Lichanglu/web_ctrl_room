#ifndef	__PPT_INDEX_H__
#define	__PPT_INDEX_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define HEADLEN                                 0x2
#define BUFLEN                                 1024
#define PORT_LISTEN_PPT  		3016
typedef struct PPT_data_info_ {
	int status;
	char PPTTextBuf[BUFLEN];
} PPT_data_info_t;

int remotePPTindexTask(void *pParam);
void PPTindexTask(void *pParam);
int getPPTDataInfo(PPT_data_info_t *pPptInfo);
void setPPTDataInfoStatus(int iStatus);
#endif	//__PPT_INDEX_H__

