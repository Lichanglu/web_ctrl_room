
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bkfs_middle.h"


//获取本机ip地址
static int getIPaddr(char *ip)
{
	FILE *fp = popen("ifconfig eth0| awk '/inet addr:/{printf(\"%s\", substr($2, 6))}'", "r");
	fread(ip, 15, 1, fp);
	pclose(fp);
	return 0;
}

//获取本机子网掩码
static int getNetmask(char *netmask)
{
	FILE *fp = popen("ifconfig eth0| awk '/Mask:/{printf(\"%s\", substr($4, 6))}'", "r");
	fread(netmask, 15, 1, fp);
	pclose(fp);
	return 0;
}

static int getGateWay(char *gateWay)
{
	FILE *fp = popen("route | awk '/default/{printf(\"%s\", $2)}'", "r");
	fread(gateWay, 15, 1, fp);
	pclose(fp);
	return 0;
}

int lcd_show_string(int fd)
{
	char netmask[16] = {0};
	char ipaddr[16]={0};
	char gateway[16]={0};
	getGateWay(gateway);
	getNetmask(netmask);
	getIPaddr(ipaddr);
	
	ClearScreen(fd);
	
	ShowString(fd,"Name:      KR9000	     ",0,5);
	ShowString(fd,(char *)ipaddr,0,20);
	ShowString(fd,(char *)netmask,0,35);
	ShowString(fd,(char *)gateway,0,50);
	printf("add :%s,mask=%s\n",ipaddr,netmask);
}


int main(int argc, char **argv)
{
	int fd =  InitLed();
	lcd_show_string(fd);
	app_weblisten_init();
	while(1) {
		sleep(1);
	}
	return 0;
}

