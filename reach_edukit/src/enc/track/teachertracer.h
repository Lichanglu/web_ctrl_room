#ifndef _TTRACER_
#define _TTRACER_

#define ENCODER_TCP_PORT 5555
#define student_PORT 5556
#define web_PORT 5557
#define		WEBCONNECT_NUM_MAX		(10)		//最大连接数


#define		CONNECT_NUM_MAX		(1)		//最大连接数


extern char STUDENTTRACE_IP[20];
extern int getServerSocket(void);
extern void TEACHERTRACER(void *pParam);
extern int teacherTracerMove(unsigned short int data);
extern int sendCmdToStudentTracer(unsigned char *data,int len,int cmd);
extern void WebListen(void *param);
extern char * GetTrackVersion();
extern void StartAutoTrack();
extern int  SwitchFirstPic(int firstpictime, int firstpic);
extern int  StopFirstPic();
#endif

