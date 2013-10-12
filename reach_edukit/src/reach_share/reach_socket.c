
#include "reach_socket.h"
//#include "media_msg.h"
#include "reach_os.h"
int32_t set_nonblocking(int32_t sock)
{
	int32_t opts;
	opts = fcntl(sock, F_GETFL);

	if(opts < 0) {
		nslog(NS_ERROR, "fcntl(sock,GETFL) : %s", strerror(errno));
		return -1;
	}

	opts = opts | O_NONBLOCK;

	if(fcntl(sock, F_SETFL, opts) < 0) {
		nslog(NS_ERROR, "fcntl(sock,SETFL,opts) : %s", strerror(errno));
		return -1;
	}

	return 0;
}

void set_net_addr(sockaddr_in_t *addr, int8_t *ip, int32_t port)
{
	r_bzero(addr, sizeof(sockaddr_in_t));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr((const char *)ip);
	addr->sin_port = htons(port);
}


void set_local_addr(sockaddr_un_t *local_addr, int8_t *unixstr_path)
{
	r_bzero(local_addr, sizeof(sockaddr_un_t));
	local_addr->sun_family = AF_LOCAL;
	r_strcpy((int8_t *)local_addr->sun_path, unixstr_path);
}

int32_t get_local_ip(int8_t *eth, int8_t *ipaddr)
{
	int32_t sock_fd;
	struct  sockaddr_in my_addr;
	struct ifreq ifr;

	unsigned int ip;

	/* Get socket file descriptor */
	if((sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		nslog(NS_ERROR, "socket : %s", strerror(errno));
		return 0;
	}

	/* Get IP Address */
	if(NULL == eth) {
		strcpy((char *)ifr.ifr_name, (const char *)"eth0");
	} else {
		strncpy((char *)ifr.ifr_name, (const char *)eth, IF_NAMESIZE);
	}

	ifr.ifr_name[IF_NAMESIZE] = '\0';

	if(ioctl(sock_fd, SIOCGIFADDR, &ifr) < 0) {
		nslog(NS_INFO, ":No Such Device %s\n", eth);
		return 0;
	}

	memcpy(&my_addr, &ifr.ifr_addr, sizeof(my_addr));
	memcpy(&ip, &(my_addr.sin_addr), 4);
	strcpy((char *)ipaddr, (const char *)inet_ntoa(my_addr.sin_addr));
	close(sock_fd);
	return (int32_t)ip;
}


int32_t async_recv_data(int32_t fd, void *buf, int32_t buflen)
{

	int32_t ret = -1;
	char *recv_buf = buf;
	int32_t total = buflen;
	int32_t recv_len = 0;

	while(1) {
		recv_len = recv(fd, recv_buf, total, 0);

		//		nslog(NS_INFO, "recv len is [%d]\n",recv_len);
		if(recv_len < 0) {
			//			nslog(NS_INFO, "error is ---[%d]\n",errno);
			if(errno == EAGAIN || errno == EINTR) {
				ret = -EAGAIN;

				break;
			} else {
				nslog(NS_INFO, "other error!\n");
				ret = -1;  //other error
				break;
			}

		}

		if(0 == recv_len) {
			ret = -1;
			break;
		}

		if(total == recv_len) {
			ret = 0;
			break;
		}

		total -= recv_len;
		recv_buf += recv_len;
	}

	return ret;
}

int32_t async_recv_msg(int32_t fd, void *buf, int32_t buflen)
{

	int32_t ret = -1;
	char *recv_buf = buf;
	int32_t total = buflen;
	int32_t recv_len = 0;

	while(1) {
		recv_len = recv(fd, recv_buf, total, 0);

		if(recv_len < 0) {
			//			nslog(NS_ERROR, "[async_recv_data] : %s", strerror(errno));
			//			nslog(NS_INFO, "error is ---[%d]\n",errno);
			if(errno == EAGAIN || errno == EINTR) {
				ret = -EAGAIN;
				break;
			} else {
				ret = -1;  //other error
				break;
			}
		}

		if(0 == recv_len) {
			ret = -1;
			break;
		}

		if(total == recv_len) {
			ret = 0;
			break;
		}

		total -= recv_len;
		recv_buf += recv_len;
	}

	return ret;
}


int32_t async_send_data(int32_t fd, void *buf, int32_t *buflen)
{

	int32_t i_loop = 0 ;
	int8_t *pbuf = buf;
	int32_t send_total_len = *buflen;
	int32_t real_len = 0;
	int32_t ret = -1;

	while(1) {
		real_len = send(fd, pbuf, send_total_len, 0);

		if(100 < i_loop++) {
			nslog(NS_INFO, "real_len is --- [%d]\n", real_len);
			nslog(NS_INFO, "async_send_data is send!\n");
			i_loop = 0;
		}

		if(real_len <  0) {
			if(errno == EAGAIN || errno == EINTR) {
				//	nslog(NS_INFO, "[%s]**********-<1>-errno : %d\n",__func__, errno);
				ret = -EAGAIN;
				*buflen = send_total_len;
				break;
			} else {
				ret = -1;
				break;  //other error
			}
		}

		if(0 == real_len) {
			ret = -1;
			break;
		}

		if((size_t)real_len == send_total_len) {
			ret = 0;
			*buflen = 0;
			break;
		}

		send_total_len -= real_len;
		pbuf += real_len;
	}

	return ret;
}

int32_t async_sendto_data(int32_t fd, void *buf, int32_t *buflen, int32_t port, int8_t *ip)
{

	int32_t i_loop = 0 ;
	int8_t *pbuf = buf;
	int32_t send_total_len = *buflen;
	int32_t real_len = 0;
	int32_t ret = -1;

	struct sockaddr_in SrvAddr ;
	socklen_t size = sizeof(SrvAddr);
	bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = r_htons(port);
	// !!!!??? zhengyb
	//	SrvAddr.sin_addr.s_addr=htonl(ip);
	SrvAddr.sin_addr.s_addr = inet_addr((const char *)ip);

	while(1) {
		real_len = r_sendto(fd, pbuf, send_total_len, 0, (struct sockaddr *)&SrvAddr, size);

		//		nslog(NS_INFO, "r_sendto ______________________[%d]\n",real_len);
		if(100 < i_loop++) {
			nslog(NS_INFO, "real_len is --- [%d]\n", real_len);
			nslog(NS_INFO, "async_send_data is send!\n");
			i_loop = 0;
		}

		if(real_len <  0) {
			//	nslog(NS_INFO, "[%s]**********-<1>-errno : %d\n",__func__, errno);
			if(errno == EAGAIN || errno == EINTR) {
				ret = -EAGAIN;
				*buflen = send_total_len;
				break;
			} else {
				ret = -1;
				break;  //other error
			}
		}

		if(0 == real_len) {
			ret = -1;
			break;
		}

		if((size_t)real_len == send_total_len) {
			//nslog(NS_INFO, "SEND IS OK!!!!!!!!!!!!!!!!!!!!!!!!\n");
			ret = 0;
			*buflen = 0;
			break;
		}

		send_total_len -= real_len;
		pbuf += real_len;
	}

	return ret;
}


/*
****************************************************************************
**函数名称: recv_long_tcp_data()
**函数功能: 封装的TCP接收数据函数,确保数据接收完整
**输入参数: socket -- tcp的sockfd
			len -- 收取数据的长度
**输出参数: buffer -- 收取到的数据
**返回值:	-1 -- 接收失败; 大于0为接收数据的长度.
****************************************************************************
*/
int32_t recv_long_tcp_data(int32_t fd, void *buf, int32_t buflen)
{
	int32_t total_recv = 0;
	int32_t recv_len = 0;

	while(total_recv < buflen) {
		recv_len = r_recv(fd, buf + total_recv, buflen - total_recv, 0);

		//add by lcs
		if(recv_len < 1 && EINTR != errno) {
			nslog(NS_INFO, "recv tcp data failed,error message:%s, recv_len = %d \n", strerror(errno), recv_len);

			if(0 == recv_len) {
				return 0;
			}

			return -1;
		} else if(recv_len < 1 && EINTR == errno) {
			usleep(10000);
			continue;
		}

		total_recv += recv_len;

		if(total_recv == buflen) {
			break;
		}
	}

	return total_recv;
}

/*
****************************************************************************
**函数名称: send_long_tcp_data()
**函数功能: 封装的TCP接发送据函数,确保数据发送完整
**输入参数: socket -- tcp的sockfd
			len -- 收取数据的长度
			buffer -- 收取到的数据
**输出参数: 无
**返回值:	-1 -- 接收失败; 大于0为发送数据的长度.
****************************************************************************
*/
int32_t send_long_tcp_data(int32_t fd, void *buf, int32_t buflen)
{
	int32_t total_send = 0;
	int32_t send_len = 0;

	while(total_send < buflen) {
		send_len = r_send(fd, buf + total_send, buflen - total_send, 0);

		if(send_len < 1) {
			nslog(NS_INFO, "send tcp data failed,rror message:%s \n", strerror(errno));
			return -1;
		}

		total_send += send_len;

		if(buflen == total_send) {
			break;
		}
	}

	return total_send;
}

int32_t create_local_tcp_socket(int8_t *unixstr_path, socket_role_t sr, block_t type)
{
	int32_t m_socket;
	sockaddr_un_t local_addr;
	int on = 1;
	m_socket = socket(AF_LOCAL, SOCK_STREAM, 0);

	if(m_socket < 0) {
		nslog(NS_INFO, "[create_local_tcp_socket] failed !!!\n");
		return -1;
	}

	if(R_NONBLOCK == type) {
		if(-1 == set_nonblocking(m_socket)) {
			nslog(NS_INFO, "[create_local_tcp_socket]   set_nonblocking failed !!!\n");
			r_close(m_socket);
			return -1;
		}
	}

	set_local_addr(&local_addr, unixstr_path);
	nslog(NS_INFO, "[create_local_tcp_socket] local_addr [%s]\n", local_addr.sun_path);
	r_setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if(R_SERVER == sr) {
		if(bind(m_socket, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
			nslog(NS_INFO, "[%s] bind is error!\n", __func__);
			r_close(m_socket);
			return -1;
		}

		if(listen(m_socket, LISTENQ) < 0) {
			nslog(NS_INFO, "[%s] listen is error!\n", __func__);
			r_close(m_socket);
			return -1;
		}
	} else {
		if(r_connect(m_socket, (sa_t *)&local_addr, ADDR_LEN) == -1) {
			nslog(NS_ERROR, "[create_local_tcp_socket] : %s", strerror(errno));
			r_close(m_socket);
			return -1;
		}
	}

	return m_socket;
}

void close_socket(int32_t fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}

