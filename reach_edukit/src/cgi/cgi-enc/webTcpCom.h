#ifndef _WEBTCPCOM_H
#define _WEBTCPCOM_H





extern int appCmdIntParse(int cmd,int invalue,int inlen,int *outvalue,int *outlen);
extern int appCmdStringParse(int cmd,char *invalue,int inlen,char  *outvalue,int *outlen);
extern int appCmdStructParse(int cmd, void *inval, int len, void *outval,int *outlen);
//
#endif
