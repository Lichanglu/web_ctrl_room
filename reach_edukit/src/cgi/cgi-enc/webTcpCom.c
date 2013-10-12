#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include "cgi.h"

#include "../../enc/middle_control.h"
#include "../../enc/input_to_channel.h"
#include "webTcpCom.h"
#include "../../enc/modules/include/log_common.h"

//#define 	WEB_TCP_SERVER_PORT		20001
static int connectToDecodeServer()
{
	int socketFd;
	struct sockaddr_in serv_addr;
	const char* pAddr  = "127.0.0.1";
	socketFd= socket(PF_INET, SOCK_STREAM, 0);
	if(socketFd < 1)
		return -1;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(ENCODESERVER_1260_PORT);
	inet_aton(pAddr,(struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero),8);
	if (connect(socketFd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) == -1)	{
//		PRINTF("Connet to server failed \n");
		close(socketFd);
		return -1;
	}
	return socketFd;
}

FILE *create_file_fd(char *file)
{
	FILE *fp_low = NULL;

	if(NULL == fp_low) {
		fp_low = fopen(file, "a+");
	}

	return fp_low;
	
}

unsigned int mid_clock(void)
{

//	unsigned int msec;
//	struct timeval    tv;

//	gettimeofday(&tv,NULL);

//	msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	unsigned int msec;
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;
	
	return msec;
}

void write_time_buf(char *buf)
{
	int n, m, s,us;
	us = mid_clock( ) ;
	s = us/ 1000;
	n = s / 3600;
	s = s % 3600;
	m = s / 60;
	s = s % 60;
	us = us %1000;
	sprintf(buf, "[%04d:%02d:%02d:%03d] : ", n, m, s,us);
}
static void write_local_file(FILE *file, char *data)
{
	char tmpbuf[512] = {0};
	int len = 0;
	write_time_buf(tmpbuf);
	strcat(tmpbuf,data);
	len = strlen(tmpbuf);
	fwrite(tmpbuf, len, 1, file);
}

/*网络发送数据*/
static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf+nWriteLen, nSize-nWriteLen, 0);
		if(nRet < 0 ) {
			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr,"network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}
			return nRet;
		}
		else if(nRet == 0) {
			fprintf(stderr,"Send Net Data Error nRet= %d\n",nRet);
			continue;
		}
		nWriteLen += nRet;
		nCount = 0;
	}
	return nWriteLen;
}



/*网络接收数据*/
static int ReadData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;

	while(nWriteLen < nSize) {
		nRet = recv(s, (char *)pBuf+nWriteLen, nSize-nWriteLen, 0);
		nWriteLen += nRet;
	}
	
	return nWriteLen;
}
//extern  FILE *TestFile;
int appCmdIntParse(int cmd,int invalue,int inlen,int *outvalue,int *outlen)
{
	webMsgInfo_1260  msg,outmsg;
	int Sockfd  =  0,retVal=0;
	char intemp[4096] ;//,outtmep[4096];
	int valueLen = 0, cmdLen = 0;
	char retValue[4096] = {0};
	int retcmd = 0;
	//char tmpbuf[512] = {0};
	
	//write_local_file(TestFile,"appCmdIntParse connectToDecodeServer before\n");
	if(Sockfd == 0)	{
		Sockfd= connectToDecodeServer();
	}
	if(Sockfd <= 0)	{
		return CLIENT_ERR_TCPCONNECT;
	}
	//write_local_file(TestFile,"appCmdIntParse connectToDecodeServer after\n");

	msg.identifier = WEB_IDENTIFIER;
	valueLen = inlen;
	cmdLen	= sizeof(webparsetype);
	msg.type = INT_TYPE;
	msg.len = sizeof(webMsgInfo_1260)+valueLen + cmdLen;
	memcpy(intemp,&msg,sizeof(webMsgInfo_1260));
	memcpy(intemp+sizeof(webMsgInfo_1260),&cmd,cmdLen);
	memcpy(intemp+(sizeof(webMsgInfo_1260) + cmdLen),&invalue,valueLen);
	if(WriteData(Sockfd, (void*)intemp,msg.len) < 1)	{
		close(Sockfd);
		return CLIENT_ERR_TCPSEND;
	}
	//sprintf(tmpbuf,"cmd:[%x],type:[%d]\n",cmd,msg.type);
	//write_local_file(TestFile,tmpbuf);
	if(ReadData(Sockfd,(void *)&outmsg,sizeof(webMsgInfo_1260)) < sizeof(webMsgInfo_1260))
	{
		close(Sockfd);
		return CLIENT_ERR_TCPRECV;
	}
	//memset(tmpbuf,0,strlen(tmpbuf));
	//sprintf(tmpbuf,"outmsg.identifier:%x,outmsg.len:%d,outmsg.type:%d\n",outmsg.identifier,outmsg.len,outmsg.type);
	//write_local_file(TestFile,tmpbuf);
	if(ReadData(Sockfd, (void *)retValue,(outmsg.len - sizeof(webMsgInfo_1260))) < 0)
	{
		//PRINTF("retValue[0]:%d,retValue[1]:%d\n",retValue[0],retValue[1]);
		close(Sockfd);	
		return CLIENT_ERR_TCPRECV;
	}
	close(Sockfd);
	memcpy(&retcmd,&retValue,sizeof(webparsetype));
	memcpy(&retVal,retValue + 4, sizeof(int));
	memcpy(outvalue,retValue + 8,sizeof(int));
	//memset(tmpbuf,0,strlen(tmpbuf));
	//sprintf(tmpbuf,"retcmd:[%x],retval:[%x],outval:%d\n",retcmd,retVal,*outvalue);
	//write_local_file(TestFile,tmpbuf);
	return retVal;
	
}


int appCmdStringParse(int cmd,char *invalue,int inlen,char  *outvalue,int *outlen)
{
	webMsgInfo_1260  msg,outmsg;
	int sockfd  =  0,retcmd = 0,retval=0;
	char intemp[4096] = {0};
//	char outtmep[4096] = {0};
	int valuelen = 0, cmdlen = 0;
	char retvalue[4096] = {0};
	//char tmpbuf[512] = {0};
	//write_local_file(TestFile,"appCmdStringParse connectToDecodeServer before\n");
	if(sockfd == 0)	{
		sockfd= connectToDecodeServer();
	}
	if(sockfd <= 0)	{
		return CLIENT_ERR_TCPCONNECT;
	}
	//write_local_file(TestFile,"appCmdStringParse connectToDecodeServer after\n");
	msg.identifier = WEB_IDENTIFIER;
	valuelen = inlen;
	cmdlen	= sizeof(webparsetype);
	msg.type = STRING_TYPE;
		
	msg.len = sizeof(webMsgInfo_1260)+valuelen + cmdlen;
	memcpy(intemp,&msg,sizeof(webMsgInfo_1260));
	memcpy(intemp+sizeof(webMsgInfo_1260),&cmd,cmdlen);
	if(invalue != NULL)
	{
		memcpy(intemp+(sizeof(webMsgInfo_1260) + cmdlen),invalue,valuelen);
	}
	if(WriteData(sockfd, (void*)intemp,msg.len) < 1)	{
		close(sockfd);
		return CLIENT_ERR_TCPSEND;
	}
	//sprintf(tmpbuf,"cmd:[%x],type:[%d]\n",cmd,msg.type);
	//write_local_file(TestFile,tmpbuf);
	if(ReadData(sockfd,(void *)&outmsg,sizeof(webMsgInfo_1260)) < sizeof(webMsgInfo_1260))
	{
		close(sockfd);
		return CLIENT_ERR_TCPRECV;
	}
	//memset(tmpbuf,0,strlen(tmpbuf));
	//sprintf(tmpbuf,"outmsg.identifier:%x,outmsg.len:%d,outmsg.type:%d\n",outmsg.identifier,outmsg.len,outmsg.type);
	//write_local_file(TestFile,tmpbuf);
	//PRINTF("outmsg.identifier:%x,outmsg.len:%d,outmsg.type:%d=%d\n",outmsg.identifier,outmsg.len,outmsg.type,(outmsg.len - sizeof(webMsgInfo_1260)));
	if(ReadData(sockfd, (void *)retvalue,(outmsg.len - sizeof(webMsgInfo_1260))) < 0)
	{
		close(sockfd);	
		return CLIENT_ERR_TCPRECV;
	}
	close(sockfd);
	memcpy(&retcmd,retvalue,cmdlen);
	memcpy(&retval,retvalue + 4,sizeof(int));
	memcpy(outvalue,retvalue+cmdlen+sizeof(int),(outmsg.len - sizeof(webMsgInfo_1260) - sizeof(webparsetype)));
	//memset(tmpbuf,0,strlen(tmpbuf));
	//sprintf(tmpbuf,"retcmd:[%x],retval:[%x],outval:%s\n",retcmd,retval,outvalue);
	//write_local_file(TestFile,tmpbuf);
	return retval;
	
}




int appCmdStructParse(int cmd,void  *invalue,int inlen,void *outvalue,int *outlen)
{
	webMsgInfo_1260  inmsg,outmsg;
	int Sockfd  =  0,retCmd = 0,retval;
	char intemp[4096];//,outtmep[4096];
	int valueLen = 0, cmdLen = 0;
	char retValue[4096] = {0};
	//char tmpbuf[1024] = {0};
	int msgHeadLen = sizeof(webMsgInfo_1260);
	//write_local_file(TestFile,"appCmdStructParse connectToDecodeServer before\n");
	if(Sockfd == 0)	{
		Sockfd= connectToDecodeServer();
	}
	if(Sockfd <= 0)	{
		return CLIENT_ERR_TCPCONNECT;
	}
	//write_local_file(TestFile,"appCmdStructParse connectToDecodeServer after\n");
	inmsg.identifier = WEB_IDENTIFIER;
	valueLen = inlen;
	cmdLen	= sizeof(int);
	inmsg.type = STRUCT_TYPE;
		
	inmsg.len = msgHeadLen+valueLen + cmdLen;
	memcpy(intemp,&inmsg,msgHeadLen);
	memcpy(intemp+msgHeadLen,&cmd,cmdLen);
	if(invalue !=NULL)
	{
		memcpy(intemp+(msgHeadLen + cmdLen),invalue,valueLen);
	}
	if(WriteData(Sockfd, (void*)intemp,inmsg.len) < 1)	{
		close(Sockfd);
		return CLIENT_ERR_TCPSEND;
	}
	//sprintf(tmpbuf,"cmd:[%x],type:[%d]\n",cmd,inmsg.type);
	//write_local_file(TestFile,tmpbuf);
	if(ReadData(Sockfd,(void *)&outmsg,msgHeadLen) < msgHeadLen)
	{
		close(Sockfd);
		return CLIENT_ERR_TCPRECV;
	}
	//memset(tmpbuf,0,strlen(tmpbuf));
	//sprintf(tmpbuf,"outmsg.identifier:%x,outmsg.len:%d,outmsg.type:%d\n",outmsg.identifier,outmsg.len,outmsg.type);
	//write_local_file(TestFile,tmpbuf);
	if(ReadData(Sockfd, (void *)retValue,(outmsg.len - msgHeadLen)) < 0)
	{
		close(Sockfd);	
		return CLIENT_ERR_TCPRECV;
	}
	close(Sockfd);
	//need some check
	memcpy(&retCmd,retValue,cmdLen);
	memcpy(&retval,retValue+cmdLen,sizeof(int));
	memcpy(outvalue,retValue+cmdLen+sizeof(int),outmsg.len-msgHeadLen-cmdLen-sizeof(int));
	//memset(tmpbuf,0,strlen(tmpbuf));
	//sprintf(tmpbuf,"retcmd:[%x],retval:[%x]\n",retCmd,retval);
	//write_local_file(TestFile,tmpbuf);
	return retval;
	
}

#if 0
/*Test main function*/
int main()
{
	int ret = 0;
	PRINTF("Web start main!!\n");
	char version[20];
	int vinput = 1, outval;
	char teststring[] = "ENC1260";
	char outvalue[256] = {0};

	while(1) {
		//ret = appCmdIntParse(10, vinput, sizeof(int), &outval, sizeof(outval));
		ret = appCmdStringParse(11, teststring, strlen(teststring),outvalue,&outval) ;
		PRINTF("ret = %d\n",outval);
/*		memset(&audio,0,sizeof(audio));
		WebGetAudioParam(&audio);
		PRINTF("#################audio####################\n");
		PRINTF("audio.SampleRate = %d\n",audio.SampleRate);
		PRINTF("audio.BitRate = %d\n",audio.BitRate);
		WebGetVideoParam(&video);
		PRINTF("video.sBitrate=%d\n",video.sBitrate);
		PRINTF("video.nQuality = %d \n",video.nQuality);
		PRINTF("video.nDataCodec=%x\n",video.nDataCodec);
		WebGetSysParam(&sys);
		PRINTF("sys.strName = %s\n",sys.strName);
		PRINTF("sys.strVer = %s\n",sys.strVer);
		PRINTF("sys.bType=%d\n",sys.bType);


		PRINTF("###########################################\n");
		audio.SampleRate  = 2;
		audio.BitRate  = 64000;
		WebSetAudioParam(&audio);
		video.nQuality = 50;
		video.sBitrate = 128;
		WebSetVideoParam(&video);
		strcpy(sys.strName,"enc1200");
		sys.bType = 3;
		WebSetSysParam(&sys);	
		input = SDI;
		PRINTF("SDI   input = %d \n",input);	
		WebSetinputSource(input);
		PRINTF("get input source \n");
		WebGetinputSource(&input);
		PRINTF("input = %d \n",input);	
//		input = SDI;
		WebSetinputSource(input);
		WebGetkernelversion(version);
		PRINTF("kernel version = %s\n",version);
		WebGetfpgaversion(version);
		PRINTF("FPGA version = %s\n",version);
*/
/*		memset(&time,0,sizeof(time));
		Websynctime(&time);
		text.xpos = 300;
		text.ypos = 400;
		strcpy(text.msgtext,"好的");
		ret =Webaddtexttodisplay(&text);
		PRINTF("add Text To display!! = %d\n",ret);
		WebUpdatesystem("/opt/reach/update.tgz");*/
		sleep(10);
	}

	return 0;
}
#endif

