/*********************************************************************
*   创建串口通信
*   作用：封装串口等相关的公用设置参数
*                                add by zm
*												2011-11-30
**********************************************************************/
#include	<stdio.h>      /*标准输入输出定义*/
#include	<stdlib.h>     /*标准函数库定义*/
#include <string.h>
#include  <unistd.h>     /*Unix 标准函数定义*/
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>      /*文件控制定义*/
#include  <termios.h>    /*POSIX 终端控制定义*/
#include  <errno.h>      /*错误号定义*/

//#include "log_common.h"



int mid_serial_open_fd(char *dev)
{
	int fd;
	/*以读写方式打开串口*/
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

	/*保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息*/
	if(tcgetattr(fd, &oldtio) != 0) {
		printf("SetupSerial 1\n");
		return -1;
	}

	memcpy(&oldtio, &newtio, sizeof(newtio));

	/*setup 1 设置字符大小*/
	newtio.c_cflag |= CLOCAL | CREAD ;
	//newtio.c_cflag |= ~CSIZE;

	/*设置停止位*/
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

	/*设置奇偶校验位*/
	switch(nevent) {
		case '0': //奇数
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;

		case 'E': //偶数
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;

		case 'N': //无奇偶校验位
			newtio.c_cflag &= ~PARENB;
			break;

		default :
			printf("the nevent is invid,the nevent= %d\n", nevent);
			break;
	}

	/*设置波特率*/
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

	/*设置停止位*/
	if(nstop == 1) {
		newtio.c_cflag &= ~CSTOPB;
	} else if(nstop == 2) {
		newtio.c_cflag |= CSTOPB;
	}

	/*设置等待时间和最小接受字符*/
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;

	/*处理未接受字符*/
	tcflush(fd, TCIFLUSH);

	/*激活新配置*/
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
