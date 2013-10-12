/*********************************************************************
*   ��������ͨ��
*   ���ã���װ���ڵ���صĹ������ò���
*                                add by zm
*												2011-11-30
**********************************************************************/
#include	<stdio.h>      /*��׼�����������*/
#include	<stdlib.h>     /*��׼�����ⶨ��*/
#include <string.h>
#include  <unistd.h>     /*Unix ��׼��������*/
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>      /*�ļ����ƶ���*/
#include  <termios.h>    /*POSIX �ն˿��ƶ���*/
#include  <errno.h>      /*����Ŷ���*/

//#include "log_common.h"



int mid_serial_open_fd(char *dev)
{
	int fd;
	/*�Զ�д��ʽ�򿪴���*/
	fd = open(dev, O_RDWR | O_NONBLOCK);

	if(-1 == fd) {
		printf("serial open fd error\n");
		return -1;
	}

	return fd;

}

int mid_serial_set_opt(int fd, int nspeed, int nbits, char nevent, int nstop)
{
	if(fd <= 0) {
		printf("FD =%d ,is invaild\n", fd);
		return -1;
	}

	struct termios newtio, oldtio;

	memset(&oldtio, 0, sizeof(oldtio));

	memset(&newtio, 0, sizeof(newtio));

	/*����������д��ڲ������ã�������������ںŵȳ���������صĳ�����Ϣ*/
	if(tcgetattr(fd, &oldtio) != 0) {
		printf("SetupSerial 1\n");
		return -1;
	}

	memcpy(&oldtio, &newtio, sizeof(newtio));

	/*setup 1 �����ַ���С*/
	newtio.c_cflag |= CLOCAL | CREAD ;
	//newtio.c_cflag |= ~CSIZE;

	/*����ֹͣλ*/
	switch(nbits) {
		case 7:
			newtio.c_cflag |= CS7;
			break;

		case 8:
			newtio.c_cflag |= CS8;
			break;

		default:
			printf("the nbits is invalid\n");
			break;
	}

	/*������żУ��λ*/
	switch(nevent) {
		case '0': //����
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;

		case 'E': //ż��
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;

		case 'N': //����żУ��λ
			newtio.c_cflag &= ~PARENB;
			break;

		default :
			printf("the nevent is invid,the nevent= %d\n", nevent);
			break;
	}

	/*���ò�����*/
	switch(nspeed) {
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;

		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;

		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;

		case 19200:
			cfsetispeed(&newtio, B19200);
			cfsetospeed(&newtio, B19200);
			break;

		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;

		case 460800:
			cfsetispeed(&newtio, B460800);
			cfsetospeed(&newtio, B460800);
			break;

		default:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;

	}

	/*����ֹͣλ*/
	if(nstop == 1) {
		newtio.c_cflag &= ~CSTOPB;
	} else if(nstop == 2) {
		newtio.c_cflag |= CSTOPB;
	}

	/*���õȴ�ʱ�����С�����ַ�*/
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;

	/*����δ�����ַ�*/
	tcflush(fd, TCIFLUSH);

	/*����������*/
	if((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
		printf("com set error");
		return -1;
	}

	printf("set done!\n");
	return 0;

}

//timeout ms
int mid_serial_read_data(int fd, char *buff, int len, int timeout)
{
	if(fd <= 0) {
		printf("FD =%d ,is invaild\n", fd);
		return -1;
	}

	//test
	//return -1;
	int ret = 0;
	fd_set rfds;
	struct timeval tv_timeout;
	//printf("\n");

	tv_timeout.tv_sec = timeout / 1000;
	tv_timeout.tv_usec = 1000 * (timeout % 1000);
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	ret = select(fd + 1, &rfds, NULL, NULL, &tv_timeout);
	//printf("ret = %d\n",ret);

	if(ret <= 0) {
		printf("read fd is error\n");
		return -1;
	}

	ret = read(fd, buff, len);

	if(ret != len) {
		printf("error,i need read len = %d,read ret = %d\n", len, ret);
	}

	return ret;
}
int mid_serial_write_data(int fd, char *buff, int len)
{
	if(fd <= 0) {
		printf("FD =%d ,is invaild\n", fd);
		return -1;
	}

	int ret = 0;
	fd_set wfds;
	int timeout = 2000;
	//	char temp[256]= {0};
	struct timeval tv_timeout;
	//writeWatchDog();//lichl
	//printf("\n");
	tv_timeout.tv_sec = timeout / 1000;
	tv_timeout.tv_usec = 1000 * (timeout % 1000);
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);
	ret = select(fd + 1, NULL, &wfds, NULL, &tv_timeout);

	//printf("ret = %d\n",ret);
	if(ret <= 0) {
		printf("write fd is error\n");
		return -1;
	}

	ret = write(fd, buff, len);

	if(len != ret) {
		printf("error,need write len = %d, ret = %d\n", len, ret);
	}

	return ret;
}

int mid_serial_close_data(int fd)
{
	if(fd <= 0) {
		printf("FD =%d ,is invaild\n", fd);
		return -1;
	}

	close(fd);
	return 0;
}
