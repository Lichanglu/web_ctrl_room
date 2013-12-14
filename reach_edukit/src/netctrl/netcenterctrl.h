#ifndef _NETCENTERCTRL_H_
#define _NETCENTERCTRL_H_


#include "stdint.h"
#include "reach_os.h"

typedef struct _list_head
{
   struct _list_head 	*next;
   struct _list_head	*prev;
   
}list_head;

typedef struct _netctrl_param
{
	int8_t stop;
	int8_t start;
	int8_t close;
	int8_t pause;
} NetCtrlParam;

typedef enum _NET_COMMND
{
    REC_START     = 0x05,
    REC_PAUSE     = 0x06,
    REC_STOP      = 0x07,
    REC_LOGIN     = 0x08,
    REC_HEARTBEAT = 0x09
}NET_MSGCODE;

typedef struct _timeout
{
	list_head stlist;
	struct timeval starttime;
	NET_MSGCODE msgcode;
	//uint8_t state;
}TimeOut;

typedef struct _netctrl_handle
{
    int32_t cenctrlfd;  //center control socket
    int32_t netctrlfd;  //net control socket
    uint32_t runstate;  // ��¼�Ƿ�ɹ�, run or stop
    //uint32_t hbstate;   //����״̬ 0��ʾû�з���������1��ʾ���������ɹ�
    
    list_head *pheadnode;
    NET_MSGCODE operstate; //��¼start stop or pause״̬ 

    pthread_mutex_t ccsocklock; //����Center Ctrl socket Lock
    pthread_mutex_t ncsocklock; //���Net Ctrl socket Lock
    pthread_mutex_t timeoutlock; //��ʱ�� lock
    pthread_mutex_t opestatelock; 
    
}NetctrlHandle;

typedef struct _netctrl_comnd
{
    NET_MSGCODE netcmd;
    uint32_t cencmd;
    int8_t *cmdname;	
    int32_t (*Package_Msg)(int8_t *, int32_t);
    int32_t (*Handle_Msg)(void*, uint32_t, void*);//(uint32_t ,void *);
    int32_t  direction; // 0: NetCtrl -> CenCtrl  1: CenCtrl -> NetCtrl
}NetCtrlComnd;





int32_t RegisterNetControlTask();






#endif



