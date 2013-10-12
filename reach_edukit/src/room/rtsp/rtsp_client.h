
#ifdef USE_LINUX_PLATFORM
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif



/***********************************************************************************************
                                             rtsp协议
************************************************************************************************/
#define PACKET_BUFFER_END       (unsigned int)0x00000000
#define MAX_RTP_PKT_LENGTH      1400
#define DEST_PORT               2234
#define H264                    96
#define aac                     64

#define MIN_CLIENT_PORT_BIT     9

#define INVALID_SOCKET_VAL 			-1
#define TRUE		            1
#define FALSE		            0

#define RTSP_LISTEN_PORT        554// 




enum{
	OPTION=1,
	DESCRIBE,
	SETUP,
	PLAY,
	PAUSE,
	TEARDOWN,
	ANNOUNCE,
	GET_PARAMETER,
	SET_PARAMETER,
	MAX_OPERATE
};

#ifdef  GDM_RTSP_SERVER
#define WORD unsigned short
#define DWORD unsigned int
#define BYTE unsigned char 
#else
//#define WORD unsigned short
typedef unsigned short WORD ;
typedef unsigned int   DWORD ;
//#define DWORD unsigned int
typedef unsigned char  BYTE;
//#define BYTE unsigned char 
#endif

typedef struct 
{
    /**//* byte 0 */
    unsigned char csrc_len:4;        /**//* expect 0 */
    unsigned char extension:1;        /**//* expect 1, see RTP_OP below */
    unsigned char padding:1;        /**//* expect 0 */
    unsigned char version:2;        /**//* expect 2 */
    /**//* byte 1 */
    unsigned char payload:7;        /**//* RTP_PAYLOAD_RTSP */
    unsigned char marker:1;        /**//* expect 1 */
    /**//* bytes 2, 3 */
    unsigned short seq_no;            
    /**//* bytes 4-7 */
    unsigned  long timestamp;        
    /**//* bytes 8-11 */
    unsigned long ssrc;            /**//* stream number is used here. */
} RTP_FIXED_HEADER;

typedef struct {
    //byte 0
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;    
         
} NALU_HEADER; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char NRI:2; 
	unsigned char F:1;    
            
             
} FU_INDICATOR; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;    
} FU_HEADER; /**//* 1 BYTES */

typedef struct
{
  int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  unsigned max_size;            //! Nal Unit Buffer size
  int forbidden_bit;            //! should be always FALSE
  int nal_reference_idc;        //! NALU_PRIORITY_xxxx
  int nal_unit_type;            //! NALU_TYPE_xxxx    
  unsigned char *buf;                    //! contains the first byte followed by the EBSP
  unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;


typedef struct strClientMsg
{
    	unsigned int uiCseq;
	int iType;
    	unsigned char aucCSeqSrc[10];
    	unsigned char ucCSeqSrcLen;
    	unsigned char aucTrans[200];
	unsigned char ucTransNum;
    	unsigned char aucMrl[20];
    	unsigned char ucMrlLen;
    	unsigned char aucClientport[3][20];
    	unsigned char ucClientportLen[3];
	unsigned char timeused;
	int roomid;//card 1/2/3/4
}STR_CLIENT_MSG;

typedef struct 
{
    /**//* byte 0 */
    unsigned char byte1;
    /**//* byte 1 */
    unsigned char byte2;
    /**//* bytes 2, 3 */
    unsigned short seq_no;            
    /**//* bytes 4-7 */
    unsigned  long timestamp;        
    /**//* bytes 8-11 */
    unsigned long ssrc;            /**//* stream number is used here. */
} RTP_FIXED_HEADER_AUDIO;

enum{
    ERR_CLIENT_MSG_PORT_TOO_SHORT=-100,    //port should larger than 9bit
};


typedef struct _RTSPClientData
{
	int bUsed;
	int udpready;
	mid_plat_socket sSocket; //tcp socket
	mid_plat_socket UdpVideoSocket;//udp video socket	
	mid_plat_socket UdpAudioSocket;//udp audio socket
	mid_plat_sockaddr_in client_addr;//tcp addr
	mid_plat_sockaddr_in Udp_video_addr;//udp video addr
	mid_plat_sockaddr_in Udp_audio_addr;//udp audio addr
	int 	client_type ;// vlc 0 ,qt 1,other -1 ;  
	int 	timeout;     //max clinet timeout
	int 	roomid ;		//card1/2/3/4	
	char *client; //just for stream output client

	// add zhengyb
	
	
	
}RTSPClientData;

typedef struct __RTSPCliParam
{
	int dspFD;     	//DSP handle
	RTSPClientData rtspcliDATA[6]; //client param infomation
}RTSPCliParam;


//******************************************************************************************************//
        /*RTSP用户信息*/
//******************************************************************************************************//
#define ISRTSPUSED(dsp,cli)				(gRTSPCliPara[dsp].rtspcliDATA[cli].bUsed == TRUE)

/*set client used*/
#define SETRTSPUSED(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].bUsed = val)

/*get socket fd*/
#define GETRTSPSOCK(dsp,cli)			(gRTSPCliPara[dsp].rtspcliDATA[cli].sSocket)

/*set socket fd*/
#define SETRTSPSOCK(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].sSocket = val)

/*get sockaddr of client*/
#define GETRTSPADDR(dsp,cli)			(gRTSPCliPara[dsp].rtspcliDATA[cli].client_addr)

/*setsockaddr of client*/
#define SETRTSPADDR(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].client_addr = val)

/*get udp video sockaddr of client*/
#define GETRTSPUDPVIDEOADDR(dsp,cli)			(gRTSPCliPara[dsp].rtspcliDATA[cli].Udp_video_addr)

/*set  udp video sockaddr of client*/
#define SETRTSPUDPVIDEOADDR(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].Udp_video_addr = val)

/*get udp audio sockaddr of client*/
#define GETRTSPUDPAUDIOADDR(dsp,cli)			(gRTSPCliPara[dsp].rtspcliDATA[cli].Udp_audio_addr)

/*set  udp audio sockaddr of client*/
#define SETRTSPUDPAUDIOADDR(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].Udp_audio_addr = val)

/*get udp video socket fd*/
#define GETRTSPUDPVIDEOSOCK(dsp,cli)			(gRTSPCliPara[dsp].rtspcliDATA[cli].UdpVideoSocket)

/*set udp video socket fd*/
#define SETRTSPUDPVIDEOSOCK(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].UdpVideoSocket = val)

/*get udp audio socket fd*/
#define GETRTSPUDPAUDIOSOCK(dsp,cli)			(gRTSPCliPara[dsp].rtspcliDATA[cli].UdpAudioSocket)

/*set udp audio socket fd*/
#define SETRTSPUDPAUDIOSOCK(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].UdpAudioSocket = val)

/*current socket if valid*/
#define ISRTSPSOCK(dsp,cli)				        (gRTSPCliPara[dsp].rtspcliDATA[cli].sSocket!= INVALID_SOCKET_VAL)

/*current UDP video socket if valid*/
#define ISRTSPUDPVIDEOSOCK(dsp,cli)			    (gRTSPCliPara[dsp].rtspcliDATA[cli].UdpVideoSocket!= INVALID_SOCKET_VAL)

/*current UDP audio socket if valid*/
#define ISRTSPUDPAUDIOSOCK(dsp,cli)			    (gRTSPCliPara[dsp].rtspcliDATA[cli].UdpAudioSocket!= INVALID_SOCKET_VAL)

/* set current udp rtsp ready status*/
#define SETRTSPUDPREADY(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].udpready = val)

#define GETRTSPUDPREADY(dsp,cli)      (gRTSPCliPara[dsp].rtspcliDATA[cli].udpready)
/*is current udp rtsp ready?*/
#define ISRTSPUDPREADY(dsp,cli)			    (gRTSPCliPara[dsp].rtspcliDATA[cli].udpready == 1)

/*get dsp handle*/
#define GETRTSPFD(dsp)				            (gRTSPCliPara[dsp].dspFD)

/*set dsp handle*/
#define SETRTSPFD(dsp,val)			            (gRTSPCliPara[dsp].dspFD=val)

/*if handle valid*/
#define ISRTSPFD(dsp)				            (gRTSPCliPara[dsp].dspFD != INVALID_FD)

#define GETRTSPCLIENTTYPE(dsp,cli)		(gRTSPCliPara[dsp].rtspcliDATA[cli].client_type)
#define SETRTSPCLIENTTYPE(dsp,cli,val)		(gRTSPCliPara[dsp].rtspcliDATA[cli].client_type = val)

#define GETRTSPROOMID(dsp,cli) 		(gRTSPCliPara[dsp].rtspcliDATA[cli].roomid)
#define SETRTSPROOMID(dsp,cli,val)	(gRTSPCliPara[dsp].rtspcliDATA[cli].roomid = val)

#define SETRTSPCLIENT(dsp,cli,client) (gRTSPCliPara[dsp].rtspcliDATA[cli].client= client)
#define GETRTSPCLIENT(dsp,cli)           (gRTSPCliPara[dsp].rtspcliDATA[cli].client)


