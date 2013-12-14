#include "teachertracer.h"
#include "commonTcp.h"
#include "commontrack.h"


typedef struct _list_head {
	struct _list_head 	*next;
	struct _list_head	*prev;

} list_head;

typedef struct _timeout {
	Int32 mode;
	Int32 index;
	Int32 timeout;
	list_head stlist;
	UInt32 starttime;
	uint8_t state;
} TimeOut;

/* ��ʼ��˫������ */
#define list_init(head) do			\
	{						\
		(head)->next = (head)->prev = (head);	\
	} while(0)

/* ��ָ��Ԫ��(where)֮�������Ԫ��(item) */
#define list_add(item, towhere) do	\
	{					\
		(item)->next = (towhere)->next;	\
		(item)->prev = (towhere);	\
		(towhere)->next = (item);	\
		(item)->next->prev = (item);	\
	} while(0)

/* ��ָ��Ԫ��(where)֮ǰ������Ԫ��(item) */
#define list_add_before(item, towhere)  \
	list_add(item,(towhere)->prev)

/* ɾ��ĳ��Ԫ�� */
#define list_remove(item) do			\
	{						\
		(item)->prev->next = (item)->next;	\
		(item)->next->prev = (item)->prev;	\
	} while(0)

/* �����������������Ԫ�� */
#define list_for_each_item(item, head)\
	for ((item) = (head)->next; (item) != (head); (item) = (item)->next)

/* �����������������Ԫ�� */
#define list_for_each_item_rev(item, head) \
	for ((item) = (head)->prev; (item) != (head); (item) = (item)->prev)

/* ���ݱ��ڵ�(item)��ȡ�ڵ����ڽṹ��(type)�ĵ�ַ */
/* �ڵ�item��ַ(member�ĵ�ַ) - ������Ԫ��member�ڽṹ���е�ƫ�� */
#define list_entry(item, type, member) \
	((type *)((char *)item - (char *)(&((type *)0)->member)))

/* �ж������Ƿ�Ϊ�� */
#define list_is_empty(head)	\
	((head)->next == (head))

/* ��ȡָ��λ����һԪ�� */
#define list_prev_item(item)\
	((head)->prev)



typedef enum {
    SINGLEPICTURE = 1,
    TWOPICTURE,
    THREEPICTURE,
    FOUREPICTURE,
    FIVEPICTURE,
    SIXPICTURE

} LayoutMode;

typedef enum {
    VGANOCHANGE = 0,
    VGACHANGE,
    VGAMAX,
} VgaState;

typedef enum {
    STUDONW = 0,
    STUUP,
    STUMAX,
} StudentState;

typedef enum {
    TEACHLOSE = 0,
    TEACHPLATFORM,
    TEACHMOVE,
    TEACHBLACKBORAD1,
    TEACHBLACKBORAD2,
    TEACHMAX,

} TeacherState;

typedef enum {
    TEACHER_INDEX = 0,		//��ʦ����
    VGA_INDEX ,				//VGA����
    STUDENT_INDEX,			//ѧ������
    TEACHER_FAR_INDEX,		//��ʦԶ������
    STUDENT_FAR_INDEX,		//ѧ��Զ������
    TEACHBLACKBORAD_INDEX1,	//���黭��
    TEACHBLACKBORAD_INDEX2,	//���黭��
    MAX_INDEX,	//���
} SubMode;

typedef enum {
    STRATEGY_2 = 0,
    STRATEGY_4 = 1,
    STRATEGY_5_1 = 2,   //5��λ������
    STRATEGY_5_2 = 3,
} STRATEGY;

typedef enum {
    ONE_MP   =  0x1,   //������
    TWO_MP_1 = 0x21,   // 2���� һ��һС (���Ͻ�)
    TWO_MP_2 = 0x22,   // 2���� �ȷ�
    TWO_MP_3 = 0x23,   // 2���� һ��һС�����½ǣ�
    TWO_MP_4 = 0x24,   // 2���� һ��һС (���Ͻ�)
    THREE_MP = 0x31,   //  3����
    FOUR_MP  = 0x41,   //  4����
    FIVE_MP  = 0x51,   //  5����
    SIX_MP_1 = 0x61,   //  6���� 1��
    SIX_MP_2 = 0x62,   //  6���� 2��
} MPMODE;


//�ӻ�����Ϣ
typedef struct _SubMppState {
	Int32 LastSubMp;    //�ϴ��л�����
	Int32 CurSubMp;     //����Ӧ���л�����
	Int32 Flag;	//ˢ�±�־ ��Ҫ�����л�
	//Int32 Index;   //�������� SubMode
	UInt32 keeptime[MAX_INDEX];
} SubMpState;


//��Ҫ�����໭����ӻ�������
typedef struct _MpState {
	UInt32 num;						//�������
	SubMpState SubMpstate[6]; //֧�����������

	Int32 StudentState;			//ѧ��ʵʱ״̬ ��StudentState
	Int32 TeacherState;			//��ʦʵʱ״̬ ��TeacherState
	Int32 VgaState;				//VGAʵʱ״̬ ��VgaState

	//Int32 LastSwitch;
	//Int32 MpType;  // 0 ���� 1���� 2 �ȷ� 3 ѧ������ʦ
	//UInt32 CurSwitch;  //ʵʱ����
} MpState;


typedef struct _TrackState {
	Int32 vga;
	Int32 students;
	Int32 teacher;
	Int32 lastvga;
	Int32 laststudents ;
	Int32 lastteacher;

	Int32 LastSwitch;
	Int32 LastMpMode;
	Int32 LastTrackMode;
	Int32 FirstPic;
} TrackState;

#define TRACKVERSION "1.0.4"

char STUDENTTRACE_IP[20] = "127.0.0.1";
#define RED				"\033[0;32;31m"
#define NONE				"\033[m"
#define		STUDENTS_STOP_TRACK	(1)		//���͸�ѧ����ֹͣ����,���л���ȫ��

#define		SEND_CMD_DELAY		(100000)	//�����л������߳���ʱ��λ��΢��

extern track_encode_info_t		g_track_encode_info;
extern track_strategy_info_t	g_track_strategy_info;
extern track_strategy_timeinfo_t	g_track_strategy_timeinfo;
extern track_class_info_t g_tClassInfo;
extern final_class_info_t g_tFinalClassInfo;
extern zlog_category_t     *ptrackzlog;
static int serverSocket  = -1;
static int studentSocket = -1;
static int WebSocket[WEBCONNECT_NUM_MAX] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int switch_students_time = 0;
char clientIp[30] = {0};
static unsigned int gHeartLive = 1;

MpState OneState;
MpState TwoState;
MpState ThreeState;
MpState FourState;
MpState FiveState;
MpState SixState;

Int32 StudentUp = 0;

Int32 gStrategy = STRATEGY_5_2;
Int32 gMpMode   = ONE_MP;

TrackState gTrackState;


static  list_head headnode;
pthread_mutex_t timeout;

static void TimeoutLock()
{
	pthread_mutex_lock(&timeout);
}

static void TimeoutunLock()
{
	pthread_mutex_unlock(&timeout);
}

static Int32 TimeoutLockInit()
{
	return pthread_mutex_init(&timeout, NULL);
}

static Int32 TimeoutdeInit()
{
	return pthread_mutex_destroy(&timeout);
}


/**
* @	��ѧ��������״̬��־��Ϊ1��ʾ����������Ϊ0��ʾ�Ͽ�����
*/
static int studentTraceTCPStatus = 0;


void setServerSocket(int socket)
{
	serverSocket = socket;
}

int getServerSocket(void)
{
	return serverSocket;
}
void setStudentSocket(int socket)
{
	studentSocket = socket;
}

int getStudentSocket(void)
{
	return studentSocket;
}



char *GetTrackVersion()
{
	return "TRACKVERSION" ;
}

char *GetSwitchString(int cmd)
{
	switch(cmd) {
		case SWITCH_TEATHER :		//�л���ʦ������
			return "@@@@@ SWITCH_TEATHER @@@@@";
			break;

		case SWITCH_STUDENTS:		//�л�ѧ��������
			return "@@@@@ SWITCH_STUDENTS @@@@@";
			break;

		case SWITCH_VGA:						//�л�VGA����
			return "@@@@@ SWITCH_VGA @@@@@";
			break;

		case SWITCH_BLACKBOARD1:				//�л��������������
			return "@@@@@ SWITCH_BLACKBOARD1 @@@@@";
			break;

		case SWITCH_BLACKBOARD2:				//�л��������������
			return "@@@@@ SWITCH_BLACKBOARD2 or SWITCH_TEACHER_PANORAMA@@@@@";
			break;

		case SWITCH_STUDENTS_PANORAMA:		//�л�ѧ��ȫ�������
			return "@@@@@ SWITCH_STUDENTS_PANORAMA @@@@@";
			break;

			//2���沼��
		case  SWITCH_2_VGA_TEATHER:
			return "@@@@@ SWITCH_2_VGA_TEATHER @@@@@";
			break;

		case  SWITCH_2_TEATHER_STU:
			return "@@@@@ SWITCH_2_TEATHER_STU @@@@@";
			break;

		case  SWITCH_2_STU_TEATHER:
			return "@@@@@ SWITCH_2_STU_TEATHER @@@@@";
			break;

		case   SWITCH_2_STUPANORAMA_TEATHER:
			return "@@@@@ SWITCH_2_STUPANORAMA_TEATHER @@@@@";
			break;

		case   SWITCH_2_TEATHER_AND_VGA:
			return "@@@@@ SWITCH_2_TEATHER_AND_VGA @@@@@";
			break;

		case   SWITCH_2_STU_AND_TEATHER:
			return "@@@@@ SWITCH_2_STU_AND_TEATHER @@@@@";
			break;

		case   SWITCH_2_STUPANORAMA_AND_TEATHER:
			return "@@@@@ SWITCH_2_STUPANORAMA_AND_TEATHER @@@@@";
			break;

		case   SWITCH_2_TEATHER_AND_STU:
			return "@@@@@ SWITCH_2_TEATHER_AND_STU @@@@@";
			break;

		case	SWITCH_2_VGA_TEATHER_1:
			return "@@@@@ SWITCH_2_VGA_TEATHER_1 @@@@@";
			break;

		case	SWITCH_2_STUPANORAMA_TEATHER_1:
			return "@@@@@ SWITCH_2_STUPANORAMA_TEATHER_1 @@@@@";
			break;

		case	SWITCH_2_STU_TEATHER_1:
			return "@@@@@ SWITCH_2_STU_TEATHER_1 @@@@@";
			break;

		case	SWITCH_2_VGA_TEATHER_2:
			return "@@@@@ SWITCH_2_VGA_TEATHER_2 @@@@@";
			break;

		case	SWITCH_2_STU_TEATHER_2:
			return "@@@@@ SWITCH_2_STU_TEATHER_2 @@@@@";
			break;

		case	SWITCH_2_STUPANORAMA_TEATHER_2:
			return "@@@@@ SWITCH_2_STUPANORAMA_TEATHER_2 @@@@@";
			break;

			//3���沼��
		case	SWITCH_3_VGA_TEATHER_STU:       //VGA�� ��ʦ���� ѧ������
			return "@@@@@ SWITCH_3_VGA_TEATHER_STU @@@@@";
			break;

		case	SWITCH_3_VGA_TEACHPANORAMA_STU :    //VGA�� ��ʦȫ�� ѧ������
			return "@@@@@ SWITCH_3_VGA_TEACHPANORAMA_STU @@@@@";
			break;

		case	SWITCH_3_VGA_TEATHER_STUPANORAMA:    //VGA�� ��ʦ���� ѧ��ȫ��
			return "@@@@@ SWITCH_3_VGA_TEATHER_STUPANORAMA @@@@@";
			break;

		case	SWITCH_3_VGA_TEACHPANORAMA_STUPANORAMA:  //VGA�� ��ʦȫ�� ѧ��ȫ��
			return "@@@@@ SWITCH_3_VGA_TEACHPANORAMA_STUPANORAMA @@@@@";
			break;

		case	SWITCH_3_BLACKBOARD1_TEATHER_STU:  //����� ��ʦ���� ѧ������
			return "@@@@@ SWITCH_3_BLACKBOARD_TEATHER_STU @@@@@";
			break;

		case	SWITCH_3_BLACKBOARD1_TEACHPANORAMA_STU : //����� ��ʦȫ�� ѧ������
			return "@@@@@ SWITCH_3_BLACKBOARD_TEACHPANORAMA_STU @@@@@";
			break;

		case	SWITCH_3_BLACKBOARD1_TEATHER_STUPANORAMA://����� ��ʦ���� ѧ��ȫ��
			return "@@@@@ SWITCH_3_BLACKBOARD_TEATHER_STUPANORAMA @@@@@";
			break;

		case	SWITCH_3_BLACKBOARD1_TEACHPANORAMA_STUPANORAMA://����� ��ʦȫ��  ѧ��ȫ��
			return "@@@@@ SWITCH_3_BLACKBOARD_TEACHPANORAMA_STUPANORAMA @@@@@";
			break;

		case	SWITCH_3_BLACKBOARD2_TEATHER_STUPANORAMA://����� ��ʦȫ��  ѧ��ȫ��
			return "@@@@@ SWITCH_3_BLACKBOARD2_TEATHER_STUPANORAMA @@@@@";
			break;

		case	SWITCH_3_BLACKBOARD2_TEATHER_STU://����� ��ʦȫ��  ѧ��ȫ��
			return "@@@@@ SWITCH_3_BLACKBOARD2_TEATHER_STU @@@@@";
			break;

		case	SWITCH_4_VGA_TEATHER_STU_TEACHPANORAMA :             //VGA�� ѧ������
			return "@@@@@ SWITCH_4_VGA_TEATHER_STU_TEACHPANORAMA @@@@@";
			break;

		case	SWITCH_4_VGA_TEACHPANORAMA_STU_TEACHPANORAMA  :      //VGA�� ѧ��ȫ��
			return "@@@@@ SWITCH_4_VGA_TEACHPANORAMA_STU_TEACHPANORAMA @@@@@";
			break;

		case	SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA :      //����� ѧ������
			return "@@@@@ SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA @@@@@";
			break;

		case	SWITCH_4_VGA_TEACHPANORAMA_STUPANORAMA_TEACHPANORAMA :  //����� ѧ��ȫ��
			return "@@@@@ SWITCH_4_VGA_TEACHPANORAMA_STUPANORAMA_TEACHPANORAMA @@@@@";
			break;

		case	SWITCH_4_BLACKBOARD1_TEATHER_STU_TEACHPANORAMA :  //����� ѧ��ȫ��
			return "@@@@@ SWITCH_4_BLACKBOARD1_TEATHER_STU_TEACHPANORAMA @@@@@";
			break;

		case	SWITCH_4_BLACKBOARD1_TEATHER_STUPANORAMA_TEACHPANORAMA :  //����� ѧ��ȫ��
			return "@@@@@ SWITCH_4_BLACKBOARD1_TEATHER_STUPANORAMA_TEACHPANORAMA @@@@@";
			break;

		case	SWITCH_4_BLACKBOARD1_TEACHPANORAMA_STU_TEACHPANORAMA :  //����� ѧ��ȫ��
			return "@@@@@ SWITCH_4_BLACKBOARD1_TEACHPANORAMA_STU_TEACHPANORAMA @@@@@";
			break;

		case	SWITCH_4_BLACKBOARD1_TEACHPANORAMA_STUPANORAMA_TEACHPANORAMA :  //����� ѧ��ȫ��
			return "@@@@@ SWITCH_4_BLACKBOARD1_TEACHPANORAMA_STUPANORAMA_TEACHPANORAMA @@@@@";
			break;

		case  SWITCH_4_VGA_TEACHPER_STU_STUPANORAMA:                      //VGA�� ��ʦ���� ѧ������ ѧ��Զ��
			return "@@@@@ SWITCH_4_VGA_TEACHPER_STU_STUPANORAMA @@@@@";

		case SWITCH_4_BLACKBOARD1_TEACHPER_STU_STUPANORAMA:                     //����� ��ʦ���� ѧ������ ѧ��Զ��
			return "@@@@@ SWITCH_4_BLACKBOARD1_TEACHPER_STU_STUPANORAMA @@@@@";

		case SWITCH_4_BLACKBOARD2_TEACHPER_STU_BLACKBOARD1:                    //����2�� ��ʦ���� ѧ������ ����1
			return "@@@@@ SWITCH_4_BLACKBOARD2_TEACHPER_STU_BLACKBOARD1 @@@@@";

		case SWITCH_4_BLACKBOARD2_TEACHPER_STUPANORAMA_BLACKBOARD1:               //����2�� ��ʦ���� ѧ��Զ�� ����1
			return "@@@@@ SWITCH_4_BLACKBOARD2_TEACHPER_STUPANORAMA_BLACKBOARD1 @@@@@";

		case	SWITCH_5    :
			return "@@@@@ SWITCH_5 @@@@@";
			break;

			//6���沼��
		case	SWITCH_6_1   :
			return "@@@@@ SWITCH_6_1 @@@@@";
			break;

		case	SWITCH_6_2   :
			return "@@@@@ SWITCH_6_2 @@@@@";
			break;

		default
				:
			return "UNKOWN CMD";
			break;
	}
}


void StartAutoTrack()
{
	gTrackState.LastTrackMode = -1;
}

static uint32_t getCurrentTime(void)
{
	struct timeval tv;
	struct timezone tz;
	uint32_t ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return (ultime);
}




static int32_t AddTimeOut(int32_t mode, int32_t index, int32_t time)
{
	TimeOut *pattr = NULL;
	list_head *pheadnode = &headnode;
	list_head *pcurnode = NULL;

	if(time == 0) {
		return 0;
	}

	TimeoutLock();
	/* �����Ƿ���ڸ��豸 */
	list_for_each_item(pcurnode, pheadnode) {
		if(NULL != pcurnode) {
			pattr = list_entry(pcurnode, TimeOut, stlist);

			if((pattr->mode == mode) && (pattr->index == index)) {
				pattr->starttime  = getCurrentTime();
				TimeoutunLock();
				return 0;
			}
		}
	}
	TimeoutunLock();

	pattr = (TimeOut *)malloc(sizeof(TimeOut));
	pattr->mode = mode;
	pattr->index = index;
	pattr->timeout = time;

	pattr->starttime  = getCurrentTime();
	//upper_msg_set_time(&pattr->starttime);

	TimeoutLock();
	list_add_before(&pattr->stlist, pheadnode);
	TimeoutunLock();

	return 0;
}

static int32_t DelTimeOut(int32_t mode, int32_t index)
{
	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;
	list_head *pheadnode = &headnode;
	TimeoutLock();
	/* �����Ƿ���ڸ��豸 */
	list_for_each_item(pcurnode, pheadnode) {
		if(NULL != pcurnode) {
			pattr = list_entry(pcurnode, TimeOut, stlist);

			if((pattr->mode == mode) && (pattr->index == index)) {

				list_remove(pcurnode);
				free(pattr);

				TimeoutunLock();
				//����Ϣ�ȴ���ʱ
				return 1;
			}
		}
	}
	TimeoutunLock();

	return 0;
}

int SwitchFirstPic(int firstpictime, int firstpic)
{
	if(firstpic == 1)
	{
		return 0;
	}
	
	if((firstpictime > 0) && (firstpictime < 100)) 
	{

		DelTimeOut(999, 0);
		AddTimeOut(999, 0, firstpictime);

		if(firstpic == 2)
		{
			gTrackState.FirstPic = SWITCH_VGA;
			teacherTracerMove(SWITCH_VGA);
		}
		else if(firstpic == 3)
		{
			gTrackState.FirstPic = SWITCH_TEACHER_PANORAMA;
			teacherTracerMove(SWITCH_TEACHER_PANORAMA);
		}
		else if(firstpic == 4)
		{
			gTrackState.FirstPic = SWITCH_STUDENTS_PANORAMA;
			teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
		}
	
	}
	return 0;	
}


int StopFirstPic()
{
	
	DelTimeOut(999, 0);
	
	return 0;	
}


static void *TimeOutdeal(void *args)
{

	list_head *pheadnode = &headnode;
	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;
	list_init(pheadnode);
	UInt32 nowtime = 0;

	while(1) {
		r_usleep(500 * 1000);
		nowtime	= getCurrentTime();

		TimeoutLock();
		/* �����Ƿ���ڸ��豸 */
		list_for_each_item(pcurnode, pheadnode) {
			if(NULL != pcurnode) {
				int32_t ret_val = 0;
				pattr = list_entry(pcurnode, TimeOut, stlist);

				//	ret_val = upper_msg_monitor_time_out_status(&pattr->starttime, pattr->timeout);
				if(nowtime >= (pattr->starttime + pattr->timeout * 1000)) {
					printf("111111111111111111111 %u %u\n", nowtime, (pattr->starttime + pattr->timeout * 1000));
					//��ʱ��ɾ��


					list_remove(pcurnode);
					free(pattr);
				}
			}
		}

		TimeoutunLock();
	}
}

//�����ʱ�Ƿ�  ���� ����0  δ�� ����1
static int32_t CheckTimeOut(int32_t mode, int32_t index)
{
	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;
	list_head *pheadnode = &headnode;
	TimeoutLock();
	/* �����Ƿ���ڸ��豸 */
	list_for_each_item(pcurnode, pheadnode) {
		if(NULL != pcurnode) {
			pattr = list_entry(pcurnode, TimeOut, stlist);

			if((pattr->mode == mode) && (pattr->index == index)) {
				TimeoutunLock();
				//����Ϣ�ȴ���ʱ
				return 1;
			}
		}
	}
	TimeoutunLock();
	return 0;
}


/**
* @	������ʦ���������͹����������̡߳�
*/
void SERVERTCP(void *Param)
{
	unsigned char 	buf[256] 		= {0};
	char 			sendBuf[256] 	= {0};
	int 			cliSocket 		= 0;
	MSGHEAD 		msg;
	MSGHEAD 		sendmsg;
	int 			len 			= 0;
	int				addr 			= 0;
	int             sendlen         = 0;
	int firstpictime     = 0;
	int firstpic     = 0;
	send_class_info_t *ptSendClassInfo = &(g_tFinalClassInfo.tSendClassInfo);
	ITRACK_DynamicParams *pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams.TrackParms.dynamic_param;
	pthread_detach(pthread_self());

	while(1) {
		cliSocket = getServerSocket();

		if(cliSocket < 0) {
			DEBUG(DL_DEBUG, "get a error server socket [%d]\n", cliSocket);
			break;
		}

		len = recv(cliSocket, &msg, sizeof(MSGHEAD), 0);

		if(len <=  0) {

			close(cliSocket);
			sleep(1);

			strcpy(clientIp, "0.0.0.0");
			DEBUG(DL_DEBUG, "recv a error ! close  socket %d %s\n", len, strerror(errno));
			setServerSocket(-1);
			break;
		}

		switch(msg.nMsg) {
			case MSG_SET_MPMODE: {
				int mpMode = 0;
				len = recv(cliSocket, buf, msg.nLen - sizeof(MSGHEAD), 0);

				if(len < 1) {
					close(getServerSocket());
					setServerSocket(-1);
				}

				mpMode = *(int *)buf;
				printf("MSG_SET_MPMODE %d %d %x %x %x %x %d\n", *(int *)buf, len, buf[0], buf[1], buf[2], buf[3], ntohl(mpMode));

				//if((mpMode > 0) && (mpMode < 70))
				{
					gMpMode = mpMode;
					zlog_debug(ptrackzlog, RED"@@@MSG_SET_MPMODE gMpMode %d@@@@ ", gMpMode);
				}

				break;
			}

			case MSG_FARCTRL:
				len = recv(cliSocket, buf, msg.nLen - sizeof(MSGHEAD), 0);

				if(len < 1) {
					close(getServerSocket());
					setServerSocket(-1);
				}

				if(msg.nLen - sizeof(MSGHEAD) < 12) {
					addr = 1;
				} else {
					addr = (int)(buf[8] | buf[9] << 8 | buf[10] << 16 | buf[11] << 24);
				}

				DEBUG(DL_DEBUG, "########addr = %d\n", addr);

				if(addr == 1 || addr == 0) {
					FarCtrlCamera(0, buf, msg.nLen - sizeof(MSGHEAD));
				}

				if(addr == 2 || addr == 0) {
					if(getStudentSocket() < 0) {
						break;
					} else {

						memcpy(sendBuf, &msg, sizeof(MSGHEAD));
						memcpy(sendBuf + sizeof(MSGHEAD), buf, msg.nLen - sizeof(MSGHEAD));

						len = send(getStudentSocket(), sendBuf, msg.nLen, 0);

						if(len < 1) {
							close(getStudentSocket());
							setStudentSocket(-1);
						}
					}
				}

				break;

			case RECORD_START:
				len = recv(cliSocket, buf, 8, 0); //����Ԥ����4��byte

				if(len < 1) {
					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_START recv error[%d:%s]\n", errno, strerror(errno));
					//close(cliSocket);
					//WebSocket[nPos]=-1;
					//return;
				}

				DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_START recv len=%d,buf:%d,%d,%d,%d,%d,%d %d %d\n", len, buf[0], buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);

				firstpic     = *((int *)buf);
				firstpictime = *(int *)&buf[4];

				DEBUG(DL_DEBUG, "WEBMSG_RECORD_START switch vga![%d][%d]\n",firstpic,firstpictime);

				if(pdynamic_param->control_mode == AUTO_CONTROL)
				{
					SwitchFirstPic(firstpictime,firstpic);
					if((firstpictime > 0) && (firstpictime < 100)) 
					{
						DEBUG(DL_DEBUG, "WEBMSG_RECORD_START switch vga![%d][%d]\n",firstpic,firstpictime);
						zlog_debug(ptrackzlog, "[TEACHER] From RecSevr Switch cmd: switch_vga_flag \n");
					}
				}
				
				memset(&g_tClassInfo, 0, sizeof(track_class_info_t));
				memset(&g_tFinalClassInfo, 0, sizeof(final_class_info_t));

				g_tFinalClassInfo.nClassInfoFlag = 1;
				g_tFinalClassInfo.nClassStartTime = getCurrentTime();

				sendmsg.nLen  = sizeof(MSGHEAD) + sizeof(unsigned short);
				sendmsg.nMsg = RECORD_STATE;
				memcpy(sendBuf, (unsigned char *)&sendmsg, sizeof(MSGHEAD));
				len = sizeof(MSGHEAD);
				sendBuf[len] = 0x0001 & 0x00ff; //sendBuf[4]��sendBuf[5]��¼��״̬0x0001,2���ֽ�
				sendBuf[len + 1] = 0x0001 >> 8;

				len = send(cliSocket, sendBuf, sendmsg.nLen, 0);

				if(len < (int)sendmsg.nLen) {
					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_START send error[%d:%s]\n", errno, strerror(errno));
					//	close(cliSocket);
					//WebSocket[nPos]=-1;
					//return;
				}

				DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_START send OK len=%d\n", len);

				break;

			case RECORD_END:
				StopFirstPic();
				if(g_tFinalClassInfo.nClassInfoFlag == 0) {
					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy web end record error,g_ClassInfoFlag=0\n");
					sendmsg.nLen = sizeof(MSGHEAD) + sizeof(unsigned short);
					sendmsg.nMsg = RECORD_STATE;
					memcpy(sendBuf, (unsigned char *)&sendmsg, sizeof(MSGHEAD));
					len = sizeof(MSGHEAD);
					sendBuf[len] = 0; //sendBuf[4]��sendBuf[5]��¼��״̬0x0000,2���ֽ�
					sendBuf[len + 1] = 0;
					len = send(cliSocket, sendBuf, sendmsg.nLen, 0);

					if(len < (int)sendmsg.nLen) {
						DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_END state send error[%d:%s]\n", errno, strerror(errno));
						//close(cliSocket);
						//return;
					}
				} else {
					int i = 0;
					g_tFinalClassInfo.nClassEndTime = getCurrentTime();
					g_tFinalClassInfo.nClassInfoFlag = 0;
					sendlen = 0;
					sendlen += sizeof(MSGHEAD);

					if(ptSendClassInfo->nPerTimeLen != 0) {
						ptSendClassInfo->nAllTimesNum = (g_tFinalClassInfo.nClassEndTime - g_tFinalClassInfo.nClassStartTime) / 1000 / 60 / ptSendClassInfo->nPerTimeLen + 1;
					} else {
						ptSendClassInfo->nAllTimesNum = 1;
					}

					if(ptSendClassInfo->nAllTimesNum > MAX_INTERACTION_NUM) {
						ptSendClassInfo->nAllTimesNum = MAX_INTERACTION_NUM;

					}

					//�����ܴ���������ͳ�ƣ�ʱ�������ϴ�ǰMAX_STANDUP_NUM��
					ptSendClassInfo->nUpTimesSum = ptSendClassInfo->nStandupPos[0] + ptSendClassInfo->nStandupPos[1] + ptSendClassInfo->nStandupPos[2] + ptSendClassInfo->nStandupPos[3];

					if(ptSendClassInfo->nUpTimesSum < 2) {
						ptSendClassInfo->nClassType = 1;
					} else {
						ptSendClassInfo->nClassType = 2;
					}

					memcpy(sendBuf + sendlen, (unsigned char *)ptSendClassInfo, sizeof(send_class_info_t));
					sendlen += sizeof(send_class_info_t);

					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_END send classinfo:\n");
					DEBUG(DL_DEBUG, "nClassType=%d\n,nUpTimesSum=%d,\n",
					      ptSendClassInfo->nClassType,
					      ptSendClassInfo->nUpTimesSum);
					DEBUG(DL_DEBUG, "nStandupPos=%d,%d,%d,%d,\n",
					      ptSendClassInfo->nStandupPos[0],
					      ptSendClassInfo->nStandupPos[1],
					      ptSendClassInfo->nStandupPos[2],
					      ptSendClassInfo->nStandupPos[3]);
					DEBUG(DL_DEBUG, "nUpToPlatformTimes=%d\n,nTeacherToStudentsAreaTimes=%d\n,nPerTimeLen=%d\n,nAllTimesNum=%d,\n",
					      ptSendClassInfo->nUpToPlatformTimes,
					      ptSendClassInfo->nTeacherToStudentsAreaTimes,
					      ptSendClassInfo->nPerTimeLen,
					      ptSendClassInfo->nAllTimesNum);
					DEBUG(DL_DEBUG, "nInteractionNum=");

					for(i = 0; i < ptSendClassInfo->nAllTimesNum; i++) {
						DEBUG(DL_DEBUG, "%d,", ptSendClassInfo->nInteractionNum[i]);
					}

					DEBUG(DL_DEBUG, "\nnStandupTimePoint=");

					for(i = 0; i < ptSendClassInfo->nUpTimesSum; i++) {
						DEBUG(DL_DEBUG, "%d,", ptSendClassInfo->nStandupTimePoint[i]);
					}

					sendmsg.nLen = sendlen;
					sendmsg.nMsg = RECORD_SENDCLASSINFO;
					memcpy(sendBuf, (unsigned char *)&sendmsg, sizeof(MSGHEAD));
					len = send(cliSocket, sendBuf, sendmsg.nLen, 0);

					if(len < sendlen) {
						DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_END send classinfo error[%d:%s]\n", errno, strerror(errno));

						//return;
					}

					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_END send ok len=%d\n", len);
				}

				break;

			case MSG_TEACHER_HEART:
				gHeartLive = 0;
				printf("------------------->heart\n");
				break;

			default
					:
				break;
		}

	}
}

/**
* @	����web���͹����������̡߳�
*/
void WebTcpProcess(void *Param)
{
	unsigned char 	buf[256] 		= {0};
	unsigned char 	sendBuf[256] 	= {0};
	int 			cliSocket 		= 0;
	WEBMSGHEAD 		msg;
	WEBMSGHEAD 		msg_send;
	int 			len 			= 0;
	int 			sendlen 		= 0;

	int             nPos            = (int)Param;
	int i = 0;
	unsigned short nSendVgaFlag = 0;
	pthread_detach(pthread_self());
	send_class_info_t *ptSendClassInfo = &(g_tFinalClassInfo.tSendClassInfo);
	//	DEBUG(DL_DEBUG,"yyyyyyyyyyyyy WebTcpProcess npos=%d\n", nPos);

	while(1) {
		cliSocket = WebSocket[nPos];

		if(cliSocket < 0) {
			DEBUG(DL_DEBUG, "yyyyyyyyyyyyy get a error web socket %d\n", cliSocket);
			return;
		}

		len = recv(cliSocket, buf, sizeof(WEBMSGHEAD), 0);

		if(len < (int)sizeof(WEBMSGHEAD)) {

			DEBUG(DL_DEBUG, "yyyyyyyyyyyyy while recv error[%d:%s]\n", errno, strerror(errno));
			close(cliSocket);
			WebSocket[nPos] = -1;
			return;
		}

		DEBUG(DL_DEBUG, "yyyyyyyyyyyyy web recv %d,%d,%d,%d\n", buf[0], buf[1], buf[2], buf[3]);
		memcpy((unsigned char *)&msg, buf, sizeof(WEBMSGHEAD));
		DEBUG(DL_DEBUG, "yyyyyyyyyyyyy web recv msg.nLen=%d, msg.nType=%d\n", msg.nLen, msg.nType);

		switch(msg.nType) {
			case WEBMSG_RECORD_START:
				len = recv(cliSocket, buf, 4, 0); //����Ԥ����4��byte

				if(len < 1) {
					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_START recv error[%d:%s]\n", errno, strerror(errno));
					close(cliSocket);
					WebSocket[nPos] = -1;
					return;
				}

				DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_START recv len=%d,buf:%d,%d\n", len, buf[0], buf[1]);
				nSendVgaFlag = *((unsigned short *)buf);

				if(nSendVgaFlag == 1) {
					//teacherTracerMove(SWITCH_VGA);
					SetVgaState();
					g_track_strategy_info.switch_vga_flag = 1;
					DEBUG(DL_DEBUG, "WEBMSG_RECORD_START switch vga!\n");

					zlog_debug(ptrackzlog, "[TEACHER] From RecSevr Switch cmd: switch_vga_flag \n");
				}

				memset(&g_tClassInfo, 0, sizeof(track_class_info_t));
				memset(&g_tFinalClassInfo, 0, sizeof(final_class_info_t));

				g_tFinalClassInfo.nClassInfoFlag = 1;
				g_tFinalClassInfo.nClassStartTime = getCurrentTime();

				msg_send.nLen  = sizeof(WEBMSGHEAD) + sizeof(unsigned short);
				msg_send.nType = WEBMSG_RECORD_STATE;
				memcpy(sendBuf, (unsigned char *)&msg_send, sizeof(WEBMSGHEAD));
				sendBuf[4] = 0x0001 & 0x00ff; //sendBuf[4]��sendBuf[5]��¼��״̬0x0001,2���ֽ�
				sendBuf[5] = 0x0001 >> 8;

				len = send(cliSocket, sendBuf, msg_send.nLen, 0);

				if(len < (int)msg_send.nLen) {
					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_START send error[%d:%s]\n", errno, strerror(errno));
					close(cliSocket);
					WebSocket[nPos] = -1;
					return;
				}

				DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_START send OK len=%d\n", len);

				break;

			case WEBMSG_RECORD_END:
				if(g_tFinalClassInfo.nClassInfoFlag == 0) {
					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy web end record error,g_ClassInfoFlag=0\n");
					msg_send.nLen = sizeof(WEBMSGHEAD) + sizeof(unsigned short);
					msg_send.nType = WEBMSG_RECORD_STATE;
					memcpy(sendBuf, (unsigned char *)&msg_send, sizeof(WEBMSGHEAD));
					sendBuf[4] = 0; //sendBuf[4]��sendBuf[5]��¼��״̬0x0000,2���ֽ�
					sendBuf[5] = 0;
					len = send(cliSocket, sendBuf, msg_send.nLen, 0);

					if(len < (int)msg_send.nLen) {
						DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_END state send error[%d:%s]\n", errno, strerror(errno));
						close(cliSocket);
						WebSocket[nPos] = -1;
						return;
					}
				} else {
					g_tFinalClassInfo.nClassEndTime = getCurrentTime();
					g_tFinalClassInfo.nClassInfoFlag = 0;
					sendlen = 0;
					sendlen += sizeof(WEBMSGHEAD);

					if(ptSendClassInfo->nPerTimeLen != 0) {
						ptSendClassInfo->nAllTimesNum = (g_tFinalClassInfo.nClassEndTime - g_tFinalClassInfo.nClassStartTime) / 1000 / 60 / ptSendClassInfo->nPerTimeLen + 1;
					} else {
						ptSendClassInfo->nAllTimesNum = 1;
					}

					if(ptSendClassInfo->nAllTimesNum > MAX_INTERACTION_NUM) {
						ptSendClassInfo->nAllTimesNum = MAX_INTERACTION_NUM;

					}

					//�����ܴ���������ͳ�ƣ�ʱ�������ϴ�ǰMAX_STANDUP_NUM��
					ptSendClassInfo->nUpTimesSum = ptSendClassInfo->nStandupPos[0] + ptSendClassInfo->nStandupPos[1] + ptSendClassInfo->nStandupPos[2] + ptSendClassInfo->nStandupPos[3];

					if(ptSendClassInfo->nUpTimesSum < 2) {
						ptSendClassInfo->nClassType = 1;
					} else {
						ptSendClassInfo->nClassType = 2;
					}

					memcpy(sendBuf + sendlen, (unsigned char *)ptSendClassInfo, sizeof(send_class_info_t));
					sendlen += sizeof(send_class_info_t);

					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_END send classinfo:\n");
					DEBUG(DL_DEBUG, "nClassType=%d\n,nUpTimesSum=%d,\n",
					      ptSendClassInfo->nClassType,
					      ptSendClassInfo->nUpTimesSum);
					DEBUG(DL_DEBUG, "nStandupPos=%d,%d,%d,%d,\n",
					      ptSendClassInfo->nStandupPos[0],
					      ptSendClassInfo->nStandupPos[1],
					      ptSendClassInfo->nStandupPos[2],
					      ptSendClassInfo->nStandupPos[3]);
					DEBUG(DL_DEBUG, "nUpToPlatformTimes=%d\n,nTeacherToStudentsAreaTimes=%d\n,nPerTimeLen=%d\n,nAllTimesNum=%d,\n",
					      ptSendClassInfo->nUpToPlatformTimes,
					      ptSendClassInfo->nTeacherToStudentsAreaTimes,
					      ptSendClassInfo->nPerTimeLen,
					      ptSendClassInfo->nAllTimesNum);
					DEBUG(DL_DEBUG, "nInteractionNum=");

					for(i = 0; i < ptSendClassInfo->nAllTimesNum; i++) {
						DEBUG(DL_DEBUG, "%d,", ptSendClassInfo->nInteractionNum[i]);
					}

					DEBUG(DL_DEBUG, "\nnStandupTimePoint=");

					for(i = 0; i < ptSendClassInfo->nUpTimesSum; i++) {
						DEBUG(DL_DEBUG, "%d,", ptSendClassInfo->nStandupTimePoint[i]);
					}

					msg_send.nLen = sendlen;
					msg_send.nType = WEBMSG_RECORD_SENDCLASSINFO;
					memcpy(sendBuf, (unsigned char *)&msg_send, sizeof(WEBMSGHEAD));
					len = send(cliSocket, sendBuf, msg_send.nLen, 0);

					if(len < sendlen) {
						DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_END send classinfo error[%d:%s]\n", errno, strerror(errno));
						close(cliSocket);
						WebSocket[nPos] = -1;
						return;
					}

					DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_END send ok len=%d\n", len);
				}

				break;

			default
					:
				DEBUG(DL_DEBUG, "yyyyyyyyyyyyy unrecognized web msg.nType %d\n", msg.nType);
				break;
		}

	}
}

/**
* @	����ʦ����Ӧ�������ļ����ˣ������Ƿ��б����������ӣ��еĻ��򴴽�
* @	���������߳�
*/
static void TEACHER_TRACETCP(void *param)
{

	DEBUG(DL_DEBUG, "create TEACHER_TRACETCP\n");
	char buffer[50] = {0};
	pthread_t TCPCmd;
	int clientSocket;
	int listenSocket;
	struct sockaddr_in clientAddr, srvAddr;
	int opt = 1;
	pthread_detach(pthread_self());
	setServerSocket(-1);
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(listenSocket < 0) {
		DEBUG(DL_DEBUG, "create socket error %s\n", strerror(errno));
		return ;
	} else {
		DEBUG(DL_DEBUG, "create socket [%d]\n", listenSocket);
	}

	bzero(&srvAddr, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	get_local_ip("eth1", buffer);
	DEBUG(DL_DEBUG, "TEACHER_TRACETCP [%s]\n", buffer);
	srvAddr.sin_addr.s_addr = inet_addr(buffer);
	srvAddr.sin_port = htons(ENCODER_TCP_PORT);
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(listenSocket, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0) {
		DEBUG(DL_DEBUG, "bind: error \n");
	}

	if(listen(listenSocket, 10) < 0) {
		DEBUG(DL_DEBUG, "listen: error\n");
	}


	DEBUG(DL_DEBUG, "TEACHER_TRACETCP successful.\n");

	while(1) {
		int result, len, nSize = 1, nLen;
		len = sizeof(clientAddr);
		clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, (unsigned int *)&len);
		DEBUG(DL_DEBUG, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@########################  encode connet you %d %s %d\n", clientSocket, inet_ntoa(clientAddr.sin_addr), listenSocket);

		if(clientSocket < 0) {
			DEBUG(DL_DEBUG, "@@@client socket error[%s]\n", strerror(errno));
			sleep(2);
			continue;
		}

		if(getServerSocket() >= 0 || gEduKit->Start == 0) {
			printf("Encode Max connect Num %d\n", gEduKit->Start);

			close(clientSocket);
			sleep(2);
		} else {

			strcpy(clientIp, inet_ntoa(clientAddr.sin_addr));
			setServerSocket(clientSocket);

			if((setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
			               sizeof(nSize))) == -1) {
				DEBUG(DL_DEBUG, "setsockopt failed");
			}

			nSize 	= 0;
			nLen 	= sizeof(nSize);
			result 	= getsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, &nSize , (DWORD *)&nLen);

			if(result == 1) {
				DEBUG(DL_DEBUG, "getsockopt()  socket:%d  result:%d\n", clientSocket, result);
			}

			nSize = 1;

			if(setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize))) {
				DEBUG(DL_DEBUG, "set IPPROTOT error\n");
			}

			result = pthread_create(&TCPCmd, NULL, (void *)SERVERTCP, NULL);
		}
	}

}

/**
* @	����ʦ������������������̣߳�����ʦ��������������
*/
static void HEART(void *param)
{
	int serverSocket;
	MSGHEAD msg;
	pthread_detach(pthread_self());
	msg.nLen = sizeof(MSGHEAD);
	msg.nMsg = 111;
	unsigned int heartlive = 0;

	while(1) {

		int len;
		serverSocket = getServerSocket();

		if(serverSocket < 1) {

			gHeartLive = 0;
			DEBUG(DL_DEBUG, "socket to server not connect[%d]\n", serverSocket);
		} else {
			len = send(serverSocket, &msg, sizeof(MSGHEAD), 0);

			if(len < 1) {
				close(getServerSocket());
				setServerSocket(-1);
				DEBUG(DL_DEBUG, "send to server heart error\n");
			} else {
				DEBUG(DL_DEBUG, "###########heart server!\n");
			}

			gHeartLive++;

			if(gHeartLive > 1) {
				close(getServerSocket());
				setServerSocket(-1);
				gHeartLive = 0;
				DEBUG(DL_DEBUG, "###########heart server close########!\n");
			}

		}

		sleep(3);

	}

}



/**
* @	�����ѧ�����������Ƿ��������粻�������ر�socket
*/
void studentTraceHeart(void *param)
{
	pthread_detach(pthread_self());

	while(1) {
		sleep(15);

		if(studentTraceTCPStatus) {
			studentTraceTCPStatus = 0;
			continue;
		} else {
			if(getStudentSocket() >= 0) {
				shutdown(getStudentSocket(), SHUT_RDWR);
				setStudentSocket(-1);
			}

			continue;
		}

	}
}

/**
* @	��ʦ����ѧ���������̣߳�connectѧ�������Լ��Ͽ���������������ѧ������������Ϣ
*/
static void STUDENT_TRACETCP(void *param)
{
	int studentSocket;
	char recvbuf[256], sendbuf[256];
	MSGHEAD msg;
	struct sockaddr_in student_addr;
	pthread_t rid;

	int		send_students = 0;
	int		recv_value		= 0;
	int recvLen, len;
	int i = 0, nSumCur = 0, nTimePointWritePos = 0, nWriteNum = 0;
	track_class_info_t tRecvClassInfo;
	unsigned long long llCurTime = 0, llLastStandTime = 0;
	unsigned int nInteractionPos = 0;
	send_class_info_t *ptSendClassInfo = &(g_tFinalClassInfo.tSendClassInfo);
	pthread_detach(pthread_self());

	studentSocket = socket(AF_INET, SOCK_STREAM, 0);

	student_addr.sin_family = AF_INET;
	student_addr.sin_port = htons(student_PORT);
	inet_pton(AF_INET, STUDENTTRACE_IP, &student_addr.sin_addr);

	DEBUG(DL_DEBUG, "student tcp tread create");

	DEBUG(DL_DEBUG, "@@@@@@@@%s %d", STUDENTTRACE_IP, student_PORT);

	while(-1 == connect(studentSocket, (struct sockaddr *)&student_addr, sizeof(struct sockaddr))) {
		DEBUG(DL_DEBUG, "student tcp connect fail\n");
		//close(studentSocket);
		//studentSocket = socket(AF_INET, SOCK_STREAM, 0);
		sleep(5);

	}

	//pthread_create(&rid,NULL,(void *)studentTraceHeart,NULL);
	setStudentSocket(studentSocket);

	while(1) {
		if(getStudentSocket() < 0) {
			do {
				DEBUG(DL_DEBUG, "########%s %d", STUDENTTRACE_IP, student_PORT);
				DEBUG(DL_DEBUG, "student tcp error");

				studentSocket = socket(AF_INET, SOCK_STREAM, 0);
				setStudentSocket(studentSocket);
				sleep(5);
			} while(-1 == connect(studentSocket, (struct sockaddr *)&student_addr, sizeof(struct sockaddr)));

			setStudentSocket(studentSocket);

			if(g_track_encode_info.track_status) {
				send_students = 1;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
			} else {
				send_students = 0;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
			}
		}

		recvLen = recv(studentSocket, &msg, sizeof(MSGHEAD), 0);

		//DEBUG(DL_DEBUG,"########## student recvLen = %d\n",recvLen);
		if(recvLen < 1) {
			DEBUG(DL_DEBUG, "recv student data error\n");
			close(studentSocket);
			setStudentSocket(-1);
		}

		switch(msg.nMsg) {
			case MSG_TEACHER_HEART:
				studentTraceTCPStatus = 1;
				//��ʦ����ѧ������������
				len = send(studentSocket, &msg, sizeof(MSGHEAD), 0);

				if(len < 1) {
					DEBUG(DL_ERROR, "send HEART to student error\n");
					close(studentSocket);
					setStudentSocket(-1);
				}

				break;

				//�õ�ѧ�����ϱ��Ŀ�����Ϣ
			case MSG_SEND_CLASSINFO:
				recvLen = recv(studentSocket, recvbuf, ntohs(msg.nLen) - sizeof(MSGHEAD), 0);
				memcpy(&tRecvClassInfo, recvbuf, sizeof(track_class_info_t));
				DEBUG(DL_DEBUG, "yyyyyyyyyy recv student classinfo %d,%d,%d,%d,%d,%d\n", tRecvClassInfo.nStandupPos[0]
				      , tRecvClassInfo.nStandupPos[1]
				      , tRecvClassInfo.nStandupPos[2]
				      , tRecvClassInfo.nStandupPos[3]
				      , tRecvClassInfo.nUpToPlatformTimes, tRecvClassInfo.nDownToStudentsAreaTimes);


				if(tRecvClassInfo.nDownToStudentsAreaTimes != 0) {
					if(g_track_strategy_info.teacher_panorama_flag == 1) {
						DEBUG(DL_DEBUG, "yyyyyyyyyyy recv student nDownToStudentsAreaTimes=%d\n", tRecvClassInfo.nDownToStudentsAreaTimes);
						g_track_strategy_info.teacher_panorama_flag = 0;
						g_track_strategy_info.switch_cmd_author = AUTHOR_STUDENTS;
						g_track_strategy_info.send_cmd  = SWITCH_TEATHER;

						zlog_debug(ptrackzlog, "[TEACHER] From Stu Switch cmd: SWITCH_TEATHER  author: AUTHOR_STUDENTS\n");
					}
				}

				if(g_track_strategy_info.students_track_flag == 0) {
					DEBUG(DL_DEBUG, "yyyyyyyyyyy recv student g_track_strategy_info.students_track_flag=%d,g_track_strategy_info.teacher_panorama_flag=%d\n", g_track_strategy_info.students_track_flag, g_track_strategy_info.teacher_panorama_flag);
					break;
				}


				if(tRecvClassInfo.nUpToPlatformTimes != 0) {
					DEBUG(DL_DEBUG, "yyyyyyyyyyy recv student nUpToPlatformTimes=%d\n", tRecvClassInfo.nUpToPlatformTimes);
					//g_track_strategy_info.teacher_panorama_flag=1;
					//g_track_strategy_info.switch_cmd_author = AUTHOR_STUDENTS;
					//g_track_strategy_info.send_cmd=SWITCH_TEACHER_PANORAMA;
				}

				if(g_tFinalClassInfo.nClassInfoFlag == 1) {
					llCurTime = (getCurrentTime() - g_tFinalClassInfo.nClassStartTime) / 1000; //��ʾ��
					g_tClassInfo.nStandupPos[0] += tRecvClassInfo.nStandupPos[0];
					g_tClassInfo.nStandupPos[1] += tRecvClassInfo.nStandupPos[1];
					g_tClassInfo.nStandupPos[2] += tRecvClassInfo.nStandupPos[2];
					g_tClassInfo.nStandupPos[3] += tRecvClassInfo.nStandupPos[3];
					g_tClassInfo.nUpToPlatformTimes += tRecvClassInfo.nUpToPlatformTimes;
					g_tClassInfo.nDownToStudentsAreaTimes += tRecvClassInfo.nDownToStudentsAreaTimes;

					ptSendClassInfo->nUpToPlatformTimes += tRecvClassInfo.nUpToPlatformTimes;
					ptSendClassInfo->nPerTimeLen = CLASSINFO_PER_TIME;

					if(llCurTime - llLastStandTime >= g_track_strategy_info.students_panorama_switch_near_time) {
						ptSendClassInfo->nStandupPos[0] += tRecvClassInfo.nStandupPos[0];
						ptSendClassInfo->nStandupPos[1] += tRecvClassInfo.nStandupPos[1];
						ptSendClassInfo->nStandupPos[2] += tRecvClassInfo.nStandupPos[2];
						ptSendClassInfo->nStandupPos[3] += tRecvClassInfo.nStandupPos[3];
						nSumCur = tRecvClassInfo.nStandupPos[0] + tRecvClassInfo.nStandupPos[1] + tRecvClassInfo.nStandupPos[2] + tRecvClassInfo.nStandupPos[3];

						if(ptSendClassInfo->nPerTimeLen != 0) {
							nInteractionPos = llCurTime / 60 / ptSendClassInfo->nPerTimeLen;

							if(nInteractionPos < MAX_INTERACTION_NUM) {
								ptSendClassInfo->nInteractionNum[nInteractionPos] += nSumCur;
							} else {
								ptSendClassInfo->nInteractionNum[MAX_INTERACTION_NUM - 1] += nSumCur;
							}

							if((nSumCur + ptSendClassInfo->nUpTimesSum) > MAX_STANDUP_NUM) {
								nWriteNum = MAX_STANDUP_NUM - ptSendClassInfo->nUpTimesSum;
							} else {
								nWriteNum = nSumCur;
							}

							for(i = 0; i < nWriteNum; i++) {
								ptSendClassInfo->nStandupTimePoint[ptSendClassInfo->nUpTimesSum + i] = (unsigned short)llCurTime;
							}

							ptSendClassInfo->nUpTimesSum += nWriteNum;
						}
					}

					llLastStandTime = llCurTime;
				}

				break;

			case MSG_TRACKAUTO: {
				recvLen = recv(studentSocket, recvbuf, msg.nMsg - sizeof(MSGHEAD), 0);

				if(recvLen < 0) {
					DEBUG(DL_DEBUG, "recv student data error\n");
					close(studentSocket);
					setStudentSocket(-1);
				} else {
					recv_value = (int)recvbuf[0];
#if 1

					//zlog_debug(ptrackzlog,"[TEACHER] From Stu Switch cmd: %s  author: AUTHOR_STUDENTS\n",\
					//    (recv_value == 1)?"SWITCH_TEATHER":"SWITCH_STUDENTS",g_track_strategy_info.students_track_flag);
					if(g_track_strategy_info.students_track_flag == 1) {
						DEBUG(DL_DEBUG, "recv students recv_value = %d\n", recv_value);
						g_track_strategy_info.switch_cmd_author = AUTHOR_STUDENTS;
						g_track_strategy_info.send_cmd			= recv_value;

						zlog_debug(ptrackzlog, "[TEACHER] From Stu Switch cmd: %s  author: AUTHOR_STUDENTS\n", \
						           (recv_value == 1) ? "SWITCH_TEATHER" : "SWITCH_STUDENTS");

						if(recv_value == SWITCH_STUDENTS) {
							StudentUp = 1;
						} else if(recv_value == SWITCH_TEATHER) {
							StudentUp = 0;
						}
					}

#else

					if(SWITCH_TEATHER == recv_value) {
						DEBUG(DL_DEBUG, "g_track_encode_info.track_status = %d\n", g_track_encode_info.track_status);

						//�����ʦ�����ڸ���״̬
						if(g_track_encode_info.track_status) {
							teacherTracerMove(SWITCH_TEATHER);
						} else {
							switch_students_time = 0;
							teacherTracerMove(SWITCH_STUDENTS);
						}
					} else {
						if(g_track_encode_info.track_status) {
							sleep(1);
							switch_students_time = 0;
							teacherTracerMove(SWITCH_STUDENTS);
						}
					}

#endif

				}

				break;
			}

			default
					:
				break;
		}
	}
}

/**
* @	�л����ԣ���Ҫ�Ǵ����ж�Ӧ������ʦ����Ӧ�����������л��Ǹ���λ������
* @	�Լ���֪ѧ������Ҫ��ʲô����
*2��λ :��ʦ+ ѧ��
*/
void strategy2(void *pParam)
{
	int				send_students = 0;

	if(g_track_encode_info.track_status == 0) {
		g_track_strategy_timeinfo.switch_flag = 1; // 1�����л�,0 ��ʱδ�� �������л�
		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_info.students_track_flag = 0;

		if(g_track_strategy_info.switch_vga_flag == 1) {
			g_track_strategy_timeinfo.vga_delaytime++;

			if(g_track_strategy_timeinfo.vga_delaytime >= g_track_strategy_info.vga_keep_time * 10) {
				send_students = 0;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
				teacherTracerMove(SWITCH_STUDENTS);
				DEBUG(DL_DEBUG, "yyyyy vga switch students!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS;
				g_track_strategy_info.switch_vga_flag = 0;
				g_track_strategy_timeinfo.vga_delaytime = 0;
			}
		}

		if(SWITCH_STUDENTS != g_track_strategy_info.last_send_cmd) {
			send_students = 0;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
			teacherTracerMove(SWITCH_STUDENTS);
			DEBUG(DL_DEBUG, "yyyyy lost teacher track!switch students!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS;
		}

		usleep(100000);
		return;
	}

	g_track_strategy_info.students_track_flag = 1;

	if(g_track_strategy_info.switch_vga_flag == 1) {
		g_track_strategy_timeinfo.vga_delaytime++;

		if(g_track_strategy_timeinfo.vga_delaytime >= g_track_strategy_info.vga_keep_time * 10) {
			if(g_track_strategy_info.send_cmd == 0) {
				g_track_strategy_info.send_cmd = SWITCH_TEATHER;
				g_track_strategy_info.switch_cmd_author = AUTHOR_TEACHER;
				DEBUG(DL_DEBUG, "yyyyy vga over,send_cmd=teacher;\n");
			}

			g_track_strategy_timeinfo.switch_flag = 1;
			g_track_strategy_info.switch_vga_flag = 0;
			g_track_strategy_timeinfo.vga_delaytime = 0;
		} else {
			usleep(100000);
			return;
		}
	}

	//�л���ʱ������ʱ
	if(g_track_strategy_timeinfo.switch_flag == 0) {
		g_track_strategy_timeinfo.delaytime++;

		if(g_track_strategy_timeinfo.delaytime >= g_track_strategy_timeinfo.switch_delaytime * 10) {
			g_track_strategy_timeinfo.switch_flag = 1;
		} else {
			usleep(100000);
			return;
		}
	}

	//�յ��л���ʦ���������
	if(SWITCH_TEATHER == g_track_strategy_info.send_cmd) {
		if(AUTHOR_TEACHER == g_track_strategy_info.switch_cmd_author) {
			//��ʦ��������  ��Ҫ��ѧ�������͸�������
			send_students = 1;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
		}

		teacherTracerMove(SWITCH_TEATHER);
		DEBUG(DL_DEBUG, "1switch teacher!\n");
		g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_timeinfo.switch_flag = 0;
		g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.teacher_switch_students_delay_time;
		g_track_strategy_info.send_cmd = 0;

	}
	//�յ��л�ѧ�����������
	else if(SWITCH_STUDENTS == g_track_strategy_info.send_cmd) {
		if(AUTHOR_TEACHER == g_track_strategy_info.switch_cmd_author) {
			send_students = 0;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
			teacherTracerMove(SWITCH_STUDENTS);
			DEBUG(DL_DEBUG, "3switch students!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS;
		} else if(AUTHOR_STUDENTS == g_track_strategy_info.switch_cmd_author) {
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
		}

		g_track_strategy_info.send_cmd = 0;
		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_timeinfo.switch_flag = 0;
		g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_panorama_switch_near_time;
	} else {
		if(g_track_strategy_info.last_send_cmd == SWITCH_STUDENTS_PANORAMA) {
			teacherTracerMove(SWITCH_STUDENTS);
			DEBUG(DL_DEBUG, "3switch students!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS;
			g_track_strategy_info.send_cmd = 0;
			g_track_strategy_timeinfo.delaytime = 0;
			g_track_strategy_timeinfo.switch_flag = 0;
			g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_near_keep_time;
		}
	}

	usleep(100000);
	return;
}

/**
* @	�л����ԣ���Ҫ�Ǵ����ж�Ӧ������ʦ����Ӧ�����������л��Ǹ���λ������
* @	�Լ���֪ѧ������Ҫ��ʲô����
*4��λ :��ʦ+ ѧ�� +ѧ��ȫ�� +����
*/
void strategy4(void *pParam)
{
	int				send_students = 0;

	if(g_track_encode_info.track_status == 0) {
		g_track_strategy_timeinfo.students_panorama_switch_near_time = 0;
		g_track_strategy_timeinfo.blackboard_time1 		= 0;
		g_track_strategy_timeinfo.leave_blackboard_time1	= 0;
		g_track_strategy_timeinfo.students_down_time = 0;
		g_track_strategy_timeinfo.students_to_platform_time = 0;
		g_track_strategy_timeinfo.switch_flag = 1; // 1�����л�,0 ��ʱδ�� �������л�
		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_info.teacher_panorama_flag = 0;
		g_track_strategy_info.students_track_flag = 0;

		if(g_track_strategy_info.switch_vga_flag == 1) {
			g_track_strategy_timeinfo.vga_delaytime++;

			if(g_track_strategy_timeinfo.vga_delaytime >= g_track_strategy_info.vga_keep_time * 10) {
				send_students = 0;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
				teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
				DEBUG(DL_DEBUG, "yyyyy vga switch students panorama!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
				g_track_strategy_info.switch_vga_flag = 0;
				g_track_strategy_timeinfo.vga_delaytime = 0;
			}
		}

		if(SWITCH_STUDENTS_PANORAMA != g_track_strategy_info.last_send_cmd) {
			send_students = 0;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
			teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
			DEBUG(DL_DEBUG, "yyyyy lost teacher track!switch students panorama!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
		}

		usleep(100000);
		return;
	}

	g_track_strategy_info.students_track_flag = 1;

	if(g_track_strategy_info.switch_vga_flag == 1) {
		g_track_strategy_timeinfo.vga_delaytime++;

		if(g_track_strategy_timeinfo.vga_delaytime >= g_track_strategy_info.vga_keep_time * 10) {
			if(g_track_strategy_info.send_cmd == 0) {
				g_track_strategy_info.send_cmd = SWITCH_TEATHER;
				g_track_strategy_info.switch_cmd_author = AUTHOR_TEACHER;
				DEBUG(DL_DEBUG, "yyyyy vga over,send_cmd=teacher;\n");
			}

			g_track_strategy_timeinfo.switch_flag = 1;
			g_track_strategy_info.switch_vga_flag = 0;
			g_track_strategy_timeinfo.vga_delaytime = 0;
		} else {
			usleep(100000);
			return;
		}
	}

	//�л���ʱ������ʱ
	if(g_track_strategy_timeinfo.switch_flag == 0) {
		g_track_strategy_timeinfo.delaytime++;

		if(g_track_strategy_timeinfo.delaytime >= g_track_strategy_timeinfo.switch_delaytime * 10) {
			g_track_strategy_timeinfo.switch_flag = 1;
		} else {
			usleep(100000);
			return;
		}
	}

	//�յ��л���ʦ���������
	if(SWITCH_TEATHER == g_track_strategy_info.send_cmd) {
		if(AUTHOR_TEACHER == g_track_strategy_info.switch_cmd_author) {
			//��ʦ��������  ��Ҫ��ѧ�������͸�������
			send_students = 1;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
		}

		if(g_track_strategy_info.blackboard_region_flag1) {
			teacherTracerMove(SWITCH_BLACKBOARD1);
			DEBUG(DL_DEBUG, "4switch blackboard!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD1;
		} else {
			teacherTracerMove(SWITCH_TEATHER);
			DEBUG(DL_DEBUG, "1switch teacher!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
		}

		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_timeinfo.switch_flag = 0;
		g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.teacher_switch_students_delay_time;
		g_track_strategy_info.send_cmd = 0;

	}
	//�յ��л�ѧ�����������
	else if(SWITCH_STUDENTS == g_track_strategy_info.send_cmd) {
		if(AUTHOR_TEACHER == g_track_strategy_info.switch_cmd_author) {
			send_students = 0;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);

			teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
			DEBUG(DL_DEBUG, "5switch students panorama!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
			g_track_strategy_info.send_cmd = 0;
			g_track_strategy_timeinfo.delaytime = 0;
			g_track_strategy_timeinfo.switch_flag = 0;
			g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_panorama_switch_near_time;
		} else {
			if(g_track_strategy_timeinfo.on_teacher_flag == 0) {
				teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
				DEBUG(DL_DEBUG, "55switch students panorama!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
				g_track_strategy_info.send_cmd = 0;
				g_track_strategy_timeinfo.delaytime = 0;
				g_track_strategy_timeinfo.switch_flag = 0;
				g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_panorama_switch_near_time;

			} else {
				g_track_strategy_timeinfo.on_teacher_delaytime++;

				if(g_track_strategy_timeinfo.on_teacher_delaytime >= g_track_strategy_info.teacher_switch_students_delay_time * 10) {
					g_track_strategy_timeinfo.on_teacher_flag = 0;
				}
			}
		}
	}
	//û���յ��κ��л����������ʦ�Ͱ�����л�
	else {
		if(SWITCH_STUDENTS_PANORAMA == g_track_strategy_info.last_send_cmd) {
			teacherTracerMove(SWITCH_STUDENTS);
			DEBUG(DL_DEBUG, "3switch students near!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS;
			g_track_strategy_timeinfo.delaytime = 0;
			g_track_strategy_timeinfo.switch_flag = 0;
			g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_near_keep_time;
		} else if(SWITCH_STUDENTS == g_track_strategy_info.last_send_cmd) {
			//�ȴ�ѧ������
			g_track_strategy_timeinfo.switch_flag = 1;
		} else {
			g_track_strategy_timeinfo.switch_flag = 1;

			if(g_track_strategy_timeinfo.on_teacher_flag == 1) {
				g_track_strategy_timeinfo.on_teacher_delaytime++;

				if(g_track_strategy_timeinfo.on_teacher_delaytime >= g_track_strategy_info.teacher_switch_students_delay_time * 10) {
					g_track_strategy_timeinfo.on_teacher_flag = 0;
				}
			}

			if(SWITCH_BLACKBOARD1 == g_track_strategy_info.last_send_cmd) {
				if(g_track_strategy_info.blackboard_region_flag1) {
					g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
				} else {
					g_track_strategy_timeinfo.blackboard_time1 = 0;
					g_track_strategy_timeinfo.leave_blackboard_time1 ++;

					if(g_track_strategy_timeinfo.leave_blackboard_time1 >= g_track_strategy_info.teacher_leave_blackboard_time1 * 10) {
						teacherTracerMove(SWITCH_TEATHER);
						DEBUG(DL_DEBUG, "11switch teacher!\n");
						g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
						g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
						g_track_strategy_timeinfo.on_teacher_delaytime = 0;
						g_track_strategy_timeinfo.on_teacher_flag = 1;
					}
				}
			} else if(SWITCH_TEATHER == g_track_strategy_info.last_send_cmd) {
				if(g_track_strategy_info.blackboard_region_flag1) {
					g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
					g_track_strategy_timeinfo.blackboard_time1 ++;

					if(g_track_strategy_timeinfo.blackboard_time1 >= g_track_strategy_info.teacher_blackboard_time1 * 10) {
						teacherTracerMove(SWITCH_BLACKBOARD1);
						DEBUG(DL_DEBUG, "44switch blackboard!\n");
						g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD1;
						g_track_strategy_timeinfo.blackboard_time1 = 0;
						g_track_strategy_timeinfo.on_teacher_delaytime = 0;
						g_track_strategy_timeinfo.on_teacher_flag = 1;
					}
				}

			}
		}
	}

	usleep(100000);
	return;
}
/**
* @	�л����ԣ���Ҫ�Ǵ����ж�Ӧ������ʦ����Ӧ�����������л��Ǹ���λ������
* @	�Լ���֪ѧ������Ҫ��ʲô����
*5��λ1 :��ʦ+ ѧ�� +ѧ��ȫ�� +����+��ʦȫ��
*/
void strategy5_1(void *pParam)
{
	int				send_students = 0;

	if(g_track_encode_info.track_status == 0) {
		g_track_strategy_timeinfo.teacher_panorama_flag = 0;
		g_track_strategy_timeinfo.students_panorama_switch_near_time = 0;
		g_track_strategy_timeinfo.blackboard_time1 		= 0;
		g_track_strategy_timeinfo.leave_blackboard_time1	= 0;
		g_track_strategy_timeinfo.panorama_time			= 0;
		g_track_strategy_timeinfo.leave_panorama_time 	= 0;
		g_track_strategy_timeinfo.teacher_panorama_switch_near_time = 0;
		g_track_strategy_timeinfo.students_down_time = 0;
		g_track_strategy_timeinfo.students_to_platform_time = 0;
		g_track_strategy_timeinfo.switch_flag = 1; // 1�����л�,0 ��ʱδ�� �������л�
		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_info.teacher_panorama_flag = 0;
		g_track_strategy_info.students_track_flag = 0;

		if(g_track_strategy_info.switch_vga_flag == 1) {
			g_track_strategy_timeinfo.vga_delaytime++;

			if(g_track_strategy_timeinfo.vga_delaytime >= g_track_strategy_info.vga_keep_time * 10) {
				send_students = 0;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
				teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
				DEBUG(DL_DEBUG, "yyyyy vga switch students panorama!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
				g_track_strategy_info.switch_vga_flag = 0;
				g_track_strategy_timeinfo.vga_delaytime = 0;
			}
		}

		if(SWITCH_STUDENTS_PANORAMA == g_track_strategy_info.last_send_cmd) {
			g_track_strategy_timeinfo.stu_time++;
			g_track_strategy_timeinfo.mv_time = 0;
		} else if(SWITCH_TEACHER_PANORAMA == g_track_strategy_info.last_send_cmd) {
			g_track_strategy_timeinfo.stu_time = 0;
			g_track_strategy_timeinfo.mv_time++;
		} else {
			//����ʱlast_send_cmd=0;
			g_track_strategy_timeinfo.mv_time = g_track_strategy_info.mv_keep_time * 10;
		}

		if(g_track_strategy_info.position1_mv_flag == 1) {
			if(SWITCH_TEACHER_PANORAMA != g_track_strategy_info.last_send_cmd) {
				if(g_track_strategy_timeinfo.stu_time >= g_track_strategy_info.mv_keep_time * 10) {
					teacherTracerMove(SWITCH_TEACHER_PANORAMA);
					g_track_strategy_info.last_send_cmd = SWITCH_TEACHER_PANORAMA;
					DEBUG(DL_DEBUG, "yyyyy find position1 move !switch teacher panorama!\n");
					g_track_strategy_timeinfo.mv_time = 0;
					g_track_strategy_timeinfo.stu_time = 0;
				}
			}

			usleep(100000);
			return;
		}

		if(SWITCH_STUDENTS_PANORAMA != g_track_strategy_info.last_send_cmd) {
			if(g_track_strategy_timeinfo.mv_time >= g_track_strategy_info.mv_keep_time * 10) {
				g_track_strategy_timeinfo.mv_time = 0;
				g_track_strategy_timeinfo.stu_time = 0;
				send_students = 0;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
				teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
				DEBUG(DL_DEBUG, "yyyyy lost teacher track!switch students panorama!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
			}
		}

		usleep(100000);
		return;
	}

	if(g_track_strategy_info.teacher_panorama_flag == 0) {
		//û�ж�Ŀ���Ͻ�̨
		g_track_strategy_timeinfo.students_to_platform_time = 0;

		if(g_track_strategy_timeinfo.teacher_panorama_flag == 1) {
			if(g_track_strategy_timeinfo.students_down_time >= g_track_strategy_info.students_down_time * 10) {
				g_track_strategy_info.students_track_flag = 1;
			} else {
				g_track_strategy_timeinfo.students_down_time++;
				g_track_strategy_info.students_track_flag = 0;
			}
		} else {
			g_track_strategy_info.students_track_flag = 1;
		}
	} else {
		//�ж�Ŀ���Ͻ�̨,��ѧ���Ͻ�̨,��һ��ʱ���ڲ��ٽ���ѧ����Ϣ
		g_track_strategy_info.students_track_flag = 0;
		g_track_strategy_timeinfo.students_to_platform_time++;

		if(g_track_strategy_timeinfo.students_to_platform_time >= g_track_strategy_info.teacher_keep_panorama_time * 10) {
			DEBUG(DL_DEBUG, "yyyyyyyyyyy students_to_platform_time over!\n");
			g_track_strategy_info.teacher_panorama_flag = 0;
			g_track_strategy_info.students_track_flag = 1;
		}
	}

	if(g_track_strategy_info.switch_vga_flag == 1) {
		g_track_strategy_timeinfo.vga_delaytime++;

		if(g_track_strategy_timeinfo.vga_delaytime >= g_track_strategy_info.vga_keep_time * 10) {
			if(g_track_strategy_info.send_cmd == 0) {
				g_track_strategy_info.send_cmd = SWITCH_TEATHER;
				g_track_strategy_info.switch_cmd_author = AUTHOR_TEACHER;
				DEBUG(DL_DEBUG, "yyyyy vga over,send_cmd=teacher;\n");
			}

			g_track_strategy_timeinfo.switch_flag = 1;
			g_track_strategy_info.switch_vga_flag = 0;
			g_track_strategy_timeinfo.vga_delaytime = 0;
		} else {
			usleep(100000);
			return;
		}
	}

	//�л���ʱ������ʱ
	if(g_track_strategy_timeinfo.switch_flag == 0) {
		g_track_strategy_timeinfo.delaytime++;

		if(g_track_strategy_timeinfo.delaytime >= g_track_strategy_timeinfo.switch_delaytime * 10) {
			g_track_strategy_timeinfo.switch_flag = 1;
		} else {
			usleep(100000);
			return;
		}
	}

	if(g_track_strategy_info.teacher_panorama_flag == 1) {
		if(SWITCH_TEACHER_PANORAMA != g_track_strategy_info.last_send_cmd) {
			teacherTracerMove(SWITCH_TEACHER_PANORAMA);
			DEBUG(DL_DEBUG, "6switch teacher panorama,recv students up to platform,teacher_panorama_flag=%d!\n", g_track_strategy_info.teacher_panorama_flag);
		}

		g_track_strategy_info.last_send_cmd = SWITCH_TEACHER_PANORAMA;
		g_track_strategy_info.send_cmd = 0;
		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_timeinfo.switch_flag = 0;
		g_track_strategy_timeinfo.teacher_panorama_flag = 1;
		g_track_strategy_timeinfo.students_down_time = 0;
		g_track_strategy_info.students_track_flag = 0;
	} else {
		//�յ��л���ʦ���������
		if(SWITCH_TEATHER == g_track_strategy_info.send_cmd) {
			if(AUTHOR_TEACHER == g_track_strategy_info.switch_cmd_author) {
				//��ʦ��������  ��Ҫ��ѧ�������͸�������
				send_students = 1;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
			}

			if(g_track_strategy_info.blackboard_region_flag1) {
				teacherTracerMove(SWITCH_BLACKBOARD1);
				DEBUG(DL_DEBUG, "4switch blackboard!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD1;
			} else {
				if((g_track_strategy_info.move_flag == 0)
				   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_left_limit) < 10)
				   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_right_limit) < 10))

				{
					teacherTracerMove(SWITCH_TEATHER);
					DEBUG(DL_DEBUG, "1switch teacher!\n");
					g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
				} else {
					teacherTracerMove(SWITCH_TEACHER_PANORAMA);
					DEBUG(DL_DEBUG, "66switch teacher panorama!\n");
					g_track_strategy_info.last_send_cmd = SWITCH_TEACHER_PANORAMA;
				}
			}

			g_track_strategy_timeinfo.delaytime = 0;
			g_track_strategy_timeinfo.switch_flag = 0;
			g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.teacher_switch_students_delay_time;
			g_track_strategy_info.send_cmd = 0;

		}
		//�յ��л�ѧ�����������
		else if(SWITCH_STUDENTS == g_track_strategy_info.send_cmd) {
			if(AUTHOR_TEACHER == g_track_strategy_info.switch_cmd_author) {
				send_students = 0;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);

				teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
				DEBUG(DL_DEBUG, "5switch students panorama!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
				g_track_strategy_info.send_cmd = 0;
				g_track_strategy_timeinfo.delaytime = 0;
				g_track_strategy_timeinfo.switch_flag = 0;
				g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_panorama_switch_near_time;
			} else {
				if(g_track_strategy_timeinfo.on_teacher_flag == 0) {
					teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
					DEBUG(DL_DEBUG, "55switch students panorama!\n");
					g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
					g_track_strategy_info.send_cmd = 0;
					g_track_strategy_timeinfo.delaytime = 0;
					g_track_strategy_timeinfo.switch_flag = 0;
					g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_panorama_switch_near_time;

				} else {
					g_track_strategy_timeinfo.on_teacher_delaytime++;

					if(g_track_strategy_timeinfo.on_teacher_delaytime >= g_track_strategy_info.teacher_switch_students_delay_time * 10) {
						g_track_strategy_timeinfo.on_teacher_flag = 0;
					}
				}
			}
		}
		//û���յ��κ��л����������ʦ�Ͱ�����л�
		else {
			if(SWITCH_STUDENTS_PANORAMA == g_track_strategy_info.last_send_cmd) {
				teacherTracerMove(SWITCH_STUDENTS);
				DEBUG(DL_DEBUG, "3switch students near!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS;
				g_track_strategy_timeinfo.delaytime = 0;
				g_track_strategy_timeinfo.switch_flag = 0;
				g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_near_keep_time;
			} else if(SWITCH_STUDENTS == g_track_strategy_info.last_send_cmd) {
				//�ȴ�ѧ������
				g_track_strategy_timeinfo.switch_flag = 1;
			} else {
				g_track_strategy_timeinfo.switch_flag = 1;

				if(g_track_strategy_timeinfo.on_teacher_flag == 1) {
					g_track_strategy_timeinfo.on_teacher_delaytime++;

					if(g_track_strategy_timeinfo.on_teacher_delaytime >= g_track_strategy_info.teacher_switch_students_delay_time * 10) {
						g_track_strategy_timeinfo.on_teacher_flag = 0;
					}
				}

				if(SWITCH_BLACKBOARD1 == g_track_strategy_info.last_send_cmd) {
					g_track_strategy_timeinfo.leave_panorama_time = 0;
					g_track_strategy_timeinfo.panorama_time = 0;

					if(g_track_strategy_info.blackboard_region_flag1) {
						g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
					} else {
						g_track_strategy_timeinfo.blackboard_time1 = 0;
						g_track_strategy_timeinfo.leave_blackboard_time1 ++;

						if(g_track_strategy_timeinfo.leave_blackboard_time1 >= g_track_strategy_info.teacher_leave_blackboard_time1 * 10) {
							if((g_track_strategy_info.move_flag == 0)
							   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_left_limit) < 10)
							   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_right_limit) < 10)) {
								teacherTracerMove(SWITCH_TEATHER);
								DEBUG(DL_DEBUG, "11switch teacher!\n");
								g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
							} else {
								teacherTracerMove(SWITCH_TEACHER_PANORAMA);
								DEBUG(DL_DEBUG, "666switch teacher panorama!\n");
								g_track_strategy_info.last_send_cmd = SWITCH_TEACHER_PANORAMA;
							}

							g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
							g_track_strategy_timeinfo.on_teacher_delaytime = 0;
							g_track_strategy_timeinfo.on_teacher_flag = 1;
						}
					}
				} else if(SWITCH_TEATHER == g_track_strategy_info.last_send_cmd) {
					if(g_track_strategy_info.blackboard_region_flag1) {
						g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
						g_track_strategy_timeinfo.blackboard_time1 ++;

						if(g_track_strategy_timeinfo.blackboard_time1 >= g_track_strategy_info.teacher_blackboard_time1 * 10) {
							teacherTracerMove(SWITCH_BLACKBOARD1);
							DEBUG(DL_DEBUG, "44switch blackboard!\n");
							g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD1;
							g_track_strategy_timeinfo.blackboard_time1 = 0;
							g_track_strategy_timeinfo.on_teacher_delaytime = 0;
							g_track_strategy_timeinfo.on_teacher_flag = 1;
						}
					} else {
						g_track_strategy_timeinfo.blackboard_time1 = 0;

						if((g_track_strategy_info.move_flag == 1)
						   && (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_left_limit) >= 10)
						   && (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_right_limit) >= 10)) {
							g_track_strategy_timeinfo.leave_panorama_time = 0;
							g_track_strategy_timeinfo.panorama_time ++;

							if(g_track_strategy_timeinfo.panorama_time >= g_track_strategy_info.teacher_panorama_time * 10) {
								teacherTracerMove(SWITCH_TEACHER_PANORAMA);
								DEBUG(DL_DEBUG, "6666switch teacher panorama!\n");
								g_track_strategy_info.last_send_cmd = SWITCH_TEACHER_PANORAMA;
								g_track_strategy_timeinfo.panorama_time = 0;
								g_track_strategy_timeinfo.on_teacher_delaytime = 0;
								g_track_strategy_timeinfo.on_teacher_flag = 1;
							}
						}
					}
				} else if((SWITCH_TEACHER_PANORAMA == g_track_strategy_info.last_send_cmd)
				          && (g_track_strategy_info.teacher_panorama_flag == 0)) {
					g_track_strategy_timeinfo.blackboard_time1 = 0;
					g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
					g_track_strategy_timeinfo.panorama_time = 0;

					if((g_track_strategy_info.move_flag == 0)
					   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_left_limit) < 10)
					   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_right_limit) < 10)) {
						g_track_strategy_timeinfo.leave_panorama_time++;

						if(g_track_strategy_timeinfo.leave_panorama_time >= g_track_strategy_info.teacher_leave_panorama_time * 10) {
							if(g_track_strategy_info.blackboard_region_flag1) {
								teacherTracerMove(SWITCH_BLACKBOARD1);
								DEBUG(DL_DEBUG, "444switch blackboard!\n");
								g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD1;
							} else {
								teacherTracerMove(SWITCH_TEATHER);
								DEBUG(DL_DEBUG, "111switch teacher!\n");
								g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
							}

							g_track_strategy_timeinfo.on_teacher_delaytime = 0;
							g_track_strategy_timeinfo.on_teacher_flag = 1;
						}
					}
				}

			}
		}
	}

	usleep(100000);
	return;
}

/**
* @	�л����ԣ���Ҫ�Ǵ����ж�Ӧ������ʦ����Ӧ�����������л��Ǹ���λ������
* @	�Լ���֪ѧ������Ҫ��ʲô����
*5��λ1 :��ʦ+ ѧ�� +ѧ��ȫ�� +����1+����2
*/
void strategy5_2(void *pParam)
{
	int				send_students = 0;

	if(g_track_encode_info.track_status == 0) {
		g_track_strategy_timeinfo.teacher_panorama_flag = 0;
		g_track_strategy_timeinfo.students_panorama_switch_near_time = 0;
		g_track_strategy_timeinfo.blackboard_time1 		= 0;
		g_track_strategy_timeinfo.leave_blackboard_time1	= 0;
		g_track_strategy_timeinfo.blackboard_time2 		= 0;
		g_track_strategy_timeinfo.leave_blackboard_time2	= 0;
		g_track_strategy_timeinfo.teacher_panorama_switch_near_time = 0;
		g_track_strategy_timeinfo.students_down_time = 0;
		g_track_strategy_timeinfo.students_to_platform_time = 0;
		g_track_strategy_timeinfo.switch_flag = 1; // 1�����л�,0 ��ʱδ�� �������л�
		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_info.teacher_panorama_flag = 0;
		g_track_strategy_info.students_track_flag = 0;

		if(g_track_strategy_info.switch_vga_flag == 1) {
			g_track_strategy_timeinfo.vga_delaytime++;

			if(g_track_strategy_timeinfo.vga_delaytime >= g_track_strategy_info.vga_keep_time * 10) {
				send_students = 0;
				sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
				teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
				DEBUG(DL_DEBUG, "yyyyy vga switch students panorama!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
				g_track_strategy_info.switch_vga_flag = 0;
				g_track_strategy_timeinfo.vga_delaytime = 0;
			}
		}

		if(SWITCH_STUDENTS_PANORAMA != g_track_strategy_info.last_send_cmd) {
			send_students = 0;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
			teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
			DEBUG(DL_DEBUG, "yyyyy lost teacher track!switch students panorama!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
		}

		usleep(100000);
		return;
	}

	if(g_track_strategy_info.teacher_panorama_flag == 0) {
		g_track_strategy_timeinfo.students_to_platform_time = 0;

		if(g_track_strategy_timeinfo.teacher_panorama_flag == 1) {
			if(g_track_strategy_timeinfo.students_down_time >= g_track_strategy_info.students_down_time * 10) {
				g_track_strategy_info.students_track_flag = 1;
			} else {
				g_track_strategy_timeinfo.students_down_time++;
				g_track_strategy_info.students_track_flag = 0;
			}
		} else {
			g_track_strategy_info.students_track_flag = 1;
		}
	} else {
		g_track_strategy_info.students_track_flag = 0;
		g_track_strategy_timeinfo.students_to_platform_time++;

		if(g_track_strategy_timeinfo.students_to_platform_time >= g_track_strategy_info.teacher_keep_panorama_time * 10) {
			DEBUG(DL_DEBUG, "yyyyyyyyyyy students_to_platform_time over!\n");
			g_track_strategy_info.teacher_panorama_flag = 0;
			g_track_strategy_info.students_track_flag = 1;
		}
	}

	if(g_track_strategy_info.switch_vga_flag == 1) {
		g_track_strategy_timeinfo.vga_delaytime++;

		if(g_track_strategy_timeinfo.vga_delaytime >= g_track_strategy_info.vga_keep_time * 10) {
			if(g_track_strategy_info.send_cmd == 0) {
				g_track_strategy_info.send_cmd = SWITCH_TEATHER;
				g_track_strategy_info.switch_cmd_author = AUTHOR_TEACHER;
				DEBUG(DL_DEBUG, "yyyyy vga over,send_cmd=teacher;\n");
			}

			g_track_strategy_timeinfo.switch_flag = 1;
			g_track_strategy_info.switch_vga_flag = 0;
			g_track_strategy_timeinfo.vga_delaytime = 0;
		} else {
			usleep(100000);
			return;
		}
	}

	//�л���ʱ������ʱ
	if(g_track_strategy_timeinfo.switch_flag == 0) {
		g_track_strategy_timeinfo.delaytime++;

		if(g_track_strategy_timeinfo.delaytime >= g_track_strategy_timeinfo.switch_delaytime * 10) {
			g_track_strategy_timeinfo.switch_flag = 1;
		} else {
			usleep(100000);
			return;
		}
	}

	//�յ��л���ʦ���������
	if(SWITCH_TEATHER == g_track_strategy_info.send_cmd) {
		if(AUTHOR_TEACHER == g_track_strategy_info.switch_cmd_author) {
			//��ʦ��������  ��Ҫ��ѧ�������͸�������
			send_students = 1;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);
		}

		if(g_track_strategy_info.blackboard_region_flag1) {
			teacherTracerMove(SWITCH_BLACKBOARD1);
			DEBUG(DL_DEBUG, "4switch blackboard1!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD1;
		} else if(g_track_strategy_info.blackboard_region_flag2) {
			teacherTracerMove(SWITCH_BLACKBOARD2);
			DEBUG(DL_DEBUG, "6switch blackboard2!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD2;
		} else {
			teacherTracerMove(SWITCH_TEATHER);
			DEBUG(DL_DEBUG, "1switch teacher!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
		}

		g_track_strategy_timeinfo.delaytime = 0;
		g_track_strategy_timeinfo.switch_flag = 0;
		g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.teacher_switch_students_delay_time;
		g_track_strategy_info.send_cmd = 0;

	}
	//�յ��л�ѧ�����������
	else if(SWITCH_STUDENTS == g_track_strategy_info.send_cmd) {
		if(AUTHOR_TEACHER == g_track_strategy_info.switch_cmd_author) {
			send_students = 0;
			sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE);

			teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
			DEBUG(DL_DEBUG, "5switch students panorama!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
			g_track_strategy_info.send_cmd = 0;
			g_track_strategy_timeinfo.delaytime = 0;
			g_track_strategy_timeinfo.switch_flag = 0;
			g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_panorama_switch_near_time;
		} else {
			if(g_track_strategy_timeinfo.on_teacher_flag == 0) {
				teacherTracerMove(SWITCH_STUDENTS_PANORAMA);
				DEBUG(DL_DEBUG, "55switch students panorama!\n");
				g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS_PANORAMA;
				g_track_strategy_info.send_cmd = 0;
				g_track_strategy_timeinfo.delaytime = 0;
				g_track_strategy_timeinfo.switch_flag = 0;
				g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_panorama_switch_near_time;

			} else {
				g_track_strategy_timeinfo.on_teacher_delaytime++;

				if(g_track_strategy_timeinfo.on_teacher_delaytime >= g_track_strategy_info.teacher_switch_students_delay_time * 10) {
					g_track_strategy_timeinfo.on_teacher_flag = 0;
				}
			}
		}
	}
	//û���յ��κ��л����������ʦ�Ͱ�����л�
	else {
		if(SWITCH_STUDENTS_PANORAMA == g_track_strategy_info.last_send_cmd) {
			teacherTracerMove(SWITCH_STUDENTS);
			DEBUG(DL_DEBUG, "3switch students near!\n");
			g_track_strategy_info.last_send_cmd = SWITCH_STUDENTS;
			g_track_strategy_timeinfo.delaytime = 0;
			g_track_strategy_timeinfo.switch_flag = 0;
			g_track_strategy_timeinfo.switch_delaytime = g_track_strategy_info.students_near_keep_time;
		} else if(SWITCH_STUDENTS == g_track_strategy_info.last_send_cmd) {
			//�ȴ�ѧ������
			g_track_strategy_timeinfo.switch_flag = 1;
		} else {
			g_track_strategy_timeinfo.switch_flag = 1;

			if(g_track_strategy_timeinfo.on_teacher_flag == 1) {
				g_track_strategy_timeinfo.on_teacher_delaytime++;

				if(g_track_strategy_timeinfo.on_teacher_delaytime >= g_track_strategy_info.teacher_switch_students_delay_time * 10) {
					g_track_strategy_timeinfo.on_teacher_flag = 0;
				}
			}

			if(SWITCH_BLACKBOARD1 == g_track_strategy_info.last_send_cmd) {
				if(g_track_strategy_info.blackboard_region_flag1) {
					g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
				} else {
					g_track_strategy_timeinfo.blackboard_time1 = 0;
					g_track_strategy_timeinfo.leave_blackboard_time1 ++;

					if(g_track_strategy_timeinfo.leave_blackboard_time1 >= g_track_strategy_info.teacher_leave_blackboard_time1 * 10) {
						if(g_track_strategy_info.blackboard_region_flag2) {
							teacherTracerMove(SWITCH_BLACKBOARD2);
							DEBUG(DL_DEBUG, "66switch blackboard2!\n");
							g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD2;
							g_track_strategy_timeinfo.leave_blackboard_time2 = 0;
						} else {
							teacherTracerMove(SWITCH_TEATHER);
							DEBUG(DL_DEBUG, "11switch teacher!\n");
							g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
							g_track_strategy_timeinfo.blackboard_time2 = 0;
						}

						g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
						g_track_strategy_timeinfo.on_teacher_delaytime = 0;
						g_track_strategy_timeinfo.on_teacher_flag = 1;
					}
				}
			} else if(SWITCH_BLACKBOARD2 == g_track_strategy_info.last_send_cmd) {
				if(g_track_strategy_info.blackboard_region_flag2) {
					g_track_strategy_timeinfo.leave_blackboard_time2 = 0;
				} else {
					g_track_strategy_timeinfo.blackboard_time2 = 0;
					g_track_strategy_timeinfo.leave_blackboard_time2 ++;

					if(g_track_strategy_timeinfo.leave_blackboard_time2 >= g_track_strategy_info.teacher_leave_blackboard_time2 * 10) {
						if(g_track_strategy_info.blackboard_region_flag1) {
							teacherTracerMove(SWITCH_BLACKBOARD1);
							DEBUG(DL_DEBUG, "44switch blackboard!\n");
							g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD1;
							g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
						} else {
							teacherTracerMove(SWITCH_TEATHER);
							DEBUG(DL_DEBUG, "111switch teacher!\n");
							g_track_strategy_info.last_send_cmd = SWITCH_TEATHER;
							g_track_strategy_timeinfo.blackboard_time1 = 0;
						}

						g_track_strategy_timeinfo.leave_blackboard_time2 = 0;
						g_track_strategy_timeinfo.on_teacher_delaytime = 0;
						g_track_strategy_timeinfo.on_teacher_flag = 1;
					}
				}
			} else if(SWITCH_TEATHER == g_track_strategy_info.last_send_cmd) {
				if(g_track_strategy_info.blackboard_region_flag1) {
					g_track_strategy_timeinfo.leave_blackboard_time1 = 0;
					g_track_strategy_timeinfo.blackboard_time1 ++;

					if(g_track_strategy_timeinfo.blackboard_time1 >= g_track_strategy_info.teacher_blackboard_time1 * 10) {
						teacherTracerMove(SWITCH_BLACKBOARD1);
						DEBUG(DL_DEBUG, "44switch blackboard1!\n");
						g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD1;
						g_track_strategy_timeinfo.blackboard_time1 = 0;
						g_track_strategy_timeinfo.on_teacher_delaytime = 0;
						g_track_strategy_timeinfo.on_teacher_flag = 1;
					}
				} else {
					g_track_strategy_timeinfo.blackboard_time1 = 0;

					if(g_track_strategy_info.blackboard_region_flag2) {
						g_track_strategy_timeinfo.leave_blackboard_time2 = 0;
						g_track_strategy_timeinfo.blackboard_time2 ++;

						if(g_track_strategy_timeinfo.blackboard_time2 >= g_track_strategy_info.teacher_blackboard_time2 * 10) {
							teacherTracerMove(SWITCH_BLACKBOARD2);
							DEBUG(DL_DEBUG, "666switch blackboard2!\n");
							g_track_strategy_info.last_send_cmd = SWITCH_BLACKBOARD2;
							g_track_strategy_timeinfo.blackboard_time2 = 0;
							g_track_strategy_timeinfo.on_teacher_delaytime = 0;
							g_track_strategy_timeinfo.on_teacher_flag = 1;
						}
					} else {
						g_track_strategy_timeinfo.blackboard_time2 = 0;
					}

				}
			}
		}
	}

	usleep(100000);
	return;
}

Int32 GetCurSwitchCmd(MpState *pMpState)
{

	if(pMpState == NULL) {
		return 0;
	}


	SubMpState *pSubMp = &pMpState->SubMpstate;


	if((2 == pMpState->num) || (1 == pMpState->num)) {
		//������ģʽ
		if((pSubMp[1].CurSubMp == -1)) {
			//������
			if(gStrategy == STRATEGY_5_1) {
				if(pSubMp[0].CurSubMp == TEACHER_INDEX) {
					return SWITCH_TEATHER;
				} else if(pSubMp[0].CurSubMp == VGA_INDEX) {
					return SWITCH_VGA;
				} else if(pSubMp[0].CurSubMp == STUDENT_INDEX) {
					return SWITCH_STUDENTS;
				} else if(pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) {
					return SWITCH_TEACHER_PANORAMA;
				} else if(pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) {
					return SWITCH_STUDENTS_PANORAMA;
				} else if(pSubMp[0].CurSubMp == TEACHBLACKBORAD_INDEX1) {
					return SWITCH_BLACKBOARD1;
				}
			}
			//˫����
			else if(gStrategy == STRATEGY_5_2) {
				if(pSubMp[0].CurSubMp == TEACHER_INDEX) {
					return SWITCH_TEATHER;
				} else if(pSubMp[0].CurSubMp == VGA_INDEX) {
					return SWITCH_VGA;
				} else if(pSubMp[0].CurSubMp == STUDENT_INDEX) {
					return SWITCH_STUDENTS;
				} else if(pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) {
					pSubMp[0].CurSubMp == TEACHER_INDEX;
					return SWITCH_TEATHER;
				} else if(pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) {
					return SWITCH_STUDENTS_PANORAMA;
				} else if(pSubMp[0].CurSubMp == TEACHBLACKBORAD_INDEX1) {
					return SWITCH_BLACKBOARD1;
				} else if(pSubMp[0].CurSubMp == TEACHBLACKBORAD_INDEX2) {
					return SWITCH_BLACKBOARD2;
				}
			} else if(gStrategy == STRATEGY_4)
				  {
				if(pSubMp[0].CurSubMp == TEACHER_INDEX) {
					return SWITCH_TEATHER;
				} else if(pSubMp[0].CurSubMp == VGA_INDEX) {
					return SWITCH_VGA;
				} else if(pSubMp[0].CurSubMp == STUDENT_INDEX) {
					return SWITCH_STUDENTS;
				} else if(pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) {
					pSubMp[0].CurSubMp = TEACHER_INDEX;
					return SWITCH_TEATHER;
				} else if(pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) {
					return SWITCH_STUDENTS_PANORAMA;
				} else if(pSubMp[0].CurSubMp == TEACHBLACKBORAD_INDEX1) {
					return SWITCH_BLACKBOARD1;
				}
			}
			//2��λ
			else if(gStrategy == STRATEGY_2) {
				if(pSubMp[0].CurSubMp == TEACHER_INDEX) {
					return SWITCH_TEATHER;
				} else if(pSubMp[0].CurSubMp == VGA_INDEX) {
					return SWITCH_VGA;
				} else if(pSubMp[0].CurSubMp == STUDENT_INDEX) {
					return SWITCH_STUDENTS;
				} else if(pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) {
					pSubMp[0].CurSubMp = TEACHER_INDEX;
					return SWITCH_TEATHER;
				} else if((pSubMp[0].CurSubMp == STUDENT_FAR_INDEX)  && (pSubMp[0].LastSubMp == -1)) {
					pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
					return SWITCH_STUDENTS;
				} else if((pSubMp[0].CurSubMp == STUDENT_FAR_INDEX)  && (pSubMp[0].LastSubMp == STUDENT_FAR_INDEX)) {
					pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
					return SWITCH_STUDENTS;
				} else if((pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) && (pSubMp[0].LastSubMp == TEACHER_INDEX)) {
					pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
					return SWITCH_TEATHER;
				} else if((pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) && (pSubMp[0].LastSubMp == VGA_INDEX)) {
					pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
					return SWITCH_VGA;
				} else if((pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) && (pSubMp[0].LastSubMp == STUDENT_INDEX)) {
					pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
					return SWITCH_STUDENTS;
				} else {
					pSubMp[0].CurSubMp = TEACHER_INDEX;
					return SWITCH_TEATHER;
				}
			} else {

			}
		}

		//2����
		else {
			if((gStrategy == STRATEGY_5_1) || (gStrategy == STRATEGY_4) || (gStrategy == STRATEGY_5_2)) {

				if(pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) {
					if((pSubMp[1].CurSubMp == VGA_INDEX)) {
						return	SWITCH_VGA;
					} else {
						return SWITCH_STUDENTS_PANORAMA;
					}
				} else {
					pSubMp[0].CurSubMp = TEACHER_INDEX;

					if((pSubMp[1].CurSubMp == VGA_INDEX)) {

						if(pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) {
							return SWITCH_VGA;
						}

						if(TWO_MP_2 == gMpMode) {
							return	SWITCH_2_TEATHER_AND_VGA;
						} else if(TWO_MP_1 == gMpMode) {
							return SWITCH_2_VGA_TEATHER;
						} else if(TWO_MP_3 == gMpMode) {
							return SWITCH_2_VGA_TEATHER_1;
						} else if(TWO_MP_4 == gMpMode) {
							return SWITCH_2_VGA_TEATHER_2;
						}

						return SWITCH_2_VGA_TEATHER;

					} else if(pSubMp[1].CurSubMp == STUDENT_FAR_INDEX) {
						if(pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) {
							return SWITCH_STUDENTS_PANORAMA;
						}



						if(TWO_MP_1 == gMpMode) {
							return	SWITCH_2_STUPANORAMA_TEATHER;
						} else if(TWO_MP_2 == gMpMode) {
							return SWITCH_2_STUPANORAMA_AND_TEATHER;
						} else if(TWO_MP_3 == gMpMode) {
							return SWITCH_2_STUPANORAMA_TEATHER_1;
						} else if(TWO_MP_4 == gMpMode) {
							return SWITCH_2_STUPANORAMA_TEATHER_2;
						}

						return SWITCH_2_STUPANORAMA_TEATHER;
					} else if(pSubMp[1].CurSubMp == STUDENT_INDEX) {
						if(pSubMp[0].CurSubMp == STUDENT_FAR_INDEX) {
							return SWITCH_STUDENTS_PANORAMA;
						}


						if(TWO_MP_1 == gMpMode) {
							return	SWITCH_2_STU_TEATHER;
						} else if(TWO_MP_2 == gMpMode) {
							return SWITCH_2_TEATHER_AND_STU;
						} else if(TWO_MP_3 == gMpMode) {
							return SWITCH_2_STU_TEATHER_1;
						} else if(TWO_MP_4 == gMpMode) {
							return SWITCH_2_STU_TEATHER_2;
						}

						return SWITCH_2_STU_TEATHER;
					} else {
						return SWITCH_2_VGA_TEATHER;
					}
				}
			}

#if 0
			//2��λ
			else if(gStrategy == STRATEGY_2) {
				pSubMp[0].CurSubMp = TEACHER_INDEX;

				if(pSubMp[1].CurSubMp == VGA_INDEX) {

					if(TWO_MP_2 == gMpMode) {
						return	SWITCH_2_VGA_TEATHER_1;
					} else if(TWO_MP_1 == gMpMode) {
						return SWITCH_2_VGA_TEATHER;
					} else if(TWO_MP_3 == gMpMode) {
						return SWITCH_2_TEATHER_AND_VGA;
					}

					return SWITCH_2_VGA_TEATHER;

				} else if((pSubMp[1].CurSubMp == STUDENT_FAR_INDEX) && (pSubMp[1].LastSubMp == STUDENT_FAR_INDEX)) {

					if(TWO_MP_3 == gMpMode) {
						//return SWITCH_TEATHER;
						return SWITCH_2_TEATHER_AND_STU;
					}

					return SWITCH_2_STU_TEATHER;
					//return SWITCH_TEATHER;
				} else if((pSubMp[1].CurSubMp == STUDENT_FAR_INDEX) && (pSubMp[1].LastSubMp == STUDENT_INDEX)) {

					if(TWO_MP_3 == gMpMode) {
						//return SWITCH_TEATHER;
						return SWITCH_2_TEATHER_AND_STU;
					}

					return SWITCH_2_STU_TEATHER;
					//return SWITCH_TEATHER;
				} else if(pSubMp[1].CurSubMp == STUDENT_FAR_INDEX) {

					if(TWO_MP_2 == gMpMode) {
						return SWITCH_TEATHER;
						//return SWITCH_2_TEATHER_AND_STU;
					}

					//return SWITCH_2_STU_TEATHER;
					return SWITCH_TEATHER;
				} else if(pSubMp[1].CurSubMp == STUDENT_INDEX) {
					if(TWO_MP_3 == gMpMode) {
						return SWITCH_2_TEATHER_AND_STU;
					}

					return SWITCH_2_STU_TEATHER;
				} else {
					return SWITCH_2_VGA_TEATHER;
				}

			}

#endif
		}

	} else if(3 == pMpState->num) {
		//5��λ
		if(gStrategy == STRATEGY_5_1) {
			if((pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_VGA_TEACHPANORAMA_STUPANORAMA;
			}

			else if((pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_VGA_TEACHPANORAMA_STU;
			} else if((pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_BLACKBOARD1_TEACHPANORAMA_STUPANORAMA;
			} else if((pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_BLACKBOARD1_TEACHPANORAMA_STU;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_VGA_TEATHER_STUPANORAMA;
			}

			else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_VGA_TEATHER_STU;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_BLACKBOARD1_TEATHER_STUPANORAMA;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_BLACKBOARD1_TEATHER_STU;
			} else {
				return SWITCH_3_VGA_TEACHPANORAMA_STUPANORAMA;
			}
		}
		//5��λ ˫����
		else if(gStrategy == STRATEGY_5_2) {

			pSubMp[0].CurSubMp == TEACHER_INDEX;

			if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_VGA_TEATHER_STUPANORAMA;
			}

			else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_VGA_TEATHER_STU;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_BLACKBOARD1_TEATHER_STUPANORAMA;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_BLACKBOARD1_TEATHER_STU;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX2) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_BLACKBOARD2_TEATHER_STUPANORAMA;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX2) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_BLACKBOARD2_TEATHER_STU;
			} else {
				return SWITCH_3_VGA_TEATHER_STUPANORAMA;
			}
		}
		//4��λ
		else if(gStrategy == STRATEGY_4) {
			pSubMp[0].CurSubMp = TEACHER_INDEX;

			if((pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_VGA_TEATHER_STUPANORAMA;
			} else if((pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_VGA_TEATHER_STU;
			} else if((pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_3_BLACKBOARD1_TEATHER_STUPANORAMA;
			} else if((pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_3_BLACKBOARD1_TEATHER_STU;
			} else {
				return SWITCH_3_VGA_TEATHER_STUPANORAMA;
			}
		}
		//2��λ
		else if(gStrategy == STRATEGY_2) {
			return  SWITCH_3_VGA_TEATHER_STU;
		}


	} else if(4 == pMpState->num) {
		if(gStrategy == STRATEGY_5_1) {
			if((pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA;
			}

			else if((pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_4_VGA_TEATHER_STU_TEACHPANORAMA;
			} else if((pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_4_BLACKBOARD1_TEATHER_STUPANORAMA_TEACHPANORAMA;
			} else if((pSubMp[0].CurSubMp == TEACHER_FAR_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_4_BLACKBOARD1_TEATHER_STU_TEACHPANORAMA;
			}


			else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA;
			}

			else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == VGA_INDEX) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_4_VGA_TEATHER_STU_TEACHPANORAMA;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
				return SWITCH_4_BLACKBOARD1_TEATHER_STUPANORAMA_TEACHPANORAMA;
			} else if((pSubMp[0].CurSubMp == TEACHER_INDEX) && (pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) && (pSubMp[2].CurSubMp == STUDENT_INDEX)) {
				return SWITCH_4_BLACKBOARD1_TEATHER_STU_TEACHPANORAMA;
			} else {
				return SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA;
			}
		} else if(gStrategy == STRATEGY_5_2) {
			pSubMp[3].CurSubMp = TEACHER_FAR_INDEX;
			pSubMp[0].CurSubMp = TEACHER_INDEX;

			if(pSubMp[1].CurSubMp == VGA_INDEX) {
				if(pSubMp[2].CurSubMp == STUDENT_FAR_INDEX) {
					return SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA;
				} else {
					return SWITCH_4_VGA_TEATHER_STU_TEACHPANORAMA;
				}
			} else if(pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) {
				if(pSubMp[2].CurSubMp == STUDENT_FAR_INDEX) {
					return SWITCH_4_BLACKBOARD1_TEATHER_STUPANORAMA_TEACHPANORAMA;
				} else {
					return SWITCH_4_BLACKBOARD1_TEATHER_STU_TEACHPANORAMA;
				}
			} else if(pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX2) {
				if(pSubMp[2].CurSubMp == STUDENT_FAR_INDEX) {
					return SWITCH_4_BLACKBOARD2_TEACHPER_STUPANORAMA_BLACKBOARD1;
				} else {
					return SWITCH_4_BLACKBOARD2_TEACHPER_STU_BLACKBOARD1;
				}

				pSubMp[3].CurSubMp = TEACHBLACKBORAD_INDEX1;
			} else {
				return SWITCH_4_VGA_TEATHER_STU_TEACHPANORAMA;
			}
		}
		//4��λ �ݲ�֧��4����
		else if(gStrategy == STRATEGY_4) {
			pSubMp[0].CurSubMp = TEACHER_INDEX;
			pSubMp[2].CurSubMp = STUDENT_INDEX;
			pSubMp[3].CurSubMp = STUDENT_FAR_INDEX;

			if(pSubMp[1].CurSubMp == TEACHBLACKBORAD_INDEX1) {

				return SWITCH_4_BLACKBOARD1_TEACHPER_STU_STUPANORAMA;
			} else {
				return SWITCH_4_VGA_TEACHPER_STU_STUPANORAMA;
			}

		}

		//2��λ
		else if(gStrategy == STRATEGY_2) {
			return SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA;
		}

	} else if((6 == pMpState->num) && (SIX_MP_1 == gMpMode)) {
		return SWITCH_6_1;
	} else if((6 == pMpState->num) && (SIX_MP_2 == gMpMode)) {
		return SWITCH_6_2;
	}

}


//|----|
//|��ʦ|
//------
//1�����л�����
UInt32	OneMpStrategy(MpState   *pMpState)
{
	Int32  CurSwitch = 0;

	if(pMpState == NULL) {
		return 0;
	}

	UInt32 newflag = 0; //��Ҫ������ʱ
	SubMpState *pSubMp = pMpState->SubMpstate;

	//	zlog_debug(ptrackzlog,"Num[1] StudentUp StudentState[%d] [%d] TeacherState[%d] [%d] VgaState[%d] [%d]\n",\
	//	pMpState->StudentState,StudentUp,pSubMp[2].Flag,pMpState->TeacherState,pSubMp[0].Flag,pMpState->VgaState,pSubMp[1].Flag);


	//���VGA���� ���� ѧ��վ�� �������л�
	if(pMpState->VgaState == VGACHANGE) {
		pSubMp[0].Flag = 1;
		DelTimeOut(pMpState->num, 0);
	}

	//���ѧ��վ�� ��ǰ������ʦ���� �������л�
	else if((StudentUp == 1) && (pSubMp[0].CurSubMp == STUDENT_FAR_INDEX || pSubMp[0].CurSubMp == VGA_INDEX)) {
		pSubMp[0].Flag = 1;
		DelTimeOut(pMpState->num, 0);
	}

#if 0
	//ѧ������ ��ǰ����ʦ���� Ҫ��ʱ
	else if((StudentUp == 1) && (pSubMp[0].CurSubMp != STUDENT_FAR_INDEX) && (pSubMp[0].CurSubMp == VGA_INDEX)) {

	}
	//��ʦ֮��״̬�л�
	else if((StudentUp == 0) && (pSubMp[0].CurSubMp != STUDENT_FAR_INDEX) && (pSubMp[0].CurSubMp == VGA_INDEX)) {
		//pSubMp[0].Flag = 1;
	} else if((StudentUp == 0) && (pSubMp[0].CurSubMp == STUDENT_FAR_INDEX || pSubMp[0].CurSubMp == VGA_INDEX) {

}
#endif

pSubMp[1].CurSubMp = -1;


//VGA������VGAΪ����
if(pSubMp[0].Flag == 1) {
	if(pMpState->VgaState == VGACHANGE) {
			pSubMp[0].CurSubMp = VGA_INDEX;
			newflag = 1;
		} else if(pMpState->TeacherState == TEACHLOSE) {
			pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
		}
		//else if((pSubMp[0].LastSubMp == STUDENT_FAR_INDEX || pSubMp[0].LastSubMp == STUDENT_INDEX)
		// && (pMpState->StudentState == STUUP) && (StudentUp == 0))
		else if((pMpState->StudentState == STUUP) && (StudentUp == 0)) {
			pSubMp[0].CurSubMp = STUDENT_INDEX;
		} else if((pMpState->StudentState == STUUP) && (StudentUp == 1)) {
			newflag = 1;
			StudentUp = 0;
			pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
		} else if(pMpState->TeacherState == TEACHPLATFORM) {
			pSubMp[0].CurSubMp = TEACHER_INDEX;
		} else if(pMpState->TeacherState == TEACHMOVE) {
			pSubMp[0].CurSubMp = TEACHER_FAR_INDEX;
		} else if((pMpState->TeacherState == TEACHBLACKBORAD1)) {
			pSubMp[0].CurSubMp = TEACHBLACKBORAD_INDEX1;
		} else if((pMpState->TeacherState == TEACHBLACKBORAD2)) {
			pSubMp[0].CurSubMp = TEACHBLACKBORAD_INDEX2;
		} else {
			pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
		}

	}

	CurSwitch =	GetCurSwitchCmd(pMpState);

	int i = 0;

	for(i = 0; i < pMpState->num; i++) {

	if(pSubMp[i].CurSubMp != pSubMp[i].LastSubMp) {
			pSubMp[i].Flag = 1;
		} else {
			pSubMp[i].Flag = 0;
		}
	}

	if(newflag == 1) {
	pSubMp[0]
		.Flag = 1;
	}

	pSubMp[0].LastSubMp =	pSubMp[0].CurSubMp;




	return CurSwitch;
}





//|----|----|
//|��ʦ|VGA |
//----------- ��ʦ Ϊ0  VGA Ϊ1
//2�����л����� ���ܴ���ֻ��һ�����
UInt32	TwoMpStrategy(MpState   *pMpState)
{
	Int32  CurSwitch = 0;
	Int32 newflag1 = 0;

	if(pMpState == NULL) {
		return 0;
	}

	SubMpState *pSubMp = pMpState->SubMpstate;


	//���VGA���� ���� ѧ��վ�� �������л�
	if((pMpState->VgaState == VGACHANGE)) {
		pSubMp[1].Flag = 1;
		DelTimeOut(pMpState->num, 1);
	}

	//���ѧ��վ�� ��ǰ������ʦ���� �������л�
	else if((StudentUp == 1) && (pSubMp[1].CurSubMp == STUDENT_FAR_INDEX || pSubMp[1].CurSubMp == VGA_INDEX)) {
		pSubMp[1].Flag = 1;
		DelTimeOut(pMpState->num, 1);
	}


	//VGA������VGAΪ����
	if(pSubMp[1].Flag == 1) {
		if(pMpState->VgaState == VGACHANGE) {
			pSubMp[1].CurSubMp = VGA_INDEX;
			newflag1 = 1;
		} else if((pMpState->StudentState == STUUP) && (StudentUp == 1)) {
			newflag1 = 1;
			StudentUp = 0;
			pSubMp[1].CurSubMp = STUDENT_FAR_INDEX;
		} else if((pMpState->StudentState == STUUP) && (StudentUp == 0)) {
			pSubMp[1].CurSubMp = STUDENT_INDEX;
		} else {
			pSubMp[1].Flag = 0;
			pSubMp[1].CurSubMp = -1;
		}

	}


	if(pSubMp[0].Flag == 1) {
		if(pMpState->TeacherState == TEACHLOSE) {
			pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
		} else if(pMpState->TeacherState == TEACHPLATFORM) {
			pSubMp[0].CurSubMp = TEACHER_INDEX;
		} else if(pMpState->TeacherState == TEACHMOVE) {
			pSubMp[0].CurSubMp = TEACHER_FAR_INDEX;
		} else if(pMpState->TeacherState == TEACHBLACKBORAD1) {
			pSubMp[0].CurSubMp = TEACHBLACKBORAD_INDEX1;
		} else if(pMpState->TeacherState == TEACHBLACKBORAD2) {
			pSubMp[0].CurSubMp = TEACHBLACKBORAD_INDEX2;
		} else {
			pSubMp[0].CurSubMp = STUDENT_FAR_INDEX;
		}
	}

	CurSwitch =	GetCurSwitchCmd(pMpState);

	int i = 0;

	for(i = 0; i < pMpState->num; i++) {

		if(pSubMp[i].CurSubMp != pSubMp[i].LastSubMp) {
			pSubMp[i].Flag = 1;
		} else {
			pSubMp[i].Flag = 0;
		}
	}

	if(newflag1 == 1) {
		pSubMp[1].Flag = 1;
	}

	pSubMp[0].LastSubMp =	pSubMp[0].CurSubMp;
	pSubMp[1].LastSubMp	=	pSubMp[1].CurSubMp;



	return CurSwitch;
}




//___________
//|��ʦ |V G|
//|ѧ�� | A |   //0 Ϊ��ʦ 1ΪVGA 2Ϊѧ��
//-----------
UInt32	ThreeMpStrategy(MpState   *pMpState)
{
	Int32  CurSwitch = 0;
	Int32  newflag2 = 0;
	Int32  newflag1 = 0;
	Int32  newflag0 = 0;

	if(pMpState == NULL) {
		return 0;
	}

	SubMpState *pSubMp = pMpState->SubMpstate;


	//���VGA���� ���� ѧ��վ�� �������л�
	if((pMpState->VgaState == VGACHANGE)) {
		pSubMp[1].Flag = 1;
		DelTimeOut(pMpState->num, 1);
	}

	//���ѧ��վ�� ��ǰ������ʦ���� �������л�
	if((StudentUp == 1) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
		pSubMp[2].Flag = 1;
		DelTimeOut(pMpState->num, 2);
	}



	if(pSubMp[2].Flag == 1) {

		if((pMpState->StudentState == STUUP) && (StudentUp == 0)) {
			pSubMp[2].CurSubMp = STUDENT_INDEX;
		} else if((pMpState->StudentState == STUUP) && (StudentUp == 1)) {
			newflag2 = 1;
			StudentUp = 0;
			pSubMp[2].CurSubMp = STUDENT_FAR_INDEX;
		} else {
			pSubMp[2].CurSubMp = STUDENT_FAR_INDEX;
		}
	}

	if(pSubMp[0].Flag == 1) {
		//if((pMpState->TeacherState == TEACHLOSE) || (pMpState->TeacherState == TEACHMOVE)) {
		if((pSubMp[0].LastSubMp == TEACHER_FAR_INDEX) && (pMpState->TeacherState == TEACHMOVE)) {
			pSubMp[0].CurSubMp = TEACHER_FAR_INDEX;
			newflag0 = 1;
		} else if((pMpState->TeacherState == TEACHLOSE)) {
			pSubMp[0].CurSubMp = TEACHER_FAR_INDEX;
		} else {
			pSubMp[0].CurSubMp = TEACHER_INDEX;
		}
	}

	if(pSubMp[1].Flag == 1) {

		if(pMpState->VgaState == VGACHANGE) {
			pSubMp[1].CurSubMp = VGA_INDEX;
			newflag1 = 1;
		} else {
			if(pMpState->TeacherState == TEACHBLACKBORAD1) {
				pSubMp[1].CurSubMp = TEACHBLACKBORAD_INDEX1;
			} else if(pMpState->TeacherState == TEACHBLACKBORAD2) {
				pSubMp[1].CurSubMp = TEACHBLACKBORAD_INDEX2;
			} else {
				pSubMp[1].CurSubMp = VGA_INDEX;
				newflag1 = 2;
			}
		}
	}


	CurSwitch =	GetCurSwitchCmd(pMpState);

	int i = 0;

	for(i = 0; i < pMpState->num; i++) {

		if(pSubMp[i].CurSubMp != pSubMp[i].LastSubMp) {
			pSubMp[i].Flag = 1;
		} else {
			pSubMp[i].Flag = 0;
		}
	}

	if(newflag0 == 1) {
		pSubMp[0].Flag = 1;
	}

	if(newflag2 == 1) {
		pSubMp[2].Flag = 1;
	}

	if(newflag1 == 1) {
		pSubMp[1].Flag = 1;
	} else if(newflag1 == 2) {
		pSubMp[1].Flag = 0;
	}

	pSubMp[0].LastSubMp =	pSubMp[0].CurSubMp;
	pSubMp[1].LastSubMp	=	pSubMp[1].CurSubMp;
	pSubMp[2].LastSubMp	=	pSubMp[2].CurSubMp;




	return CurSwitch;
}


//___________
//|��ʦ |V G|
//|ѧ�� | A |   //0 Ϊ��ʦ 1ΪVGA 2Ϊѧ�� 3 Ϊȫ��
//|ȫ�� |   |
//-----------
UInt32	FourMpStrategy(MpState   *pMpState)
{

	Int32  CurSwitch = 0;
	Int32 newflag1 = 0;
	Int32 newflag2 = 0;

	if(pMpState == NULL) {
		return 0;
	}

	SubMpState *pSubMp = pMpState->SubMpstate;


	//���VGA���� ���� ѧ��վ�� �������л�
	if((pMpState->VgaState == VGACHANGE)) {
		pSubMp[1].Flag = 1;

		DelTimeOut(pMpState->num, 1);
	}

	//���ѧ��վ�� ��ǰ������ʦ���� �������л�
	if((StudentUp == 1) && (pSubMp[2].CurSubMp == STUDENT_FAR_INDEX)) {
		pSubMp[2].Flag = 1;

		DelTimeOut(pMpState->num, 2);
	}

	//ѧ��
	if(pSubMp[2].Flag == 1) {
		if((pMpState->StudentState == STUUP) && (StudentUp == 0)) {
			pSubMp[2].CurSubMp = STUDENT_INDEX;
		} else if((pMpState->StudentState == STUUP) && (StudentUp == 1)) {
			newflag2 = 1;
			StudentUp = 0;
			pSubMp[2].CurSubMp = STUDENT_FAR_INDEX;
		} else {
			pSubMp[2].CurSubMp = STUDENT_FAR_INDEX;
		}
	}

	//��ʦ
	if(pSubMp[0].Flag == 1) {
		if(pMpState->TeacherState == TEACHLOSE) {
			pSubMp[0].CurSubMp = TEACHER_FAR_INDEX;
		} else {
			pSubMp[0].CurSubMp = TEACHER_INDEX;
		}
	}

	//VGA
	if(pSubMp[1].Flag == 1) {

		if(pMpState->VgaState == VGACHANGE) {
			pSubMp[1].CurSubMp = VGA_INDEX;
			newflag1 = 1;
		} else {
			if(pMpState->TeacherState == TEACHBLACKBORAD1) {
				pSubMp[1].CurSubMp = TEACHBLACKBORAD_INDEX1;
			} else if(pMpState->TeacherState == TEACHBLACKBORAD2) {
				pSubMp[1].CurSubMp = TEACHBLACKBORAD_INDEX2;
			} else {
				pSubMp[1].CurSubMp = VGA_INDEX;
				newflag1 = 2;
			}
		}
	}

	//��ʦȫ��
	if(pSubMp[3].Flag == 1) {


		pSubMp[3].CurSubMp = TEACHER_FAR_INDEX;

	}



	CurSwitch =	GetCurSwitchCmd(pMpState);

	int i = 0;

	for(i = 0; i < pMpState->num; i++) {

		if(pSubMp[i].CurSubMp != pSubMp[i].LastSubMp) {
			pSubMp[i].Flag = 1;
		} else {
			pSubMp[i].Flag = 0;
		}
	}

	if(newflag2 == 1) {
		pSubMp[2].Flag = 1;
	}

	if(newflag1 == 1) {
		pSubMp[1].Flag = 1;
	} else if(newflag1 == 2) {
		pSubMp[1].Flag = 0;
	}

	pSubMp[0].LastSubMp =	pSubMp[0].CurSubMp;
	pSubMp[1].LastSubMp	=	pSubMp[1].CurSubMp;
	pSubMp[2].LastSubMp	=	pSubMp[2].CurSubMp;
	pSubMp[3].LastSubMp	=	pSubMp[3].CurSubMp;



	return CurSwitch;
}

//___________
//|��ʦ |V G|
//|ѧ�� | A |   //0 Ϊ��ʦ 1ΪVGA 2Ϊѧ�� 3 Ϊȫ��
//|ȫ�� |   |
//-----------
UInt32	SixMpStrategy(MpState   *pMpState)
{

	Int32  CurSwitch = 0;

	if(pMpState == NULL) {
		return 0;
	}

	SubMpState *pSubMp = pMpState->SubMpstate;


	//ѧ��
	if(pSubMp[2].Flag == 1) {
		pSubMp[2].CurSubMp = STUDENT_INDEX;

	}

	//��ʦ
	if(pSubMp[0].Flag == 1) {
		pSubMp[0].CurSubMp = TEACHER_INDEX;
	}

	//VGA
	if(pSubMp[1].Flag == 1) {
		pSubMp[1].CurSubMp = TEACHBLACKBORAD_INDEX1;
	}

	//��ʦȫ��
	if(pSubMp[3].Flag == 1) {
		pSubMp[3].CurSubMp = TEACHER_FAR_INDEX;
	}

	//��ʦȫ��
	if(pSubMp[4].Flag == 1) {
		pSubMp[4].CurSubMp = TEACHER_FAR_INDEX;
	}

	//��ʦȫ��
	if(pSubMp[5].Flag == 1) {
		pSubMp[5].CurSubMp = TEACHER_FAR_INDEX;
	}

	CurSwitch =	GetCurSwitchCmd(pMpState);


	int i = 0;

	for(i = 0; i < pMpState->num; i++) {

		if(pSubMp[i].CurSubMp != pSubMp[i].LastSubMp) {
			pSubMp[i].Flag = 1;
		} else {
			pSubMp[i].Flag = 0;
		}
	}

	pSubMp[0].LastSubMp =	pSubMp[0].CurSubMp;
	pSubMp[1].LastSubMp	=	pSubMp[1].CurSubMp;
	pSubMp[2].LastSubMp	=	pSubMp[2].CurSubMp;
	pSubMp[3].LastSubMp	=	pSubMp[3].CurSubMp;
	pSubMp[4].LastSubMp	=	pSubMp[4].CurSubMp;
	pSubMp[5].LastSubMp	=	pSubMp[5].CurSubMp;

	return CurSwitch;
}


UInt8 *GetIndexString(UInt32 index)
{
	switch(index) {
		case TEACHER_INDEX:
			return "Teacher";

		case VGA_INDEX:
			return "Vga";

		case STUDENT_INDEX:
			return "Student";

		case TEACHER_FAR_INDEX:
			return "Teacher_far";

		case STUDENT_FAR_INDEX:
			return "Student_far";

		case TEACHBLACKBORAD_INDEX1:
			return "Teachblackborad1";

		case TEACHBLACKBORAD_INDEX2:
			return "Teachblackborad2";

		default
				:
			return "Unkown";
	}
}

UInt32 SwitchMpStrategyInit()
{

	UInt32 i = 0;
	UInt32 j = 0;
	MpState *pMpState = NULL;


	for(i = 1; i <= 6; i++) {
		if(i == 1) {
			pMpState = &OneState;
			pMpState->num = 1;
		} else if(i == 2) {
			pMpState = &TwoState;
			pMpState->num = 2;

		} else if(i == 3) {
			pMpState = &ThreeState;
			pMpState->num = 3;
		} else if(i == 4) {
			pMpState = &FourState;
			pMpState->num = 4;
		} else if(i == 5) {
			pMpState = &FiveState;
			pMpState->num = 5;
		} else if(i == 6) {
			pMpState = &SixState;
			pMpState->num = 6;
		}

		zlog_debug(ptrackzlog, RED"MpMode [%d]\n"NONE , i);

		pMpState->StudentState = -1;
		pMpState->VgaState     = -1;
		pMpState->TeacherState = -1;



		for(j = 0; j < pMpState->num; j++) {

			pMpState->SubMpstate[j].keeptime[TEACHER_INDEX] = g_track_strategy_info.teacher_switch_students_delay_time;
			pMpState->SubMpstate[j].keeptime[TEACHBLACKBORAD_INDEX1] = g_track_strategy_info.teacher_switch_students_delay_time;
			pMpState->SubMpstate[j].keeptime[TEACHBLACKBORAD_INDEX2] = g_track_strategy_info.teacher_switch_students_delay_time;
			pMpState->SubMpstate[j].keeptime[TEACHER_FAR_INDEX] = g_track_strategy_info.teacher_switch_students_delay_time;//g_track_strategy_info.teacher_leave_panorama_time;
			pMpState->SubMpstate[j].keeptime[VGA_INDEX] =     g_track_strategy_info.vga_keep_time;
			pMpState->SubMpstate[j].keeptime[STUDENT_INDEX] = g_track_strategy_info.students_near_keep_time;
			pMpState->SubMpstate[j].keeptime[STUDENT_FAR_INDEX] = g_track_strategy_info.students_panorama_switch_near_time;

			pMpState->SubMpstate[j].CurSubMp = -1;
			pMpState->SubMpstate[j].LastSubMp = -1;
			pMpState->SubMpstate[j].Flag = 0;



		}

		gTrackState.vga          = VGANOCHANGE;
		gTrackState.students     = STUDONW;
		gTrackState.teacher      = TEACHLOSE;
		gTrackState.lastvga      = -1;
		gTrackState.laststudents = -1;
		gTrackState.lastteacher  = -1;
		gTrackState.LastSwitch   = -1;
		gTrackState.LastTrackMode = -1;
		gTrackState.LastMpMode   = -1;

		for(j = 0; j < pMpState->num; j++) {
			//zlog_debug(ptrackzlog,RED"           %s",GetIndexString(0),GetIndexString(1),GetIndexString(2),GetIndexString(3),GetIndexString(4));
			zlog_debug(ptrackzlog, RED"Strategy[%d] SubMp[%d]  keeptime teach[%d] vga[%d] stu[%d] teach_[%d] stu_[%d] black1[%d] black2[%d]\n"NONE , gStrategy , j, \
			           pMpState->SubMpstate[j].keeptime[0], pMpState->SubMpstate[j].keeptime[1], pMpState->SubMpstate[j].keeptime[2], \
			           pMpState->SubMpstate[j].keeptime[3], pMpState->SubMpstate[j].keeptime[4], pMpState->SubMpstate[j].keeptime[5], pMpState->SubMpstate[j].keeptime[6]);
		}

	}


	return 1;
}

MpState *GetMpHand(UInt32 num)
{

	MpState    *pMpState = NULL;

	if(num == ONE_MP) {
		pMpState       = &OneState;
	} else if((num == TWO_MP_1) || (num == TWO_MP_2) || (num == TWO_MP_3) || (num == TWO_MP_4)) {
		pMpState       = &TwoState;
	} else if(num == THREE_MP) {
		pMpState       = &ThreeState;
	} else if(num == FOUR_MP) {
		pMpState       = &FourState;
	} else if(num == FIVE_MP) {
		pMpState       = &FiveState;

	} else if((num == SIX_MP_1) || (num == SIX_MP_2)) {
		pMpState       = &SixState;

	} else {
		return 0;
	}


	return pMpState;
}


/*==============================================================================
    ����: <SwitchPictureLayout>
    ����:
    ����:
    ����ֵ:
    Created By ��� 2013.09.16 17:49:05 For Web
==============================================================================*/
UInt32 SwitchPictureLayout(MpState *pMpState, TrackState *pTrackState)
{

	int i = 0;
	int CurSwitch = 0;
	SubMpState *pSubMp   = NULL;

	int LastSwitch = 0;

	if(pMpState == NULL) {
		return 0;
	}


	pSubMp 		   = pMpState->SubMpstate;
	LastSwitch     = pTrackState->LastSwitch;


	//ȫ����û�����仯 ���л�
	//if((pMpState->StudentState == student) && (pMpState->TeacherState == teach) && (pMpState->VgaState == vga))
	//if((pMpState->TeacherState == teach) && (pMpState->VgaState == vga))
	{
		//	return 0;
	}

	pMpState->StudentState = pTrackState->students;
	pMpState->TeacherState = pTrackState->teacher;
	pMpState->VgaState 	   = pTrackState->vga;


	//�жϸ��Ի�����ʱ��� ��ʾ״̬
	for(i = 0; i < pMpState->num; i++) {
		pSubMp[i].Flag = 0;
		zlog_debug(ptrackzlog, "i = %d CurSubMp[%s] LastSubMp[%s]\n", i, GetIndexString(pSubMp[i].CurSubMp), GetIndexString(pSubMp[i].LastSubMp));

		//��ʱδ�� ����֮ǰ����
		if(CheckTimeOut(pMpState->num, i)) {
			pSubMp[i].Flag = 0;
			pSubMp[i].CurSubMp = pSubMp[i].LastSubMp;

		}
		//��ʱ���˿����л�
		else {
			//if(pSubMp[i].CurSubMp != pSubMp[i].LastSubMp)
			{
				pSubMp[i].Flag = 1;
				//pSubMp[i].LastSubMp = pSubMp[i].CurSubMp;
			}

		}
	}



	for(i = 0; i < pMpState->num; i++) {
		if(pSubMp[i].Flag == 1) {
			zlog_debug(ptrackzlog, "-----time out ---Flag[%d]\n", i);
		}
	}


#if 0

	//ȫ����û�����仯 ���л�
	for(i = 0; i < pMpState->num; i++) {
		if(pSubMp[i].Flag == 1) {
			break;
		}
	}

	if(i == pMpState->num) {
		return 0;
	}

#endif

	zlog_debug(ptrackzlog, "Ip[%s] Strategy[%d] Num[%d] [%s] StudentUp[%d] StudentState[%d] [%d] TeacherState[%d] [%d] VgaState[%d] [%d]\n", \
	           clientIp, gStrategy, pMpState->num, GetSwitchString(pTrackState->LastSwitch) , StudentUp, pMpState->StudentState, pSubMp[2].Flag, pMpState->TeacherState, pSubMp[0].Flag, pMpState->VgaState, pSubMp[1].Flag);

	if(1 == pMpState->num) {
		CurSwitch	= OneMpStrategy(pMpState);
	} else if(2 == pMpState->num) {
		CurSwitch	= TwoMpStrategy(pMpState);
	} else if(3 == pMpState->num) {
		//printf("%d\n",pMpState->VgaState);
		CurSwitch = ThreeMpStrategy(pMpState);
	} else if(4 == pMpState->num) {
		CurSwitch = FourMpStrategy(pMpState);
	} else if(5 == pMpState->num) {

	} else if(6 == pMpState->num) {
		CurSwitch = SixMpStrategy(pMpState);
	} else {
		return 0;
	}


	//	printf("------------------------------------\n");
	if(CurSwitch != LastSwitch) {
		if(CurSwitch == 0) {
			zlog_debug(ptrackzlog, RED"Warning CurSwitch == 0!!!!!!!\n"NONE , CurSwitch);
		} else {
			if(teacherTracerMove(CurSwitch)) {
				pTrackState->LastSwitch = CurSwitch;
			}

			zlog_debug(ptrackzlog, RED"@@@@@@@ [%x] %s \n"NONE , CurSwitch, GetSwitchString(CurSwitch));

		}
	}

	i = 0;

	for(i = 0; i < pMpState->num; i++) {
		if((pSubMp[i].Flag == 1) && (pSubMp[i].CurSubMp >= 0) && (pSubMp[i].CurSubMp < MAX_INDEX)) {
			AddTimeOut(pMpState->num, i, pSubMp[i].keeptime[pSubMp[i].CurSubMp]);

			zlog_debug(ptrackzlog, "--------AddTimeOut MpId[%d] SubMode[%s] keeptime[%d]\n-------------------------\n", i, GetIndexString(pSubMp[i].CurSubMp), pSubMp[i].keeptime[pSubMp[i].CurSubMp]);
			//AddTimeOut(num,i,5);
		}
	}

	return 1;
}

Int32 SetVgaState()
{
	gTrackState.vga = VGACHANGE;
}


UInt32 ChangeState(MpState *pMpState, TrackState *pTrackState)
{

	if(pTrackState == NULL) {
		return 0;
	}

	int j = 0;

	static UInt32 InBlackboard1Flag = 0;
	static UInt32 InBlackboard2Flag = 0;

	static UInt32 OutBlackboard1ToNearFlag = 0;
	static UInt32 OutBlackboard2ToNearFlag = 0;

	static UInt32 OutBlackboard1ToFarFlag = 0;
	static UInt32 OutBlackboard2ToFarFlag = 0;

	static UInt32 OutNearToFarFlag = 0;
	static UInt32 OutFarToNearFlag = 0;

	//g_track_encode_info.track_status = 1;
	//g_track_strategy_info.move_flag = 0;

	for(j = 0; j < pMpState->num; j++) {
		pMpState->SubMpstate[j].keeptime[TEACHER_INDEX]          = g_track_strategy_info.teacher_switch_students_delay_time;
		pMpState->SubMpstate[j].keeptime[TEACHBLACKBORAD_INDEX1] = g_track_strategy_info.teacher_switch_students_delay_time;
		pMpState->SubMpstate[j].keeptime[TEACHBLACKBORAD_INDEX2] = g_track_strategy_info.teacher_switch_students_delay_time;
		pMpState->SubMpstate[j].keeptime[TEACHER_FAR_INDEX]      = g_track_strategy_info.teacher_switch_students_delay_time;
		pMpState->SubMpstate[j].keeptime[VGA_INDEX]              = g_track_strategy_info.vga_keep_time;
		pMpState->SubMpstate[j].keeptime[STUDENT_INDEX]          = g_track_strategy_info.students_near_keep_time;
		pMpState->SubMpstate[j].keeptime[STUDENT_FAR_INDEX]      = g_track_strategy_info.students_panorama_switch_near_time;
	}

	//��ʦ��ʧ
	if(g_track_encode_info.track_status == 0) {
		g_track_strategy_info.students_track_flag = 0;
		StudentUp = 0;

		if((gStrategy == STRATEGY_5_1) && (g_track_strategy_info.position1_mv_flag == 1)) {
			//if((gStrategy == STRATEGY_5_1) && (g_track_strategy_info.position1_mv_flag == 1) && (pMpState->num == 1)) {
			pTrackState->teacher = TEACHMOVE;

			for(j = 0; j < pMpState->num; j++) {
				pMpState->SubMpstate[j].keeptime[TEACHER_FAR_INDEX] = g_track_strategy_info.mv_keep_time;
				pMpState->SubMpstate[j].keeptime[STUDENT_FAR_INDEX] = g_track_strategy_info.mv_keep_time;
			}
		} else if((gStrategy == STRATEGY_5_1) && (g_track_strategy_info.position1_mv_flag == 0)) {
			pTrackState->teacher = TEACHLOSE;

			for(j = 0; j < pMpState->num; j++) {
				pMpState->SubMpstate[j].keeptime[TEACHER_FAR_INDEX] = g_track_strategy_info.mv_keep_time;
				pMpState->SubMpstate[j].keeptime[STUDENT_FAR_INDEX] = g_track_strategy_info.mv_keep_time;
			}
		} else {
			pTrackState->teacher = TEACHLOSE;
		}

		InBlackboard1Flag = 0;
		InBlackboard2Flag = 0;

		OutBlackboard1ToNearFlag = 0;
		OutBlackboard2ToNearFlag = 0;

		OutBlackboard1ToFarFlag = 0;
		OutBlackboard2ToFarFlag = 0;

		OutNearToFarFlag = 0;
		OutFarToNearFlag = 0;

	}

	//�������״̬����
	else if(g_track_strategy_info.blackboard_region_flag1 == 1) {

		g_track_strategy_info.students_track_flag = 1;
		OutBlackboard1ToNearFlag = 0;
		OutBlackboard2ToNearFlag = 0;
		OutBlackboard1ToFarFlag  = 0;
		OutBlackboard2ToFarFlag  = 0;
		OutNearToFarFlag = 0;
		OutFarToNearFlag = 0;

		if((pTrackState->lastteacher != TEACHBLACKBORAD1) && (InBlackboard1Flag == 0)) {
			InBlackboard1Flag = 1;
			AddTimeOut(100, 1, g_track_strategy_info.teacher_blackboard_time1);
			zlog_debug(ptrackzlog, "--------AddTimeOut 100 1 %d\n", g_track_strategy_info.teacher_blackboard_time1);
		}

		if(CheckTimeOut(100, 1) == 0) {
			pTrackState->teacher = TEACHBLACKBORAD1;
			InBlackboard1Flag = 0;
		}


	} else if((g_track_strategy_info.blackboard_region_flag2 == 1) && (gStrategy == STRATEGY_5_2)) {
		g_track_strategy_info.students_track_flag = 1;
		OutBlackboard1ToNearFlag = 0;
		OutBlackboard2ToNearFlag = 0;
		OutBlackboard1ToFarFlag  = 0;
		OutBlackboard2ToFarFlag  = 0;
		OutNearToFarFlag = 0;
		OutFarToNearFlag = 0;


		if(pTrackState->lastteacher != TEACHBLACKBORAD2 && InBlackboard2Flag == 0) {
			InBlackboard2Flag = 1;
			AddTimeOut(100, 2, g_track_strategy_info.teacher_blackboard_time2);
			zlog_debug(ptrackzlog, "--------AddTimeOut 100 2 %d\n", g_track_strategy_info.teacher_blackboard_time2);
		}

		if(CheckTimeOut(100, 2) == 0) {
			pTrackState->teacher = TEACHBLACKBORAD2;
			InBlackboard2Flag = 0;
		}

	} else {
		g_track_strategy_info.students_track_flag = 1;
		InBlackboard1Flag = 0;
		InBlackboard2Flag = 0;



		//ֹͣ״̬
		if((g_track_strategy_info.move_flag == 0)
		   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_left_limit) < 10)
		   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_right_limit) < 10))
			//if(g_track_strategy_info.move_flag==0)

		{

			if((abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_left_limit) < 10)
			|| (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_right_limit) < 10)) {
				zlog_debug(ptrackzlog, "g_track_strategy_info.cur_pan_tilt-g_track_strategy_info.cam_left_limit)<10");
			}

			//OutBlackboard1ToFarFlag = 0;
			//OutBlackboard1ToFarFlag = 0;
			OutNearToFarFlag = 0;

			//�ϸ����ƶ�״̬
			if(TEACHMOVE == pTrackState->lastteacher && OutFarToNearFlag == 0) {

				AddTimeOut(100, 5, g_track_strategy_info.teacher_leave_panorama_time);
				OutFarToNearFlag = 1;
				zlog_debug(ptrackzlog, "--------AddTimeOut 100 5 %d\n", g_track_strategy_info.teacher_blackboard_time1);
			}

			//�ϸ��ǰ���1
			else if(TEACHBLACKBORAD1 == pTrackState->lastteacher && OutBlackboard1ToNearFlag == 0) {
				AddTimeOut(100, 6, g_track_strategy_info.teacher_leave_blackboard_time1);
				OutBlackboard1ToNearFlag = 1;
				zlog_debug(ptrackzlog, "--------AddTimeOut 100 6 %d\n", g_track_strategy_info.teacher_leave_blackboard_time1);
			}

			//�ϸ��ǰ���2
			else if(TEACHBLACKBORAD2 == pTrackState->lastteacher && OutBlackboard2ToNearFlag == 0) {
				AddTimeOut(100, 7, g_track_strategy_info.teacher_leave_blackboard_time2);
				OutBlackboard2ToNearFlag = 1;
				zlog_debug(ptrackzlog, "--------AddTimeOut 100 7 %d\n", g_track_strategy_info.teacher_leave_blackboard_time2);
			}

			if((CheckTimeOut(100, 5) == 0) && (CheckTimeOut(100, 6) == 0) && (CheckTimeOut(100, 7) == 0)) {
				pTrackState->teacher = TEACHPLATFORM;
				OutFarToNearFlag = 0;
				OutBlackboard1ToNearFlag = 0;
				OutBlackboard2ToNearFlag = 0;
			}
		}
		//�ƶ�״̬
		else {
			OutFarToNearFlag = 0;

			//OutBlackboard1ToNearFlag = 0;
			//OutBlackboard2ToNearFlag = 0;
			//�ϸ���ֹͣ״̬
			if(TEACHPLATFORM == pTrackState->lastteacher && OutNearToFarFlag == 0) {
				OutNearToFarFlag = 1;
				AddTimeOut(100, 8, g_track_strategy_info.teacher_panorama_time);

				zlog_debug(ptrackzlog, "--------AddTimeOut 100 8 %d\n", g_track_strategy_info.teacher_panorama_time);
			}

			//�ϸ��ǰ���1
			else if(TEACHBLACKBORAD1 == pTrackState->lastteacher && OutBlackboard1ToFarFlag == 0) {
				OutBlackboard1ToFarFlag = 1;
				AddTimeOut(100, 9, g_track_strategy_info.teacher_leave_blackboard_time1);
				zlog_debug(ptrackzlog, "--------AddTimeOut 100 9 %d\n", g_track_strategy_info.teacher_leave_blackboard_time1);
			}

			//�ϸ��ǰ���2
			else if(TEACHBLACKBORAD2 == pTrackState->lastteacher && OutBlackboard2ToFarFlag == 0) {
				OutBlackboard2ToFarFlag = 1;
				AddTimeOut(100, 10, g_track_strategy_info.teacher_leave_blackboard_time2);

				zlog_debug(ptrackzlog, "--------AddTimeOut 100 10 %d\n", g_track_strategy_info.teacher_leave_blackboard_time2);
			}


			if((CheckTimeOut(100, 8) == 0) && (CheckTimeOut(100, 9) == 0) && (CheckTimeOut(100, 10) == 0)) {
				pTrackState->teacher = TEACHMOVE;
				OutNearToFarFlag = 0;
				OutBlackboard1ToFarFlag = 0;
				OutBlackboard2ToFarFlag = 0;

			}
		}
	}

	if((g_track_strategy_info.send_cmd == SWITCH_STUDENTS) && (g_track_strategy_info.switch_cmd_author == AUTHOR_STUDENTS)) {

		pTrackState->students = STUUP;
	} else {
		pTrackState->students = STUDONW;
	}

	//pTrackState->students = STUDONW;
}

#if 0

//
int MpChangeLayout()
{
#if 0
	static Int32 InBlackboardFlag = 0;
	static Int32 OutBlackboardFlag = 0;

	Int32 vga = VGANOCHANGE;
	static Int32 students = STUDONW;
	static Int32 teacher  = TEACHLOSE;

	static Int32 lastvga = -1;
	static Int32 laststudents = -1;
	static Int32 lastteacher  = -1;
#endif

	static Int32 jj = 0;
	static Int32 ii = 1;


#if 0
	//g_track_encode_info.track_status = 0;

	jj++;

	if(jj == 10) {
		g_track_encode_info.track_status = 1;
		g_track_strategy_info.move_flag  = 0;
		//teacher = TEACHPLATFORM;

	} else if(jj == 13) {
		g_track_strategy_info.move_flag  = 1;
		//teacher = TEACHPLATFORM;

	} else if(jj == 16) {
		g_track_strategy_info.move_flag  = 0;
		//teacher = TEACHPLATFORM;

	} else if(jj == 19) {
		g_track_strategy_info.move_flag  = 1;
		//teacher = TEACHPLATFORM;

	} else if(jj == 22) {
		g_track_strategy_info.move_flag  = 1;
		g_track_strategy_info.blackboard_region_flag1 = 1;
		zlog_debug(ptrackzlog, RED"in blackboard_region_flag1\n"NONE);
		//teacher = TEACHPLATFORM;

	} else if(jj == 25) {

		g_track_strategy_info.move_flag  = 0;
		g_track_strategy_info.blackboard_region_flag1 = 0;
		//gTrackState.teacher = TEACHMOVE;
	} else if(jj == 28) {

		g_track_strategy_info.move_flag  = 1;
		g_track_strategy_info.blackboard_region_flag1 = 0;
		//gTrackState.teacher = TEACHMOVE;
	} else if(jj == 35) {
		g_track_strategy_info.move_flag  = 1;
		g_track_strategy_info.blackboard_region_flag1 = 1;
		zlog_debug(ptrackzlog, RED"in blackboard_region_flag1\n"NONE);
		// vga = VGACHANGE;
	} else if(jj == 38) {
		g_track_encode_info.track_status = 1;
		g_track_strategy_info.move_flag  = 0;
		g_track_strategy_info.blackboard_region_flag1 = 0;

		zlog_debug(ptrackzlog, RED"out blackboard_region_flag1\n"NONE);
		//teacher = TEACHBLACKBORAD1;
	} else if(jj == 40) {
		g_track_encode_info.track_status = 1;
		g_track_strategy_info.move_flag  = 0;
		g_track_strategy_info.blackboard_region_flag1 = 1;

		zlog_debug(ptrackzlog, RED"in blackboard_region_flag1\n"NONE);
		//teacher = TEACHBLACKBORAD1;
	} else if(jj == 60) {
		g_track_encode_info.track_status = 1;
		g_track_strategy_info.move_flag  = 0;
		g_track_strategy_info.blackboard_region_flag1 = 0;

		zlog_debug(ptrackzlog, RED"out blackboard_region_flag1\n"NONE);
		//students = STUUP;
		//StudentUp = 1;
	} else if(jj == 80) {
		//students = STUDONW;

		g_track_encode_info.track_status = 1;
		g_track_strategy_info.move_flag  = 0;
		g_track_strategy_info.blackboard_region_flag2 = 1;
		zlog_debug(ptrackzlog, RED"in blackboard_region_flag2\n"NONE);
	} else if(jj == 100) {
		//students = STUUP;
		//StudentUp = 1;

		g_track_encode_info.track_status = 1;
		g_track_strategy_info.move_flag  = 1;
		g_track_strategy_info.blackboard_region_flag2 = 0;

		zlog_debug(ptrackzlog, RED"out blackboard_region_flag2\n"NONE);

	} else if(jj == 120) {
		g_track_encode_info.track_status = 1;
		g_track_strategy_info.move_flag  = 0;
		//students = STUUP;
		//StudentUp = 1;
		g_track_strategy_info.blackboard_region_flag1 = 1;
	} else if(jj == 140) {
		gTrackState.students = STUUP;
		StudentUp = 1;
		g_track_strategy_info.move_flag  = 1;

	} else if(jj == 160) {
		gTrackState.students = STUUP;
		StudentUp = 1;
		g_track_strategy_info.blackboard_region_flag1 = 0;

	} else if(jj == 180) {
		gTrackState.vga = VGACHANGE;

	} else if(jj == 200) {
		gTrackState.students = STUDONW;

	} else if(jj == 220) {
		gTrackState.students = STUUP;
		StudentUp = 1;
		g_track_encode_info.track_status = 1;
		g_track_strategy_info.move_flag  = 1;
		g_track_strategy_info.blackboard_region_flag1 = 1;


	} else if(jj == 222) {
		gTrackState.students = STUUP;
		StudentUp = 1;
		g_track_strategy_info.blackboard_region_flag1 = 0;

	} else if(jj == 234) {
		gTrackState.students = STUDONW;

	} else if(jj == 240) {
		g_track_encode_info.track_status = 0;
		g_track_strategy_info.move_flag  = 1;
		g_track_strategy_info.blackboard_region_flag1 = 0;
		g_track_strategy_info.blackboard_region_flag2 = 0;
		//teacher = TEACHLOSE;

	} else if(jj == 260) {
		ii++;

		if(ii == 1) {
			gMpMode	= ONE_MP;
		}

		if(ii == 2) {
			gMpMode	= TWO_MP_1;
		}
		//else if(ii == 3)
		//{
		//	gMpMode	= TWO_MP_2;
		//	}
		//	else if(ii == 4)
		//	{
		//		gMpMode	= TWO_MP_3;
		//	}
		else if(ii == 3) {
			gMpMode	= THREE_MP;
		} else if(ii == 4) {
			gMpMode	= FOUR_MP;
			ii = 0;
		}

		zlog_debug(ptrackzlog, RED"@@@@@@@@@@@**111*@@@@@@@@@@@@@ %d\n"NONE, ii);

		jj = 0;
	}

#endif


#if 0

	//��ʦ״̬ ����
	if(g_track_encode_info.track_status == 0) {
		teacher = TEACHLOSE;
		g_track_strategy_info.students_track_flag = 0;
		InBlackboardFlag = 0;
		OutBlackboardFlag = 0;
	}

	//�������״̬����
	else if((g_track_strategy_info.blackboard_region_flag1 == 1) && (lastteacher != TEACHBLACKBORAD1) && (InBlackboardFlag == 0)) {
		AddTimeOut(100, 1, g_track_strategy_info.teacher_blackboard_time1);
		zlog_debug(ptrackzlog, "--------AddTimeOut 100 1 %d\n", g_track_strategy_info.teacher_blackboard_time1);
		g_track_strategy_info.students_track_flag = 1;

		InBlackboardFlag = 1;
	} else if((g_track_strategy_info.blackboard_region_flag1 == 1) && (CheckTimeOut(100, 1) == 0) && InBlackboardFlag) {
		teacher = TEACHBLACKBORAD1;
		g_track_strategy_info.students_track_flag = 1;
		InBlackboardFlag = 0;
	}

	else if((g_track_strategy_info.blackboard_region_flag2 == 1) && (gStrategy == STRATEGY_5_2) && (lastteacher != TEACHBLACKBORAD2) && (InBlackboardFlag == 0)) {
		AddTimeOut(100, 2, g_track_strategy_info.teacher_blackboard_time2);
		zlog_debug(ptrackzlog, "--------AddTimeOut 100 2 %d\n", g_track_strategy_info.teacher_blackboard_time2);
		//teacher = TEACHBLACKBORAD2;
		g_track_strategy_info.students_track_flag = 1;
		InBlackboardFlag = 1;
	} else if((g_track_strategy_info.blackboard_region_flag2 == 1) && (gStrategy == STRATEGY_5_2) && (CheckTimeOut(100, 2) == 0)) {
		teacher = TEACHBLACKBORAD2;
		g_track_strategy_info.students_track_flag = 1;
		InBlackboardFlag = 0;
	}

	//�뿪����״̬����

	else if((g_track_strategy_info.blackboard_region_flag1 == 0) && (lastteacher == TEACHBLACKBORAD1) && (OutBlackboardFlag == 0)) {
		AddTimeOut(100, 3, g_track_strategy_info.teacher_leave_blackboard_time1);
		g_track_strategy_info.students_track_flag = 1;
		OutBlackboardFlag = 1;
		zlog_debug(ptrackzlog, "--------AddTimeOut 100 3 %d\n", g_track_strategy_info.teacher_leave_blackboard_time1);
	}

	else if((g_track_strategy_info.blackboard_region_flag2 == 0) && (gStrategy == STRATEGY_5_2) && (lastteacher == TEACHBLACKBORAD2) && (OutBlackboardFlag == 0)) {
		AddTimeOut(100, 4, g_track_strategy_info.teacher_leave_blackboard_time2);
		zlog_debug(ptrackzlog, "--------AddTimeOut 100 4 %d\n", g_track_strategy_info.teacher_leave_blackboard_time2);
		g_track_strategy_info.students_track_flag = 1;
		OutBlackboardFlag = 1;
	} else if((g_track_strategy_info.blackboard_region_flag1 == 0) && (CheckTimeOut(100, 3) == 0) && (gStrategy != STRATEGY_5_2)) {
		InBlackboardFlag = 0;
		OutBlackboardFlag = 0;
		g_track_strategy_info.students_track_flag = 1;

		if((g_track_strategy_info.move_flag == 0)
		   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_left_limit) < 10)
		|| (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_right_limit) < 10)) {
			teacher = TEACHPLATFORM;
		} else {
			teacher = TEACHMOVE;
		}
	} else if((g_track_strategy_info.blackboard_region_flag1 == 0) && (CheckTimeOut(100, 3) == 0)
	          && (g_track_strategy_info.blackboard_region_flag2 == 0) && (CheckTimeOut(100, 4) == 0)
	          && (gStrategy == STRATEGY_5_2))

	{
		InBlackboardFlag = 0;
		OutBlackboardFlag = 0;
		g_track_strategy_info.students_track_flag = 1;


		if((g_track_strategy_info.move_flag == 0)
		   || (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_left_limit) < 10)
		|| (abs(g_track_strategy_info.cur_pan_tilt - g_track_strategy_info.cam_right_limit) < 10)) {
			//�ϸ����ƶ�״̬
			if(TEACHMOVE == lastteacher) {
				AddTimeOut(100, 5, g_track_strategy_info.teacher_leave_panorama_time);
			}

			if(CheckTimeOut(100, 5) == 0) {
				teacher = TEACHPLATFORM;
			}
		} else {
			//�ϸ����ƶ�״̬
			if(TEACHPLATFORM == lastteacher) {
				AddTimeOut(100, 6, g_track_strategy_info.teacher_panorama_time);
			}

			if(CheckTimeOut(100, 6) == 0) {
				teacher = TEACHMOVE;
			}
		}
	} else {
		//teacher  = TEACHLOSE;
	}


	//ѧ��״̬ ����
	//teacher = TEACHBLACKBORAD;

	if((g_track_strategy_info.send_cmd == SWITCH_STUDENTS) && (g_track_strategy_info.switch_cmd_author == AUTHOR_STUDENTS)) {

		//students = STUUP;
	} else {
		//students = STUDONW;
	}

#endif

	ChangeState(&gTrackState);

	//VGA״̬ ����
	if((gTrackState.vga != gTrackState.lastvga) || (gTrackState.students != gTrackState.laststudents) || (gTrackState.teacher != gTrackState.lastteacher))
		//if((vga != lastvga)  || (teacher != lastteacher))
	{
		zlog_debug(ptrackzlog, RED"@@@***@@@@ VGA[%d] TEACHER[%d] STUDENT[%d] @@@***@@@ \n"NONE , gTrackState.vga, gTrackState.teacher, gTrackState.students);

		gTrackState.lastvga = gTrackState.vga;
		gTrackState.laststudents = gTrackState.students;
		gTrackState.lastteacher  = gTrackState.teacher;
	}

	SwitchPictureLayout(gMpMode, &gTrackState);
	gTrackState.vga = VGANOCHANGE;

#if 0
	static int i = 0;

	while(1) {
		usleep(500 * 1000);
		i++;

		if(i == 1) {
			//zlog_debug(ptrackzlog,RED"@@@@@@@ [%d] \n"NONE ,i);
			SwitchPictureLayout(6, VGANOCHANGE, TEACHLOSE, STUDONW);
		}

		if(i == 2) {
			//zlog_debug(ptrackzlog,RED"@@@@@@@ [%d] \n"NONE ,i);
			SwitchPictureLayout(6, VGANOCHANGE, TEACHPLATFORM, STUUP);
		}


		if(i == 3) {
			//zlog_debug(ptrackzlog,RED"@@@@@@@ [%d] \n"NONE ,i);
			SwitchPictureLayout(6, VGANOCHANGE, TEACHMOVE, STUDONW);
		}


		if(i == 4) {
			//zlog_debug(ptrackzlog,RED"@@@@@@@ [%d] \n"NONE ,i);
			SwitchPictureLayout(6, VGANOCHANGE, TEACHBLACKBORAD, STUUP);


		}

		if(i == 5) {
			//zlog_debug(ptrackzlog,RED"@@@@@@@ [%d] \n"NONE ,i);
			SwitchPictureLayout(6, VGACHANGE, TEACHBLACKBORAD, STUUP);

			i = 0;
		}
	}

	usleep(500 * 1000);

	switch(LayoutMode) {
		case SINGLEPICTURE:
			break;

		case TWOPICTURE:
			break;

		case THREEPICTURE:

			break;

		case FOUREPICTURE:
			FourePictureLayout();
			break;

		case FIVEPICTURE:
			break;

		case SIXPICTURE:
			break;

		default
				:
			break;

	}

#endif

	return 0;
}

#endif
UInt32 FirstPicSwitch(TrackState *pTrackState)
{
	int i = 0;
	if(pTrackState->FirstPic <= 0)
	{
		return 0;
	}

	//����׻����Ƿ�ʱ
	if(CheckTimeOut(999, 0)) 

	//��ʱδ��
	{
		//DelTimeOut(999, 0);
	}
	//��ʱ����
	else
	{
		pTrackState->FirstPic = -1;
		pTrackState->LastSwitch = -1;
	}

	return 1;
}

/**
* @	�л����ԣ���Ҫ�Ǵ����ж�Ӧ������ʦ����Ӧ�����������л��Ǹ���λ������
* @	�Լ���֪ѧ������Ҫ��ʲô����
*/
void SENDMOVE_TEACHER(void *pParam)
{
	pthread_detach(pthread_self());
	g_track_strategy_timeinfo.switch_flag = 1;
	Int32 send_students = 0;
	SwitchMpStrategyInit();
	send_students = 1;
	Int32 LastMpMode = -1;
	static int i = 0;
	sleep(2);
	ITRACK_DynamicParams *pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams.TrackParms.dynamic_param;

	while(!gEduKit->Start) {
		zlog_debug(ptrackzlog, RED"@@@ wait  Start Ing @@@@ ");
		sleep(1);
	}

	while(sendCmdToStudentTracer((unsigned char *)&send_students, sizeof(int), MSG_SET_TRACK_TYPE)) {

		zlog_debug(ptrackzlog, RED"@@@not connect student @@@@ ");
		sleep(1);
	}

	g_track_strategy_info.students_track_flag = 0;

	while(1) {

		usleep(200 * 1000);
		MpState *pMpState = GetMpHand(gMpMode);

		if(pMpState == NULL) {
			zlog_debug(ptrackzlog, RED"@@@error gMpMode %d@@@@ ", gMpMode);
			continue;
		}

		if((gMpMode != gTrackState.LastMpMode) || (pdynamic_param->control_mode != gTrackState.LastTrackMode)) {
			gTrackState.LastMpMode = gMpMode;
			gTrackState.LastTrackMode = pdynamic_param->control_mode;
			gTrackState.LastSwitch = -1;
		}

		//��ȡvga ��ʦ ѧ��״̬
		ChangeState(pMpState, &gTrackState);

		//VGA״̬ ����
		if((gTrackState.vga != gTrackState.lastvga) || (gTrackState.students != gTrackState.laststudents) || (gTrackState.teacher != gTrackState.lastteacher))
			//if((vga != lastvga)  || (teacher != lastteacher))
		{
			zlog_debug(ptrackzlog, RED"@@@***@@@@ VGA[%d] TEACHER[%d] STUDENT[%d] @@@***@@@ \n"NONE , gTrackState.vga, gTrackState.teacher, gTrackState.students);

			gTrackState.lastvga      = gTrackState.vga;
			gTrackState.laststudents = gTrackState.students;
			gTrackState.lastteacher  = gTrackState.teacher;
		}


		//printf("gTrackState.FirstPic [%d] = %d [%s]\n",gTrackState.FirstPic,TRACKVERSION);

		if(gTrackState.FirstPic > 0)
		{
			FirstPicSwitch(&gTrackState);
		}
		else
		{
			SwitchPictureLayout(pMpState, &gTrackState);
		}
		gTrackState.vga = VGANOCHANGE;

	}
}

/**
* @	��������ʦ������������Ҫ���̣߳�������ѧ����������Ҫ���߳�
*/
void TEACHERTRACER(void *pParam)
{
	pthread_t rtmpid[6] ;
	int result;
	pthread_detach(pthread_self());
	TimeoutLockInit();

	//result =  ConfigGetKey("studenttracer.ini", "studenttracer", "ipaddr", STUDENTTRACE_IP);

	//if(result != 0) {
	//DEBUG(DL_ERROR, "Get studenttrace ipaddr failed\n");
	//}

	result = pthread_create(&rtmpid[0], NULL, (void *)TimeOutdeal, (void *)NULL);

	if(result < 0) {
		DEBUG(DL_ERROR, "create TEACHER_TRACETCP() failed\n");
	}


	result = pthread_create(&rtmpid[1], NULL, (void *)TEACHER_TRACETCP, (void *)NULL);

	if(result < 0) {
		DEBUG(DL_ERROR, "create TEACHER_TRACETCP() failed\n");
	}

#if 1
	result = pthread_create(&rtmpid[2], NULL, (void *)HEART, (void *)NULL);


	if(result < 0) {
		DEBUG(DL_ERROR, "create HEART() failed\n");
	}


	result = pthread_create(&rtmpid[3], NULL, (void *)STUDENT_TRACETCP, (void *)NULL);


	if(result < 0) {
		DEBUG(DL_ERROR, "create STUDENT_TRACETCP() failed\n");
	}

	result = pthread_create(&rtmpid[4], NULL, (void *)SENDMOVE_TEACHER, (void *)NULL);


	if(result < 0) {
		DEBUG(DL_ERROR, "create SENDMOVE_TEACHER() failed\n");
	}

#endif

	while(1) {
		sleep(5);
	}
}


static int GetWebSocketNum()
{
	int i = 0;

	for(i = 0; i < WEBCONNECT_NUM_MAX; i++) {
		if(WebSocket[i] == -1) {
			return i;
		}
	}

	return -1;
}
/**
* @	����ʦ����Ӧ�������ļ����ˣ������Ƿ��б����������ӣ��еĻ��򴴽�
* @	���������߳�
*/
void WebListen(void *param)
{
	pthread_t TCPCmd[WEBCONNECT_NUM_MAX];
	int clientSocket;
	int listenSocket;
	int nPos = 0;
	WEBMSGHEAD 		msg_send;
	unsigned char 	sendBuf[256] 	= {0};
	struct sockaddr_in clientAddr, srvAddr;
	int opt = 1;
	pthread_detach(pthread_self());
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	bzero(&srvAddr, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvAddr.sin_port = htons(web_PORT);
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(listenSocket, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0) {
		DEBUG(DL_DEBUG, "yyyyyyyyyyyyyyyy web bind error:");
	}

	if(listen(listenSocket, 10) < 0) {
		DEBUG(DL_DEBUG, "yyyyyyyyyyyyyyyy web listen error:");
	}

	while(1) {
		int result, len, nSize = 1, nLen;
		len = sizeof(clientAddr);
		clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, (unsigned int *)&len);
		DEBUG(DL_DEBUG, "yyyyyyyyyyyyyyyyyyyyyyyy web connet you %d\n", clientSocket);

		if(0 > clientSocket) {
			DEBUG(DL_DEBUG, "yyyyyyyyyyyyyyyyyyyyyyyy accept errno=%d:%s\n", errno, strerror(errno));
			close(clientSocket);
			usleep(100000);
			continue;
		}

		nPos = GetWebSocketNum();

		if(nPos == -1) {
			DEBUG(DL_DEBUG, "yyyyyyyyyyyyyyyyyyyyyyyy web socket max \n");
			//������Ҫ����һ����Ϣ
			msg_send.nLen = sizeof(WEBMSGHEAD);
			msg_send.nType = WEBMSG_RECORD_GETMAXSOCKET;
			memcpy(sendBuf, (unsigned char *)&msg_send, sizeof(WEBMSGHEAD));
			len = send(clientSocket, sendBuf, msg_send.nLen, 0);

			if(len < (int)msg_send.nLen) {
				DEBUG(DL_DEBUG, "yyyyyyyyyyyyy WEBMSG_RECORD_GETMAXSOCKET send error[%d:%s]\n", errno, strerror(errno));
				close(clientSocket);
				return;
			}

			close(clientSocket);
			continue;
		} else {
			//DEBUG(DL_DEBUG,"yyyyyyyyyyyyyyyyyyyyyyyy web socket num=%d \n",nPos);
			WebSocket[nPos] = clientSocket;
		}

		if((setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
		sizeof(nSize))) == -1) {
			DEBUG(DL_DEBUG, "yyyyyyyyyyyyyyyyy setsockopt failed");
		}

		nSize 	= 0;
		nLen 	= sizeof(nSize);
		result 	= getsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, &nSize , (DWORD *)&nLen);

		if(result == 1) {
			DEBUG(DL_DEBUG, "yyyyyyyyyyyyyyy getsockopt()  socket:%d  result:%d\n", clientSocket, result);
		}

		nSize = 1;

		if(setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize))) {
			DEBUG(DL_DEBUG, "set IPPROTOT error\n");
		}

		result = pthread_create(&TCPCmd[nPos], NULL, (void *)WebTcpProcess, (void *)nPos);

		if(result) {
			close(clientSocket);	//
			DEBUG(DL_ERROR, "creat pthread WebTcpProcess error	= %d!\n" , errno);
			continue;
		}
	}

}


/**
* @	����ʦ����Ӧ�ı����������л�����
*/
int teacherTracerMove(unsigned short int data)
{

	ITRACK_DynamicParams *pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams.TrackParms.dynamic_param;


	int ret = 1;

#if 1
	unsigned char sendData[256];
	int len, num;
	MSGHEAD msg;
	msg.nLen = 2 + sizeof(MSGHEAD);
	msg.nMsg = 48;
	memcpy(sendData, &msg, sizeof(MSGHEAD));
	num = sizeof(MSGHEAD);
	sendData[num] = data & 0xff;
	sendData[num + 1] = (data >> 8) & 0xff;

	printf("&&&&&&&&&&&&&&&&&&&&&&%x %x\n", sendData[num], sendData[num + 1]);

	if(getServerSocket() < 0) {
		ret = 0;
	} else {
		len = send(getServerSocket(), sendData, 2 + sizeof(MSGHEAD), 0);

		if(len < 1) {
			close(getServerSocket());
			setServerSocket(-1);
			ret = 0;
		}

	}

	zlog_debug(ptrackzlog, RED"[TEACHER] To Recser Switch cmd: %s [%d]\n"NONE, GetSwitchString(data), ret);
#endif

	if(AUTO_CONTROL != pdynamic_param->control_mode) {
		return 0;
	}


	return ret;
}

/**
* @	��ѧ�������������
*/
int sendCmdToStudentTracer(unsigned char *data, int len, int cmd)
{
	unsigned char sendData[256];
	int lenth;
	MSGHEAD msg;
	msg.nLen = len + sizeof(MSGHEAD);
	msg.nMsg = cmd;
	memcpy(sendData, &msg, sizeof(MSGHEAD));
	memcpy(sendData + sizeof(MSGHEAD), data, len);

	if(getStudentSocket() < 0) {
		return -1;
	} else {
		lenth = send(getStudentSocket(), sendData, len + sizeof(MSGHEAD), 0);

		if(len < 1) {
			close(getStudentSocket());
			setStudentSocket(-1);
			return -1;
		}
	}

	return 0;
}

