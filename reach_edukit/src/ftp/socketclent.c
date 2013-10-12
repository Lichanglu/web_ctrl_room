#if 1
#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero
#include "reach_upload.h"
#include "common.h"
/*
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <unistd.h>
 */

#define SERVER_PORT 10000
#define BUFFER_SIZE 1024*4
#define FILE_NAME_MAX_SIZE 512

static int32_t package_add_xml_leaf(xmlNodePtr child_node, xmlNodePtr far_node, char *key_name, char *key_value)
{
	child_node = xmlNewNode(NULL, BAD_CAST key_name);
	xmlAddChild(far_node, child_node);
	xmlAddChild(child_node, xmlNewText(BAD_CAST key_value));
	return 1;
}

static int32_t ftp_buffer_config(char *send_buf, char *usrname, char *password)
{
	xmlChar *temp_xml_buf;
	int size = 0;
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr temp 				= NULL;
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	package_add_xml_leaf(body_node, root_node, "MsgBody", "");
	package_add_xml_leaf(temp,  head_node, "usrname", usrname);
	package_add_xml_leaf(temp,  head_node, "password", password);

	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	memcpy(send_buf, temp_xml_buf, size);
	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t ftp_buffer_upload(char *send_buf, char *localpath, char *romotepath, char *port)
{

	xmlChar *temp_xml_buf;
	int size = 0;
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr temp 				= NULL;
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	package_add_xml_leaf(body_node, root_node, "MsgBody", "");
		
	package_add_xml_leaf(temp,  head_node, "UpLoadLocalPath", localpath);
	package_add_xml_leaf(temp,  head_node, "UpLoadRomtePath", romotepath);
	package_add_xml_leaf(temp,  head_node, "Port", port);

	
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	memcpy(send_buf, temp_xml_buf, size);
	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}



int testsocket()
{
  	printf("---------------------\n");
    //设置一个socket地址结构client_addr,代表客户机internet地址, 端口
    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr)); //把一段内存区的内容全部设置为0
    client_addr.sin_family = AF_INET; //internet协议族
    client_addr.sin_addr.s_addr = htons(INADDR_ANY); //INADDR_ANY表示自动获取本机地址
    client_addr.sin_port = htons(0); //0表示让系统自动分配一个空闲端口
    //创建用于internet的流协议(TCP)socket,用client_socket代表客户机socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }

    //把客户机的socket和客户机的socket地址结构联系起来
    if (bind(client_socket, (struct sockaddr*) &client_addr,
            sizeof(client_addr)))
    {
        printf("Client Bind Port Failed!\n");
        exit(1);
    }

    //设置一个socket地址结构server_addr,代表服务器的internet地址, 端口
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
	
    server_addr.sin_family = AF_INET;
    if (inet_aton("192.168.4.44", &server_addr.sin_addr) == 0) //服务器的IP地址来自程序的参数
    {
        printf("Server IP Address Error! \n");
        exit(1);
    }

    server_addr.sin_port = htons(SERVER_PORT);
    socklen_t server_addr_length = sizeof(server_addr);
    // 向服务器发起连接,连接成功后client_socket代表了客户机和服务器的一个socket连接
    if (connect(client_socket, (struct sockaddr*) &server_addr,
            server_addr_length) < 0)
    {
        printf("Can Not Connect To %s!\n", "192.168.4.186");
        exit(1);
    }

   
    char buffer[BUFFER_SIZE]={0};
	int size;
	MsgHeader *pmsg = (MsgHeader *)buffer;
  	size = ftp_buffer_config(buffer + MSGLEN, "reach","123456");
	pmsg->sMsgType = FTP_COM_MSGTYPE_SERVER_CONFIG;
	pmsg->sLen     = MSGLEN+size;
	printf("%s",buffer + MSGLEN);
    // 发送文件名

	int nameLength = tcp_send_longdata(client_socket, buffer, pmsg->sLen);
    if (nameLength < 0)
    {
        printf("File name Error! \n");
        exit(0);
    }

	while(1)
	{
		sleep(60);
		memset(buffer, 0, sizeof(buffer));
	  	size = ftp_buffer_upload(buffer + MSGLEN, "/home/reach/xuchong/exploe","ftp://192.168.4.39/tftproot/xc","333");
		pmsg->sMsgType = FTP_COM_MSGTYPE_SERVER_UPLOAD_FILE;
		pmsg->sLen     = MSGLEN+ size;
		printf("%s",buffer + MSGLEN);
	    // 发送文件名

		nameLength = tcp_send_longdata(client_socket, buffer, pmsg->sLen);
	  
	    if (nameLength < 0)
	    {
	        printf("File name Error! \n");
	        exit(0);
	    }

	}

	 // 发送文件名
	
	sleep(1000);
    close(client_socket);
    return 0;
}
#endif

