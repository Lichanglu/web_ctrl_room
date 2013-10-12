#ifndef _PARAMS_H_
#define _PARAMS_H_

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


#define CONFIG_TABLE_FILE		("config.xml")

#define MAX_VIDEO_NUM		(2)
#define MAX_AUDIO_NUM		(2)

#define ADTS                    0x53544441
#define AAC                     0x43414130
#define PCM                     0x4d435030

#define MIC_INPUT               1
#define LINE_INPUT              0


#endif

