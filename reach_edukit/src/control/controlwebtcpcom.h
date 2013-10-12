#ifndef _WEBTCPCOM_H
#define _WEBTCPCOM_H

#include "control.h"
#include "../enc/middle_control.h"
#define CTRL_TO_ROOM_SERVER_PORT				(20020)

#define CTRL_TO_ROOM_MSG_MAX_LENGHT				(264)

#define MSG_TO_ROOM_VODPLAY_CTRL				(0X8024)

#define VODFILE_MAX_FILENAME					(512)


#define MSG_GET_REC_INFO					0X2027				//ZL


typedef enum vod_play_status{
    VODCTRL_PLAY,
	VODCTRL_RESUME,
    VODCTRL_PAUSE,
    VODCTRL_SEEK,
    VODCTRL_STOP
} vod_play_status_t;

typedef enum seek_level{
	SEEK_LEVEL_B4 = -4,
	SEEK_LEVEL_B8,
	SEEK_LEVEL_B16,
	SEEK_LEVEL_B32 ,
	
	SEEK_LEVEL_F4 = 1,
	SEEK_LEVEL_F8,
	SEEK_LEVEL_F16,
	SEEK_LEVEL_F32
	
} seek_level_t;

typedef struct _vodplay_ctrl_{
	vod_play_status_t	ctrl_mode;
	seek_level_t		seek_level;
	int8_t				filename[VODFILE_MAX_FILENAME];
}vodplay_ctrl;


int appCmdIntParse(int cmd,int invalue,int inlen,int *outvalue,int *outlen,int port);
int appCmdStringParse(int cmd,char *invalue,int inlen,char  *outvalue,int *outlen,int port);
int appCmdStructParse(int cmd, void *inval, int len, void *outval,int *outlen,int port);


int controlWebSetPreviewPassword(char *password);
int PanelGetRecInfo(PanelRecInfo *recinfo);

extern int32_t ctrl_vodplay_ctrl(vodplay_ctrl *play_ctrl);



#endif
