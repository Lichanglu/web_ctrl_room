#ifndef	__RECORD_XML_H__
#define	__RECORD_XML_H__
#include "stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nslog.h"
#include "record.h"
#define	VIDEO_DIR	"resource/videos"
#define	IMG_DIR		"resource/images"
#define	INFO_XML	"resource/info.xml"
#define	INDEX_XML	"resource/index.xml"
#define	BLUE_X		"resource/blue"

int8_t *hd2sd(int8_t *sd_dir, int8_t *hd_dir);

int32_t create_ContentInfo_xml(int8_t *course_root_dir, ContentInfo_t *ci);

void record_info_file_node(FILE *file_node_fp, record_report_info_t *rep_info);
void record_info_file_node_jpeg(FILE *file_node_fp, record_report_info_t *rep_info);
void record_blue_node(FILE *blue_node_fp, int32_t start, int32_t end);
int32_t create_course_record_xml(int8_t *course_root_dir, int32_t course_record_totaltime, stream_type_sindex_sum_t *s);
void set_record_req_msg(int8_t *reqbuf, int32_t result, int8_t *course_root_dir);
#endif //__RECORD_XML_H__

