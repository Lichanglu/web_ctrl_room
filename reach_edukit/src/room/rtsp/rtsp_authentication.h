
//add by zhangmin 
#ifdef GDM_RTSP_SERVER

#ifndef _RTSP_AUTHENCTICATION_H__
#define _RTSP_AUTHENCTICATION_H__


int app_authentication(char *usr,char *passwd);
int app_authentication_stop(void);
int app_authentication_start(void);
int app_authentication_get_passwd(char *buff);
int app_authentication_get_user(char *buff);

int app_rtsp_authentication_clear(void);
int app_rtsp_authentication_begin(char *usr);
int app_rtsp_authentication_stop(char * usr);
#endif


#endif

