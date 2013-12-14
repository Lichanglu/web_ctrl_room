
#include "record.h"
#include "record_xml.h"
#include "media_msg.h"
#include "dirmd5.h"

mp4_course_record_t *register_mp4_course_record_mode(mp4_course_record_condition_t *crc);
void unregister_mp4_course_record_mode(mp4_course_record_t **cr);
static int32_t get_mp4_file_limits(void)
{
	int32_t max_split_len = MAX_SPLIT_LEN;

	return max_split_len;
}

void mp4_record_resume(mp4_record_t *mr)
{
	mr->rs = RECORD_RESUME;
}

void mp4_record_pause(mp4_record_t *mr)
{
	mr->rs = RECORD_PAUSE;
}

void mp4_record_close(mp4_record_t *mr)
{
	mr->rs = RECORD_CLOSE;
}

record_status_t get_mp4_record_status(mp4_record_t *mr)
{
	return mr->rs;
}

void jpeg_record_resume(jpeg_record_t *jr)
{
	jr->rs = RECORD_RESUME;
}

void jpeg_record_pause(jpeg_record_t *jr)
{
	jr->rs = RECORD_PAUSE;
}

void jpeg_record_close(jpeg_record_t *jr)
{
	jr->rs = RECORD_CLOSE;
}

record_status_t get_jpeg_record_status(jpeg_record_t *jr)
{
	return jr->rs;
}

record_status_t get_course_record_status(course_record_t *cr)
{
	if(NULL == cr) {
		return -1;
	}

	return cr->rs;
}


static int aac_adts_overwrite_sampling(unsigned char *pdata)
{
	unsigned char sampling_frequency_index;
	unsigned char new_sampling_frequency_index = 0;
	unsigned char profile = (pdata[2] & 0xc0) >> 6;
	unsigned char new_profile = 0;

	sampling_frequency_index = (pdata[2] & 0x3c) >> 2;
	nslog(NS_INFO, "pdata[2]=[0x%2x],sampling_frequency_index=[0x%x]profile=[0x%x]\n", pdata[2], sampling_frequency_index, profile);

	return 0;
}

void *mp4_record_process(void *arg)
{
	nslog(NS_INFO, "start ...");
	mp4_record_t *mr = (mp4_record_t *)arg;
	int32_t sindex = mr->sindex;
	nslog(NS_INFO, "start ...sindex[%d]", sindex);
	MuxWriter *pMW = &(mr->MW);
	int32_t frame_socket = -1;
	int32_t ret = -1;
	int32_t pd_len = sizeof(parse_data_t);
	parse_data_t pd;
	int32_t AudioStreamIdx = -1;
	int32_t VideoStreamIdx = -1;
	uint32_t Vreftime = 0;
	uint32_t Areftime = 0;
	int64_t Apts = 0;
	int64_t Vpts = 0;
	int64_t preApts = 0;
	int64_t preVpts = 0;
	int32_t firstIframe = 0;
	int32_t Iframe = 0;
	int32_t firststartRecord = 1;
	uint32_t Vtotalsize = 0;
	uint32_t Atotalsize = 0;
	int32_t video_ok = 1;
	int32_t audio_ok = 1;
	int32_t pause_ok = 1;
	int32_t record_flag = 1;

	int64_t pause_Vpts = 0;
	int64_t pause_Apts = 0;

	int32_t Vflag = 0;
	int32_t Aflag = 0;
	while(1) {
		r_memset(&pd, 0, pd_len);

		if(mr->get_media_buf_info_callback(mr, &pd) <  0) {
			ms_delay(30);

			if(RECORD_PAUSE == get_mp4_record_status(mr)) {
				if(pause_ok) {
					nslog(NS_INFO, "pause sindex:[%d] is OK!", sindex);
					pause_ok = 0;
					audio_ok = 1;
					video_ok = 1;
				}

				record_flag = 1;
			}

			if(RECORD_CLOSE == get_mp4_record_status(mr)) {
				nslog(NS_WARN, "mp4 RECORD_CLOSE[%d].num:[%d]", sindex, get_msg_num(mr->course_data_fd));
				MediaWriteTrailer(pMW);
				break;
			}

			continue;
		}

		if(RECORD_PAUSE == get_mp4_record_status(mr)) {
			if(R_AUDIO == pd.data_type) {
				Areftime = pd.time_tick;
			} else {
				Vreftime = pd.time_tick;
			}

			mr->release_media_buf_callback(&pd);
			firstIframe = 0;
			ms_delay(100);

			if(pause_ok) {
				nslog(NS_INFO, "pause sindex:[%d] Vpts:[%lld] Apts:[%lld]is OK!", sindex, Vpts, Apts);
				pause_ok = 0;
				audio_ok = 1;
				video_ok = 1;
			}

			record_flag = 1;
			continue;
		}

		if(record_flag) {
			if(R_AUDIO == pd.data_type) {
				Areftime = pd.time_tick;
			} else {
				Vreftime = pd.time_tick;
			}

			firstIframe = 0;
			pause_Vpts = Vpts;
			pause_Apts = Apts;
			record_flag = 0;

			audio_ok = 1;
			video_ok = 1;
		}

		if((0 == firstIframe) && (R_VIDEO == pd.data_type)) {
			if(I_FRAME != pd.flags) {
				mr->release_media_buf_callback(&pd);
				Vreftime = pd.time_tick;
				continue;
			}

			Vreftime = pd.time_tick;
			//nslog(NS_ERROR,"FUCK_GOD_1130_3<Vreftime : %u> <stream_id : %d> \n",Vreftime,mr->record_info.media_msg_type);
			if(firststartRecord) {
				nslog(NS_ERROR,"SHIRT_GOD_1016 audio_SampleRate : %d  audio_BitRate : %d\n",
					mr->record_info.audio_SampleRate ,mr->record_info.audio_BitRate);
				if(mr->record_info.audio_SampleRate > 0 && mr->record_info.audio_BitRate > 0) {
					AudioStreamIdx = AudioStreamAdd(pMW, CODEC_ID_AAC, mr->record_info.audio_SampleRate, mr->record_info.audio_BitRate, 2, 1);
					nslog(NS_ERROR,"SHIRT_GOD_1016 \n");
					if(AudioStreamIdx < 0) {
						nslog(NS_ERROR, "AudioStreamAdd is failed!!!");
						return NULL;
					}
				}

				VideoStreamIdx = VideoStreamAdd(pMW, CODEC_ID_H264, mr->record_info.width, mr->record_info.height, 0, STREAM_FRAME_RATE);

				if(VideoStreamIdx < 0) {
					nslog(NS_ERROR, "VideoStreamAdd is failed!!!");
					return NULL;
				}

				Vreftime = 0;
				Areftime = 0;
				firststartRecord = 0;
			}

			firstIframe = 1;
		}

		if(R_AUDIO == pd.data_type) {
			if(firstIframe) {

				if(Aflag) {
					Apts = (pd.time_tick - Areftime) + pause_Apts;

					if((Apts - preApts) <= 0) {
						Apts = preApts + 1;
					}

					preApts = Apts;
				}

				if(!Apts) {
					Areftime = pd.time_tick;
					Aflag = 1;
				}

				//	aac_adts_overwrite_sampling(pd.data);
				//	if(0 == pd.sindex)
				//			nslog(NS_INFO, "Apts:[%lld]", Apts);
				if(MediaWriteFrame(pMW, pd.data, pd.data_len, Apts, AudioStreamIdx, 1, 1) < 0) {
					nslog(NS_WARN, "audio[%d] MediaWriteFrame is failed : [%s]", sindex, strerror(errno));
					nslog(NS_ERROR,"FUCK_GOD_1014 : MSG_TYPE : %d\n",mr->record_info.media_msg_type);
					pd.data_len = 0;
				}


				Atotalsize += pd.data_len;

				if(audio_ok) {

					nslog(NS_INFO,"STREAM_ID : %d\n",mr->record_info.media_msg_type);
					nslog(NS_INFO, "=<Asindex> <ASample> <ABit> <Apts>");
					nslog(NS_INFO, "=<%7d> <%7d> <%4d> <%4d>",
					      sindex,  mr->record_info.audio_SampleRate,
					      mr->record_info.audio_BitRate, Apts);
					nslog(NS_INFO, "=<sindex> <Sample> <Bit>");
					nslog(NS_INFO, "=<%6d> <%6d> <%3d>\n",
					      pd.sindex, pd.sample_rate, pd.code_rate);
					audio_ok = 0;
					pause_ok = 1;
				}

			} else {
				Areftime = pd.time_tick;
			}
		}
		else
		{

			if(Vflag) {
				Vpts = (pd.time_tick - Vreftime) + pause_Vpts;

				if((Vpts - preVpts) <= 0) {
					Vpts = preVpts + 1;
				}

				preVpts = Vpts;
			}

			if(!Vpts) {
				Vreftime = pd.time_tick;
				Vflag = 1;
			}

			Iframe = ((I_FRAME == pd.flags) ? 1 : 0);

			//	if(0 == pd.sindex)
			//		nslog(NS_INFO, "Vpts:[%lld]", Vpts);
			if(MediaWriteFrame(pMW, pd.data, pd.data_len, Vpts, VideoStreamIdx, Iframe, 0) < 0) {
				nslog(NS_WARN, "video[%d] MediaWriteFrame is failed:[%s]", sindex, strerror(errno));
				pd.data_len = 0;
			}

			Vtotalsize += pd.data_len;

			if(video_ok) {
				nslog(NS_INFO,"STREAM_ID : %d\n",mr->record_info.media_msg_type);
				nslog(NS_INFO, "=<Vsindex> <Vwidth> <Vheight> <Vpts>");
				nslog(NS_INFO, "=<%7d> <%6d> <%7d> <%4d>",
				      sindex,  mr->record_info.width,
				      mr->record_info.height,  Vpts);
				nslog(NS_INFO, "=<sindex> <width> <height>");
				nslog(NS_INFO, "=<%6d> <%5d> <%6d>\n",
				      pd.sindex, pd.width, pd.height);
				video_ok = 0;
				pause_ok = 1;
			}

			record_report_info_t *rep_info = (record_report_info_t *)r_malloc(sizeof(record_report_info_t));
			r_memset(rep_info, 0, sizeof(record_report_info_t));
			rep_info->sindex = sindex;
			rep_info->width = pd.width;
			rep_info->height = pd.height;
			rep_info->totaltime = Vpts / 1000;
			rep_info->totalsize = Vtotalsize + Atotalsize;

			if(!pd.data_len) {
				rep_info->totaltime = 0;
			}

			#if 0 // add zl
			if(rep_info->totaltime == 0)
			{
				nslog(NS_ERROR,"FUCK_GOD_1130 ---<Vpts : %lld> <Apts : %lld>---<pd.data_len : %d> ---<pd.time_tick : %lld> ---<Vreftime : %u> ---<Areftime : %u> ---<pause_Apts :%lld> ---<pause_Vpts : %lld>\n",
					Vpts,Apts,pd.data_len,pd.time_tick,Vreftime,Areftime,pause_Apts,pause_Vpts);

				nslog(NS_ERROR,"FUCK_GOD_1130 ---<preVpts : %lld> ---<preApts : %lld> ----<MEDIA_TYPE: %d> <stream_id:%d>\n"
					,preVpts,preApts,pd.data_type,mr->record_info.media_msg_type);
			}
			#endif

			rep_info->blue_flag = pd.blue_flag;
			r_strcpy(rep_info->filename, mr->filename);

			msgque msgp;
			r_memset(&msgp, 0, sizeof(msgque));
			msgp.msgtype = sindex + 1;
			msgp.msgbuf = (int8_t *)rep_info;
			ret = r_msg_send(mr->course_msg_fd, &msgp, sizeof(msgque) - sizeof(long), IPC_NOWAIT);

			if(ret < 0) {
				if(rep_info) {
					r_free(rep_info);
				}

				ms_delay(30);
				nslog(NS_ERROR, "r_msg_send[%d]  : %s", sindex, strerror(errno));
			}
		}

		mr->release_media_buf_callback(&pd);
	}

	nslog(NS_INFO, "end ...sindex[%d]", sindex);
	return mr;
}

int32_t mp4_record_start(mp4_record_t *mr)
{
	if(r_pthread_create(&(mr->p), NULL, mp4_record_process, (void *)mr)) {
		nslog(NS_ERROR, "r_pthread_create failed !!!\n");
		return -1;
	}

	return 0;
}


mp4_record_t *register_mp4_record_mode(mp4_record_condition_t *rcond)
{

	if(NULL == rcond) {
		nslog(NS_ERROR, "param is NULL");
		return NULL;
	}

	nslog(NS_INFO, "start ...sindex[%d]", rcond->sindex);
	MuxWriter *pMW = NULL;
	mp4_record_t *mr = (mp4_record_t *)r_malloc(sizeof(mp4_record_t));

	if(NULL == mr) {
		nslog(NS_ERROR, "mp4_record_t is NULL");
		return NULL;
	}

	memset(mr, 0, sizeof(mp4_record_t));
	mr->sindex = rcond->sindex;
	pMW = &(mr->MW);
	MediaWriterInit(pMW);

	if(MediaFileCreate(pMW, rcond->record_file, 1) < 0) {
		nslog(NS_ERROR, "MediaFileCreate is failed: %s", strerror(errno));
		MediaWriterClose(pMW);
		r_free(mr);
		return NULL;
	}

	int8_t *p = r_strstr((const int8_t *)(rcond->record_file), (const int8_t *)"videos/");

	if(NULL == p) {
		nslog(NS_ERROR, "r_strstr is NULL");
		MediaWriterClose(pMW);
		r_free(mr);
		return NULL;
	}
	// add zl

	mr->audio_base_time = 0;
	mr->audio_chace_time = 0;
	mr->video_base_time = 0;
	mr->video_chace_time = 0;

	r_strcpy(mr->filename, p);
	mr->record_info = rcond->record_info;
	mr->course_msg_fd = rcond->course_msg_fd;
	mr->course_data_fd = rcond->course_data_fd;
	mr->get_media_buf_info_callback	 = rcond->get_media_buf_info_callback;
	mr->release_media_buf_callback = rcond->release_media_buf_callback;

	mr->record_start = mp4_record_start;
	mr->record_resume = mp4_record_resume;
	mr->record_pause = mp4_record_pause;
	mr->record_close = mp4_record_close;
	nslog(NS_INFO, "end.sindex:[%d]", rcond->sindex);
	return mr;
}

void unregister_mp4_record_mode(mp4_record_t **mr)
{
	mp4_record_t *pmr = *mr;

	if(NULL == pmr) {
		nslog(NS_ERROR, "NULL == pmr");
		return;
	}

	int32_t sindex = pmr->sindex;
	nslog(NS_INFO, "start ...[%d]", sindex);
	MediaWriterClose(&(pmr->MW));
	r_free(pmr);
	*mr = NULL;
	nslog(NS_INFO, "end ...[%d]", sindex);
}
#if 1
static int32_t fdata2file(int8_t *filename, int8_t *data, int32_t data_len)
{
	FILE *fp = NULL;

	if((fp = fopen(filename, "w")) == NULL) {
		nslog(NS_ERROR, "fopen[%s]  : %s", filename, strerror(errno));
		return -1;
	}

	if(NULL != data || 0 < data_len) {
		if(1 > fwrite(data, data_len, 1, fp)) {
			nslog(NS_WARN, "fwrite  : %s", strerror(errno));
		}
	}

	fclose(fp);
	return 0;
}
#endif
static int32_t data2file(int8_t *filename, int8_t *data, int32_t data_len)
{
	int32_t fd = -1;

	if((fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC | O_DSYNC, 0666)) < 0) {
		nslog(NS_ERROR, "open[%s]  : %s", filename, strerror(errno));
		return -1;
	}

	if(NULL != data || 0 < data_len) {
		if(0 > write(fd, data, data_len)) {
			nslog(NS_WARN, "write  : %s", strerror(errno));
			close(fd);
			return -1;
		}
	}

	close(fd);
	return 0;
}


#define	MAX_JPEG_NUM	512
void *jpeg_record_process(void *arg)
{
	jpeg_record_t *jr = (jpeg_record_t *)arg;
	int32_t sindex = jr->sindex;
	nslog(NS_INFO, "start jpeg sindex [%d]...", sindex);
	int32_t ret = -1;
	int32_t pd_len = sizeof(parse_data_t);
	parse_data_t pd;
	int32_t jpeg_index = 1;
	int8_t  sd_jpegname[512] = {0};
	int8_t  hd_jpegname[512] = {0};
	uint32_t reftime = 0;
	uint32_t curtime = 0;
	uint32_t starttime = 0;
	uint32_t firsttime = 0;
	uint32_t jts = 0;
	uint32_t pause_jts = 0;
	int32_t jpeg_ok = 1;
	int32_t pause_ok = 1;
	int32_t record_flag = 1;
	uint32_t jpeg_lasttime = 0;
	uint32_t pause_starttime = 0;
	uint32_t jpeg_starttime = 0;
	uint32_t pause_lasttime = 0;
	starttime = get_time();
	while(1) {

		r_memset(&pd, 0, pd_len);

		if(jr->get_jpeg_buf_info_callback(jr, &pd) <  0) {

			if(RECORD_PAUSE == get_jpeg_record_status(jr)) {
				if(pause_ok) {
					nslog(NS_INFO, "pause sindex:[%d] NUM: [%d] is OK!", sindex, jpeg_index);
					pause_ok = 0;
					jpeg_ok = 1;
					pause_starttime = get_time();
				}

				record_flag = 1;
				pause_lasttime = get_time();
			}

			if(RECORD_CLOSE == get_jpeg_record_status(jr)) {
				nslog(NS_WARN, "RECORD_CLOSE[%d].num:[%d]", sindex, get_msg_num(jr->course_data_fd));
				break;
			}

			ms_delay(500);
			continue;
		}

		if(RECORD_PAUSE == get_jpeg_record_status(jr) || MAX_JPEG_NUM < jpeg_index) {
			reftime = pd.time_tick;
			jr->release_jpeg_buf_callback(&pd);

			if(pause_ok) {
				nslog(NS_INFO, "pause sindex:[%d] NUM: [%d] is OK!", sindex, jpeg_index);
				pause_ok = 0;
				jpeg_ok = 1;
				pause_starttime = get_time();
			}

			pause_lasttime = get_time();
			record_flag = 1;
			ms_delay(50);
			continue;
		}

		if(record_flag) {
			reftime = pd.time_tick;

			if(pause_lasttime) {
				jpeg_starttime = get_time();
			}

			pause_jts = jts + (pause_starttime - jpeg_lasttime) + (jpeg_starttime - pause_lasttime);
			record_flag = 0;
			pause_starttime = 0;
			jpeg_lasttime = 0;
			jpeg_starttime = 0;
			pause_lasttime = 0;
		}

		if(jpeg_ok) {
			nslog(NS_INFO, "sindex:[%d] pd.sindex:[%d] is OK!", sindex, pd.sindex);
			pause_ok = 1;
			jpeg_ok = 0;
		}

		curtime = pd.time_tick;

		if(jts) {
			jts = (curtime - reftime) + pause_jts + (firsttime - starttime);
			jpeg_lasttime = get_time();
		}

		if(!jts) {
			reftime = pd.time_tick;
			firsttime = get_time();
			jts = firsttime - starttime;
			nslog(NS_WARN, " firsttime:[%u] tarttime:[%u]",  firsttime, starttime);
		}

		r_sprintf(hd_jpegname, "%s/%d.jpg", jr->jpeg_dir, jpeg_index);
		nslog(NS_ERROR,"FUCK_GOD_1114 JPEG NUM :%d\n",jpeg_index);

		if(1 == jpeg_index) {
			if(fdata2file(hd_jpegname, pd.data, pd.data_len) < 0) {
				nslog(NS_WARN, "hd data2file is failed!");
				pd.data_len = 0;
			}

			hd2sd(sd_jpegname, hd_jpegname);

			if(fdata2file(sd_jpegname, pd.data, pd.data_len) < 0) {
				nslog(NS_WARN, "sd data2file is failed!");
				pd.data_len = 0;
			}
		} else {
			if(data2file(hd_jpegname, pd.data, pd.data_len) < 0) {
				nslog(NS_WARN, "hd data2file is failed!");
				pd.data_len = 0;
			}

			hd2sd(sd_jpegname, hd_jpegname);

			if(data2file(sd_jpegname, pd.data, pd.data_len) < 0) {
				nslog(NS_WARN, "sd data2file is failed!");
				pd.data_len = 0;
			}
		}

		jpeg_index ++;
		record_report_info_t *rep_info = (record_report_info_t *)r_malloc(sizeof(record_report_info_t));
		r_memset(rep_info, 0, sizeof(record_report_info_t));
		rep_info->sindex = sindex;
		rep_info->width = pd.width;
		rep_info->height = pd.height;
		rep_info->totaltime = jts / 1000;

		if(!pd.data_len) {
			rep_info->totaltime = 0;
		}
//		nslog(NS_WARN, " pd.time_tick : [%lld] jts:[%u] totaltime:[%d]",
//				pd.time_tick, jts, rep_info->totaltime);
		rep_info->totalsize = pd.data_len;
		rep_info->blue_flag = 0;//pd.blue_flag;
		int8_t *p = r_strstr(hd_jpegname, "images/");

		if(NULL == p) {
			nslog(NS_ERROR, "hd_jpegname : [%s] not image/", hd_jpegname);
			jr->release_jpeg_buf_callback(&pd);
			continue;
		}

		r_strcpy(rep_info->filename, p);
		msgque msgp;
		r_memset(&msgp, 0, sizeof(msgque));
		msgp.msgtype = sindex + 1;
		msgp.msgbuf = (int8_t *)rep_info;
		//	nslog(NS_INFO, "jr->course_msg_fd[%d]:[%d]", jr->course_msg_fd, msgp.msgtype);
		ret = r_msg_send(jr->course_msg_fd, &msgp, sizeof(msgque) - sizeof(long), IPC_NOWAIT);

		if(ret < 0) {
			if(rep_info) {
				r_free(rep_info);
			}

			nslog(NS_ERROR, "r_msg_send[%d]  : %s", sindex + 1, strerror(errno));
			ms_delay(50);
		}

		jr->release_jpeg_buf_callback(&pd);
	}

	nslog(NS_INFO, "end sindex : [%d].", sindex);
	return jr;
}

int32_t jpeg_record_start(jpeg_record_t *jr)
{
	if(r_pthread_create(&(jr->p), NULL, jpeg_record_process, (void *)jr)) {
		nslog(NS_ERROR, "r_pthread_create failed !!!\n");
		return -1;
	}

	return 0;
}

jpeg_record_t *register_jpeg_record_mode(jpeg_record_condition_t *rcond)
{
	nslog(NS_INFO, "start ...");
	int32_t sindex = 0;
	if(NULL == rcond) {
		nslog(NS_ERROR, "register_jpeg_record_mode param is NULL");
		return NULL;
	}

	jpeg_record_t *jr = (jpeg_record_t *)r_malloc(sizeof(jpeg_record_t));

	if(NULL == jr) {
		nslog(NS_ERROR, "jpeg_record_t malloc is failed !!!");
		return NULL;
	}
	r_memset(jr, 0, sizeof(jpeg_record_t));
	jr->sindex = rcond->sindex;
	r_strcpy(jr->jpeg_dir, rcond->record_file_dir);
	jr->record_info = rcond->record_info;
	jr->course_data_fd = rcond->course_data_fd;
	jr->course_msg_fd = rcond->course_msg_fd;
	jr->get_jpeg_buf_info_callback = rcond->get_jpeg_buf_info_callback;
	jr->release_jpeg_buf_callback = rcond->release_jpeg_buf_callback;

	jr->record_start = jpeg_record_start;
	jr->record_resume = jpeg_record_resume;
	jr->record_pause = jpeg_record_pause;
	jr->record_close = jpeg_record_close;
	nslog(NS_INFO, "end.");
	return jr;
}

void unregister_jpeg_record_mode(jpeg_record_t **jr)
{
	nslog(NS_INFO, "start ...");
	jpeg_record_t *pjr = *jr;

	if(NULL == pjr) {
		return;
	}

	r_free(pjr);
	*jr = NULL;
	nslog(NS_INFO, "end ...");
}

int32_t mp4_course_record_start(mp4_course_record_t *cr)
{
	int32_t sindex = 0;

	for(sindex = 0; sindex < cr->mindex_sum; sindex ++) {
		if(NULL != cr->mr[sindex]) {
			if(cr->mr[sindex]->record_start(cr->mr[sindex]) < 0) {
				return -1;
			}
		}
	}

	return 0;
}

void mp4_course_record_pause(mp4_course_record_t *cr)
{
	int32_t sindex = 0;

	for(sindex = 0; sindex < cr->mindex_sum; sindex ++) {
		if(NULL != cr->mr[sindex]) {
			if(RECORD_CLOSE != cr->mr[sindex]->rs) {
				cr->mr[sindex]->record_pause(cr->mr[sindex]);
			}
		}
	}
}

void mp4_course_record_resume(mp4_course_record_t *cr)
{
	int32_t sindex = 0;

	for(sindex = 0; sindex < cr->mindex_sum; sindex ++) {
		if(NULL != cr->mr[sindex]) {
			cr->mr[sindex]->record_resume(cr->mr[sindex]);
		}
	}
}

void mp4_course_record_close(mp4_course_record_t *cr)
{
	int32_t sindex = 0;
	nslog(NS_INFO, "start ...");

	if(NULL == cr) {
		return;
	}

	for(sindex = 0; sindex < cr->mindex_sum; sindex ++) {
		if(NULL != cr->mr[sindex]) {
			cr->mr[sindex]->record_close(cr->mr[sindex]);
			r_pthread_join(cr->mr[sindex]->p, NULL);
		}
	}

	nslog(NS_INFO, "end .");
}

mp4_course_record_t *register_mp4_course_record_mode(mp4_course_record_condition_t *crc)
{
	nslog(NS_INFO, " start ...mindex_sum:[%d]", crc->mindex_sum);
	mp4_course_record_t *cr = (mp4_course_record_t *)r_malloc(sizeof(mp4_course_record_t));

	if(NULL == cr) {
		nslog(NS_ERROR, "cr is NULL");
		return NULL;
	}

	r_bzero(cr, sizeof(mp4_course_record_t));
	int32_t sindex = 0;
	int32_t hd_sindex = 1;
	int32_t sd_sindex = 1;
	mp4_record_condition_t rc;
	r_bzero(&rc, sizeof(mp4_record_condition_t));
	rc.course_data_fd = crc->course_data_fd;
	rc.course_msg_fd = crc->course_msg_fd;
	rc.get_media_buf_info_callback = crc->get_media_buf_info_callback;
	rc.release_media_buf_callback = crc->release_media_buf_callback;
	cr->mindex_sum = crc->mindex_sum;

	for(sindex = 0; sindex < crc->mindex_sum; sindex ++) {
		rc.sindex = sindex;
		r_memcpy(&(rc.record_info), &(crc->rinfo[sindex]), sizeof(record_info_t));

		if(RECORD_HD == crc->rinfo[sindex].stream_type)
		{
			r_sprintf(rc.record_file, "%s/HD/%s/%d_%d.mp4",
			          crc->course_root_dir, VIDEO_DIR,
			          hd_sindex, crc->video_split_index);
			hd_sindex ++;
		}
		else if(RECORD_SD == crc->rinfo[sindex].stream_type)
		{
			r_sprintf(rc.record_file, "%s/SD/%s/%d_%d.mp4",
			          crc->course_root_dir, VIDEO_DIR,
			          sd_sindex, crc->video_split_index);
			sd_sindex ++;
		}
		else
		{
			if(0 == sindex) {
				hd_sindex ++;
			} else if(1 == sindex) {
				sd_sindex ++;
			}
		}

		if((crc->rinfo[sindex].width > 0)
		   && (crc->rinfo[sindex].height > 0)) {
			cr->mr[sindex] = register_mp4_record_mode(&rc);

			if(NULL == cr->mr[sindex]) {
				nslog(NS_ERROR, "sindex:[%d] register_mp4_record_mode is failed!!", sindex);
				unregister_mp4_course_record_mode(&cr);
				return NULL;
			}
		} else {
			nslog(NS_WARN, "sindex:[%d] Not register_mp4_record_mode", sindex);
			cr->mr[sindex] = NULL;
		}
	}

	cr->record_start = mp4_course_record_start;
	cr->record_pause = mp4_course_record_pause;
	cr->record_resume = mp4_course_record_resume;
	cr->record_close = mp4_course_record_close;
	nslog(NS_INFO, "end .");
	return cr;
}

void unregister_mp4_course_record_mode(mp4_course_record_t **cr)
{
	nslog(NS_INFO, "start ...");
	mp4_course_record_t *mcr = *cr;

	if(NULL == mcr) {
		nslog(NS_ERROR, "NULL == mcr");
		return;
	}

	int32_t sindex = 0;
	nslog(NS_WARN, "mcr->mindex_sum:[%d]", mcr->mindex_sum);

	for(sindex = 0; sindex < mcr->mindex_sum; sindex ++) {
		nslog(NS_WARN, "mcr->mr[%d]:[%p]", sindex, mcr->mr[sindex]);

		if(NULL != mcr->mr[sindex]) {
			unregister_mp4_record_mode(&(mcr->mr[sindex]));
		}
	}

	r_free(mcr);
	*cr = NULL;
	nslog(NS_INFO, "end ...");
}

void course_record_resume(course_record_t *cr)
{
	if(NULL == cr) {
		return;
	}

	mp4_course_record_t *mcr = cr->mcr;

	if(NULL == mcr) {
		return;
	}

	jpeg_record_t *jr = cr->jr;
	mcr->record_resume(mcr);

	if(NULL != jr) {
		jr->record_resume(jr);
	}

	cr->rs = RECORD_RESUME;
}
void course_record_pause(course_record_t *cr)
{
	if(NULL == cr) {
		return;
	}

	mp4_course_record_t *mcr = cr->mcr;

	if(NULL == mcr) {
		return;
	}

	jpeg_record_t *jr = cr->jr;
	mcr->record_pause(mcr);

	if(NULL != jr) {
		jr->record_pause(jr);
	}

	cr->rs = RECORD_PAUSE;
}

static void create_preview_to_img(int8_t *course_root_dir, stream_type_sindex_sum_t *s)
{
	nslog(NS_INFO, "start...");
	int32_t sindex = 0;
	int32_t hd_sindex = 1;
	int32_t sd_sindex = 1;
	int8_t record_file[512] = {0};
	int8_t preview_file[512] = {0};

	int8_t preview_file_HD[512] = {0};
	int8_t cp_cmd[512] = {0};

	#if 0
	for(sindex = 0; sindex < s->hd_sindex_sum; sindex ++) {
		r_sprintf(record_file, "%s/HD/%s/%d_1.mp4",
		          course_root_dir, VIDEO_DIR, hd_sindex);
		r_sprintf(preview_file, "%s/HD/%s/start%d.jpg",
		          course_root_dir, IMG_DIR, hd_sindex);

		if(access(record_file, F_OK) == 0) {
			if(get_file_size(record_file) > 0) {
				MediaGenIndexImage(record_file, preview_file, 0, 0, 0);
				r_memset(record_file, 0, 512);
				r_memset(preview_file, 0, 512);
			}
		}

		hd_sindex ++;
	}

	for(sindex = 0; sindex < s->sd_sindex_sum; sindex ++) {
		r_sprintf(record_file, "%s/SD/%s/%d_1.mp4",
		          course_root_dir, VIDEO_DIR, sd_sindex);
		r_sprintf(preview_file, "%s/SD/%s/start%d.jpg",
		          course_root_dir, IMG_DIR, sd_sindex);

		if(access(record_file, F_OK) == 0) {
			if(get_file_size(record_file) > 0) {
				MediaGenIndexImage(record_file, preview_file, 0, 0, 0);
				r_memset(record_file, 0, 512);
				r_memset(preview_file, 0, 512);
			}
		}

		sd_sindex ++;
	}
	nslog(NS_INFO, "end...");
#endif

	// add junbao
	for(sindex = 0; sindex < s->sd_sindex_sum; sindex ++)
	{
		r_sprintf(record_file, "%s/SD/%s/%d_1.mp4",
				  course_root_dir, VIDEO_DIR, sd_sindex);
		r_sprintf(preview_file, "%s/SD/%s/start%d.jpg",
				  course_root_dir, IMG_DIR, sd_sindex);

		if(access(record_file, F_OK) == 0) {
			if(get_file_size(record_file) > 0) {
				nslog(NS_INFO,"Debug_create_image start!\n");
				MediaGenIndexImage(record_file, preview_file, 0, 0, 0);
				nslog(NS_INFO,"Debug_create_image over!\n");
				r_sprintf(preview_file_HD, "%s/HD/%s/start%d.jpg",
						  course_root_dir, IMG_DIR, sd_sindex);
				r_sprintf(cp_cmd, "cp %s %s", preview_file, preview_file_HD);

				nslog(NS_INFO, "create_preview_to_img:[%s]", cp_cmd);
#if 1
				if (CopyFile(preview_file, preview_file_HD) != 0)
					nslog(NS_ERROR, "cp_cmd error");
				else
					nslog(NS_INFO, "cp_cmd sucess");
#else
				if (r_system((int8_t *)cp_cmd) != 0)
					nslog(NS_ERROR, "cp_cmd error");
				else
					nslog(NS_INFO, "cp_cmd sucess");
#endif
				r_memset(record_file, 0, 512);
				r_memset(preview_file, 0, 512);
			}
		}

		sd_sindex ++;
	}

	nslog(NS_INFO, "end...");
}

static void create_jpeg_preview_to_img(int8_t *course_root_dir, stream_type_sindex_sum_t *s)
{
	nslog(NS_INFO, "start ...");
	int8_t record_file[512] = {0};
	int8_t preview_file[512] = {0};

	if(s->jpeg_sindex_sum > 0) {
		r_sprintf(record_file, "%s/HD/%s/1.jpg",
		          course_root_dir, IMG_DIR);
		r_sprintf(preview_file, "%s/HD/%s/start%d.jpg",
		          course_root_dir, IMG_DIR, s->hd_sindex_sum + 1);
		filecpy(preview_file, record_file);

		r_sprintf(record_file, "%s/SD/%s/1.jpg",
		          course_root_dir, IMG_DIR);
		r_sprintf(preview_file, "%s/SD/%s/start%d.jpg",
		          course_root_dir, IMG_DIR, s->sd_sindex_sum + 1);
		filecpy(preview_file, record_file);
	}

	nslog(NS_INFO, "end ...");
}

void course_record_stop(course_record_t *cr)
{
	nslog(NS_INFO, "start ...");

	if(NULL == cr) {
		return;
	}

	mp4_course_record_t *mcr = cr->mcr;

	if(NULL == mcr) {
		return;
	}

	mcr->record_close(mcr);

	if(NULL != mcr) {
		unregister_mp4_course_record_mode(&mcr);
	}

	jpeg_record_t *jr = cr->jr;

	if(NULL != jr) {
		jr->record_close(jr);
		r_pthread_join(jr->p, NULL);
		unregister_jpeg_record_mode(&jr);
	}

	nslog(NS_INFO, "end ...");
}

static void md5sumfile(int8_t *course_root_dir)
{
	nslog(NS_INFO, "start ...");

	int8_t md5sum_cmd[1024 * 1024] = {0};
	r_sprintf(md5sum_cmd, "find %s -type f | grep -v mp4 | xargs -i md5sum {} > md5sum.info", course_root_dir);
	r_system(md5sum_cmd);
	nslog(NS_INFO, "end ...");
}

void course_record_close(course_record_t *cr)
{
	nslog(NS_INFO, "start ...");

	if(NULL == cr) {
		return;
	}

	int32_t msg_num = 0;

	do {
		msg_num = get_msg_num(cr->course_data_msgid);
		nslog(NS_INFO, "msg_num:[%d]", msg_num);
		ms_delay(100);
	} while(msg_num);

	cr->print_close = 1;
	r_pthread_join(cr->print_p, NULL);
	course_record_stop(cr);
	cr->rs = RECORD_CLOSE;
	r_pthread_join(cr->p, NULL);

	create_course_record_xml(cr->course_root_dir, cr->course_record_totaltime, &(cr->s));

	nslog(NS_ERROR,"FUCK_GOD_1019  MODE : %d\n",cr->mode_type);
	if(1 == cr->crc.video_split_index && cr->mode_type == RESOURCES_USB_DISK_MODE) {
		create_preview_to_img(cr->course_root_dir, &(cr->s));
	}

	nslog(NS_INFO, "DirDetectionFile Before...");
	//DirDetectionFile(cr->course_root_dir);
	nslog(NS_INFO, "DirDetectionFile after...");
	nslog(NS_INFO, "end ...");
}

static int32_t open_record_info_xml_tmp(course_record_t *cr, FILE **info_xml_x_fp, FILE **blue_x_fp)
{
	nslog(NS_INFO, "start ...");
	int8_t info_xml_x[1024] = {0};
	int8_t blue_x[1024] = {0};
	int32_t sindex = 0;
	int32_t hd_sindex = 1;
	int32_t sd_sindex = 1;

	for(sindex = 0; sindex < cr->sindex_sum; sindex ++) {

		if(RECORD_HD == cr->crc.rinfo[sindex].stream_type) {
			r_sprintf(info_xml_x, "%s/HD/%s.%d", cr->course_root_dir, INFO_XML, hd_sindex);
			r_sprintf(blue_x, "%s/HD/%s.%d", cr->course_root_dir, BLUE_X, hd_sindex);
			hd_sindex ++;
		} else if(RECORD_SD == cr->crc.rinfo[sindex].stream_type) {
			r_sprintf(info_xml_x, "%s/SD/%s.%d", cr->course_root_dir, INFO_XML, sd_sindex);
			r_sprintf(blue_x, "%s/SD/%s.%d", cr->course_root_dir, BLUE_X, sd_sindex);
			sd_sindex ++;
		} else if(RECORD_JPEG == cr->crc.rinfo[sindex].stream_type) {
			r_sprintf(info_xml_x, "%s/SD/%s.%d", cr->course_root_dir, INFO_XML, sd_sindex);
			r_sprintf(blue_x, "%s/SD/%s.%d", cr->course_root_dir, BLUE_X, sd_sindex);
		} else {
			nslog(NS_WARN, "stream_type[%d]:[%d]", sindex, cr->crc.rinfo[sindex].stream_type);

			if(0 == sindex) {
				r_sprintf(info_xml_x, "%s/HD/%s.%d", cr->course_root_dir, INFO_XML, hd_sindex);
				r_sprintf(blue_x, "%s/HD/%s.%d", cr->course_root_dir, BLUE_X, hd_sindex);
				hd_sindex ++;
			} else if(1 == sindex) {
				r_sprintf(info_xml_x, "%s/SD/%s.%d", cr->course_root_dir, INFO_XML, sd_sindex);
				r_sprintf(blue_x, "%s/SD/%s.%d", cr->course_root_dir, BLUE_X, sd_sindex);
				sd_sindex ++;
			}
		}

		if((info_xml_x_fp[sindex] = fopen(info_xml_x, "w")) == NULL) {
			nslog(NS_ERROR, "fopen[%d] [%s] : %s", sindex, info_xml_x, strerror(errno));
			return -1;
		}

		nslog(NS_INFO, "info fopen[%d]:[%s]", sindex, info_xml_x);

		if((blue_x_fp[sindex] = fopen(blue_x, "w")) == NULL) {
			nslog(NS_ERROR, "fopen[%d] [%s] : %s", sindex, blue_x, strerror(errno));
			return -1;
		}

		nslog(NS_INFO, "blue fopen[%d]:[%s]", sindex, blue_x);
	}

	nslog(NS_INFO, "end ...");
	return 0;
}

static int32_t close_record_info_xml_tmp(FILE **info_xml_x_fp, FILE **blue_x_fp, int32_t sindex_sum)
{
	nslog(NS_INFO, "start ...");
	int32_t sindex = 0;

	for(sindex = 0; sindex < sindex_sum; sindex ++) {
		if(fclose(info_xml_x_fp[sindex]) < 0) {
			nslog(NS_ERROR, "info_xml_x_fp fclose[%d]  : %s", sindex, strerror(errno));
			return -1;
		}

		nslog(NS_INFO, "info fclose[%d]", sindex);

		if(fclose(blue_x_fp[sindex]) < 0) {
			nslog(NS_ERROR, "blue_x_fp fclose[%d]  : %s", sindex, strerror(errno));
			return -1;
		}

		nslog(NS_INFO, "blue fclose[%d]", sindex);
	}

	nslog(NS_INFO, "end ...");
	return 0;
}

static void check_mp4_split_totaltime(record_report_info_t *rep_info, int32_t mindex_sum)
{
	int32_t i = 0;
	int32_t min = rep_info[0].totaltime;

	for(i = 1; i < mindex_sum; i ++) {
		if(min >= rep_info[i].totaltime && 0 != rep_info[i].totaltime) {
			min = rep_info[i].totaltime;
		}
	}

	for(i = 0; i < mindex_sum; i ++) {
		if (0 != rep_info[i].totaltime)
			rep_info[i].totaltime = min;
	}
}

void *course_record_process(void *arg)
{
	nslog(NS_INFO, "start ...");
	course_record_t *cr = (course_record_t *)arg;
	mp4_course_record_t *mcr = cr->mcr;
	jpeg_record_t *jr = cr->jr;
	int32_t course_msg_fd = cr->course_msg_fd;
	int32_t sindex = 0;
	int32_t rindex = 0;
	int32_t mindex = 0;
	int32_t mindex_sum = mcr->mindex_sum;
	int32_t ret = 0;
	record_report_info_t *rep_info = NULL;
	r_memset(cr->rep_info_tmp, 0, sizeof(record_report_info_t) * RECODE_MAX_STREAM);
	msgque msgp;
	r_memset(&msgp, 0, sizeof(msgque));
	FILE *info_xml_x_fp[RECODE_MAX_STREAM] = {0};

	FILE *blue_x_fp[RECODE_MAX_STREAM] = {0};
	int32_t blue_start_flag[RECODE_MAX_STREAM];

	for(mindex = 0; mindex < RECODE_MAX_STREAM; mindex ++) {
		blue_start_flag[mindex] = 1;
	}

	int32_t blue_end_flag[RECODE_MAX_STREAM] = {0};
	int32_t blue_start[RECODE_MAX_STREAM] = {0};
	int32_t blue_end[RECODE_MAX_STREAM] = {0};
	fpos_t pos;
	int32_t course_totaltime[RECODE_MAX_STREAM] = {0};
	uint32_t starttime = 0;
	uint32_t curtime = 0;
	uint32_t jpeg_starttime = 0;
	uint32_t jpeg_curtime = 0;
	uint32_t jpeg_endtime = 0;
	int32_t split_flag = 1;
	int32_t split_sindex = -1;
	int32_t overtime = 0;
	int32_t jpeg_preview = 1;

	if(open_record_info_xml_tmp(cr, info_xml_x_fp, blue_x_fp) < 0) {
		nslog(NS_ERROR, "open_record_info_xml_tmp is failed!");
		return NULL;
	}

	if(NULL != jr) {
		if(jr->record_start(jr) < 0) {
			unregister_jpeg_record_mode(&jr);
			nslog(NS_ERROR, "jr->record_start is failed!");
			return NULL;
		}
	}

	if(NULL != mcr) {
		if(mcr->record_start(mcr) < 0) {
			unregister_mp4_course_record_mode(&mcr);
			nslog(NS_ERROR, "mcr->record_start is failed!");
			return NULL;
		}
	}

	starttime = get_run_time() / 1000;

	while(1) {
		curtime = get_run_time() / 1000;

		if(cr->max_course_record_duration > 0) {
			if(((curtime - starttime) >= cr->max_course_record_duration) && (!overtime)) {
				if(RECORD_CLOSE != get_course_record_status(cr)) {
					cr->record_pause(cr);
				}

				int8_t *reqbuf = (int8_t *)r_malloc(512);
				r_memset(reqbuf, 0, 512);
				set_record_req_msg(reqbuf, 0, cr->course_root_dir);
				pack_header_msg_xml(reqbuf, r_strlen(reqbuf + MSG_HEAD_LEN), EDU_MSG_VER);
				msgque msgp;
				r_memset(&msgp, 0, sizeof(msgque));

				// zl
				if(cr->mode_type == RESOURCES_USB_DISK_MODE)
				{
					msgp.msgtype = REQ_MSG_BASE_TYPE_USB;
				}
				else
				{
					msgp.msgtype = REQ_MSG_BASE_TYPE;
				}
				msgp.msgbuf = (int8_t *)reqbuf;
				ret = r_msg_send(cr->room_ctrl_msg_fd, &msgp, sizeof(msgque) - sizeof(long), IPC_NOWAIT);

				if(ret < 0) {
					if(reqbuf) {
						r_free(reqbuf);
					}

					overtime = 0;
				} else {
					overtime = 1;
				}

				nslog(NS_INFO, "roomfd:[%d] reqbuf:[%s] max_course_record_duration : [%d]\n",
				      cr->room_ctrl_msg_fd, (reqbuf + MSG_HEAD_LEN), cr->max_course_record_duration);
			}
		}


		ret = r_msg_recv(course_msg_fd, &msgp, sizeof(msgque) - sizeof(long), 0, IPC_NOWAIT);

		if(ret < 0) {
			if(RECORD_PAUSE == get_course_record_status(cr)) {
				ms_delay(30);
				continue;
			}

			if(RECORD_CLOSE == get_course_record_status(cr)) {
				nslog(NS_INFO, "RECORD_CLOSE");
				cr->course_record_totaltime = course_totaltime[rindex] + cr->rep_info_tmp[rindex].totaltime;

				if(0 >= cr->course_record_totaltime || !mindex_sum) {
					jpeg_endtime = get_run_time() / 1000;
					if(jpeg_curtime > 0) {
						cr->course_record_totaltime = jpeg_starttime + (jpeg_endtime - jpeg_curtime);
					}
				}

				for(mindex = 0; mindex < mindex_sum; mindex ++) {
					check_mp4_split_totaltime(cr->rep_info_tmp, mindex_sum);
					record_info_file_node(info_xml_x_fp[mindex], &(cr->rep_info_tmp[mindex]));

					if(blue_end_flag[mindex] && !blue_start_flag[mindex]) {
						blue_end[mindex] = course_totaltime[mindex] + cr->rep_info_tmp[mindex].totaltime;
						record_blue_node(blue_x_fp[mindex], blue_start[mindex], blue_end[mindex]);
						nslog(NS_INFO, "bule[%d] start:[%ds] end:[%ds]",
						      mindex, blue_start[mindex], blue_end[mindex]);
					}
				}

				break;
			}

			ms_delay(30);
			continue;
		}

		rep_info = (record_report_info_t *)(msgp.msgbuf);

		#if 0


		nslog(NS_ERROR,"FUCK_GOD_1014  , ---------------<H264><%2d:%d> <%4d*%4d> <%s> <%8d> <%5d> ",
					      rep_info->sindex,
					    //  (cr->mcr->mr[sindex]->rs),
					      rep_info->blue_flag,
					      rep_info->height,
					      rep_info->width,
					      rep_info->filename,
					      rep_info->totalsize / 1024,
					      rep_info->totaltime);
		#endif
		r_memcpy(&(cr->rep_info_tmp[rep_info->sindex]), rep_info, sizeof(record_report_info_t));

		if((cr->sindex_sum == rep_info->sindex + 1) && (NULL != jr)) {
			//	nslog(NS_INFO, "[%d]rep_info[%d]:<%p>",  cr->sindex_sum, rep_info->sindex, rep_info);
			record_info_file_node_jpeg(info_xml_x_fp[rep_info->sindex], rep_info);
			jpeg_starttime = rep_info->totaltime;
			jpeg_curtime = get_run_time() / 1000;

			if(jpeg_preview && rep_info->totalsize) {
				create_jpeg_preview_to_img(cr->course_root_dir, &(cr->s));
				jpeg_preview = 0;
			}
		} else {

			if(rep_info->blue_flag) {
				if(blue_start_flag[rep_info->sindex]) {
					blue_start[rep_info->sindex] = course_totaltime[rep_info->sindex] + cr->rep_info_tmp[rep_info->sindex].totaltime;
					blue_end[rep_info->sindex] = 0;
					fgetpos(blue_x_fp[rep_info->sindex], &pos);
					record_blue_node(blue_x_fp[rep_info->sindex], blue_start[rep_info->sindex], blue_end[rep_info->sindex]);
					fsetpos(blue_x_fp[rep_info->sindex], &pos);
					blue_start_flag[rep_info->sindex] = 0;
					blue_end_flag[rep_info->sindex] = 1;
					nslog(NS_INFO, "bule[%d] start:[%ds]", rep_info->sindex, blue_start[rep_info->sindex]);
				}
			} else {
				if(blue_end_flag[rep_info->sindex]) {
					blue_end[rep_info->sindex] = course_totaltime[rep_info->sindex] + cr->rep_info_tmp[rep_info->sindex].totaltime;
					record_blue_node(blue_x_fp[rep_info->sindex], blue_start[rep_info->sindex], blue_end[rep_info->sindex]);
					blue_start_flag[rep_info->sindex] = 1;
					blue_end_flag[rep_info->sindex] = 0;
					nslog(NS_INFO, "bule[%d] start:[%ds] end:[%ds]", rep_info->sindex, blue_start[rep_info->sindex], blue_end[rep_info->sindex]);
				}
			}

			rindex = rep_info->sindex;

			if((get_mp4_file_limits() > rep_info->totalsize) && (rep_info->sindex == split_sindex)) {
				split_flag = 1;
			}

			if((get_mp4_file_limits() <= rep_info->totalsize) && split_flag) {
				split_sindex = rep_info->sindex;

				for(mindex = 0; mindex < mindex_sum; mindex ++) {
					check_mp4_split_totaltime(cr->rep_info_tmp, mindex_sum);
					record_info_file_node(info_xml_x_fp[mindex], &(cr->rep_info_tmp[mindex]));
					course_totaltime[mindex] += cr->rep_info_tmp[mindex].totaltime;
				}

				r_memset(cr->rep_info_tmp, 0, sizeof(record_report_info_t) * RECODE_MAX_STREAM);

				pthread_mutex_lock(&(cr->print_mutex));
				mcr->record_close(mcr);
				unregister_mp4_course_record_mode(&mcr);

				if(1 == cr->crc.video_split_index) {
					create_preview_to_img(cr->course_root_dir, &(cr->s));
				}

				cr->crc.video_split_index ++;
				mcr = register_mp4_course_record_mode(&(cr->crc));

				if(NULL == mcr) {
					nslog(NS_ERROR, "register video_split_index : [%d] mp4 course record is failed", cr->crc.video_split_index);
					r_free(rep_info);
					pthread_mutex_unlock(&(cr->print_mutex));
					break;
				}

				cr->mcr = mcr;
				pthread_mutex_unlock(&(cr->print_mutex));

				if(mcr->record_start(mcr) < 0) {
					unregister_mp4_course_record_mode(&mcr);
					r_free(rep_info);
					break;
				}

				split_flag = 0;
			}
		}

		r_free(rep_info);
	}

	if(close_record_info_xml_tmp(info_xml_x_fp, blue_x_fp, cr->sindex_sum) < 0) {
		nslog(NS_ERROR, "close_record_info_xml_tmp is failed!");
		return NULL;
	}

	nslog(NS_INFO, "end ...");
	return cr;
}

void *print_course_record_process(void *arg)
{
	nslog(NS_INFO, "start ...");
	course_record_t *cr = (course_record_t *)arg;

	if(NULL == cr) {
		nslog(NS_ERROR, " cr is NULL!");
		return NULL;
	}

	int32_t sindex = 0;
	int32_t delay_count = 0;
	int32_t delay = 0;

	while(1) {
		if(cr->print_close) {
			goto EXIT;
		}

		nslog(NS_INFO, "<Cd:%s> <plit:%d> <St:%d> <Did:%d> <Mid: %d>",
		      cr->course_root_dir,
		      cr->crc.video_split_index,
		      cr->get_record_status(cr),
		      cr->course_data_msgid,
		      cr->course_msg_fd);
		nslog(NS_INFO, "<Si:St:B> <High*Widt> <Filename      > <Totalsize> <Totaltime>");
		pthread_mutex_lock(&(cr->print_mutex));

		for(sindex = 0; sindex < RECODE_MAX_STREAM; sindex ++) {
			if(NULL != cr->mcr) {
				if(NULL != cr->mcr->mr[sindex]
				   && ((RECORD_SD == cr->crc.rinfo[sindex].stream_type)
				       || (RECORD_HD == cr->crc.rinfo[sindex].stream_type))) {
					nslog(NS_INFO, "<H264><%2d:%2d:%d> <%4d*%4d> <%s> <%8d> <%5d> ",
					      sindex,
					      (cr->mcr->mr[sindex]->rs),
					      cr->rep_info_tmp[sindex].blue_flag,
					      cr->rep_info_tmp[sindex].height,
					      cr->rep_info_tmp[sindex].width,
					      cr->rep_info_tmp[sindex].filename,
					      cr->rep_info_tmp[sindex].totalsize / 1024,
					      cr->rep_info_tmp[sindex].totaltime);
				} else if(NULL != cr->jr && RECORD_JPEG == cr->crc.rinfo[sindex].stream_type) {
					nslog(NS_INFO, "<JEPG><%2d:%2d:%d> <%4d*%4d> <%s> <%8d> <%5d> ",
					      sindex,
					      (cr->jr->rs),
					      cr->rep_info_tmp[sindex].blue_flag,
					      cr->rep_info_tmp[sindex].height,
					      cr->rep_info_tmp[sindex].width,
					      cr->rep_info_tmp[sindex].filename,
					      cr->rep_info_tmp[sindex].totalsize / 1024,
					      cr->rep_info_tmp[sindex].totaltime);
				}
			}
		}

		pthread_mutex_unlock(&(cr->print_mutex));

		delay_count = 20;

		while(delay_count --) {
			if(cr->print_close) {
				goto EXIT;
			}

			if(delay) {
				ms_delay(500);
			}

			delay = 1;
		}

	}

EXIT:
	nslog(NS_INFO, "end ...");
}

int32_t course_record_start(course_record_t *cr)
{
	if(r_pthread_create(&(cr->p), NULL, course_record_process, (void *)cr)) {
		nslog(NS_ERROR, "r_pthread_create failed !!!\n");
		return -1;
	}

	if(r_pthread_create(&(cr->print_p), NULL, print_course_record_process, (void *)cr)) {
		nslog(NS_ERROR, "r_pthread_create failed !!!\n");
		return -1;
	}

	return 0;
}


/*
struct tm {
	int tm_sec;	//seconds
	int tm_min;	//minutes
	int tm_hour;	//hours
	int tm_mday;	//day of the month
	int tm_mon;	//month
	int tm_year;	//year
	int tm_wday;	//day of the week
	int tm_yday;	//day in the year
	int tm_isdst;	//daylight saving time
}
*/

static void make_course_dir_name(int8_t *course_dir_name, int32_t roomid, int8_t *dev_id,int8_t *path_suffix,int8_t *record_time)
{
//	localtime_t t;
//	get_localtime(&t);
	r_sprintf(course_dir_name, "%sr%d%s%s",record_time,roomid, dev_id,path_suffix);
}

int8_t *get_course_root_dir(course_record_t *cr)
{
	return cr->course_root_dir;
}

static int32_t create_course_recovery(int8_t *recovery_course, int8_t *record_root_dir, int8_t *course_dir_name)
{
	int8_t recovery_root_dir[256] = {0};
	r_snprintf(recovery_root_dir, 255, "%s/recovery", record_root_dir);
	mkdir(recovery_root_dir, 0777);
	r_snprintf(recovery_course, 255, "%s/%s", recovery_root_dir, course_dir_name);
	return data2file(recovery_course, NULL, 0);
}

static int32_t create_course_template(int8_t *course_root_dir, int8_t *course_dir_name, int8_t *dev_id, int8_t *record_root_dir, int32_t roomid,int8_t *path_suffix,int8_t *record_time)
{
	nslog(NS_INFO, "start ...");
	int8_t record_file_cmd[512] = {0};
	if(access(COURSE_TEMPLATE_DIR_NEW, F_OK) != 0)
	{
		nslog(NS_INFO,"THERE IS NO A RECORD_TEMP_FILE!\n");
		r_snprintf(record_file_cmd,511,"cp -rf %s %s/",COURSE_TEMPLATE_DIR,RECORD_DEFAULT_ROOT_DIR);
		nslog(NS_INFO,"RECORD_CMD_1 : %s\n",record_file_cmd);
		if(r_system(record_file_cmd) != 0)
		{
			nslog(NS_ERROR, "course_init_cmd : [%s] is failed : [%s]!!!", record_file_cmd, strerror(errno));
			return -1;
		}
	}
	else
	{
		nslog(NS_INFO,"THERE IS A RECORD_TEMP_FILE!\n");
	}
	nslog(NS_INFO,"CREATE RECORD FILE BGING!\n");
	make_course_dir_name(course_dir_name, roomid, dev_id,path_suffix,record_time);
	r_sprintf(course_root_dir, "%s/%s",
	          record_root_dir, course_dir_name);
	mkdir(course_root_dir, 0777);
	nslog(NS_INFO, "course_root_dir:[%s]\n", course_root_dir);
	int8_t course_init_cmd[1024] = {0};
	r_snprintf(course_init_cmd, 1023, "cp -rf %s/* %s",
	           COURSE_TEMPLATE_DIR_NEW, course_root_dir);


	nslog(NS_INFO,"RECORD_CMD_2 : %s\n",course_init_cmd);
	if(r_system(course_init_cmd) != 0) {
		nslog(NS_ERROR, "course_init_cmd : [%s] is failed : [%s]!!!", course_init_cmd, strerror(errno));
		return -1;
	}
	nslog(NS_INFO,"CREATE RECORD FILE OVER!\n");
	nslog(NS_INFO, "course_init_cmd:[%s]", course_init_cmd);
	return 0;
}

static void check_record_root_dir(int8_t *record_root_dir)
{
	int32_t ret = -1;

	int8_t mk_root_cmd[128] = {0};

	if(!r_strlen(record_root_dir)) {
		r_snprintf(mk_root_cmd, 127, "mkdir -p %s", RECORD_DEFAULT_ROOT_DIR);
		r_system(mk_root_cmd);
		r_strcpy(record_root_dir, RECORD_DEFAULT_ROOT_DIR);
		nslog(NS_WARN, "%s", mk_root_cmd);
	}
}

static int8_t *get_RecordID(course_record_t *cr)
{
	return cr->RecordID;
}
// add zl

#if 1
static int32_t get_record_time(course_record_t *cr)
{
	return (cr->rep_info_tmp[0].totaltime);
}

static int8_t *get_record_start_time(course_record_t *cr)
{
	return cr->RecDateTime;
}

#endif
static void strRevcpy(int8_t *dst, char *src)
{
	int32_t i = 0;
	int32_t j = r_strlen(src) - 1;
	for(i = 0; i < r_strlen(src); i ++, j --) {
		dst[i] = src[j];
	}
}

static void check_ContentInfo(ContentInfo_t *ci, int8_t *course_dir_name)
{
	if(!r_strlen(ci->CName)) {
		r_strcpy(ci->CName, course_dir_name);
	}

	if(!r_strlen(ci->RecordID)) {
		strRevcpy(ci->RecordID, course_dir_name);
	}
}

static int32_t  rm_dir_file(int8_t *dir_file)
{
	int8_t rmcmd[512] = {0};
	r_snprintf(rmcmd, 511, "rm -rf %s", dir_file);
	if(r_system(rmcmd) != 0) {
		nslog(NS_ERROR, "rmcmd is failed : [%s]", rmcmd);
		return -1;
	}
	return 0;
}

course_record_t *register_course_record_mode(course_record_condition_t *cond)
{
	nslog(NS_INFO, "start ...");
	int32_t cret = -1;
	int32_t rret = -1;
	int8_t course_dir_name[128] = {0};
	int8_t course_root_dir[256] = {0};

	course_record_t *cr = NULL;
	jpeg_record_t *jr = NULL;

	int32_t course_data_msgid = -1;
	int32_t course_msg_msgid  = -1;

	mp4_course_record_t *mcr = NULL;
	mp4_course_record_condition_t crc;
	int32_t msgid_msg_key = 0;
	int32_t msgid_data_key= 0;
	int8_t mk_root_cmd[128] = {0};

	#if 1  //zl
	if(cond->resources_mode.Resources_Type == RESOURCES_USB_DISK_MODE)
	{
		check_record_root_dir(cond->resources_mode.Course_Name);
		cret = create_course_template(course_root_dir, course_dir_name,
									  cond->dev_id, cond->resources_mode.Course_Name, cond->roomid,RECORD_PATH_SUFFIX_MOVIE,cond->RecDatefileTime);
	}
	else
	{
		check_record_root_dir(cond->resources_mode.Course_Name);
		cret = create_course_template(course_root_dir, course_dir_name,
									  cond->dev_id, cond->resources_mode.Course_Name, cond->roomid,RECORD_PATH_SIFFIX_RES,cond->RecDatefileTime);
	}

	#else
	check_record_root_dir(cond->record_root_dir);
	cret = create_course_template(course_root_dir, course_dir_name,
	                              cond->dev_id, cond->record_root_dir, cond->roomid);
	#endif

	if(cret < 0) {
		nslog(NS_ERROR, "create_course_template is failed!");
		goto EXIT;
	}

	check_ContentInfo(&(cond->cinfo), course_dir_name);

	if(create_ContentInfo_xml(course_root_dir, &(cond->cinfo)) < 0) {
		nslog(NS_ERROR, "create_ContentInfo_xml is failed!");
		goto EXIT;
	}

	cr = (course_record_t *)r_malloc(sizeof(course_record_t));

	if(NULL == cr) {
		nslog(NS_ERROR, "course_record_t r_malloc is failed");
		goto EXIT;
	}

	r_memset(cr, 0, sizeof(course_record_t));

//	if(cond->resources_mode.Resources_Type != RESOURCES_USB_DISK_MODE){
		rret = create_course_recovery(cr->recovery_course, cond->resources_mode.Course_Name, course_dir_name);
		if(rret < 0) {
			nslog(NS_ERROR, "create_course_recovery is failed!");
			goto EXIT;
		}
//	}

	#if 1     //zl
	if(cond->resources_mode.Resources_Type == RESOURCES_USB_DISK_MODE)
	{
		msgid_msg_key = RECORD_MSG_BASE_KEY_USB;
		msgid_data_key = RECORD_DATA_BASE_KEY_USB;
	}
	else
	{
		msgid_msg_key = RECORD_MSG_BASE_KEY;
		msgid_data_key = RECORD_DATA_BASE_KEY;
	}

	course_data_msgid = r_msg_create_u(msgid_data_key + cond->roomid);
	if(course_data_msgid < 0) {
		nslog(NS_ERROR, "r_msg_create course_data_msgid is failed!");
		goto EXIT;
	}

	course_msg_msgid = r_msg_create_u(msgid_msg_key + cond->roomid);
	if(course_msg_msgid < 0) {
		nslog(NS_ERROR, "r_msg_create  course_msg_msgid is failed!");
		goto EXIT;
	}

	#else

	int32_t course_data_msgid = r_msg_create_u(RECORD_DATA_BASE_KEY + cond->roomid);

	if(course_data_msgid < 0) {
		nslog(NS_ERROR, "r_msg_create course_data_msgid is failed!");
		goto EXIT;
	}

	int32_t course_msg_msgid = r_msg_create_u(RECORD_MSG_BASE_KEY + cond->roomid);

	if(course_msg_msgid < 0) {
		nslog(NS_ERROR, "r_msg_create  course_msg_msgid is failed!");
		goto EXIT;
	}

	#endif



	if(NULL != cond->get_jpeg_buf_info_callback
	   && NULL != cond->release_jpeg_buf_callback
	   && RECORD_JPEG == cond->rinfo[cond->sindex_sum - 1].stream_type) {
	   	nslog(NS_ERROR,"FUCK_GOD_1114 ----- RECORD JPEG VAILD!\n");
		jpeg_record_condition_t rc;
		rc.sindex = cond->sindex_sum - 1;
		rc.course_data_fd = course_data_msgid;
		rc.course_msg_fd = course_msg_msgid;
		r_memcpy(&(rc.record_info), &(cond->rinfo[cond->sindex_sum - 1]), sizeof(record_info_t));
		r_sprintf(rc.record_file_dir, "%s/HD/%s", course_root_dir, IMG_DIR);
		rc.get_jpeg_buf_info_callback = cond->get_jpeg_buf_info_callback;
		rc.release_jpeg_buf_callback = cond->release_jpeg_buf_callback;
		jr = register_jpeg_record_mode(&rc);

		if(NULL == jr) {
			nslog(NS_ERROR, "register_jpeg_record_mode failed");
			goto EXIT;
		}

		cr->jr = jr;
	} else {
		cr->jr = NULL;
		nslog(NS_WARN, "Not Jpeg ...");
	}

//	mp4_course_record_condition_t crc;
	r_memcpy(crc.course_root_dir, course_root_dir, 256 - 1);
	crc.video_split_index = 1;
	crc.course_msg_fd = course_msg_msgid;
	crc.course_data_fd = course_data_msgid;
	crc.roomid = cond->roomid;

	if(NULL == cr->jr) {
		crc.mindex_sum = cond->sindex_sum;
	} else {
		crc.mindex_sum = cond->sindex_sum - 1;
	}

	r_memcpy(crc.rinfo, cond->rinfo, sizeof(record_info_t) * RECODE_MAX_STREAM);
	crc.get_media_buf_info_callback = cond->get_media_buf_info_callback;
	crc.release_media_buf_callback = cond->release_media_buf_callback;
	mcr = register_mp4_course_record_mode(&crc);

	if(NULL == mcr) {
		nslog(NS_ERROR, "register_mp4_course_record_mode failed");
		goto EXIT;
	}

	r_memcpy(&(cr->crc), &crc, sizeof(mp4_course_record_condition_t));
	r_memcpy(cr->course_root_dir, course_root_dir, 256 - 1);
	cr->max_course_record_duration = cond->max_course_record_duration;

	if(cr->max_course_record_duration < 0) {
		cr->max_course_record_duration = DEFAULT_MAX_COURSE_DUR;
	}

	int32_t sindex = 0;
	r_memset(&(cr->s), 0, sizeof(stream_type_sindex_sum_t));

	nslog(NS_ERROR,"FUCK_GOD_1014,stream_num  :%d\n",cond->sindex_sum);

	for(sindex = 0; sindex < cond->sindex_sum; sindex ++) {
		if(RECORD_HD == cond->rinfo[sindex].stream_type) {
			cr->s.hd_sindex_sum ++;
		} else if(RECORD_SD == cond->rinfo[sindex].stream_type) {
			cr->s.sd_sindex_sum ++;
		} else if(RECORD_JPEG == cond->rinfo[sindex].stream_type) {
			cr->s.jpeg_sindex_sum ++;
		} else {
			nslog(NS_WARN, "stream_type[%d]:[%d]", sindex, cond->rinfo[sindex].stream_type);

			if(0 == sindex) {
				cr->s.hd_sindex_sum ++;
			} else if(1 == sindex) {
				cr->s.sd_sindex_sum ++;
			}
		}
	}

	pthread_mutex_init(&(cr->print_mutex), NULL);
	//cr->max_course_record_duration = 1800;
	cr->course_record_totaltime = 0;
	cr->course_data_msgid = course_data_msgid;
	cr->course_msg_fd = course_msg_msgid;
	cr->sindex_sum = cond->sindex_sum;
	r_memcpy(cr->RecordID, cond->cinfo.RecordID, 128);
	cr->get_RecordID = get_RecordID;
	r_memcpy(cr->RecDateTime, cond->cinfo.RecDateTime, 32);  // add zl
	cr->get_record_time = get_record_time;						// add zl
	cr->get_record_start_time = get_record_start_time;						//add zl
	cr->mcr = mcr;
	cr->record_start = course_record_start;
	cr->record_resume = course_record_resume;
	cr->record_pause = course_record_pause;
	cr->record_close = course_record_close;
	cr->get_record_status = get_course_record_status;
	cr->get_course_root_dir = get_course_root_dir;
	cr->room_ctrl_msg_fd = cond->room_ctrl_msg_fd;
	cr->print_close = 0;
	cr->mode_type = cond->resources_mode.Resources_Type;
	nslog(NS_INFO, "end.");
	return cr;
EXIT:
	nslog(NS_ERROR, "register_course_record_mode is failed!");

	if(0 == cret) {
		nslog(NS_INFO, "rm course_root_dir");
		rm_dir_file(course_root_dir);
	}

	if(0 == rret) {
		nslog(NS_INFO, "rm recovery_course");
		remove(cr->recovery_course);
	}

	if(course_data_msgid > -1) {
		nslog(NS_INFO, "r_msg_del course_data_msgid");
		r_msg_del(cr->course_data_msgid);
	}

	if(course_msg_msgid > -1) {
		nslog(NS_INFO, "r_msg_del course_msg_fd");
		r_msg_del(cr->course_msg_fd);
	}

	if(jr) {
		r_free(jr);
	}

	if(cr) {
		r_free(cr);
	}

	nslog(NS_ERROR, "register_course_record_mode is failed end!");
	return NULL;
}
static void getRecourseName(char *recourse_name, char *recovery_course)
{
	char *p = NULL;
	char recovery[] = "recovery/";
	p = r_strstr(recovery_course, recovery);
	if(NULL != p) {
		r_strcpy(recourse_name, p+r_strlen(recovery));
	}
}

void unregister_course_record_mode(course_record_t **cr)
{
	nslog(NS_INFO, "start ...");
	course_record_t *pcr = *cr;
	char recourse_name[128] = {0};
	if(NULL == pcr) {
		return;
	}

	pthread_mutex_destroy(&(pcr->print_mutex));
	remove(pcr->recovery_course);
	r_msg_del(pcr->course_msg_fd);
	r_msg_del(pcr->course_data_msgid);
	nslog(NS_INFO, "start ...[%s]", basename(pcr->recovery_course));
	add_recourse_list_fileinfo(basename(pcr->recovery_course));
	r_free(pcr);
	*cr = NULL;
	nslog(NS_INFO, "end ...");
}


