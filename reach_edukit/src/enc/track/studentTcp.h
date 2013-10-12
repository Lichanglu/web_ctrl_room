#ifndef _STUDENTTCP_
#define _STUDENTTCP_




extern void studentTrace(void *param);
extern int studentTracerMove(unsigned char data);
extern void SENDDATATOTEACHER(void *pParam);
extern int sendCmdToTeacherTracer(unsigned char *data,int len,int cmd);
extern void SendClassInfotoTeacher();

#endif
