/*********************************************************************

Co.Name	:	   Reach Software
FileName:		   remotectrl2.c
Creater：     yangshh                    Date: 2010-08-18
Function：	  Remote Control Module
Other Description:	Control Camera	for com port

**********************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>

#include "remotectrl.h"

//#include "commontrack.h"
/*remote config File */

extern  int gRemoteFD;
extern  int gStuRemoteFD;
RemoteConfig gRemote;
int compipe[2];
char FarCtrlList[7][20] = {{"EVI-D70"},	//protocol for remote
	{"EVI-D100"},
	{"PELCO-D"},
	{"PELCO-P"},
	{"VCC-H80PI"},
	{""},
	{""}
};


static unsigned long writen(int fd, const void *vptr, size_t n)
{
	unsigned long nleft;
	unsigned long nwritten;
	const char      *ptr;

	ptr = vptr;
	nleft = n;

	while(nleft > 0) {
		if((nwritten = write(fd, ptr, nleft)) <= 0) {
			if(nwritten < 0 && errno == EINTR) {
				nwritten = 0;    /* and call write() again */
			} else {
				return(-1);    /* error */
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return(n);
}

/*send data to tty com*/
int SendDataToCom(int fd, unsigned char *data, int len)
{
	unsigned long real_len = 0 ;
	//int i;

	if((real_len = writen(fd, data, len)) != len) {
		printf("SendDataToCom() write tty error\n");
		return -1;
	}

	r_usleep(20000);
	return (real_len);
}


int SendDataToComNoDelay(int fd, unsigned char *data, int len)
{
	unsigned long real_len = 0 ;

	if((real_len = writen(fd, data, len)) != len) {
		printf("SendDataToCom() write tty error\n");
		return -1;
	}

	return (real_len);
}



/*initial camera*/
int InitCamera(int fd)
{
	unsigned char comm[16];
	int length = 0;
	int ret = -1;

	if(fd == -1) {
		goto INITCEXIT;
	}

	memset(comm, 0, sizeof(comm));
	comm[0] = 0x88;
	comm[1] = 0x01;
	comm[2] = 0x00;
	comm[3] = 0x01;
	comm[4] = 0xFF;
	length = 5;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);
	memset(comm, 0, sizeof(comm));
	comm[0] = 0x88;
	comm[1] = 0x30;
	comm[2] = 0x01;
	comm[3] = 0xff;
	length = 4;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);

	memset(comm, 0, sizeof(comm));
	comm[0] = 0x81;
	comm[1] = 0x09;
	comm[2] = 0x00;
	comm[3] = 0x02;
	comm[4] = 0xFF;
	length = 5;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);

	memset(comm, 0, sizeof(comm));
	comm[0] = 0x81;
	comm[1] = 0x01;
	comm[2] = 0x06;
	comm[3] = 0x08;
	comm[4] = 0x02;
	comm[5] = 0xFF;
	length = 6;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);

	memset(comm, 0, sizeof(comm));
	comm[0] = 0x81;
	comm[1] = 0x01;
	comm[2] = 0x7d;
	comm[3] = 0x01;
	comm[4] = 0x03;
	comm[5] = 0x00;
	comm[6] = 0x00;
	comm[7] = 0xFF;
	length = 8;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);

	memset(comm, 0, sizeof(comm));
	comm[0] = 0x81;
	comm[1] = 0x01;
	comm[2] = 0x04;
	comm[3] = 0x38;
	comm[4] = 0x02;
	comm[5] = 0xFF;
	length = 6;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);

	memset(comm, 0, sizeof(comm));
	comm[0] = 0x81;
	comm[1] = 0x01;
	comm[2] = 0x04;
	comm[3] = 0x06;
	comm[4] = 0x03;
	comm[5] = 0xFF;
	length = 6;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);

	printf("InitCamera() Succeed\n");
	return 0;
INITCEXIT:
	printf("InitCamera() Error\n");
	return -1;
}

int CCtrlCodeAnalysis1(BYTE *pCode, char *strCode, BYTE Address, BYTE hSpeed, BYTE vSpeed, short nHRange, short nWRange)
{
	char pstr[16];

	int nIndex = 0;
	char *ptmp = 0;
	char *ptmp1 = 0;
	int nHIndex = 3;
	int nWIndex = 3;

	ptmp1 = strCode;
	ptmp = strstr(strCode, ",");
	memset(pstr, 0, 16);

	while(ptmp != NULL) {
		strncpy(pstr, ptmp1, ptmp - ptmp1);

		if(pstr[0] == 0x5e) {
			pCode[nIndex] = Address;
		} else if(pstr[0] == 'H' && pstr[1] == '(') {
			pCode[nIndex] = hSpeed;
		} else if(pstr[0] == 'V' && pstr[1] == '(') {
			pCode[nIndex] = vSpeed;
		} else if(pstr[0] == '0' && pstr[1] == 'Y') {
			pCode[nIndex] = (nWRange >> (4 * nWIndex)) & 0xf;
			nWIndex--;
		} else if(pstr[0] == '0' && pstr[1] == 'Z') {
			pCode[nIndex] = (nHRange >> (4 * nHIndex)) & 0xf;
			nHIndex--;
		} else {
			pCode[nIndex] = strtol(pstr, 0, 16);
		}

		nIndex++;
		ptmp1 = ptmp + 1;
		ptmp = strstr(ptmp + 1, ",");
		memset(pstr, 0, 16);
	}

	strcpy(pstr, ptmp1);

	if(pstr[0] == '=') {
		if(pstr[1] == '7') {
			pCode[nIndex] = pCode[1] ^ pCode[2] ^ pCode[3] ^ pCode[4] ^ pCode[5] ^ pCode[6];
		}
	} else if(pstr[0] == '+') {
		if(pstr[1] == '5') {
			pCode[nIndex] = (pCode[1] + pCode[2] + pCode[3] + pCode[4] + pCode[5]) % 256;
		}
	} else {
		pCode[nIndex] = strtol(pstr, 0, 16);
	}

	return nIndex + 1;
}
/*control command Analysis*/
int CCtrlCodeAnalysis(unsigned char *pCode, char *strCode,
                      unsigned char Address, unsigned char hSpeed,
                      unsigned char vSpeed)
{
	char pstr[16];
	int nIndex = 0;
	char *ptmp = 0;
	char *ptmp1 = 0;

	memset(pstr, 0, 16);

	ptmp1 = strCode;
	ptmp = strstr(strCode, ",");

	while(ptmp != NULL) {
		strncpy(pstr, ptmp1, ptmp - ptmp1);

		if(pstr[0] == 0x5e) {
			pCode[nIndex] = Address;
		} else if(pstr[0] == 'H' && pstr[1] == '(') {
			pCode[nIndex] = hSpeed;
		} else if(pstr[0] == 'V' && pstr[1] == '(') {
			pCode[nIndex] = vSpeed;
		} else {
			pCode[nIndex] = strtol(pstr, 0, 16);
		}

		nIndex++;
		ptmp1 = ptmp + 1;
		ptmp = strstr(ptmp + 1, ",");
		memset(pstr, 0, 16);
	}

	strcpy(pstr, ptmp1);

	if(pstr[0] == '=') {
		if(pstr[1] == '7') {
			pCode[nIndex] = pCode[1] ^ pCode[2] ^ pCode[3] ^ pCode[4] ^ pCode[5] ^ pCode[6];
		}
	} else if(pstr[0] == '+') {
		if(pstr[1] == '5') {
			pCode[nIndex] = (pCode[1] + pCode[2] + pCode[3] + pCode[4] + pCode[5]) % 256;
		}
	} else {
		pCode[nIndex] = strtol(pstr, 0, 16);
	}

	//Print(pCode,nIndex+1);
	return nIndex + 1;
}


/*
##	Camera control up start
##  fd ------handle operation
##  addr-----Camera operation address
##	speed----Camera operation speed
##
*/
int CCtrlUpStart(int fd, int addr, int speed)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, vSpeed, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[UPSTART].comm);
	/*vSpeed=5    5=8*/
	vSpeed = gRemote.VSSlid.speed[speed];
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, vSpeed, vSpeed);

	if(size <= 0 || fd == -1) {
		printf("CCtrlUpStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlUpStart()->SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlUpStart() End\n");
	return 0;

}

/*
##	Camera control UP stop
##  fd ------handle operation
##  addr-----Camera operation address
##
*/
int CCtrlUpStop(int fd, int addr)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[UPSTOP].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlUpStop() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlUpStop() SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlUpStop() End\n");

	return 0;

}


/*
##	Remote control down start
##  fd ------handle operation
##  addr-----Camera address operation
##	speed----Camera trans speed
##
*/
int CCtrlDownStart(int fd, int addr, int speed)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, vSpeed, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[DOWNSTART].comm);
	/*vSpeed=5*/
	vSpeed = gRemote.VSSlid.speed[speed];
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, vSpeed);

	if(size <= 0 || fd == -1) {
		printf("CCtrlDownStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlDownStart()->SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlDownStart() End\n");
	return 0;

}

/*
##	Remote control down Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlDownStop(int fd, int addr)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[DOWNSTOP].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlUpStop() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlUpStop() SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlDownStop() End\n");
	return 0;

}


/*
##
##	Remote control Left Start
##  fd ------handle operation
##  addr-----Camera address operation
##	speed----Camera trans speed
##
##
*/
int CCtrlLeftStart(int fd, int addr, int speed)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, hSpeed, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[LEFTSTART].comm);
	/*hSpeed=5*/
	hSpeed = gRemote.HSSlid.speed[speed];
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlLeftStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlLeftStart()->SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlLeftStart() End\n");
	return 0;

}

/*
##
##	Remote control Left Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlLeftStop(int fd, int addr)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[LEFTSTOP].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlLeftStop() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlLeftStop() SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlLeftStop() End\n");
	return 0;

}

/*
##
##	Remote control right Start
##  fd ------handle operation
##  addr-----Camera address operation
##	speed----Camera trans speed
##
*/
int CCtrlRightStart(int fd, int addr, int speed)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, hSpeed, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[RIGHTSTART].comm);
	/*hSpeed=5*/
	hSpeed = gRemote.HSSlid.speed[speed];
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlRightStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlRightStart()->SendDataToCom() Error\n");
		return -1;
	}

	printf( "CCtrlRightStart() End [%x]\n",nAddr);
	return 0;

}

/*
##
##	Remote control Right Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlRightStop(int fd, int addr)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[RIGHTSTOP].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlRightStop() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlRightStop() SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlRightStop() End\n");
	return 0;

}

/*
##
##	Remote control Zoom out  start
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlZoomAddStart(int fd, int addr)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[DADDSTART].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlZoomAddStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlZoomAddStart() SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlZoomAddStart() End\n");
	return 0;

}


/*
##	Remote control Zoom out Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlZoomAddStop(int fd, int addr)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[DADDSTOP].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlZoomAddStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlZoomAddStart() SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlZoomAddStart() End\n");
	return 0;

}

/*
##
##	Remote control Zoom In start
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlZoomDecStart(int fd, int addr)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[DDECSTART].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlZoomAddStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlZoomAddStart() SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlZoomAddStart() End\n");
	return 0;

}


/*
##
##	Remote control Zoom In Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlZoomDecStop(int fd, int addr)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[DDECSTOP].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlZoomAddStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlZoomAddStart() SendDataToCom() Error\n");
		return -1;
	}

	printf("CCtrlZoomAddStart() End\n");
	return 0;

}

int CCtrlPositionPreset(int fd, int addr, int position)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[POSITIONPRESET].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlPositionPreset() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	pCode[5] = position; //第5位表示预置位
	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlPositionPreset() SendDataToCom() Error\n");
		return -1;
	}
	
	printf( "CCtrlPositionPreset() End [%x][%d]\n",nAddr,position);
	return 0;

}
int CCtrlSetPositionPreset(int fd, int addr, int position)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote.CCode[POSITIONPRESETSET].comm);
	/*Address1=81*/
	nAddr  = gRemote.Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("CCtrlPositionPreset() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	pCode[5] = position; //第5位表示预置位
	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCtrlPositionPreset() SendDataToCom() Error\n");
		return -1;
	}

	printf( "CCtrlPositionPreset() End [%x][%d]\n",nAddr,position);
	return 0;

}
int CCMovetoPos(int fd, int addr, RECT rc, POINT pt)
{
	int nWidth = rc.right - rc.left;
	int nHeight = rc.bottom - rc.top;
	int nShiftH = pt.y - rc.top - nHeight / 2;
	int nShiftW = pt.x - rc.left - nWidth / 2;
	int nAddr, size, ret = -1;
	double nZoomPos1;
	BYTE pCode[16];
	char strCode[256];
	char strKey[64];
	char strtmp[8];
	double Range;
	short RangeH;
	short RangeW;
	short nZoomPos = GetZoomPos(fd, addr);

	if(nZoomPos < 0) {
		return 0;
	}

	if(nZoomPos <= 0x5000) {
		nZoomPos1 = 3.141 + tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	} else if(nZoomPos <= 0x6000) {
		nZoomPos1 = 3.141 + (double)(3.14 / (double)4) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	} else {
		nZoomPos1 = 3.141 + (double)(3.14 / (double)16) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	}

	Range = (double)2592 * atan(1.8 / (double)nZoomPos1) / 3.141;

	RangeH = -(double)(Range * ((double)nShiftH * 2 / (double)nHeight) * 3 / (double)4);
	RangeW = (double)(Range * ((double)nShiftW * 2 / (double)nWidth));

	memset(pCode, 0, 16);
	memset(strCode, 0, 256);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);
	strcpy(strCode, gRemote.CCode[REL_POSITION].comm);
	nAddr  = gRemote.Addr.addr[addr];
	size = CCtrlCodeAnalysis1(pCode, strCode, nAddr, 14, 14, RangeH, RangeW);

	//Print(pCode,size);
	if(size <= 0 || fd == -1) {
		printf("CCMovetoPos() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCMovetoPos() SendDataToCom() Error\n");
		return -1;
	}

	return 0;
}

int CCZoomInRect(int fd, int addr, RECT rc, RECT childRc)
{
	POINT pt;
	int nAddr, size, ret = -1;
	BYTE pCode[16];
	char strCode[256];
	char strKey[64];
	char strtmp[8];
	double RangeRate;
	double Range;
	short nZoomPos = 0;
	double nZoomPos1;
	nZoomPos = GetZoomPos(fd, addr);

	if(nZoomPos < 0) {
		return 0;
	}

	pt.x = childRc.left + (childRc.right - childRc.left) / 2;
	pt.y = childRc.top + (childRc.bottom - childRc.top) / 2;
	CCMovetoPos(fd, addr, rc, pt);

	if(nZoomPos <= 0x5000) {
		nZoomPos1 = 3.141 + tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	}

	else if(nZoomPos <= 0x6000) {
		nZoomPos1 = 3.141 + (double)(3.141 / (double)4) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	} else {
		nZoomPos1 = 3.141 + (double)(3.141 / (double)16) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	}

	Range = (double)360 * atan(1.8 / (double)nZoomPos1) / 3.14;

	RangeRate = (double)((double)(childRc.bottom - childRc.top) / (double)(rc.bottom - rc.top)) > (double)((double)(childRc.right - childRc.left) / (double)(rc.right - rc.left)) ? (double)(childRc.bottom - childRc.top) / (double)(rc.bottom - rc.top) : (double)(childRc.right - childRc.left) / (double)(rc.right - rc.left);

	Range = Range * RangeRate;
	nZoomPos1 = 1.8 / tan(Range * 3.141 / 360);
	nZoomPos = 2 * 32768 * atan((nZoomPos1 - 3.1) / 27.9) / 3.141;

	memset(pCode, 0, 16);
	memset(strCode, 0, 256);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);
	strcpy(strCode, gRemote.CCode[DDEC_DIRECT].comm);
	nAddr  = gRemote.Addr.addr[addr];
	size = CCtrlCodeAnalysis1(pCode, strCode, nAddr, 14, 14, nZoomPos, 0);

	if(size <= 0 || fd == -1) {
		printf("CCZoomInRect() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("CCZoomInRect() SendDataToCom() Error\n");
		return -1;
	}

	return 0;

}
unsigned long readn(int fd,  void *vptr, size_t n)
{
	unsigned long nleft;
	unsigned long nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;

	while(nleft > 0) {
		if((nwritten = read(fd, ptr, nleft)) <= 0) {
			if(nwritten < 0 && errno == EINTR) {
				nwritten = 0;    /* and call write() again */
			} else {
				return(-1);    /* error */
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return(n);
}
int GetZoomPos(int fd, int addr)
{
	BYTE pCode[16];
	char strCode[256];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;
	static short nPos = 0;
	memset(pCode, 0, 16);
	memset(strCode, 0, 256);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);
	strcpy(strCode, gRemote.CCode[ZOOMPOSQUERY].comm);
	nAddr  = gRemote.Addr.addr[addr];
	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("GetZoomPos() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("GetZoomPos() SendDataToCom() Error\n");
		return -1;
	}

	usleep(2000);
	memset(pCode, 0, 16);
	ret = read(compipe[0], pCode, 16);

	if(ret < 0) {
		printf("GetZoomPos() read from pip Error\n");
		//return nPos;
	}

	//printf("0=%2x  %2x  %2x  %2x  %2x  %2x  %2x  ret=%d\n",pCode[0],pCode[1],pCode[2],pCode[3],pCode[4],pCode[5],pCode[6],ret);

	if(ret == 7) {
		nPos = 0;
		nPos |= pCode[2];
		nPos = nPos << 4;
		nPos |= pCode[3];
		nPos = nPos << 4;
		nPos |= pCode[4];
		nPos = nPos << 4;
		nPos |= pCode[5];

		//return nPos;
	}

	return nPos;

}

int GetPan_TiltPos(int fd, int addr, int *nPanPos, int *nTiltPos)
{
	BYTE pCode[16];
	char strCode[256];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 256);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);
	strcpy(strCode, gRemote.CCode[PAN_TILTPOSQUERY].comm);
	nAddr  = gRemote.Addr.addr[addr];
	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		printf("GetPan_TiltPos() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		printf("GetPan_TiltPos() SendDataToCom() Error\n");
		return -1;
	}

	memset(pCode, 0, 16);
	//ret=read(fd, pCode, 11);
	ret = 11;
	pCode[2] = 19;

	if(ret == 11) {
		*nPanPos |= pCode[2];
		*nPanPos = *nPanPos << 4;
		*nPanPos |= pCode[3];
		*nPanPos = *nPanPos << 4;
		*nPanPos |= pCode[4];
		*nPanPos = *nPanPos << 4;
		*nPanPos |= pCode[5];

		*nTiltPos |= pCode[6];
		*nTiltPos = *nTiltPos << 4;
		*nTiltPos |= pCode[7];
		*nTiltPos = *nTiltPos << 4;
		*nTiltPos |= pCode[8];
		*nTiltPos = *nTiltPos << 4;
		*nTiltPos |= pCode[9];
	} else {
		return 0;
	}

	return 1;
}
/*Trans parity*/
int TransParity(int parity)
{
	int ch = 'N';

	switch(parity)	{
		case 0:
			ch = 'N';			//none
			break;

		case 1:
			ch = 'O';    		//odd
			break;

		case 2:
			ch = 'E';			//even
			break;

		default:
			printf("TransParity() parity:%d  Error\n", parity);
			break;
	}

	return ch;
}


/*Initial remote config file*/
int InitRemoteStruct(int index)
{
	int cnt, ret;
	char filename[50], buf[60], buf1[20];

	if((index < 0) && (index > PROTNUM - 1)) {
		index = 0;
	}

	memset(&gRemote, 0, sizeof(gRemote));
	/*Camera No.*/
	strcpy(filename, "/etc/camera/");
	strcat(filename, FarCtrlList[index]);
	strcat(filename, ".ini");

	strcpy(gRemote.ptzName, filename);

	printf("Config File: %s\n", filename);

	/*Speed control*/
	for(cnt = 1; cnt < SLID_MAX_NUM; cnt++) {
		memset(buf, 0, sizeof(buf));
		memset(buf1, 0, sizeof(buf1));
		sprintf(buf1, "%d", cnt);
		ret = ConfigGetKey(filename, "HSeepSlid", buf1, buf);

		if(ret == -10) {    //file no exist
			printf("file no exist %s\n", gRemote.ptzName);
			return -1;
		}

		if(ret != 0) {
			continue;
		}

		gRemote.HSSlid.speed[cnt] = strtol(buf, 0, 16);
		memset(buf, 0, sizeof(buf));
		memset(buf1, 0, sizeof(buf1));
		sprintf(buf1, "%d", cnt);
		ret = ConfigGetKey(filename, "VSeepSlid", buf1, buf);

		if(ret != 0) {
			continue;
		}

		gRemote.VSSlid.speed[cnt] = strtol(buf, 0, 16);

	}

	/*operation addr*/
	for(cnt = 1; cnt < ADDR_MAX_NUM ; cnt++)  {
		memset(buf, 0, sizeof(buf));
		memset(buf1, 0, sizeof(buf1));
		sprintf(buf1, "Address%d", cnt);
		ret = ConfigGetKey(filename, "Address", buf1, buf);

		if(ret != 0) {
			continue;
		}

		gRemote.Addr.addr[cnt] = strtol(buf, 0, 16);;
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Up_Start", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[UPSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Up_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[UPSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Down_Start", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[DOWNSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Down_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[DOWNSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Left_Start", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[LEFTSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Left_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[LEFTSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Right_Start", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[RIGHTSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Right_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[RIGHTSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DADD_Start", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[DADDSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DADD_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[DADDSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DDEC_Start", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[DDECSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DDEC_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[DDECSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Rel_position", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[REL_POSITION].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DDEC_Direct", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[DDEC_DIRECT].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "ZoomPosQuery", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[ZOOMPOSQUERY].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Pan_TiltPosQuery", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[PAN_TILTPOSQUERY].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "PositionPreset", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[POSITIONPRESET].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "PositionPresetSet", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[POSITIONPRESETSET].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Limit_set", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[LIMITE_SET].comm, buf);
	}


	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Limit_clear", buf);

	if(ret == 0) {
		strcpy(gRemote.CCode[LIMITE_CLEAR].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "Comm", "StopBit", buf);

	if(ret == 0) {
		gRemote.comm.stopbits = atoi(buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "Comm", "DataBit", buf);

	if(ret == 0) {
		gRemote.comm.databits = atoi(buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "Comm", "Baud", buf);

	if(ret == 0) {
		gRemote.comm.baud = atoi(buf);
	}


	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "Comm", "Parity", buf);

	if(ret == 0) {
		gRemote.comm.parity = TransParity(atoi(buf));
	}

	return 0;

}


#if 1
static void SetSpeed(int fd, int speed)
{
	int i;
	int status;
	struct termios Opt;
	int speed_arr[] = { B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
	                    B38400, B19200, B9600, B4800, B2400, B1200, B300,
	                  };
	int name_arr[] = {115200, 38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,
	                  19200,  9600, 4800, 2400, 1200,  300,
	                 };

	tcgetattr(fd, &Opt);

	for(i = 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {
		if(speed == name_arr[i]) {
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);

			if(status != 0) {
				printf("tcsetattr fd, errmsg = %s\n", strerror(errno));
				return;
			}

			tcflush(fd, TCIOFLUSH);
		}
	}
}

static int SetParity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;

	if(tcgetattr(fd, &options)  !=  0) {
		printf("SetupSerial 1");
		return(-1);
	}

	options.c_cflag &= ~CSIZE;
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	options.c_oflag  &= ~OPOST;                     /*Output*/
	options.c_iflag   &= ~IXON;                     //0x11
	options.c_iflag   &= ~ICRNL;                    //0x0d
	//      options.c_cflag|=CLOCAL;
	//      options.c_cflag|=CREAD;
	//      options.c_cflag&=~CRTSCTS;

	switch(databits) {
		case 7:
			options.c_cflag |= CS7;
			break;

		case 8:
			options.c_cflag |= CS8;
			break;

		default:
			printf("Unsupported data size\n");
			return (-1);
	}

	switch(parity) {
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;     /* Clear parity enable */
			options.c_iflag &= ~INPCK;              /* Enable parity checking */
			break;

		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;

		case 'e':
		case 'E':
			options.c_cflag |= PARENB;      /* Enable parity */
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;

		case 'S':
		case 's':                                                       /*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;

		default:
			printf("Unsupported parity\n");
			return (-1);
	}

	switch(stopbits) {
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;

		case 2:
			options.c_cflag |= CSTOPB;
			break;

		default:
			printf("Unsupported stop bits\n");
			return (-1);
	}

	/* Set input parity option */
	if(parity != 'n') {
		options.c_iflag |= INPCK;
	}

	tcflush(fd, TCIFLUSH);
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;                                 /* define the minimum bytes data to be readed*/

	if(tcsetattr(fd, TCSANOW, &options) != 0) {
		printf("SetupSerial 3");
		return (-1);
	}

	return (0);
}

static int OpenPort(int port_num)
{
	char port[4][20] = {{"/dev/ttyO0"}, {"/dev/ttyO1"}, {"/dev/ttyO2"}, {"/dev/ttyO3"}};
	int fd;

	if(port_num > 3 || port_num < 0) {
		printf("port num error:%d\n", port_num);
		return -1;
	}


	if((fd = r_open(port[port_num], O_RDWR | O_NOCTTY)) < 0) { //O_NDELAY
		printf("ERROR: failed to open %s, errno=%d\n", port[port_num], errno);
		return -1;
	} else {
		printf("Open %s success, fd = %d \n", port[port_num], fd);
	}

	fcntl(fd, F_SETFL, FNDELAY);
	return fd;
}

static  void ClosePort(int fd)
{
	if(fd != -1) {
		r_close(fd);
	}
}

static int InitPort(int port_num, int baudrate,
                    int databits, int stopbits, int parity)
{
	int fd;

	fd = OpenPort(port_num);

	if(fd == -1) {
		return -1;
	}

	SetSpeed(fd, baudrate);

	if(SetParity(fd, databits, stopbits, parity) == -1) {
		printf("Set Parity Error!\n");
		ClosePort(fd);
		return -1;
	}

	return fd;
}


#endif


/*Camera control initial*/
int CameraCtrlInit(int com)
{
	int ret = -1;
	int fd;
	int baudrate = 9600, databits = 8;
	int stopbits = 1, parity = 'N';
	baudrate = gRemote.comm.baud;
	databits = gRemote.comm.databits;
	parity = gRemote.comm.parity;
	stopbits = gRemote.comm.stopbits;

	if(pipe(compipe)) {
		printf("Create gRemote pipe error/n");
	};

	int flag = fcntl(compipe[0], F_GETFL, 0);

	flag |= O_NONBLOCK;

	if(fcntl(compipe[0], F_SETFL, flag) < 0) {
		printf("Set  O_NONBLOCK pipe error/n");

	}

	fd = InitPort(com, baudrate, databits, stopbits, parity);

	if(fd <= 0) {
		return -1;
	}

	ret = InitCamera(fd);

	if(ret != 0) {
		return -1;
	}

	return fd;
}


void Print(BYTE *data, int len)
{
	int i;

	printf("\n");

	for(i = 0; i < len; i++) {
		printf("%2x  ", data[i]);
	}

	printf("\n");
}
void FarCtrlCameraEx(int dsp, unsigned char *data, int len)
{
	int type, speed, addr = 1;
	int fd;
	RECT rc;
	RECT rc2;
	POINT pt;
	type = (int)(data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24);
	speed = (int)(data[4] | data[5] << 8 | data[6] << 16 | data[7] << 24);

	if(len < 12) {
		addr = 1;
	} else {
		addr = (int)(data[8] | data[9] << 8 | data[10] << 16 | data[11] << 24);
	}

	if(addr < 1) {
		addr = 1;
	}

	memcpy(&rc, data + 12, sizeof(rc));
	memcpy(&pt, data + 12 + sizeof(rc), sizeof(pt));
	memcpy(&rc2, data + 12 + sizeof(rc), sizeof(rc2));

	if(gRemoteFD <= 0) {
		printf("Remote Port Open Failed\n");
	}

	fd = gRemoteFD;
	printf("Extype:%d    Exlen:%dsizeof(RECT)=%d\n", type, len, sizeof(RECT));
	printf("Exspeed:%d\n", speed);
	printf("rc.bottom=%ld,rc.left=%ld,rc.right=%ld,rc.top=%ld\n", rc.bottom, rc.left, rc.right, rc.top);
	printf("pt.x=%d,pt.y=%d\n", pt.x, pt.y);

	//CCMovetoPos(fd,addr,rc,pt);
	if(type == 13) {
		CCMovetoPos(fd, addr, rc, pt);
	} else if(type == 18) {

		CCZoomInRect(fd, addr, rc, rc2);
	}
}
/*
** Remote
**Send MSG_FARCTRL Message
**MSG_FARCTRL message header
**4BYTES    for  Type
**4BYTES    fpr  Speed
Type
2            UP Start
16          UP Stop
3            DOWN Start
17          DOWN Stop
0            LEFT Start
14          LEFT Stop
1            RIGHT Start
15          RIGHT Stop
8            FOCUS ADD Start
22          FOCUS ADD Stop
9            FOCUS DEC Start
23          FOCUS DEC Stop
Speed
1            LOW
5            MID
10          HIGH

 */

void FarCtrlCamera(int dsp, unsigned char *data, int len)
{
	int type, speed, addr = 1;
	int fd;
	int remotefd = - 1;
	//Print(data,len);
	type = (int)(data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24);
	speed = (int)(data[4] | data[5] << 8 | data[6] << 16 | data[7] << 24);

	if(len < 12) {
		addr = 1;
	} else {
		addr = (int)(data[8] | data[9] << 8 | data[10] << 16 | data[11] << 24);
	}

	if(addr < 1) {
		addr = 1;
	}

	addr = 1;
	if(dsp == 0)
	{
		remotefd = gRemoteFD;
	}
	else if(dsp == 1)
	{
		remotefd = gStuRemoteFD;
	}
	else
	{
		printf("invalid fd\n");
		return ;
	}
	
	if(remotefd < 0) {
		printf("Remote Port Open Failed\n");
	}

	fd = remotefd;
	printf( "0000type = %d %d\n",type,addr);
	switch(type) {
		case 2:		//UP Start
			CCtrlUpStart(fd, addr, speed);
			break;

		case 16:	//UP stop
			CCtrlUpStop(fd, addr);
			//SetTrackStatus(fd,0);
			break;

		case 3:		//DOWN start
			CCtrlDownStart(fd, addr, speed);
			break;

		case 17:	//DOWN stop
			CCtrlDownStop(fd, addr);
			//SetTrackStatus(fd,1);
			break;

		case 0:		// LEFT Start
			CCtrlLeftStart(fd, addr, speed);
			break;

		case 14:	//LEFT Stop
			CCtrlLeftStop(fd, addr);
			break;

		case 1:		//RIGHT Start
			CCtrlRightStart(fd, addr, speed);
			break;

		case 15:    //RIGHT Stop
			CCtrlRightStop(fd, addr);
			break;

		case 8:		//FOCUS ADD Start
			CCtrlZoomAddStart(fd, addr);
			break;

		case 22:	//FOCUS ADD Stop
			CCtrlZoomAddStop(fd, addr);
			break;

		case 9:		//FOCUS DEC Start
			CCtrlZoomDecStart(fd, addr);
			break;

		case 23:	//FOCUS DEC Stop
			CCtrlZoomDecStop(fd, addr);
			break;

		case 10:	//预置位，speed表示预置位号
			CCtrlPositionPreset(fd, addr, speed);
			break;

		case 11:	//设置预置位，speed表示预置位号
			CCtrlSetPositionPreset(fd, addr, speed);
			break;

		case 12://设置自动跟踪模块的手动和自动模式0自动1手动
			printf("0000track mode speed = %d\n", speed);
			if(dsp == 0)
			{
				server_set_track_type(speed);
			}
			else if(dsp == 1)
			{
				stuserver_set_track_type(speed);
			}
			else
			{
				
			}
			
			data[8] = 2;
			break;

		case 13:
			FarCtrlCameraEx(fd, data, len);

		case 18:
			FarCtrlCameraEx(fd, data, len);

		default:
			printf("FarCtrlCamera() command Error\n");
			break;
	}

}


/*###########################################*/
#if 0
/*摄像头的控制*/
int CameraControl()
{
	int ret = -1;
	int fd;
	int baudrate = 9600, databits = 8;
	int stopbits = 1, parity = 'N';

	baudrate = gRemote.comm.baud;
	databits = gRemote.comm.databits;
	parity = gRemote.comm.parity;
	stopbits = gRemote.comm.stopbits;


	Printf("Initial succeed\n");
	fd = InitPort(PORT_COM2, baudrate, databits, stopbits, parity);
	ret = InitCamera(fd);

	if(ret != 0) {
		return -1;
	}

	/*	sleep(1);
		CCtrlUpStart(fd,1,10);
		sleep(10);
		CCtrlUpStop(fd,1);
		usleep(100);

		CCtrlDownStart(fd,1,10);
		sleep(10);
		CCtrlDownStop(fd,1);
		usleep(100);
	*/
	CCtrlLeftStart(fd, 1, 10);
	sleep(10);
	CCtrlLeftStop(fd, 1);
	/*	usleep(100);
		CCtrlRightStart(fd,1,10);
		sleep(20);
		CCtrlRightStop(fd,1);
		usleep(100);
		CCtrlZoomAddStart(fd,1);
		sleep(10);
		CCtrlZoomAddStop(fd,1);
		usleep(100);
		CCtrlZoomDecStart(fd,1);
		sleep(20);
		CCtrlZoomDecStop(fd,1);
		usleep(100);*/
	return 0;
}

#endif

