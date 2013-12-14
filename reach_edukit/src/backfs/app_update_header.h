#ifndef _APP_UPDATE_HEADER_H__
#define _APP_UPDATE_HEADER_H__

#define ADD_UPDATE_HEADER

typedef struct {
	int info_len; //sizeof(UP_HEADER_INFO)
	unsigned int checksum; //结构体的checksum
	char product_id[16]; //ENC1200VGA/SDI
	int major_version;         // 预留, 主版本必须一致，否则不升级 比如3.0,2.0等
	int minor_version;		  // 预留 0
	int pcb_version; //预留,
	int kernel_version; 		//预留,
	int fpga_version; 			//预留,
	int vss_version;		  // 预留,
	int force;                //预留,强制  //不必关注主版本号一致性???
	int compressmode;         //预留,压缩模式，预留
	unsigned int filesystem;  //预留,文件系统 yaffs ,jaffs
	char target[16];          //预留 ，比如kernel ,app等
	unsigned char custom_info[32];   //定制产品信息
	char reserve[8];		//预留
} UP_HEADER_INFO; //len

#define FIRST_UPDATE_HEADER "##################SZREACH#######"  //32 char 

#define UPDATE_HEARER_LEN 256 //空位填充0xff



int app_header_info_ok(char *name);
int app_add_extern_buff(char *name);
int app_del_extern_buff(char *name);


#endif


