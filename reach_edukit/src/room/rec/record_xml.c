

#include "record_xml.h"
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
#define	blue_node(xmlfp, start, end)	fprintf(xmlfp, "<bluetime  start=\"%d\" end=\"%d\"/>\n", start, end)
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

void set_record_req_msg(int8_t *reqbuf, int32_t result, int8_t *course_root_dir)
{

	r_sprintf((reqbuf + MSG_HEAD_LEN), \
	          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"\
	          "<RequestMsg>\n"\
	          "\t<MsgHead>\n"\
	          "\t\t<MsgCode>36001</MsgCode>\n"\
	          "\t\t<PassKey>RecServer</PassKey>\n"\
	          "\t</MsgHead>\n"\
	          "\t<MsgBody>\n"\
	          "\t\t<RecReport>\n"\
	          "\t\t\t<Result>%d</Result>\n"\
	          "\t\t\t<FileName>%s</FileName>\n"\
	          "\t\t\t<PassKey>AllPlatform</PassKey>\n"\
	          "\t\t</RecReport>\n"\
	          "\t</MsgBody>\n"\
	          "</RequestMsg>\n", \
	          result, course_root_dir);
}

int8_t *hd2sd(int8_t *sd_dir, int8_t *hd_dir)
{
	r_strcpy(sd_dir, hd_dir);
	int8_t *p = r_strstr(sd_dir, "/HD/");

	if(NULL == p) {
		return NULL;
	}

	r_memcpy(p, "/SD/", 4);
	return sd_dir;
}


static void file_complete_copy(FILE *fp1, FILE *fp2, int8_t *strkey)
{
	char line[256] = {0};
	char *p = NULL;

	while(1) {
		p = fgets(line, 255, fp2);

		if(NULL == p) {
			break;
		}
		p = strstr(line, strkey);

		if(NULL == p) {
			break;
		}
		fprintf(fp1, "\t\t\t%s", line);
		fflush(fp1);
	}
}

static void info2index(FILE *index_fp, FILE *info_fp)
{
	char line[256] = {0};
	char *p = NULL;
	int width = 0;
	int height = 0;
	int starttime = 0;
	int time = 0;
	char filename[128] = {0};
	char image[128] = {0};
	int ret = 0;
	xml_head(index_fp);
	indexs_head_node(index_fp);
	fflush(index_fp);

	while(1) {
		if(NULL == info_fp) {
			break;
		}

		ret = fscanf(info_fp, FILE_NODE_FORMAT, &width, &height, &starttime, filename);
		p = strstr(filename, "</file>");

		if(NULL == p) {
			break;
		}

		strncpy(image, filename, p - filename);

		p = fgets(line, 255, info_fp);

		if(NULL == p) {
			break;
		}

		time = starttime;
		index_node(index_fp, time, image);
		fflush(index_fp);

	}

	indexs_end_node(index_fp);
}

int32_t create_ContentInfo_xml(int8_t *course_root_dir, ContentInfo_t *ci)
{
	if(course_root_dir == NULL) {
		nslog(NS_ERROR, "create_ContentInfo_xml : [%s] is failed !", course_root_dir);
		return -1;
	}

	nslog(NS_INFO, "create_ContentInfo_xml : [%s] start ...", course_root_dir);
	int8_t ContentInfo_xml[512] = {0};
	r_sprintf(ContentInfo_xml, "%s/ContentInfo.xml", course_root_dir);
	FILE *ContentInfo_xml_fp = NULL;

	if((ContentInfo_xml_fp = fopen(ContentInfo_xml, "w")) == NULL) {
		nslog(NS_ERROR, "fopen  : %s", strerror(errno));
		return -1;
	}

	xml_head(ContentInfo_xml_fp);
	ContentInfo_node_head(ContentInfo_xml_fp, ci->RecordID, ci->MainTeacher, ci->CName,
	                      ci->ScmType, ci->RecDateTime, ci->Notes);
	type_node(ContentInfo_xml_fp,
	          "SD",
	          "SD/resource/content.htm",
	          "SD/resource/content_pad.htm",
	          "SD/resource/content_pad.htm",
	          "SD/resource/content_i.htm");
	type_node(ContentInfo_xml_fp,
	          "HD",
	          "HD/resource/content.htm",
	          "HD/resource/content_pad.htm",
	          "HD/resource/content_pad.htm",
	          "HD/resource/content_i.htm");
	ContentInfo_node_end(ContentInfo_xml_fp);
	fclose(ContentInfo_xml_fp);
	nslog(NS_INFO, "create_ContentInfo_xml end .");
	return 0;
}

void record_info_file_node(FILE *file_node_fp, record_report_info_t *rep_info)
{
	if(0 == rep_info->width || 0 == rep_info->height) {
		return;
	}

	nslog(NS_INFO, "width:[%d] height:[%d] totaltime:[%d] filename:[%s]",
	      rep_info->width, rep_info->height, rep_info->totaltime, rep_info->filename);
	file_node(file_node_fp, rep_info->width, rep_info->height, rep_info->totaltime, rep_info->filename);
	fflush(file_node_fp);
}

void record_info_file_node_jpeg(FILE *file_node_fp, record_report_info_t *rep_info)
{
	if(0 == rep_info->width || 0 == rep_info->height) {
		return;
	}

	file_node_jpeg(file_node_fp, rep_info->width, rep_info->height, rep_info->totaltime, rep_info->filename);
	fflush(file_node_fp);
}

void record_blue_node(FILE *blue_node_fp, int32_t start, int32_t end)
{
	blue_node(blue_node_fp, start, end);
	fflush(blue_node_fp);
}

int32_t create_course_record_xml(int8_t *course_root_dir, int32_t course_record_totaltime, stream_type_sindex_sum_t *s)
{
	nslog(NS_INFO, "start...");

	if(NULL == course_root_dir || NULL == s) {
		nslog(NS_INFO, "param is error!");
		return -1;
	}

	int32_t sindex = 0;
	int32_t hd_sindex = 1;
	int32_t sd_sindex = 1;
	int32_t hd_sindex_sum = s->hd_sindex_sum;
	int32_t sd_sindex_sum = s->sd_sindex_sum;
	int32_t jpeg_sindex_sum = s->jpeg_sindex_sum;
	FILE *hd_info_xml_fp = NULL;
	FILE *sd_info_xml_fp = NULL;
	FILE *hd_index_xml_fp = NULL;
	FILE *sd_index_xml_fp = NULL;
	int8_t hd_info_xml[1024] = {0};
	int8_t sd_info_xml[1024] = {0};
	int8_t hd_index_xml[1024] = {0};
	int8_t sd_index_xml[1024] = {0};

	int8_t hd_info_xml_tmp[1024] = {0};
	int8_t sd_info_xml_tmp[1024] = {0};
	int8_t hd_index_xml_tmp[1024] = {0};
	int8_t sd_index_xml_tmp[1024] = {0};

	FILE *info_xml_x_fp = NULL;
	int8_t info_xml_x[1024] = {0};
	FILE *blue_x_fp = NULL;
	int8_t blue_x[1024] = {0};

	r_sprintf(hd_info_xml_tmp, "%s/HD/%s.tmp", course_root_dir, INFO_XML);

	if((hd_info_xml_fp = fopen(hd_info_xml_tmp, "w")) == NULL) {
		nslog(NS_ERROR, "fopen  : %s", strerror(errno));
		return -1;
	}

	hd2sd(sd_info_xml_tmp, hd_info_xml_tmp);

	if((sd_info_xml_fp = fopen(sd_info_xml_tmp, "w")) == NULL) {
		nslog(NS_ERROR, "fopen  : %s", strerror(errno));
		return -1;
	}

	r_sprintf(hd_index_xml_tmp, "%s/HD/%s.tmp", course_root_dir, INDEX_XML);

	if((hd_index_xml_fp = fopen(hd_index_xml_tmp, "w")) == NULL) {
		nslog(NS_ERROR, "fopen  : %s", strerror(errno));
		return -1;
	}

	hd2sd(sd_index_xml_tmp, hd_index_xml_tmp);

	if((sd_index_xml_fp = fopen(sd_index_xml_tmp, "w")) == NULL) {
		nslog(NS_ERROR, "fopen  : %s", strerror(errno));
		return -1;
	}

	for(sindex = 0; sindex < s->hd_sindex_sum; sindex ++) {
		r_sprintf(info_xml_x, "%s/HD/%s.%d", course_root_dir, INFO_XML, sindex + 1);

		if(get_file_size(info_xml_x) <= 0) {
			hd_sindex_sum --;
		}
	}

	for(sindex = 0; sindex < s->sd_sindex_sum; sindex ++) {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sindex + 1);

		if(get_file_size(info_xml_x) <= 0) {
			sd_sindex_sum --;
		}
	}
	if(s->jpeg_sindex_sum > 0) {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sindex + 1);

		if(get_file_size(info_xml_x) <= 0) {
			jpeg_sindex_sum --;
		}
	}
	nslog(NS_INFO, "xml_head hd_info_xml_fp start...");
	xml_head(hd_info_xml_fp);
	stream_head_node(hd_info_xml_fp, hd_sindex_sum + jpeg_sindex_sum, course_record_totaltime);
	fflush(hd_info_xml_fp);
	nslog(NS_INFO, "xml_head hd_info_xml_fp end...");
	nslog(NS_INFO, "xml_head sd_info_xml_fp start...");
	xml_head(sd_info_xml_fp);
	stream_head_node(sd_info_xml_fp, sd_sindex_sum + jpeg_sindex_sum, course_record_totaltime);
	fflush(sd_info_xml_fp);
	nslog(NS_INFO, "xml_head sd_info_xml_fp end...");
	nslog(NS_INFO, "channel_node hd_info_xml_fp start...");
	for(sindex = 0; sindex < s->hd_sindex_sum; sindex ++) {

		r_sprintf(info_xml_x, "%s/HD/%s.%d", course_root_dir, INFO_XML, hd_sindex);
		r_sprintf(blue_x, "%s/HD/%s.%d", course_root_dir, BLUE_X, hd_sindex);

		if((info_xml_x_fp = fopen(info_xml_x, "r")) == NULL) {
			nslog(NS_ERROR, "fopen[%d]  : %s", sindex, strerror(errno));
			return -1;
		}

		if((blue_x_fp = fopen(blue_x, "r")) == NULL) {
			nslog(NS_ERROR, "fopen[%d] [%s] : %s", sindex, blue_x, strerror(errno));
			return -1;
		}

		if(get_file_size(info_xml_x) > 0) {
			channel_head_node(hd_info_xml_fp, hd_sindex, "mp4");
			file_complete_copy(hd_info_xml_fp, info_xml_x_fp, "</file>");
			channel_blue_start_node(hd_info_xml_fp);
			file_complete_copy(hd_info_xml_fp, blue_x_fp, "/>");
			channel_blue_end_node(hd_info_xml_fp);
			fflush(hd_info_xml_fp);
		}

		hd_sindex ++;
		fclose(blue_x_fp);
		fclose(info_xml_x_fp);
		info_xml_x_fp = NULL;
	}
	nslog(NS_INFO, "channel_node hd_info_xml_fp end...");
	nslog(NS_INFO, "channel_node sd_info_xml_fp start...");
	for(sindex = 0; sindex < s->sd_sindex_sum; sindex ++) {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sd_sindex);
		r_sprintf(blue_x, "%s/SD/%s.%d", course_root_dir, BLUE_X, sd_sindex);

		if((info_xml_x_fp = fopen(info_xml_x, "r")) == NULL) {
			nslog(NS_ERROR, "fopen[%d]  : %s", sindex, strerror(errno));
			return -1;
		}

		if((blue_x_fp = fopen(blue_x, "r")) == NULL) {
			nslog(NS_ERROR, "fopen[%d] [%s] : %s", sindex, blue_x, strerror(errno));
			return -1;
		}

		if(get_file_size(info_xml_x) > 0) {
			channel_head_node(sd_info_xml_fp, sd_sindex, "mp4");
			file_complete_copy(sd_info_xml_fp, info_xml_x_fp, "</file>");
			channel_blue_start_node(sd_info_xml_fp);
			file_complete_copy(sd_info_xml_fp, blue_x_fp, "/>");
			channel_blue_end_node(sd_info_xml_fp);
			fflush(sd_info_xml_fp);
		}

		sd_sindex ++;
		fclose(blue_x_fp);
		fclose(info_xml_x_fp);
		info_xml_x_fp = NULL;
	}
	nslog(NS_INFO, "channel_node sd_info_xml_fp end...");
	info_xml_x_fp = NULL;
	nslog(NS_INFO, "channel_node jpeg_info_xml_fp start...");
	if(s->jpeg_sindex_sum > 0) {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sd_sindex);

		if((info_xml_x_fp = fopen(info_xml_x, "r")) == NULL) {
			nslog(NS_ERROR, "fopen[%d]  : %s", sindex, strerror(errno));
		}
		if(jpeg_sindex_sum) {
			channel_head_node(sd_info_xml_fp, sd_sindex, "jpg");
			file_complete_copy(sd_info_xml_fp, info_xml_x_fp, "</file>");
			channel_end_node(sd_info_xml_fp);
			rewind(info_xml_x_fp);
			channel_head_node(hd_info_xml_fp, hd_sindex, "jpg");
			file_complete_copy(hd_info_xml_fp, info_xml_x_fp, "</file>");
			channel_end_node(hd_info_xml_fp);
			fflush(sd_info_xml_fp);
			fflush(hd_info_xml_fp);
		}
	}
	nslog(NS_INFO, "channel_node jpeg_info_xml_fp end...");
	
	if(NULL != info_xml_x_fp) {
		rewind(info_xml_x_fp);
	}

	info2index(sd_index_xml_fp, info_xml_x_fp);

	if(NULL != info_xml_x_fp) {
		rewind(info_xml_x_fp);
	}

	info2index(hd_index_xml_fp, info_xml_x_fp);
	fflush(sd_index_xml_fp);
	fflush(hd_index_xml_fp);

	if(NULL != info_xml_x_fp) {
		fclose(info_xml_x_fp);
	}


	stream_end_node(hd_info_xml_fp);
	stream_end_node(sd_info_xml_fp);

	fclose(sd_info_xml_fp);
	fclose(hd_info_xml_fp);
	fclose(sd_index_xml_fp);
	fclose(hd_index_xml_fp);

	r_sprintf(hd_info_xml, "%s/HD/%s", course_root_dir, INFO_XML);
	rename(hd_info_xml_tmp, hd_info_xml);
	hd2sd(sd_info_xml, hd_info_xml);
	rename(sd_info_xml_tmp, sd_info_xml);

	r_sprintf(hd_index_xml, "%s/HD/%s", course_root_dir, INDEX_XML);
	rename(hd_index_xml_tmp, hd_index_xml);
	hd2sd(sd_index_xml, hd_index_xml);
	rename(sd_index_xml_tmp, sd_index_xml);
	hd_sindex = 1;
	sd_sindex = 1;

	for(sindex = 0; sindex < s->hd_sindex_sum; sindex ++)  {
		r_sprintf(info_xml_x, "%s/HD/%s.%d", course_root_dir, INFO_XML, hd_sindex);
		remove(info_xml_x);
		r_sprintf(blue_x, "%s/HD/%s.%d", course_root_dir, BLUE_X, hd_sindex);
		remove(blue_x);
		hd_sindex ++;
	}

	for(sindex = 0; sindex < s->sd_sindex_sum + s->jpeg_sindex_sum; sindex ++)  {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sd_sindex);
		remove(info_xml_x);
		r_sprintf(blue_x, "%s/SD/%s.%d", course_root_dir, BLUE_X, sd_sindex);
		remove(blue_x);
		sd_sindex ++;
	}

	nslog(NS_INFO, "end...");
	return 0;
}



