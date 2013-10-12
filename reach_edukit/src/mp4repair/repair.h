#ifndef _REPAIR_H_
#define _REPAIR_H_

#include "reach_os.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>  
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "Media.h"
#include "dirmd5.h"

#define  CONFIG                 "./repairroot.conf"   //根目录配置文件路径
#define  RECOVERY_PATH 			"/recovery"			  //修复文件目录索引





#define  MAX_REPAIR_NUM  		(2*1024)									  //最大修复目录数 
//#define  MAX_FILE_PATH_LEN      (2*1024)							
#define  DIRLEN                 (200)
#define  MAXCHANE                (10)

#define  HD_VIDEO     			"/HD/resource/videos"
#define  SD_VIDEO    			"/SD/resource/videos"
#define  INFO_XML_HD_INDEX 		"/HD/resource/info.xml."
#define  INFO_XML_SD_INDEX		"/SD/resource/info.xml."

/* 预览图路径 */
#define  HD_RESOURCE_IMAGE_PRE  "/HD/resource/images/"
#define  SD_RESOURCE_IMAGE_PRE  "/SD/resource/images/"

#define  REPAIR_TEMPFORM   		 ".temp.mp4"
#define  REPAIR_TMPFORM 		 ".tmp"
#define  REPAIR_VIDEOFORM    	 ".mp4"


#define  REPAIR_HD    (0x1)
#define  REPAIR_SD    (0x2)
#define  REPAIR_ALL   (0x3)

#define  JPEG_STREAM   2
#define  HD_STREAM     1
#define  SD_STREAM     0

#define 	MSG_HEAD_LEN	sizeof(MsgHeader)
#define  MAX_XML_PATH	 (sizeof(XML_PATH)/(sizeof(int8_t)*DIRLEN))	    //最大修复XML个数
typedef enum _stream_type {
	RECORD_HD,
	RECORD_SD,
	RECORD_JPEG
} stream_type_t;
typedef struct _stream_type_sindex_sum {
	int32_t hd_sindex_sum;
	int32_t sd_sindex_sum;
	int32_t jpeg_sindex_sum;
} stream_type_sindex_sum_t;
#define	VIDEO_DIR	"resource/videos"
#define	IMG_DIR		"resource/images"
#define	INFO_XML	"resource/info.xml"
#define	INDEX_XML	"resource/index.xml"
#define	BLUE_X		"resource/blue"

#define	xml_head(xmlfp)	fprintf(xmlfp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
//info.xml
#define	stream_head_node(xmlfp, channel, totaltime) fprintf(xmlfp, "<stream channel=\"%d\" totaltime=\"%d\">\n", channel, totaltime)
#define	stream_end_node(xmlfp) fprintf(xmlfp, "</stream>\n");
#define	channel_head_node(xmlfp, id, files_type) fprintf(xmlfp, "\t<channel>\n\t\t<ID>%d</ID>\n\t\t<files type=\"%s\">\n", id, files_type)
#define	channel_end_node(xmlfp) fprintf(xmlfp, "\t\t</files>\n\t\t<replaces/>\n\t\t<comments/>\n\t</channel>\n")
#define	channel_blue_start_node(xmlfp) fprintf(xmlfp, "\t\t</files>\n\t\t<replaces/>\n\t\t<comments/>\n\t\t<bluescreen>\n")
#define	channel_blue_end_node(xmlfp) fprintf(xmlfp, "\t\t</bluescreen>\n\t</channel>\n")
#define	file_node(xmlfp, width, height, totaltime, filename) fprintf(xmlfp, "<file width=\"%d\" height=\"%d\" totaltime=\"%d\">%s</file>\n", width, height, totaltime, filename)
#define	file_node_jpeg(xmlfp, width, height, starttime, filename) fprintf(xmlfp, "<file width=\"%d\" height=\"%d\" starttime=\"%d\">%s</file>\n", width, height, starttime, filename)

#define	FILE_NODE_FORMAT	"<file width=\"%d\" height=\"%d\" starttime=\"%d\">%s</file>"
#define	FILE_NODE_FORMAT1	"<file width=\"%d\" height=\"%d\" totaltime=\"%d\">%s</file>"
#define	blue_node(xmlfp, start, end)	fprintf(xmlfp, "<bluetime  start=\"%d\" end=\"%d\"/>\n", start, end)
#define BLUE_FORMAT 		"<bluetime  start=\"%d\" end=\"%d\"/>"

//index.xml
#define	indexs_head_node(xmlfp) fprintf(xmlfp, "<ReachIndex>\n\t<images>\n\t\t<startimage image=\"\" duration=\"0\"/>\n\t\t<endimage image=\"\" duration=\"0\"/>\n\t</images>\n\t<animates></animates>\n\t<indexs>\n")
#define	indexs_end_node(xmlfp) fprintf(xmlfp, "\t</indexs>\n</ReachIndex>\n")
#define	index_node(xmlfp, time, image) fprintf(xmlfp, "\t\t<index time=\"%d\" image=\"%s\"/>\n", time, image)

//ContentInfo.xml
#define	type_node(xmlfp, typename, pc, pad, phoneA, phoneI) \
	fprintf(xmlfp,\
	        "\t\t<Type Name=\"%s\">\n"\
	        "\t\t\t<PC>%s</PC>\n"\
	        "\t\t\t<Pad>%s</Pad>\n"\
	        "\t\t\t<Phone_A>%s</Phone_A>\n"\
	        "\t\t\t<Phone_I>%s</Phone_I>\n"\
	        "\t\t</Type>\n",  \
	        typename, pc, pad, phoneA, phoneI)
#define	ContentInfo_node_head(xmlfp, RecordID, MainTeacher, CName, ScmType, RecDateTime, Notes)\
	fprintf(xmlfp, \
	        "<ContentInfo>\n"\
	        "\t<RecordID>%s</RecordID>\n"\
	        "\t<MainTeacher>%s</MainTeacher>\n"\
	        "\t<CName>%s</CName>\n"\
	        "\t<ScmType>%s</ScmType>\n"\
	        "\t<RecDateTime>%s</RecDateTime>\n"\
	        "\t<Notes>%s</Notes>\n"\
	        "\t<TypeInfo Num=\"2\">\n", \
	        RecordID, MainTeacher, CName, ScmType, RecDateTime, Notes)
#define	ContentInfo_node_end(xmlfp)	\
	fprintf(xmlfp, \
	        "\t</TypeInfo>\n"\
	        "</ContentInfo>\n")

typedef int32_t (* operator)(int8_t *,void *);
typedef struct _MsgHeader //消息头
{
    unsigned short sLen;		//长度
    unsigned short sVer;		//版本
    unsigned short sMsgType;	//消息类型
    unsigned short sData;	//保留
}MsgHeader;



#endif
