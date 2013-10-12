#ifndef _REC_CLIENT_H_
#define _REC_CLIENT_H_

#include <pthread.h>

#define ROOM_MAX_BOX 			(6)
#define MAX_UDP_LEN				65535


typedef struct _IFrameFlag{
	int 					IFrameRecived[ROOM_MAX_BOX];
	pthread_mutex_t  		mutex[ROOM_MAX_BOX];
} IFrameFlag;

typedef struct _netSockets{
	int 					udpSocket[ROOM_MAX_BOX];
	pthread_mutex_t  		mutextAccess[ROOM_MAX_BOX];
} netSockets;

typedef struct _GlobalData {
    int             quit;                /* Global quit flag */
    int             frames;              /* Video frame counter */
    int             videoBytesProcessed; /* Video bytes processed counter */
    int             soundBytesProcessed; /* Sound bytes processed counter */
    int             samplingFrequency;   /* Sound sampling frequency */
    int             imageWidth;          /* Width of clip */
    int             imageHeight;         /* Height of clip */
    int				stopPlay;
    pthread_mutex_t mutex;               /* Mutex to protect the global data */
} GlobalData;


typedef struct _REC_CLIENT {
	char			ipaddr[64];
	char			pdata[MAX_UDP_LEN+128];
	int				channel;
	int				index;

	GlobalData		gbl;
	netSockets		loaderSockets;
	IFrameFlag		iFrameFlag;
}REC_CLIENT;

typedef struct _ST_MARK_ {
	unsigned int offset;
	unsigned int length;
	unsigned int lasttime;
	unsigned int enable;
	unsigned int connected;
}ST_MARK;

typedef struct _RecFrame {
	int channel;
	int width;
	int height;
	int length;
	int code;
	int flag;
	int timestamp;
	int framerate;
	unsigned char *data;
	unsigned char *AudioData;
    int IsAudio;
	ST_MARK videomark;
	ST_MARK Audiomark;
}RecFrame;


int create_rec_client_inst(void **handle, char *ipaddr, int channel, int inst_index);

int recvFrame(void *handle, RecFrame *pFrame, const int channel, const int flag);

int recvFrame2(int *socket_st, int st_num, RecFrame *pFrame,
						ST_MARK *pmark, unsigned char *pdata, unsigned int *enable_index);

int recvFrame3(int socket_st, RecFrame *pFrame, unsigned char *pdata);

int ParseIDRHeader(unsigned char *pdata);

void gblSetStopPlay2(void *handle);

int gblGetStopPlay2(void *handle);


#endif