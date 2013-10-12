#include "osd.c"
#include "rwini.h"

//type != 0

int write_logo_info(int input);

int  postype_2_pixel(int input, TextInfo *pinfo)
{
	unsigned int width, height;
	//capture_get_input_hw(input, &width, &height);
	width = 1920;
	height = 1080;

	if(pinfo == NULL) {
		fprintf(stderr, "pinfo is NULL, return!");
		return -1;
	}

	int *x = &pinfo->xpos;
	int *y = &pinfo->ypos;
	int type = pinfo->postype;
	int text_width = strlen(pinfo->msgtext) * 16;

	if(height == 540) {
		height = height * 2;
	}

	if(type == ABSOLUTE2) {
		return -1;
	} else if(type == TOP_LEFT) {
		*x = 48;
		*y = 16;
	} else if(type == TOP_RIGHT) {
		*x = width - text_width - 48;
		*y = 16;
	} else if(type == BOTTOM_LEFT) {
		*x = 32;
		*y = height - 64 - 16;
	} else if(type == BOTTOM_RIGHT) {
		*x = width - text_width;
		*y = height - 64 - 16;
	} else if(type == CENTERED) {
		*x = (width) / 2;
		*y = height / 2 ;
	} else {
		*x = 0;
		*y = 0;
	}

	return 0;
}

int  postype_2_pixel1(int input, int type, int *x, int *y)
{
	unsigned int width, height;
	//capture_get_input_hw(input, &width, &height);
	width = 1920;
	height = 1080;

	if(height == 540) {
		height = height * 2;
	}

	if(type == ABSOLUTE2) {
		return -1;
	} else if(type == TOP_LEFT) {
		*x = 0;
		*y = 0;
	} else if(type == TOP_RIGHT) {
		*x = width ;
		*y = 0;
	} else if(type == BOTTOM_LEFT) {
		*x = 0;
		*y = height;
	} else if(type == BOTTOM_RIGHT) {
		*x = width;
		*y = height ;
	} else if(type == CENTERED) {
		*x = (width) / 2;
		*y = height / 2 ;
	} else {
		*x = 0;
		*y = 0;
	}

	return 0;
}


int app_get_logo(int channel, char *data, int vallen, char *outdata)
{
	int ret = 0;
	LogoInfo logo;
	int high = HIGH_STREAM;

	if(high != HIGH_STREAM) {
		printf("warnning,not support low stream set logo\n");
		return -1;
	}

	ret = get_logo_info(channel, &logo);
	//	ret = read_logo_info(id, logo);

	if(ret == 0) {
		printf("read logoinfo failed ret=%d\n", ret);
		memcpy(outdata, &logo, sizeof(LogoInfo));
		return ret;
	}

	memcpy(outdata, &logo, sizeof(LogoInfo));
	printf("LOGO:x=%d,y=%d,enable=%d,alpha=%d\n", logo.x, logo.y, logo.enable, logo.alpha);
	return ret;
}

int app_set_logo(int channel , char *data, int vallen, char *outdata)
{
	LogoInfo *logo = (LogoInfo *)data;
	int ret = 0;
	int high = HIGH_STREAM;

	if(high != HIGH_STREAM) {
		printf("warnning,not support low stream set logo\n");
		return -1;
	}


	if(vallen < sizeof(LogoInfo)) {
		ret = -1;
		printf("WRONG:vallen < sizeof(LogoInfo) \n");
		return ret;
	}


	printf("LOGO:x=%d,y=%d,enable=%d,alpha=%d,name=%s\n", logo->x, logo->y, logo->enable, logo->alpha, logo->filename);

	if(logo->alpha < 0 || logo->x < 0 || logo->x > 1920 || logo->y < 0 || logo->y > 1080) {
		ret = -2;
		return ret;
	}

	if(channel == SIGNAL_INPUT_1) {
		strcpy(logo->filename, "logo_0.png");
	} else if(channel == SIGNAL_INPUT_2) {
		strcpy(logo->filename, "logo_1.png");
	} else {
		strcpy(logo->filename, "logo_2.png");
	}

	//	logo->show = 1;
	//	logoflag = getShowLogoTextFlag();
	//	logoflag = logoflag | OnlyShowLogo;
	//	printf("LOGO:showlogotextflag = %d\n", logoflag);
	//	setShowLogoTextFlag(logoflag);
	printf("LOGO:x=%d,y=%d,enable=%d,alpha=%d,name=%s\n",
	       logo->x, logo->y, logo->enable, logo->alpha, logo->filename);

	//	logo->alpha = (logo->alpha)*0x80/100;
	add_logo_osd(channel, logo);
	set_logo_info(channel, logo);
	ret = write_logo_info(channel);

	if(ret == 0) {
		printf("save logoinfo to file failed ret =%d\n", ret);
		return ret;
	}

	return ret;
}



int app_update_logo(int channel, char *indata, char *outdata)
{
	int ret = 0;
	char com[256] = {0};
	char filename[256] = {0};

	int high = HIGH_STREAM;

	if(high != HIGH_STREAM) {
		printf("warnning,not support low stream set logo\n");
		return -1;
	}

	system("sync");

	strcpy(filename, indata);

	strcpy(outdata, indata);


	// check the logo file
	ret =  check_logo_png(channel, filename);

	if(ret == 0) {
		snprintf(com, sizeof(com), "mv %s %s_%d.png", filename, LOGOFILE, channel);
		system(com);
		printf("success change logo file. com=%s\n", com);
	}

	printf("check logo ret = %d\n", ret);
	return ret;
}




int read_text_info(int id, TextInfo *text)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//int 			enable = 0;
	const char config_file[] = TEXT_CONFIG;
	int 			rst = -1;
	printf("LOGO=%p\n", text);
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char	title[24] = {0};
	sprintf(title, "text_%d", id);

	ret =  ConfigGetKey((char *)config_file, title, "x", temp);

	if(ret != 0) {
		printf("Failed to Get logo x\n");
		goto EXIT;
	}

	text->xpos = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "y", temp);

	if(ret != 0) {
		printf("Failed to Get logo y\n");
		goto EXIT;
	}

	text->ypos = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "enable", temp);

	if(ret != 0) {
		printf("Failed to Get logo enable\n");
		goto EXIT;
	}

	text->enable = atoi(temp);

	ret =  ConfigGetKey((char *)config_file, title, "alpha", temp);

	if(ret != 0) {
		printf("Failed to Get logo alpha\n");
		goto EXIT;
	}

	text->alpha = atoi(temp);

	ret =  ConfigGetKey((char *)config_file, title, "showtime", temp);

	if(ret != 0) {
		printf("Failed to Get logo isThrough\n");
		goto EXIT;
	}

	text->showtime = atoi(temp);

	//pos type
	ret =  ConfigGetKey((char *)config_file, title, "postype", temp);

	if(ret != 0) {
		printf("Failed to Get logo isThrough\n");
		goto EXIT;
	}

	text->postype = atoi(temp);


	ret =  ConfigGetKey((char *)config_file, title, "content", temp);

	if(ret != 0) {
		printf("Failed to Get logo filename\n");
		goto EXIT;
	}

	printf("\n");
	//temp[15] = 0;
	strcpy(text->msgtext, temp);
	printf("\n");
	rst = 1;
EXIT:
	//pthread_mutex_unlock(&gSetP_m.save_sys_m);
	printf("\n");
	return rst;
}


int write_text_info(int input)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	const char config_file[] = TEXT_CONFIG;
	int 			rst = 0;
	TextInfo 		text ;
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char    title[24];
	sprintf(title, "text_%d", input);

	ret = get_text_info(input, & text);

	if(ret < 0) {
		printf("get text info failed!\n");
		return ret;
	}

	sprintf(temp, "%d", text.xpos);
	ret =  ConfigSetKey((char *)config_file, title, "x", temp);

	if(ret != 0) {
		printf("Failed to Get logo x\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.ypos);
	ret =  ConfigSetKey((char *)config_file, title, "y", temp);

	if(ret != 0) {
		printf("Failed to Get logo y\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.enable);
	ret =  ConfigSetKey((char *)config_file, title, "enable", temp);

	if(ret != 0) {
		printf("Failed to Get logo enable\n");
		goto EXIT;
	}

	strcpy(temp, text.msgtext);
	temp[strlen(text.msgtext)] = '\0';
	ret =  ConfigSetKey((char *)config_file, title, "content", temp);

	if(ret != 0) {
		printf("Failed to Get logo filename\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.alpha);
	ret =  ConfigSetKey((char *)config_file, title, "alpha", temp);

	if(ret != 0) {
		printf("Failed to Get logo alpha\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.showtime);
	ret =  ConfigSetKey((char *)config_file, title, "showtime", temp);

	if(ret != 0) {
		printf("Failed to Get logo isThrough\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.postype);
	ret =  ConfigSetKey((char *)config_file, title, "postype", temp);

	if(ret != 0) {
		printf("Failed to Get logo isThrough\n");
		goto EXIT;
	}

	/*
		sprintf(temp, "%d", text->show);
		ret =  ConfigSetKey((char *)config_file, title, "show", temp);

		if(ret != 0) {
			printf("Failed to Get logo show\n");
			goto EXIT;
		}
	*/
	rst = 1;
EXIT:
	//pthread_mutex_unlock(&gSetP_m.save_sys_m);

	return rst;
}


int read_logo_info(int id,  LogoInfo *logo)
{
	printf("logo =%p\n", logo);
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	int 			rst = -1;
	const char config_file[] = LOGO_CONFIG;
	char    title[24];
	sprintf(title, "logo_%d", id);

	ret =  ConfigGetKey((char *)config_file, title, "x", temp);

	if(ret != 0) {
		printf("Failed to Get logo x\n");
		goto EXIT;
	}

	logo->x = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "y", temp);

	if(ret != 0) {
		printf("Failed to Get logo y\n");
		goto EXIT;
	}

	logo->y = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "enable", temp);

	if(ret != 0) {
		printf("Failed to Get logo enable\n");
		goto EXIT;
	}

	logo->enable = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "filename", temp);

	if(ret != 0) {
		printf("Failed to Get logo filename\n");
		goto EXIT;
	}

	temp[15] = 0;
	strcpy(logo->filename, temp);

	ret =  ConfigGetKey((char *)config_file, title, "alpha", temp);

	if(ret != 0) {
		printf("Failed to Get logo alpha\n");
		goto EXIT;
	}

	logo->alpha = atoi(temp);

	/*
		ret =  ConfigGetKey((char *)config_file, title, "show", temp);

		if(ret != 0) {
			printf("Failed to Get logo show\n");
			goto EXIT;
		}

		logo->show = atoi(temp);
	*/

	ret =  ConfigGetKey((char *)config_file, title, "postype", temp);

	if(ret != 0) {
		printf("Failed to Get logo isThrough\n");
		goto EXIT;
	}

	logo->postype = atoi(temp);


	rst = 1;
EXIT:
	//pthread_mutex_unlock(&gSetP_m.save_sys_m);

	return rst;
}

int write_logo_info(int input)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	int				rst = 0;
	const char *config_file = LOGO_CONFIG ;
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char    title[24];
	LogoInfo logo;
	sprintf(title, "logo_%d", input);


	ret = get_logo_info(input, &logo);

	if(ret < 0) {
		printf("get text info failed!\n");
		return ret;
	}

	sprintf(temp, "%d", logo.x);
	ret =  ConfigSetKey((char *)config_file, title, "x", temp);

	if(ret != 0) {
		printf("Failed to Set logo x\n");
		goto EXIT;
	}

	sprintf(temp, "%d", logo.y);
	ret =  ConfigSetKey((char *)config_file, title, "y", temp);

	if(ret != 0) {
		printf("Failed to Set logo y\n");
		goto EXIT;
	}

	sprintf(temp, "%d", logo.enable);
	ret =  ConfigSetKey((char *)config_file, title, "enable", temp);

	if(ret != 0) {
		printf("Failed to Set logo enable\n");
		goto EXIT;
	}


	ret =  ConfigSetKey((char *)config_file, title, "filename", logo.filename);

	if(ret != 0) {
		printf("Failed to Set filename\n");
		goto EXIT;
	}

	sprintf(temp, "%d", logo.alpha);
	ret =  ConfigSetKey((char *)config_file, title, "alpha", temp);

	if(ret != 0) {
		printf("Failed to Set alpha \n");
		goto EXIT;
	}

	/*
		sprintf(temp, "%d", logo->show);
		ret =  ConfigSetKey((char *)config_file, title, "show", temp);

		if(ret != 0) {
			printf("Failed to Set show \n");
			goto EXIT;
		}
	*/
	sprintf(temp, "%d", logo.postype);
	ret =  ConfigSetKey((char *)config_file, title, "postype", temp);

	if(ret != 0) {
		printf("Failed to Set isThrough \n");
		goto EXIT;
	}


	rst = 1;
EXIT:
	//pthread_mutex_unlock(&gSetP_m.save_sys_m);
	return rst;
}





int app_get_text(int channel, char *data, int vallen, char *outdata)
{
	int ret = 0;

	int high = HIGH_STREAM;
	TextInfo text  ;

	ret = get_text_info(channel, &text);

	if(high != HIGH_STREAM || ret < 0) {
		printf("warnning,not support low stream set logo\n");
		return -1;
	}

	memcpy(outdata, &text, sizeof(TextInfo));
	printf("id=%d:text:x=%d,y=%d,enable=%d,alpha=%d\n", channel, text.xpos, text.ypos, text.enable, text.alpha);
	return ret;
}

int app_set_text(int channel, char *data, int vallen, char *outdata)
{
	if(vallen != sizeof(TextInfo)) {
		printf("vallen=%d \n", vallen);
		printf("WRONG:vallen < sizeof(LogoInfo) \n");
		return -1;
	}

	printf("display text enter ....\n");
	TextInfo *tinfo = (TextInfo *)(data);
	int ret = 0;
	int high = HIGH_STREAM;
	TextInfo text;

	ret = get_text_info(channel, &text);

	if(high != HIGH_STREAM || ret < 0) {
		printf("warnning,not support low stream set logo\n");
		return -1;
	}

	if(vallen < sizeof(TextInfo)) {
		ret = -1;
		printf("warnning,vallen < sizeof(TextInfo)\n");
		return ret;
	}

	if(tinfo->xpos < 0 || tinfo->xpos > 1920 || tinfo->ypos < 0 || tinfo->ypos > 1080) {
		ret = -2;
		return ret;
	}

	printf("postype = %d ..\n", tinfo->postype);
	postype_2_pixel(channel, tinfo);
	tinfo->enable = text.enable;
	/*
		if(0 != strlen(tinfo->msgtext))
			tinfo->show = 1;
		else
			tinfo->show = 0;
	*/

	ret = add_text_info(channel, tinfo);

	set_text_info(channel, tinfo);

	write_text_info(channel);

	//memcpy(outdata, tinfo, sizeof(TextInfo));

	return 0;
}


/*获得文本长度*/
int get_text_len(char *Data)
{
	int n_Len = strlen(Data);
	unsigned char *lpData = (unsigned char *)Data;
	int i = 0;
	int Sum = 0;

	for(i = 0; /**pStr != '\0'*/ i < n_Len ;) {
		if(*lpData <= 127) {
			i++;
			lpData++;
		} else {
			i += 2;
			lpData += 2;
		}

		Sum++;
	}

	return Sum;
}




int ENC_set_osd_text(int channel_num, unsigned char *data, int len)
{
	RecAVTitle recavtitle;
	int textXpos = 0;
	int textYpos = 0;
	int high = HIGH_STREAM;
	int time_displaye = 0;
	TextInfo text;
	int ret = 0;


	if(channel_num >= CHANNEL_INPUT_MAX) {
		printf("error,the osd channel num is to big =%d\n", channel_num);
		return -1;
	}

	ret = get_text_info(channel_num, &text);

	printf("channel_num=%d,vp=%d,high=%d\n", channel_num, channel_num, high);

	if(high != HIGH_STREAM || ret < 0) {
		printf("warnning,not support high stream\n");
		return -1;
	}

	//	id = input;
	memset(&recavtitle, 0x00000, sizeof(RecAVTitle));
	memcpy(&recavtitle, data, len);
	printf("display the text =%s =recavtitle.len=%d\n", recavtitle.Text, recavtitle.len);

	if(recavtitle.len > 0) {

		int len = get_text_len(recavtitle.Text);
		int size_len = get_text_size(recavtitle.Text);
		int charlen = strlen(recavtitle.Text);
		printf("len = %d=%d=%d\n", len, size_len, charlen);

		if(len != 0) {
			textXpos = recavtitle.x;
			textYpos = recavtitle.y;

			if(textXpos % 32) {
				textXpos -= (textXpos % 32);
			}

			if(textYpos % 32) {
				textYpos -= (textYpos % 32);
			}

			printf("textXpos = %d  textYpos = %d\n\n", textXpos, textYpos);

			//临时修改
			//		if((H264Height() - textYpos) < 64) {
			//			textYpos = H264Height() - 64;
			//		}

			text.xpos = textXpos;
			text.ypos = textYpos;
			//			set_text_info( input,&text);

			if(check_CHN(recavtitle.Text, charlen)) {
				char temp[512] = {0};
				code_convert("gb2312", "utf-32", recavtitle.Text, charlen, temp, charlen * 4);
				show_text_info(channel_num, temp, size_len, &text);
			} else {
				show_text_info(channel_num, recavtitle.Text, size_len, &text);
			}
		} else {
			hide_osd_view(channel_num, WINDOW_TEXT_OSD);
		}

		if(len != recavtitle.len) {
			printf("display the time \n");
			set_time_display(channel_num, 1);
		} else {
			set_time_display(channel_num, 0);
		}
	} else {
		set_time_display(channel_num, 0);
		hide_osd_view(channel_num, WINDOW_TEXT_OSD);
	}

	time_displaye = get_time_display(channel_num);

	if(time_displaye == 0) {
		hide_osd_view(channel_num, WINDOW_TIME_OSD);
	}

	return 0;
}


int app_get_osd_max_xypos(int channel, int *out)
{
	unsigned short x_pos = 0, y_pos = 0;
	int input = SIGNAL_INPUT_1;
	unsigned int width, height;

	capture_get_input_hw(channel, &width, &height);
	printf("input=%d,width*height=%dX%d\n", input, width, height);
	x_pos = width - 64;
	y_pos = height - 64;
	*out = (x_pos << 16) | y_pos;
	printf("max_x_pos=%d,max_y_pos=%d\n", x_pos, y_pos);
	return 0;
}

int app_osd_enable(WebEnableTextInfo *info, int get, int channel)
{

	int high = HIGH_STREAM;
	int ret = 0;
	TextInfo text ;
	LogoInfo logo ;

	ret = get_text_info(channel, &text);

	if(high != HIGH_STREAM || ret < 0) {
		printf("warnning,not support low stream set logo\n");
		return -1;
	}

	ret = get_logo_info(channel, &logo);

	if(ret < 0) {
		printf("warnning,get failed!\n");
		return -1;
	}

	if(get) {
		info->logo_show = logo.enable ;
		info->text_show = text.enable;
	} else {
		if(logo.enable != info->logo_show) {
			logo.enable = info->logo_show;

			if(logo.enable) {
				add_logo_osd(channel, &logo);
			} else {
				hide_osd_view(channel, WINDOW_LOGO_OSD);
			}

			set_logo_info(channel, &logo);
		}

		if(text.showtime != 0) {
			set_time_display(channel, info->text_show);
			text.showtime = info->text_show;
		}

		if(text.enable != info->text_show) {
			text.enable = info->text_show;

			if(text.enable) {
				text.alpha = text.alpha * 100 / 0x80; //还原最初alpha值
				add_text_info(channel, &text);
			} else {
				hide_osd_view(channel, WINDOW_TEXT_OSD);
				hide_osd_view(channel, WINDOW_TIME_OSD);
			}

			set_text_info(channel, &text);
		}

		write_text_info(channel);
		write_logo_info(channel);
	}

	printf("channel = %d, LOGO->enable=%d,text.[%d]->enable=%d\n", channel, logo.enable, channel, text.enable);
	return 0;
}



