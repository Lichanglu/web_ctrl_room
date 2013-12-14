/*
2013-12-4 10:28:41  2013-12-4 21:28:50
2013-12-7 15:11:27

gcc -g -Wall -o xmld  xml_demo.c -I./ -lxml2

gcc -g -Wall -o xmld.so  xml_demo.c -D__TEST_REC_XML__ -shared -I./ -lxml2


说明：
1. 当使用dom树来解析xml文档时，由于默认的方式是把节点间的空白当作第一个子节 点，所以为了能和常说的第一个子节点相符，需调用xmlKeepBlanksDefault (0)函数来忽略这种空白。
2. 对于使用xmlChar* xmlNodeGetContent(xmlNodePtr cur)函数获取节点内容后，必须调用xmlFree()来对所分配的内存进行

*/
////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>		/* time() */
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include "reach_os.h"
#include "xml_base.h"
//#define  __TEST_REC_XML__


/*根据room中录制模块提供课件信息添加到上面xml文件中*/
typedef	struct RecourseInfo_s {
	char    FileName[128];
	char    FileCName[128];
	char    Duration[32];
	char    FileSize[64];
	char    RcdTime[32];
} RecourseInfo_t;

////////////////////////////////////////////////////////////////////////////////
#define     VALUE_LEN       256
#define     PATH_LEN        512
#define     FILE_NAME_LEN   128
#define     REACH_REC_DIR   "/opt/Rec"
#define     REACH_REC_XML   "/var/www/html/cgi-bin/file.xml"

#define PRINTF(fmt,arg...)  {printf("[%s %s|%s|%s:%-4d]"fmt,__DATE__, __TIME__,__FILE__, __func__, __LINE__,##arg);}

//create xml document
//xmlDocPtr     g_doc ;
//xmlNodePtr    g_root ;
xmlNodePtr      son1, son2, son3, son4;

char            FtpInfo[64] = {0};

static long long      totalSize   = 0;
static int      idx         = 0;

RecourseInfo_t  g_pInfo;
#define CONFIG_TABLE_FILE					("/usr/local/reach/.config/control_config.xml")
#define XML_USER_NAME_MAX_LENGTH			(128)
#define XML_USER_PASSWORD_MAX_LENGTH		(128)
#define XML_VALUE_MAX_LENGTH				(256)
#define MSG_USER_KEY						(BAD_CAST "User")
#define MSG_PASSWORD_KEY					(BAD_CAST "Password")
#define MSG_GUEST_KEY						(BAD_CAST "Guest")
#define MSG_GUEST_PASSWD_KEY				(BAD_CAST "GuestPasswd")
#define CONFIG_PARAMS_USER_INFO_KEY			(BAD_CAST "user_info")
typedef struct _user_info_
{
	uint8_t username[XML_USER_NAME_MAX_LENGTH];
	uint8_t password[XML_USER_PASSWORD_MAX_LENGTH];
	uint8_t guest_name[XML_USER_NAME_MAX_LENGTH];
	uint8_t guest_passwd[XML_USER_PASSWORD_MAX_LENGTH];
} user_info;
static int32_t read_user_info(xmlDocPtr pdoc, xmlNodePtr pnode, user_info *pinfo)
{
	xmlNodePtr node;

	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		printf("---[read_user_info]--- failed, params is NULL!\n");
		return -1;
	}

	node = get_children_node(pnode, MSG_USER_KEY);
	if(node){
		r_memset(pinfo->username, 0, XML_USER_NAME_MAX_LENGTH);
		get_current_node_value((char *)pinfo->username, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	node = get_children_node(pnode, MSG_PASSWORD_KEY);
	if(node){
		r_memset(pinfo->password, 0, XML_USER_PASSWORD_MAX_LENGTH);
		get_current_node_value((char *)pinfo->password, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	node = get_children_node(pnode, MSG_GUEST_KEY);
	if(node){
		r_memset(pinfo->guest_name, 0, XML_USER_NAME_MAX_LENGTH);
		get_current_node_value((char *)pinfo->guest_name, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	node = get_children_node(pnode, MSG_GUEST_PASSWD_KEY);
	if(node){
		r_memset(pinfo->guest_passwd, 0, XML_USER_PASSWORD_MAX_LENGTH);
		get_current_node_value((char *)pinfo->guest_passwd, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	return 0;
}


static int32_t read_userinfo_table_file(const int8_t *xml_file, user_info *Authentication)
{
	int32_t index = 0;
	int32_t room_count = 0;
	xmlDocPtr pdoc;
	xmlNodePtr proot_node;
	xmlNodePtr puser_info_node;
	int ret = 0;
	parse_xml_t px;

	if(NULL == xml_file || NULL == Authentication){
		printf("--[read_params_table_file] failed, params is NULL!\n");
		return -1;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		ret = -1;
		printf("--[read_params_table_file] failed, init_file_dom_tree error, xml file: %s\n", xml_file);
		goto cleanup;
	}

	pdoc = px.pdoc;
	proot_node = px.proot;

	/* 用户信息 */
	puser_info_node = get_children_node(proot_node, CONFIG_PARAMS_USER_INFO_KEY);
	if(puser_info_node){
		read_user_info(pdoc, puser_info_node, Authentication);
	}


cleanup:

	release_dom_tree(px.pdoc);

	return ret;
}


static void getGuestPasswd(char *passwd)
{
	user_info ui;
	read_userinfo_table_file((const int8_t *)CONFIG_TABLE_FILE, &ui);
	strcpy(passwd, ui.guest_passwd);
}
static int32_t get_local_ip(int8_t *eth, int8_t *ipaddr)
{
	int32_t sock_fd;
	struct  sockaddr_in my_addr;
	struct ifreq ifr;

	unsigned int ip;

	/* Get socket file descriptor */
	if((sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("socket : %s", strerror(errno));
		return 0;
	}

	/* Get IP Address */
	if(NULL == eth) {
		strcpy((char *)ifr.ifr_name, (const char *)"eth0");
	} else {
		strncpy((char *)ifr.ifr_name, (const char *)eth, 4);
	}

	ifr.ifr_name[4] = '\0';

	if(ioctl(sock_fd, SIOCGIFADDR, &ifr) < 0) {
		printf(":No Such Device %s\n", eth);
		return 0;
	}

	memcpy(&my_addr, &ifr.ifr_addr, sizeof(my_addr));
	memcpy(&ip, &(my_addr.sin_addr), 4);
	strcpy((char *)ipaddr, (const char *)inet_ntoa(my_addr.sin_addr));
	close(sock_fd);
	return (int32_t)ip;
}
static char *getFtpInfo(void)
{
	char localip[16] = {0};
	char passwd[128] = {0};
	get_local_ip("eth0", localip);
	getGuestPasswd(passwd);
	sprintf(FtpInfo, "%s:21:/:guest:%s", localip, passwd);
	return FtpInfo;
}

////////////////////////////////////////////////////////////////////////////////


/* 2013-12-5 17:03:29  测试OK

*/
xmlNodePtr xml_node_find_key(xmlNodePtr cur, char *keyname, char *keyvalue)
{
	//PRINTF("\n");

	xmlNodePtr tmp;
	xmlChar *pContentCur;

	if(NULL == cur || NULL == keyname) {
		//PRINTF("Error Input the param\n");
		return NULL;
	}

	//PRINTF("--------------Input-cur->name[%s],keyname[%s]keyvalue[%s] \n" , cur->name,keyname,keyvalue);

	cur = cur->xmlChildrenNode;

	while(cur != NULL) {
		if(NULL == keyvalue && (!xmlStrcmp(cur->name, BAD_CAST keyname))) {
			PRINTF("---------------idx[%d\t]name=[%s]find[%s] \n" , ++idx, cur->name, keyname);

			return cur;
		} else if(!xmlStrcmp(cur->name, BAD_CAST keyname)) {
			//keyname同则进

			//PRINTF("idx[%d\t]name=[%s] \n", ++idx, cur->name);
			pContentCur     =  xmlNodeGetContent(cur);

			if(!xmlStrcmp(pContentCur, BAD_CAST keyvalue)) {
				//keyvalue非空则比较，同则立即退出。 2013-12-6 20:16:29 memory
				PRINTF("---------------idx[%d\t]name=[%s]find[%s][%s] \n" , ++idx, cur->name, pContentCur, keyvalue);
				return cur;
			}

			xmlFree(pContentCur);
		}

		if(cur->xmlChildrenNode) {
			if(NULL != (tmp = xml_node_find_key(cur, keyname, keyvalue))) {
				//PRINTF("---------------idx[%d\t]name=[%s][%s] \n", ++idx, cur->name,xmlNodeGetContent(cur));
				return tmp;
			}
		}

		cur = cur->next;

	}

	return NULL;
}


xmlNodePtr xml_node_new(xmlChar *node_name, xmlChar *node_value)
{
	xmlNodePtr pnode = NULL;
	pnode = xmlNewNode(NULL, node_name);
	xmlNodeSetContent(pnode, node_value);
	PRINTF("---------------new node[%s][%s] \n", pnode->name, xmlNodeGetContent(pnode));

	return pnode;
}
void xml_node_free(xmlNodePtr node)
{
	if(NULL == node) {
		return ;
	}

	xmlUnlinkNode(node);
	xmlFreeNode(node);
}

void xml_node_del(xmlNodePtr node)
{
	if(NULL == node) {
		return ;
	}

	xmlNodePtr tempNode;
	tempNode = node->next;

	xmlUnlinkNode(node);
	xmlFreeNode(node);

	node = tempNode;
}


/* 2013-12-5 17:03:29  测试OK

*/
xmlNodePtr xml_node_find(xmlNodePtr cur, xmlNodePtr find)
{
	//PRINTF("\n");

	xmlNodePtr tmp;
	//xmlChar* pContentCur;
	//xmlChar* pContentFind;
	static int idx = 0;

	if(NULL == cur || NULL == find) {
		PRINTF("Error the param\n");
		return NULL;
	}

	cur = cur->xmlChildrenNode;
	//pContentFind    = (char*)xmlNodeGetContent(find);

	while(cur != NULL) {
		//PRINTF("idx[%d\t]name=[%s] \n", ++idx, cur->name);
		//pContentCur     = (char*)xmlNodeGetContent(cur);

		if((!xmlStrcmp(cur->name, find->name))
		   && (!xmlStrcmp(xmlNodeGetContent(cur), xmlNodeGetContent(find)))) {
			PRINTF("---------------idx[%d\t]name=[%s]find[%s][%s] \n" , ++idx, cur->name, xmlNodeGetContent(cur), xmlNodeGetContent(find));
			return cur;
		}

		if(cur->xmlChildrenNode) {
			if(NULL != (tmp = xml_node_find(cur, find))) {
				//PRINTF("---------------idx[%d\t]name=[%s][%s] \n", ++idx, cur->name,xmlNodeGetContent(cur));
				return tmp;
			}
		}

		//xmlFree(pContentCur);

		cur = cur->next;

	}

	//xmlFree(pContentFind);

	return NULL;
}


/*2013-12-6 19:00:37
param3(TYPE):switch{case:}
*/
void xml_node_mod(xmlNodePtr root, char *keyname, int add_sub)
{
	PRINTF("\n");

	xmlChar    *value;
	xmlNodePtr  node;
	int         iRet            = -1;
	char        content[128]    = {0};

	node    = xml_node_find_key(root, keyname, NULL);

	if(node) {
		value   = xmlNodeGetContent(node);

		if(value) {
			iRet    = atoi((char *)value) + add_sub;
			sprintf(content, "%d", iRet);

			PRINTF("content[%s]\n", content);

			xmlNodeSetContent(node, BAD_CAST content);
			xmlFree(value);
		}
	}
}


void xml_ergodic_node(xmlDocPtr doc, xmlNodePtr cur)
{
	xmlChar *key;
	cur = cur->xmlChildrenNode;

	while(cur != NULL) {
		if((!xmlStrcmp(cur->name, (const xmlChar *)"keyword"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			PRINTF("keyword: %s\n", key);
			xmlFree(key);
		}

		cur = cur->next;
	}

	return;
}

int     xml_ergodic(char *filename)
{
	xmlDocPtr   doc    = NULL;
	xmlNodePtr  cur    = NULL;
	char       *name    = NULL;
	char       *value   = NULL;

	xmlKeepBlanksDefault(0);

	doc = xmlParseFile(filename); //创建Dom树

	if(doc == NULL) {
		PRINTF("Loading xml file failed.\n");
		return -1;
	}

	cur = xmlDocGetRootElement(doc); //获取根节点

	if(cur == NULL) {
		PRINTF("empty file \n");
		xmlFreeDoc(doc);
		return -1;
	}


	cur = cur->xmlChildrenNode;

	while(cur != NULL) {
		if((!xmlStrcmp(cur->name, (const xmlChar *)"FileName"))) {
			xml_ergodic_node(doc, cur);
		}

		cur = cur->next;
	}


	//walk the tree
	cur = cur->xmlChildrenNode; //get sub node

	while(cur != NULL) {
		name = (char *)(cur->name);
		value = (char *)xmlNodeGetContent(cur);
		PRINTF("name is:%s|value:%s;\n", name, value);
		xmlFree(value);
		cur = cur->next;
	}

	xmlFreeDoc(doc);//释放xml解析库所用资源
	xmlCleanupParser();
	return 0;
}
/*
用这个函数异常
*/
int     xml_open(xmlDoc **doc, xmlNode  **root, char *xmlname)
{
	PRINTF("xmlname=[%s]\n", xmlname);

	//读取XML文档
	xmlKeepBlanksDefault(0);

	// load an exist xml file.
	//doc   = xmlParseFile(file_name); //段错误
	*doc  = xmlReadFile(xmlname, "UTF-8", XML_PARSE_NOBLANKS | XML_PARSE_NODICT);

	if(*doc == NULL) {
		PRINTF("Document not parsed successfully. \n");
		return -1;
	}

	// get root
	*root = xmlDocGetRootElement(*doc);

	if(*root == NULL) {
		PRINTF("xmlDocGetRootElement empty document\n");
		xmlFreeDoc(*doc);

		return -1;
	}

	return 0;
}
int 	xml_zsg(xmlDocPtr doc, xmlNodePtr  root, char *fileName)
{
	PRINTF("\n");

	//3.遍历节点,修改与删除
	xmlNodePtr head = root->children->next;

	while(head != NULL) {
		if(head->type == XML_ELEMENT_NODE) {

			PRINTF("Name:%s \n", head->name);
			PRINTF("Content:%s \n", xmlNodeGetContent(head->children));

			// update test05
			if((!xmlStrcmp(head->name, (const xmlChar *)fileName))) {
				xmlNodeSetContent(head->children, (const xmlChar *)fileName);
			}

			// remove node
			if(!xmlStrcmp(head->name, (const xmlChar *)"FileName")) {
				//xmlNodeGetContent();
				if(!xmlStrcmp(head->name, (const xmlChar *)"FileName")) {

				}

				xmlNodePtr tempNode;
				tempNode = head->next;
				xmlUnlinkNode(head);
				xmlFreeNode(head);
				head = tempNode;
				break; //continue;
			}
		}

		head = head->next;
	}

	return 0;
}
/*2013-12-7 11:28:11 modify 双指针问题
xmlDoc** doc   xmlDocPtr   doc

*/
int     xml_create(xmlDoc **doc, xmlNode  **root)
{
	PRINTF("\n");

	//xmlDocPtr   doc;
	//xmlNodePtr  root;

	// create xml document
	*doc   = xmlNewDoc(BAD_CAST"1.0");

	if(*doc == NULL) {
		PRINTF("Document not parsed successfully. \n");
		return -1;
	}

	*root  = xmlNewNode(NULL, BAD_CAST"RequestMsg");

	if(*root == NULL) {
		PRINTF("empty document\n");
		xmlFreeDoc(*doc);
		return -1;
	}

	//设置文档根节点
	xmlDocSetRootElement(*doc, *root);

	return 0;
}
int     xml_init(xmlNodePtr node)
{
	PRINTF("\n");

	// create xml document

	//create son and grandson

	son1 = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(node, son1);
	xmlNewTextChild(son1, NULL, BAD_CAST "ReturnCode", BAD_CAST "0");
	xmlNewTextChild(son1, NULL, BAD_CAST "PassKey",    BAD_CAST "1");

	//create node and add content

	son1     = xmlNewNode(NULL, BAD_CAST"MsgBody");
	xmlAddChild(node, son1);

	son2 = xmlNewNode(NULL, BAD_CAST "LoginResp");
	xmlAddChild(son1, son2);
	//xmlAddChild(son2, xmlNewText(BAD_CAST "1"));
	xmlNewTextChild(son2, NULL, BAD_CAST "Result",    BAD_CAST "0");
	xmlNewTextChild(son2, NULL, BAD_CAST "FtpServer", (const xmlChar *)getFtpInfo());
	xmlNewTextChild(son2, NULL, BAD_CAST "Option",    BAD_CAST "0");

	son3 = xmlNewNode(NULL, BAD_CAST "GroupList");
	xmlAddChild(son2, son3);
	xmlNewTextChild(son3, NULL, BAD_CAST "Group", BAD_CAST "1");

	son3 = xmlNewNode(NULL, BAD_CAST "FileList");
	xmlAddChild(son2, son3);

	return 0;

}

int  xml_close(xmlDocPtr doc, char *xmlname)
{
	//PRINTF("\n");

	//save xml
	int iRet = -1;

	if(NULL == doc || NULL == xmlname) {
		PRINTF("Failed to the Param\n");
		return -1;
	}

	//iRet = xmlSaveFormatFile(filename,g_doc,1); //1.UTF-8树格式 2.xmlSaveFile默认线格式，
	iRet = xmlSaveFormatFileEnc(xmlname, doc, "utf-8", 1);

	if(-1 == iRet) {
		PRINTF("create a xml=[%d] bytes\n", iRet);
		return -1;
	}

	// 释放资源
	xmlFreeDoc(doc);        //释放xml解析库所用资源
	//xmlCleanupParser();     //xmlParseFile(argv[1]);//创建Dom树
	//xmlMemoryDump ();

	PRINTF("create a xml=bytes[%d]filename[%s] \n", iRet, xmlname);
	return 0;
}



void formatTime(time_t time1, char *szTime)
{
	//PRINTF("\n");

	struct tm tm1;
	localtime_r(&time1, &tm1);
	sprintf(szTime, "%4d-%02d-%02d %02d:%02d:%02d",
	        tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday,
	        tm1.tm_hour,      tm1.tm_min,   tm1.tm_sec);

	//PRINTF("szTime=[%s]\n",szTime);

}
/*2013-12-6 20:19:13 add
dir_ergodic_size  rec_dir_size

*/
long long rec_dir_size(char *path)  //main函数的argv[1] char * 作为 所需要遍历的路径 传参数给listDir
{PRINTF("\n");
	DIR            *pDir ;  //定义一个DIR类的指针
	struct dirent  *ent  ;   //定义一个结构体 dirent的指针，dirent结构体见上
	//    int             i   =0  ;
	char            childpath[512];  //定义一个字符数组，用来存放读取的路径

	struct stat     st;

	pDir = opendir(path); //  opendir方法打开path目录，并将地址付给pDir指针
	if(NULL == pDir) {
		PRINTF("[%s]%s\n", path, strerror(errno));
		return -1;
	}
	memset(childpath, 0, sizeof(childpath)); //将字符数组childpath的数组元素全部置零


	while((ent = readdir(pDir)) != NULL) { //读取pDir打开的目录，并赋值给ent, 同时判断是否目录为空，不为空则执行循环体

		if(ent->d_type & DT_DIR) { //读取 打开目录的文件类型 并与 DT_DIR进行位与运算操作，即如果读取的d_type类型为DT_DIR (=4 表示读取的为目录)

			if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
				//如果读取的d_name为 . 或者.. 表示读取的是当前目录符和上一目录符, 用contiue跳过，不进行下面的输出
			{
				continue;
			}

			sprintf(childpath, "%s/%s", path, ent->d_name); //如果非. ..则将 路径 和 文件名d_name 付给childpath, 并在下一行prinf输出

		//PRINTF("path-------------------:[%s]\n",childpath);

			rec_dir_size(childpath);  //递归读取下层的字目录内容， 因为是递归，所以从外往里逐次输出所有目录（路径+目录名），
			//然后才在else中由内往外逐次输出所有文件名

		} else { //如果读取的d_type类型不是 DT_DIR, 即读取的不是目录，而是文件，则直接输出 d_name, 即输出文件名
			sprintf(childpath, "%s/%s", path, ent->d_name);
			//后续可以将范围缩小为统计MP4
			stat(childpath, &st);
			totalSize += st.st_size;

			//PRINTF("---[%d][%s]\n",totalSize, ent->d_name);/* 打印出该目录下的所有内容 */
		}
	}

	closedir(pDir); //2013-12-7 11:09:51 add

	PRINTF("---[%lld][%s]\n",totalSize,path);
	return totalSize;
}

int     rec_dir_check(char *dirname, char *mtime)
{
	//PRINTF("\n");

	//char    mtime[26]                   = {0};
	char    currentFile[VALUE_LEN]      = {0};
	char    currentTimeInfo[VALUE_LEN]  = {0};
	struct stat buf;

	if(NULL == dirname) {
		return -1;
	}

	memset(currentFile,     0, VALUE_LEN);
	memset(currentTimeInfo, 0, VALUE_LEN);
#if 1
	sprintf(currentFile,     "%s/%s/%s", REACH_REC_DIR, dirname, "ContentInfo.xml");

	if(access(currentFile, F_OK)  != 0) {
		//PRINTF("Error access currentFile[%s]\n",currentFile);
		return -1;
	}
#endif
	//if( strcmp(direntp->d_name, "recovery")  == 0 )
	//continue;
	//
	//PRINTF("opendir[%s]---open \n",direntp->d_name);
	//sprintf(currentDir, "%s/%s/%s",REACH_REC_DIR,dirname,"HD/resource/videos");
	//sprintf(currentFile, "%s/%s",currentDir,direnList->d_name );
	//PRINTF("access currentFile[%s]\n",currentFile);

	//
	sprintf(currentTimeInfo, "%s/%s/%s", REACH_REC_DIR, dirname, "Time.info");

	if(access(currentTimeInfo, F_OK) == 0) {
		FILE *timefp = fopen(currentTimeInfo, "r");

		if(!timefp) {
			return -1;
		}

		memset(mtime, 0, 26);
		fread(mtime, 19, 1, timefp);
		fclose(timefp);
	} else {
		//memset(&buf, 0, sizeof(struct stat));
		stat(currentFile, &buf);
		formatTime(buf.st_mtime, mtime);
	}

	if(0 == strlen(mtime)) {
		strcpy(mtime,  "1900-01-01 00:00:00");
	}

	//sprintf(pRecInfo->FileSize, "%0.2f",totalSize/(1024*1024));
	PRINTF("----------OK--mtime[%s][%s]\n", mtime, dirname);

	return 0;
}

int     rec_dir_ergodic2(char *dirname)
{
	PRINTF("opendir[%s]\n", dirname);

	int     iRet            = -1;
	char    RcdTime[26]     = {0};
	char    FileSize[64]    = {0};
	int     rowIndex        = 0;
	char cdir[256] = {0};

	DIR             *dirp ;
	struct dirent   *direntp;


	//查找FileList节点
	//son3    = xml_node_find_key(root, "FileList",NULL);

	dirp = opendir(dirname);

	if(dirp == NULL) {
		return -1;
	}

	while(1) { //while((direntp=readdir(dirp))!=NULL)
		direntp = readdir(dirp);

		if(direntp == NULL) {
			break;
		}

		//校验课件目录是否合法 1:md5.info 2:~/recovery
		if(direntp->d_name[0] != '.' && (direntp->d_type & DT_DIR)) {
			//从目录中获取节点信息
			iRet  = rec_dir_check(direntp->d_name, RcdTime);

			if(-1 == iRet) {
				continue;
			}
			PRINTF("%s\n", direntp->d_name);
			sprintf(cdir, "%s/%s", REACH_REC_DIR, direntp->d_name);
			totalSize   = 0;
			totalSize        = rec_dir_size(cdir);
			totalSize /=(1024*1024);
			sprintf(FileSize, "%lld", totalSize);
			PRINTF("\n");

			//将新节点加入到XML文件
			son4    = xmlNewNode(NULL, BAD_CAST "FileInfo");
			xmlAddChild(son3, son4);
			xmlNewTextChild(son4, NULL, BAD_CAST "FileName", (const xmlChar *)direntp->d_name);
			xmlNewTextChild(son4, NULL, BAD_CAST "FileCName", (const xmlChar *)direntp->d_name);
			xmlNewTextChild(son4, NULL, BAD_CAST "Duration",  BAD_CAST "00");
			xmlNewTextChild(son4, NULL, BAD_CAST "FileSize", (const xmlChar *)FileSize);
			xmlNewTextChild(son4, NULL, BAD_CAST "RcdTime", (const xmlChar *)RcdTime);
			xmlNewTextChild(son4, NULL, BAD_CAST "Group",     BAD_CAST "null");
			xmlNewTextChild(son4, NULL, BAD_CAST "Notes",     BAD_CAST "null");
			xmlNewTextChild(son4, NULL, BAD_CAST "FtpInfo", (const xmlChar *)getFtpInfo());
			PRINTF("\n");

			++rowIndex;
			PRINTF("RecInfo idx[%d]:FileName[%s]filesize[%s]RcdTime[%s]\n", rowIndex , direntp->d_name, FileSize, RcdTime);

		}
	}

	closedir(dirp);

	sprintf(FileSize, "%d", rowIndex);
	xmlNewTextChild(son3, NULL, BAD_CAST "TotalPage", BAD_CAST "1");
	xmlNewTextChild(son3, NULL, BAD_CAST "TotalCount", (const xmlChar *)FileSize);

	PRINTF("closedir dirname[%s] idx[%d] \n", dirname, rowIndex);

	return 0;
}


//case PAGE_FILEMGR_SHOW:
int     rec_dir_ergodic(char *dirname)
{
	//PRINTF("\n");

	//char    recDir[PATH_LEN] = {0}; //REACH_REC_DIR
	char    formData[1024 ] = {0};
	char    mtime[26] = {};
	int     rowIndex        = 0;
	float   totalSize       = 0;
	char    stotalSize[64]  = {0};
	struct stat buf;
	char    currentDir[VALUE_LEN] = {0};
	char    currentFile[VALUE_LEN] = {0};
	char    currentTimeInfo[VALUE_LEN] = {0};

	DIR *dirp, *dirList;
	struct dirent *direntp;
	struct dirent *direnList;


	PRINTF("opendir[%s]\n", dirname);

	dirp = opendir(dirname);

	if(dirp == NULL) {
		return -1;
	}

	//PRINTF("opendir 0000[%s]---coming \n",recDir);

	while(1) {
		direntp = readdir(dirp);

		if(direntp == NULL) {
			break;
		}

		//校验课件目录是否合法 1:md5.info 2:~/recovery
		if(direntp->d_name[0] != '.' && (direntp->d_type & DT_DIR)) {
			//PRINTF("d_name[%s]---open \n",direntp->d_name);

			memset(currentFile,     0, VALUE_LEN);
			memset(currentTimeInfo, 0, VALUE_LEN);

			sprintf(currentFile, "%s/%s/%s", dirname, direntp->d_name, "md5.info");

			if(access(currentFile, F_OK)  != 0) {
				continue;
			}

			sprintf(currentTimeInfo, "%s/%s/%s", dirname, direntp->d_name, "Time.info");

			if(strcmp(direntp->d_name, "recovery")  == 0) {
				continue;
			}

			//
			// HD/resource/videos/
			PRINTF("opendir[%s]---open \n", direntp->d_name);
			sprintf(currentDir, "%s/%s/%s", dirname, direntp->d_name, "HD/resource/videos");

			dirList = opendir(currentDir);

			if(dirList != NULL) {
				//PRINTF("opendir 1111[%s]\n",currentDir);
				totalSize = 0;

				while(1) {
					direnList = readdir(dirList);

					if(direnList == NULL) {
						break;
					}

					if(direnList->d_name[0] != '.') {
						//memset(currentFile, 0x00, sizeof(currentFile));

						sprintf(currentFile, "%s/%s", currentDir, direnList->d_name);
						//PRINTF("access currentFile[%s]\n",currentFile);

						//memset(&buf, 0, sizeof(struct stat));
						stat(currentFile, &buf);

						totalSize = totalSize + (float)buf.st_size;

						if(access(currentTimeInfo, F_OK) == 0) {
							FILE *timefp = fopen(currentTimeInfo, "r");

							if(!timefp) {
								break;
							}

							memset(mtime, 0, 26);
							fread(mtime, 19, 1, timefp);
							fclose(timefp);
						}

						//else
						//{
						formatTime(buf.st_mtime, mtime);
						//}
					}
				}

				closedir(dirList);
			}

			//
			sprintf(formData, "1courseName[%s]:createDate[%s]:totalSize[%0.2f]:dir_idx[%d]"
			        , direntp->d_name
			        , mtime
			        , totalSize / (1024 * 1024)
			        , ++rowIndex
			       );
			strcpy(g_pInfo.FileCName, direntp->d_name);
			strcpy(g_pInfo.FileName, direntp->d_name);
			strcpy(g_pInfo.Duration, "00");
			strcpy(g_pInfo.RcdTime,  mtime);
			sprintf(g_pInfo.FileSize, "%0.2f", totalSize / (1024 * 1024));

			PRINTF("formData[%s]\n", formData);

			sprintf(stotalSize, "%0.2f", totalSize / (1024 * 1024));

			son4 = xmlNewNode(NULL, BAD_CAST "FileInfo");
			xmlAddChild(son3, son4);
			xmlNewTextChild(son4, NULL, BAD_CAST "FileName", (const xmlChar *)direntp->d_name);
			xmlNewTextChild(son4, NULL, BAD_CAST "FileCName", (const xmlChar *)direntp->d_name);
			xmlNewTextChild(son4, NULL, BAD_CAST "Duration",  BAD_CAST "00");
			xmlNewTextChild(son4, NULL, BAD_CAST "FileSize", (const xmlChar *)stotalSize);
			xmlNewTextChild(son4, NULL, BAD_CAST "RcdTime", (const xmlChar *)mtime);
			xmlNewTextChild(son4, NULL, BAD_CAST "Group",     BAD_CAST "null");
			xmlNewTextChild(son4, NULL, BAD_CAST "Notes",     BAD_CAST "null");
			xmlNewTextChild(son4, NULL, BAD_CAST "FtpInfo", (const xmlChar *)getFtpInfo());
			//add_recourse_list_fileinfo(g_pInfo);
		}
	}

	closedir(dirp);

	sprintf(stotalSize, "%d", rowIndex);
	xmlNewTextChild(son3, NULL, BAD_CAST "TotalPage", BAD_CAST "1");
	xmlNewTextChild(son3, NULL, BAD_CAST "TotalCount", (const xmlChar *)stotalSize);

	return 0;
}

/*
1. 初始化XML文件
2. 查找指定节点
3. 对指定目录进行遍历获取信息

*/
int add_recourse_list_fileinfo(char *recourse_name)
{

	int         iRet    = -1;

	xmlDocPtr   doc     = NULL;
	xmlNodePtr  root    = NULL;

	char    FileSize[64] = {0};
	char    RcdTime[26] = {0};
	char recoursedir[256] = {0};
	xmlInitParser();

	//从目录中获取节点信息
	rec_dir_check(recourse_name, RcdTime);

	totalSize   = 0;
	sprintf(recoursedir, "%s/%s", REACH_REC_DIR, recourse_name);
	totalSize        = rec_dir_size(recoursedir)/(1024*1024);
	sprintf(FileSize, "%lld", totalSize);

	PRINTF("RecInfo:FileName[%s]filesize[%s]RcdTime[%s]\n"
	       , recourse_name, FileSize, RcdTime);


	iRet = xml_open(&doc, &root, REACH_REC_XML);

	if(-1 == iRet || NULL == doc || NULL == root) {
		PRINTF("Error xml_open\n");
		return -1;
	}
	//查找FileList节点
	son3    = xml_node_find_key(root, "FileList", NULL);

	//将新节点加入到XML文件
	son4    = xmlNewNode(NULL, BAD_CAST "FileInfo");
	xmlAddChild(son3, son4);
	xmlNewTextChild(son4, NULL, BAD_CAST "FileName", (const xmlChar *)recourse_name);
	xmlNewTextChild(son4, NULL, BAD_CAST "FileCName", (const xmlChar *)recourse_name);
	xmlNewTextChild(son4, NULL, BAD_CAST "Duration",  BAD_CAST "00");
	xmlNewTextChild(son4, NULL, BAD_CAST "FileSize", (const xmlChar *)FileSize);
	xmlNewTextChild(son4, NULL, BAD_CAST "RcdTime", (const xmlChar *)RcdTime);
	xmlNewTextChild(son4, NULL, BAD_CAST "Group",     BAD_CAST "null");
	xmlNewTextChild(son4, NULL, BAD_CAST "Notes",     BAD_CAST "null");
	xmlNewTextChild(son4, NULL, BAD_CAST "FtpInfo", (const xmlChar *)getFtpInfo());

	xml_node_mod(root, "TotalCount", 1);
	//del 2013-12-6 18:59:21

	xml_close(doc, REACH_REC_XML);
	xmlCleanupParser();     //xmlParseFile(argv[1]);//创建Dom树

	return 0;
}
/*2013-12-7 13:08:40 modify Param
//"file2.xml"
xmlNodePtr find  = NULL;
pnode = xml_node_new(BAD_CAST "FileName",BAD_CAST "20131125143400r0_CL3000_111");

del_recourse_list_fileinfo("file2.xml", pnode);

int del_recourse_list_fileinfo(char *FileName)

根据传进的文件名（课件名） 删除上面xml文件FileInfo节点*/
int del_recourse_list_fileinfo(char *recourse_name)
{

	if(NULL == recourse_name) {
		//PRINTF("Failed to the Param\n");
		return -1;
	}
	char *keyname = "FileName";
	int         iRet    = -1;
	xmlNodePtr  find    = NULL;
	xmlNodePtr  root    = NULL;
	//xmlNodePtr  pnode   = NULL;
	xmlDocPtr   doc     = NULL;

	xmlInitParser();

	iRet    = xml_open(&doc, &root, REACH_REC_XML);

	//pnode = xml_node_new(BAD_CAST "FileName",BAD_CAST "20131125143400r0_CL3000_111");
	//pnode   = xml_node_new(BAD_CAST"FileName",BAD_CAST FileName);

	idx     = 0;
	//find    = xml_node_find(root, pnode);
	find    = xml_node_find_key(root, keyname, recourse_name);

	if(find && find->parent) {
		find = find->parent;

		xml_node_del(find);

		//2013-12-6 19:16:28 add
		xml_node_mod(root, "TotalCount", -1);

		PRINTF("Success to xml_node_del TotalCount\n");
		iRet = 0;
	} else {
		iRet = -1;
		PRINTF("Failed to the xml_node_find_key\n");
	}

	//xml_node_free(pnode);

	xml_close(doc,REACH_REC_XML);

	xmlCleanupParser();     //xmlParseFile(argv[1]);//创建Dom树

	return iRet;
}
/*读取xml文件为cgi提供相应的课件信息*/
int get_recourse_list_info(char *filename, char *out_buf)
{
	xmlChar    *xml_buf = NULL;
	int32_t 	xmllen  = 0;

	int         iRet = -1;
	xmlDocPtr   doc  = NULL;
	xmlNodePtr  root = NULL;

	iRet = xml_open(&doc, &root, filename);

	xmlDocDumpFormatMemoryEnc(doc, &xml_buf, &xmllen, "UTF-8", 1);

	memcpy(out_buf, xml_buf, xmllen);

	xmlFree(xml_buf);

	xml_close(doc, filename);

	return 0;

}
/*

遍历/opt/Rec目录获取相关信息生成上面xml文件*/
int init_recourse_list_xml()
{
	int         iRet = -1;
	xmlDocPtr   doc  = NULL;  //xmlDocPtr  xmlDoc*
	xmlNodePtr  root = NULL;  //xmlNodePtr xmlNode*

	iRet = xml_create(&doc, &root);

	if(-1 == iRet) {
		PRINTF("Error xml_create\n");
		return -1;
	}

	iRet = xml_init(root);

	if(-1 == iRet) {
		PRINTF("Error xml_init\n");
		return -1;
	}

	//rec_dir_ergodic(REACH_REC_DIR);
	rec_dir_ergodic2(REACH_REC_DIR);


	xml_close(doc, REACH_REC_XML);

	return 0;
}

#ifdef RECOURSE_LIST_XML

int main(int argc, char **argv)
{
	init_recourse_list_xml();
	return 0;
}

#endif


