#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/time.h>

#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>//
//#include <linux/icmp.h>
#include <strings.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include "app_network.h"


#define BUFSIZE 8192

struct route_info {
	u_int dstAddr;
	u_int srcAddr;
	u_int gateWay;
	char ifName[IF_NAMESIZE];
};



int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
	struct nlmsghdr *nlHdr;
	int readLen = 0, msgLen = 0;

	do {
		/* Recieve response from the kernel */
		if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0) {
			perror("SOCK READ: ");
			return -1;
		}

		nlHdr = (struct nlmsghdr *)bufPtr;

		/* Check if the header is valid */
		if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
			perror("Error in recieved packet");
			return -1;
		}

		/* Check if the its the last message */
		if(nlHdr->nlmsg_type == NLMSG_DONE) {
			break;
		} else {
			/* Else move the pointer to buffer appropriately */
			bufPtr += readLen;
			msgLen += readLen;
		}

		/* Check if its a multi part message */
		if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
			/* return if its not */
			break;
		}
	} while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

	return msgLen;
}
///* For printing the routes. */
//void printRoute(struct route_info *rtInfo)
//{
//	char tempBuf[512];
//
//	/* Print Destination address */
//	if(rtInfo->dstAddr != 0) {
//		strcpy(tempBuf, (char *)inet_ntoa(rtInfo->dstAddr));
//	} else {
//		sprintf(tempBuf, "*.*.*.*\t");
//	}
//
//	fprintf(stdout, "%s\t", tempBuf);
//
//	/* Print Gateway address */
//	if(rtInfo->gateWay != 0) {
//		strcpy(tempBuf, (char *)inet_ntoa(rtInfo->gateWay));
//	} else {
//		sprintf(tempBuf, "*.*.*.*\t");
//	}
//
//	fprintf(stdout, "%s\t", tempBuf);
//
//	/* Print Interface Name*/
//	fprintf(stdout, "%s\t", rtInfo->ifName);
//
//	/* Print Source address */
//	if(rtInfo->srcAddr != 0) {
//		strcpy(tempBuf, (char *)inet_ntoa(rtInfo->srcAddr));
//	} else {
//		sprintf(tempBuf, "*.*.*.*\t");
//	}
//
//	fprintf(stdout, "%s\n", tempBuf);
//}


/* For parsing the route info returned */
void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo, char *gateway)
{
	struct rtmsg *rtMsg;
	struct rtattr *rtAttr;
	int rtLen;
	char *tempBuf = NULL;

	tempBuf = (char *)malloc(100);
	rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

	/* If the route is not for AF_INET or does not belong to main routing table
	then return. */
	if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN)) {
		return;
	}


	/* get the rtattr field */
	rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
	rtLen = RTM_PAYLOAD(nlHdr);

	for(; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
		switch(rtAttr->rta_type) {
			case RTA_OIF:
				if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
				break;

			case RTA_GATEWAY:
				rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
				break;

			case RTA_PREFSRC:
				rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
				break;

			case RTA_DST:
				rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
				break;
		}
	}

	if(strstr((char *)inet_ntoa(*(struct in_addr *)(rtInfo->dstAddr)), "0.0.0.0")) {
		printf("gateWay:[%s]\n", (char *)inet_ntoa(*(struct in_addr *)rtInfo->gateWay));
		sprintf(gateway, "%s", (char *)inet_ntoa(*(struct in_addr *)rtInfo->gateWay));
	}

	//printRoute(rtInfo);
	free(tempBuf);
	return;
}

//获取默认网关
int get_gateway(char *gateway)
{
	struct nlmsghdr *nlMsg;
	struct rtmsg *rtMsg;
	struct route_info *rtInfo;
	char msgBuf[BUFSIZE];
	struct in_addr	addr ;
	unsigned int 	value;
	unsigned int 	  dwGateWay;

	int sock, len, msgSeq = 0;
	//	char buff[1024];


	/* Create Socket */
	if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) <= 0) {
		perror("Socket Creation: ");
	}

	/* Initialize the buffer */
	memset(msgBuf, 0, BUFSIZE);

	/* point the header and the msg structure pointers into the buffer */
	nlMsg = (struct nlmsghdr *)msgBuf;
	rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

	/* Fill in the nlmsg header*/
	nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
	nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

	nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
	nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
	nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

	/* Send the request */
	if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) <= 0) {
		printf("Write To Socket Failed...\n");
		return -1;
	}

	/* Read the response */
	if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) <= 0) {
		printf("Read From Socket Failed...\n");
		return -1;
	}

	/* Parse and print the response */
	rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

	for(; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len)) {
		memset(rtInfo, 0, sizeof(struct route_info));
		parseRoutes(nlMsg, rtInfo, gateway);
	}

	inet_aton(gateway, &addr);
	memcpy(&value, &addr, 4);
	dwGateWay = value;

	free(rtInfo);
	close(sock);
	return dwGateWay;
}

unsigned int getGateWay(char *interface_name)
{
	FILE *fp;
	char buf[512];
	uint32_t gateway = 0;
	// route
	fp = fopen("/proc/net/route", "r");

	if(fp == NULL) {
		perror("open file /proc/net/route failed");
		return -1;
	}

	while(fgets((char *)buf, sizeof(buf), fp) != NULL) {
		unsigned long dest, gate;
		char buffer[20] = {0};
		dest = gate = 0;
		sscanf(buf, "%s%lx%lx", buffer, &dest, &gate);

		if(dest == 0 && gate != 0 && (0 == strcmp(buffer, (char *)interface_name))) {
			gateway = gate;
			struct in_addr *addr = (struct in_addr *)&gateway;
			printf("    GateWay:%s\n", inet_ntoa(*addr));

			break;
		}
	}

	fclose(fp);

	return gateway;
}

//获取本机ip地址
unsigned int GetIPaddr(char *interface_name)
{
	int s;
	unsigned int ip;

	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if(ioctl(s, SIOCGIFADDR, &ifr) < 0) {
		perror("ioctl");
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_addr;

	printf(" [GetIPaddr] IP:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);

	return ip;
}

//获取本机子网掩码
unsigned int GetNetmask(char *interface_name)
{
	int s;
	unsigned int ip;

	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if(ioctl(s, SIOCGIFNETMASK, &ifr) < 0) {
		perror("ioctl");
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_netmask;

	printf("        Netmask:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);

	return ip;
}

//获取组播ip
unsigned int GetBroadcast(char *interface_name)
{
	int s;
	unsigned int ip;

	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface_name);

	if(ioctl(s, SIOCGIFBRDADDR, &ifr) < 0) {
		perror("ioctl");
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_broadaddr;

	printf("        Broadcast:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);

	return ip;

}


#if 0
int main()
{
	char gateway[255] = {0};
	get_gateway(gateway);
	printf("Gateway:%s\n", gateway);
}
#endif



/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
static int in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while(nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if(nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

#define	MAX_DUP_CHK		(8 * 128)
#define	DEFDATALEN		(64 - 8)	/* 默认数据长度*/
#define	MAXPACKET		(200)		/* 最大包大小 */


/*ICMP 报文发送socket*/
static int 	g_icmp_sockfd = 0;
char 	rcvd_tbl[MAX_DUP_CHK / 8];
#define ICMP_ECHO		8	/* Echo Request			*/
/*ICMP 定义*/

#define	A(bit)			rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)			(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)		(A(bit) |= B(bit))
#define	CLR(bit)		(A(bit) &= (~(B(bit))))
#define	TST(bit)		(A(bit) & B(bit))


/*建立ICMP 报文socket*/
void InitICMP(void)
{
	struct protoent *proto;
	int hold = 0;

	if(!(proto = getprotobyname("icmp"))) {
		printf("unknown protocol icmp.\n");
		return ;
	}

	if((g_icmp_sockfd = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
		perror("ping: socket");
		return ;
	}

	hold = 48 * 1024;
	setsockopt(g_icmp_sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&hold, sizeof(hold));
	return ;
}

/*close*/
int cleanICMP(void)
{
	close(g_icmp_sockfd);
	return 0;
}

/*发送ICMP 报文主要是解决网口插拔之后网络不通的bug*/
void SendICMPmessage(void)
{
	register struct icmp *icp;
	unsigned char outpack[MAXPACKET];
	int ntransmitted = 0 , ident;
	int mx_dup_ck = MAX_DUP_CHK;
	int datalen = DEFDATALEN;
	register int cc;
	char ipaddr[20] = "192.168.22.22";
	struct sockaddr_in to;

	bzero((char *)&to, sizeof(struct sockaddr));
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = inet_addr(ipaddr);
	ident = getpid() & 0xFFFF;

	icp = (struct icmp *)outpack;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;				/* ID */
	CLR(icp->icmp_seq % mx_dup_ck);
	gettimeofday((struct timeval *)&outpack[8], (struct timezone *)NULL);

	cc = datalen + 8;			/* skips ICMP portion */

	/* compute ICMP checksum here */
	icp->icmp_cksum = in_cksum((u_short *)icp, cc);
	sendto(g_icmp_sockfd, (char *)outpack, cc, 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in));
	printf("ICMP message!!!!!\n");
	return ;
}


/*设置网络ip地址*/
void SetEthConfigIP(int eth , unsigned int ipaddr, unsigned netmask)
{
	int ret;
	struct in_addr addr1, addr2;
	char temp[200];

	memset(temp, 0, sizeof(temp));
	memcpy(&addr1, &ipaddr, 4);
	memcpy(&addr2, &netmask, 4);

	if(eth == 0) {
		strcpy(temp, "ifconfig eth0 ");
	} else if(eth == 1) {
		strcpy(temp, "ifconfig eth0:1 ");
	}

	strcat(temp, inet_ntoa(addr1));
	strcat(temp, " netmask ");
	strcat(temp, inet_ntoa(addr2));
	printf("command:#%s#\n", temp);

	ret = system(temp);

	if(ret < 0) {
		printf("ifconfig %s up ERROR\n", inet_ntoa(addr1));
	}
}

/*设置网关*/
void SetEthConfigGW(int eth, unsigned int gw)
{
	char temp[200];
	struct in_addr addr1;
	int ret;

	memset(temp, 0, sizeof(temp));
	memcpy(&addr1, &gw, 4);

	if(eth == 0) {
		strcpy(temp, "route add default gw ");
	} else if(eth == 1) {
		strcpy(temp, "route add default gw ");
	}

	strcat(temp, inet_ntoa(addr1));
	printf("command:#%s#\n", temp);

	ret = system(temp);

	if(ret < 0) {
		printf("SetEthConfigGW() command:%s ERROR\n", temp);
	}

}


/*设置MAC地址*/
int SetMacAddr(int fd)
{
	char mac_addr[20];

	//	GetMACAddr(fd, (unsigned char *)mac_addr);
	printf("MAC addr = %s\n", mac_addr);
	//SplitMacAddr(mac_addr, gSysParaT.sysPara.szMacAddr, 6);
	return 0;
}


void SetEthDhcp()
{
	printf("i will set dhcp.\n");

	system("kill -1 `cat /var/run/dhcpcd-eth0.pid`");
	system("/sbin/dhcpcd eth0");
	system("ifconfig eth0");
	return ;
}