/************************************************************************************
*  用来添加update header.
*												add by zm
*															2011-6-8
*
*
*
**************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include  <dirent.h>

#include "app_update_header.h"
#include "app_product.h"

#if 0
#include "log.h"
#include "log_common.h"
#else
#define PRINTF printf
#endif

//#define NEED_1200_HEADER

static UP_HEADER_INFO g_up_header_info ;
static char  g_extern_buff[256] = {0};
static int g_init_flag = 0;
static char g_board_type[16] = {0};

static	char g_enc1100_first_head[8] = {0x7E, 0x7E, 0x7E, 0x7E, 0x31, 0x31, 0x30, 0x30};
static	char g_enc1200_first_head[8] = {0x7E, 0x7E, 0x7E, 0x7E, 0x48, 0x45, 0x4E, 0x43};


//获取文件的总长
static unsigned int get_file_size(FILE *fp)
{
	if(fp == NULL) {
		return 0;
	}

	unsigned int file_len = 0;
	fseek(fp, 0L, SEEK_END);
	file_len = ftell(fp);
	PRINTF("info:file len =%u,the fp =%p\n", file_len, fp);
	fseek(fp, 0L, SEEK_SET);
	return file_len;
}

//获取checksum值
static int sys_checksum_int(char *buf, int len)
{
	int i;
	int lcheck = 0;
	int *ptr = (int *)buf;

	len >>= 2;

	for(i = 0; i < len; i ++) {
		lcheck += *ptr++;
	}

	return lcheck;
}

//判断头文件是否匹配
//先阶段只匹配product_id
static int  app_compare_header_info(UP_HEADER_INFO *old, UP_HEADER_INFO *newinfo)
{
	//int ret =  memcmp(old,newinfo,sizeof(UP_HEADER_INFO));
	//PRINTF("ret = %d\n",ret);
	//PRINTF("%s,%d,%d\n",old->product_id,old->info_len,old->checksum);
	//PRINTF("%s,%d,%d\n",newinfo->product_id,newinfo->info_len,newinfo->checksum);
	if(strcmp(old->product_id, newinfo->product_id) == 0) {
		return 0;
	}

	PRINTF("[app_compare_header_info] old->product_id : [%s], newinfo->product_id : [%s]\n", old->product_id, newinfo->product_id);
	return -1;
}

//填充256字节
static int app_fill_extern_buff()
{
	int len = 0;
	memcpy(g_extern_buff, FIRST_UPDATE_HEADER, 32);
	len += 32;
	memcpy(g_extern_buff + 32, &g_up_header_info, sizeof(UP_HEADER_INFO));
	return 0;
}

//初始化一下
void app_init_header_info()
{
	if(g_init_flag != 0) {
		return;
	}

	memset(g_extern_buff, 0xff, sizeof(g_extern_buff));
	memset(&g_up_header_info, 0, sizeof(UP_HEADER_INFO));
	g_up_header_info.info_len = sizeof(UP_HEADER_INFO);
	snprintf(g_up_header_info.product_id, sizeof(g_up_header_info.product_id), "%s", UPDATE_PRODUCT_ID);
	g_up_header_info.major_version = UPDATE_MAJOR_VERSION ;
	g_up_header_info.checksum = sys_checksum_int(g_up_header_info.product_id, g_up_header_info.info_len - 2 * sizeof(int));
	app_fill_extern_buff();
	PRINTF("app_init_header_info\n");
	g_init_flag = 1;
	return ;
}

int app_add_header_info(char *buf)
{
	UP_HEADER_INFO up_header_info;
	memset(&up_header_info, 0, sizeof(UP_HEADER_INFO));
	up_header_info.info_len = sizeof(UP_HEADER_INFO);

	if(!strcmp(g_board_type, "CL4000_DVI")) {
		snprintf(up_header_info.product_id, sizeof(up_header_info.product_id), "CL4000DVI");
	} else if(!strcmp(g_board_type, "CL4000_SDI")) {
		snprintf(up_header_info.product_id, sizeof(up_header_info.product_id),  "CL4000SDI");
	} else if(!strcmp(g_board_type, "ENC1200")) {
		snprintf(up_header_info.product_id, sizeof(up_header_info.product_id),  "ENC1200DVI");
	} else if(!strcmp(g_board_type, "DEC1100")) {
		snprintf(up_header_info.product_id, sizeof(up_header_info.product_id),  "DEC1100");
	} else {
		snprintf(up_header_info.product_id, sizeof(up_header_info.product_id), "%s", g_board_type);
		PRINTF("WARNING,please app_add_header_info!!!\n");
	}

	PRINTF("app_add_header_info : [%s]\n", g_board_type);

	up_header_info.major_version = UPDATE_MAJOR_VERSION ;
	up_header_info.checksum = sys_checksum_int(up_header_info.product_id, up_header_info.info_len - 2 * sizeof(int));
	memcpy(buf, FIRST_UPDATE_HEADER, 32);
	memcpy(buf + 32, &up_header_info, sizeof(UP_HEADER_INFO));
	PRINTF("app_add_header_info end\n");
	return 0;
}

//读入256字节，确认是否是带头的。
//0 表示是带正确header
// -2 表示不带header头
//-1 表示带header头，但是不正确
int app_header_info_ok(char *name)
{
	app_init_header_info();
	char buff[320] = {0};
	FILE *fp = NULL;
	unsigned int fp_len = 0;
	int ret = 0;
	fp = fopen(name, "r");

	if(fp == NULL) {
		PRINTF("ERROR,fopen the file %s is failed \n", name);
		return 2;
	}

	fp_len = get_file_size(fp);
	PRINTF("21the old file  len is %u.\n", fp_len);

	if(fp_len <= 256) {
		PRINTF("1ERROR!\n");
		fclose(fp);
		fp = NULL;
		return 2;
	}

#ifdef NEED_1200_HEADER
	memset(buff, 0, sizeof(buff));
	ret = fread(buff, 1, 8, fp);
	PRINTF("fread read_len = %d\n", ret);
#endif


	ret = fread(buff, 1, 256, fp);

	if(ret < 256) {
		PRINTF("2ERROR\n");
		fclose(fp);
		fp = NULL;
		return 2;
	}

	fclose(fp);
	fp = NULL;

	if(memcmp(buff, FIRST_UPDATE_HEADER, 32) != 0) {
		PRINTF("ERROR,first error\n");
		return 2;
	}

	UP_HEADER_INFO newinfo;
	memset(&newinfo, 0, sizeof(UP_HEADER_INFO));
	memcpy(&newinfo, buff + 32, sizeof(UP_HEADER_INFO));

	return app_compare_header_info(&g_up_header_info, &newinfo);
}

#define FIRST_HEAD "EncoderManager"
//添加256字节头在tgz前面
int app_add_extern_buff(char *name)
{
	char  extern_buff[256] = {0};
	app_add_header_info(extern_buff);
	FILE *fp_old = NULL;
	FILE *fp = NULL;
	int ret = 0;
	unsigned int fp_len = 0;
	unsigned int fp_old_len = 0;
	char filename[256] = {0};
	char pname[128] = {0};
	int pname_len = strlen(name) - 4;
	memcpy(pname, name, pname_len);
	memcpy(pname + pname_len, ".bin", 4);
	char *first_head = NULL;


	if(!strcmp(g_board_type, "ENC1100")) {
		first_head = g_enc1100_first_head;
	} else {
		first_head = g_enc1200_first_head;
	}

	sprintf(filename, "%s.%s", FIRST_HEAD, pname);
	PRINTF("filename= #%s#,#%s#\n", filename, pname);
	fp_old = fopen(name, "r");

	if(fp_old == NULL) {
		PRINTF("ERROR,fopen the file %s is failed \n", name);
		return -1;
	}

	fp = fopen(filename, "w+");

	if(fp == NULL) {
		fclose(fp_old);
		fp_old = NULL;
		PRINTF("ERROR,fopen the file %s is failed \n", filename);
		return -1;
	}

	// first header
	ret = fwrite(first_head, 1, 8, fp);

	if(ret != 8) {
		PRINTF("ERROR,the file write first header is error\n");
		fclose(fp);
		fclose(fp_old);
		fp = fp_old = NULL;
		return -1;
	}

	if(strcmp(g_board_type, "ENC1100")) {
		//second header
		ret = fwrite(extern_buff, 1, sizeof(extern_buff), fp);

		if(ret != sizeof(extern_buff)) {
			PRINTF("ERROR,the file write second header is error\n");
			fclose(fp);
			fclose(fp_old);
			fp = fp_old = NULL;
			return -1;
		}
	}

	fp_old_len = get_file_size(fp_old);
	PRINTF("22the old file  len is %u.\n", fp_old_len);

	char buff[1024 * 100] = {0};
	int read_len = 0;
	int write_len = 0;

	while(feof(fp_old) == 0) {
		memset(buff, 0, sizeof(buff));
		read_len = fread(buff, 1, sizeof(buff), fp_old);
		PRINTF("fread read_len = %d\n", read_len);
		write_len = fwrite(buff, 1, read_len, fp);
		PRINTF("fwrite read_len = %d\n", write_len);

		if(read_len != write_len) {
			PRINTF("error\n");
			fclose(fp);
			fclose(fp_old);
			fp = fp_old = NULL;
			return -1;
		}
	}

	fp_len = get_file_size(fp);

	PRINTF("1the new file  len is %u.\n", fp_len);

	fclose(fp);
	fclose(fp_old);
	sync();
	return 0;
}
/*********************************************************************************
* 去掉文件尾部的256字节
*
*
*
***********************************************************************************/
//int app_del_extern_buff(char *filename)
//{
//	int fd = 0;
//	FILE *fp = NULL;
//	int ret = 0;
//	unsigned int fp_len = 0;
//	fp = fopen(filename,"a+");
//	if(fp == NULL)
//	{
//		PRINTF("ERROR,fopen the file %s is failed \n",filename);
//		return -1;
//	}
//	fp_len = get_file_size(fp);
//	PRINTF("the file old len is %ld.\n",fp_len)
//	if(fp_len <= 256)
//	{
//		PRINTF("error,the file is small\n");
//		return -1;
//	}
//	fd = fileno(fp);
//	if(fd == -1)
//	{
//		PRINTF("ERROR,get the file no is failed\n");
//		return -1;
//	}
//
//	ret = ftruncate(fd,fp_len - 256);
//	if(ret == -1)
//	{
//		PRINTF("ERROR,ftruncate file is error\n");
//		return -1;
//	}
//	fclose(fp);
//	return 0;
//}

//去掉头部的256字节 ，生产temp_update.tgz
int app_del_extern_buff(char *name)
{
	app_init_header_info();

	FILE *fp_old = NULL;
	FILE *fp = NULL;
	//int ret = 0;
	unsigned int fp_len = 0;
	unsigned int fp_old_len = 0;
	char filename[256] = {0};
	char tempbuff[320] = {0};
	fp_old = fopen(name, "r");
	char buff[1024 * 100] = {0};
	int read_len = 0;
	int write_len = 0;

	if(fp_old == NULL) {
		PRINTF("ERROR,fopen the file %s is failed \n", name);
		return -1;
	}


#ifdef NEED_1200_HEADER
	memset(tempbuff, 0, sizeof(tempbuff));
	read_len = fread(tempbuff, 1, 8, fp_old);
	PRINTF("fread read_len = %d\n", read_len);
#endif

	memset(tempbuff, 0, sizeof(tempbuff));

	if(fread(tempbuff, 1, UPDATE_HEARER_LEN, fp_old) != UPDATE_HEARER_LEN) {
		PRINTF("ERROR,this file is to small\n");
		return -1;
	}

	sprintf(filename, "%s_temp", name);
	fp = fopen(filename, "w+");

	if(fp == NULL) {
		fclose(fp_old);
		PRINTF("ERROR,fopen the file %s is failed \n", filename);
		return -1;
	}

	fp_old_len = get_file_size(fp_old);
	PRINTF("23the old file  len is %u.\n", fp_old_len);

	if(fp_old_len < 256) {
		fclose(fp_old);
		fclose(fp);
		fp = fp_old = NULL;
		return -1;
	}








#ifdef NEED_1200_HEADER
	memset(buff, 0, sizeof(buff));
	read_len = fread(buff, 1, 8, fp_old);
	PRINTF("fread read_len = %d\n", read_len);
#endif
	read_len = fread(buff, 1, 256, fp_old);
	PRINTF("fread read_len = %d\n", read_len);

	if(read_len < 256) {
		fclose(fp);
		fp = fp_old = NULL;
		return -1;
	}

	while(feof(fp_old) == 0) {
		memset(buff, 0, sizeof(buff));
		read_len = fread(buff, 1, sizeof(buff), fp_old);
		//PRINTF("fread read_len = %d\n",read_len);
		write_len = fwrite(buff, 1, read_len, fp);

		//PRINTF("fwrite read_len = %d\n",write_len);
		if(read_len != write_len) {
			fclose(fp);
			fp = fp_old = NULL;
			PRINTF("error\n");
			return -1;
		}
	}


	fp_len = get_file_size(fp);
	PRINTF("1the new file  len is %u.\n", fp_len);

	fclose(fp);
	fclose(fp_old);
	sync();
	return 0;
}


#ifdef PC_APP
static char g_box_ver[10] = {0};
int main(int argc, char **argv)
{
#if 0
	char cmdbuf[256] = {0};
	char tgzfile[128] = {0};

	char decfile[100] = {0};

	if(argc < 4) {
		PRINTF("Usga:%s [BOARD_TYPE] [BOX_VER]\n", argv[0]);
		return -1;
	}

	memcpy(g_board_type, argv[1], strlen(argv[1]));
	memcpy(g_box_ver, argv[2], 10);

	if(argc == 4) {
		snprintf(decfile, sizeof(decfile), "%s", argv[3]);
		PRINTF("the decfile is #%s#\n", decfile);
	}

	//非dec1100
	if(strstr(g_board_type, "DEC1100") == NULL) {
		sprintf(cmdbuf, "cd %s.%s ; tar cfvz ../%s.%s.tgz . ; cd ..", g_board_type, g_box_ver, g_board_type, g_box_ver);
		PRINTF("%s\n", cmdbuf);
		system(cmdbuf);
		sprintf(tgzfile, "%s.%s.tgz", g_board_type, g_box_ver);
	} else {
		strcpy(tgzfile, decfile);
	}

	app_add_extern_buff(tgzfile);

#else
	char filename[256] = {0};
	int ret = 0;
	char command[320] = {0};
	strcpy(filename, argv[1]);
	ret = app_header_info_ok(filename);

	if(ret == 0) {
		if(app_del_extern_buff(filename) == -1) {
			PRINTF("error! del the file extern_buff is error\n");
			return -1;
		}

		sprintf(command, "rm -rf %s;mv %s_temp %s", filename, filename, filename);
		PRINTF("command=%s\n", command);
		system(command);

	} else {
		if(ret == 2) {
			PRINTF("This file isn't have add extern update header\n");
		} else {
			PRINTF("This file have add extern update header,but the header have some error\n");
			return -1;
		}
	}

#endif


	return 0;
}
#endif

