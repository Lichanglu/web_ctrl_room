/*
 * =====================================================================================
 *
 *       Filename:  receive_module.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2012-11-5, 09:52:32
 *       Revision:
 *       Compiler:  gcc
 *
 *         Author:  liuchsh
 *        Company:  szreach.com
 *
 * =====================================================================================
 */
// add zl
#include "media_msg.h"
#include "media_frame.h"    // #include "receive_module.h"
#include "reach_udp_recv.h"


static int32_t create_room_pthread(recv_room_handle *recv_hand);
static void *room_receive_deal(void *arg);
static int32_t receive_parse_stream(MSGHEAD *msg, stream_handle *M_handle, void *in_data, int32_t in_data_len, udp_send_module_handle *p_udp_hand);
static int32_t receive_connect_server(stream_handle *M_handle);
static int32_t init_room_module(void *recv_hand);
static int32_t deinit_room_module(void *recv_hand);
static int32_t close_room_connect(void *recv_hand);
static int32_t open_stream_connect(stream_handle *stream_hand);
static int32_t close_stream_connect(stream_handle *stream_hand);
static int32_t open_room_connect(void *recv_hand);
static int32_t set_recv_to_ctrl_msgid(int32_t msgid, stream_handle *stream_hand);   
static int32_t get_recv_to_ctrl_msgid( stream_handle *stream_hand);   
static int32_t set_recv_to_live_msgid(int32_t msgid, stream_handle *stream_hand);  
static int32_t get_recv_to_live_msgid( stream_handle *stream_hand);  
static int32_t set_recv_to_rec_msgid(int32_t msgid, stream_handle *stream_hand);  
static int32_t set_recv_to_usb_rec_msgid(int32_t msgid, stream_handle *stream_hand);  

static int32_t get_recv_to_rec_msgid( stream_handle *stream_hand); 
static  int32_t set_room_info(recv_param *param, void *recv_hand, int32_t param_len);
static int32_t  get_stream_connect(stream_handle *stream_hand);

static int32_t set_rec_status(stream_handle *stream_hand, uint8_t status);
static int32_t set_usb_rec_status(stream_handle *stream_hand, uint8_t status);

static int32_t set_live_status(stream_handle *stream_hand, uint8_t status);
static int32_t get_stream_socket(stream_handle *stream_hand);
static void * recv_real_print(void *recv_handle);
//static int32_t memset_pri_status(recv_print *pinfo);    // add zl
static int32_t recv_enc_tcp_data(stream_handle *p_handle, void *buf, int32_t buflen);
static int32_t create_stream_pthread(stream_handle *handle);  // add zl

static int32_t free_recv_module_pool(recv_room_handle *recv_hand)
{
	if(recv_hand == NULL)
	{
		return -1;
	}
	recv_room_handle *recv_handle = (recv_room_handle *)recv_hand;
	int32_t index = 0;
	for(index = 0; index < recv_handle->stream_num ; index)
	{
		if(recv_handle->stream_hand[index].frame_data != NULL)
		{
			r_free(recv_handle->stream_hand[index].frame_data);
			recv_handle->stream_hand[index].frame_data = NULL;
		}
		if(recv_handle->stream_hand[index].audio_data != NULL)
		{
			r_free(recv_handle->stream_hand[index].audio_data);
			recv_handle->stream_hand[index].audio_data = NULL;
		}
	}
	return 0;
}

static int32_t malloc_recv_stream_pool(recv_room_handle *recv_hand ,int32_t stream_id)
{
	if(recv_hand == NULL)
	{
		return -1;
	}
	recv_room_handle *recv_handle = (recv_room_handle *)recv_hand;

	if(recv_handle->stream_hand[stream_id].frame_data == NULL)
	{
		recv_handle->stream_hand[stream_id].frame_data = (int8_t *)r_malloc(MAX_FRAME_LEN);
	}
	
	if(recv_handle->stream_hand[stream_id].audio_data == NULL)
	{
		recv_handle->stream_hand[stream_id].audio_data = (int8_t *)r_malloc(MAX_AUDIO_LEN);
	}

	r_memset(recv_handle->stream_hand[stream_id].audio_data, 0, MAX_AUDIO_LEN);
	r_memset(recv_handle->stream_hand[stream_id].frame_data, 0, MAX_FRAME_LEN);
	return 0;
}


static int32_t set_stream_com_info(stream_handle *info)
{
	int32_t ret = -1;
	int32_t enc_id = 0;
	if(info == NULL)
	{
		return ret;
	}
	enc_id = info->enc_id;
	switch(enc_id)
	{
		case 1:									// 合成编码器
			info->pull_module = PULL_IN_UDP;
			info->push_module = NO_COMMUNICATION;
			if(info->stream_id == 1)
			{
				info->pull_udp_port = UDP_SYN_TO_ROOM_HD;
			}
			else
			{
				info->pull_udp_port = UDP_SYN_TO_ROOM_SD;
			}
			info->port = TCP_SYN_TO_ROOM ;
			info->rec_mode = RECODE_MODE_MOVIE;
			info->rec_audio_falg = RECODE_AUDIO_ON;
			info->live_audio_falg = RECODE_AUDIO_ON;
			info->recv_audio_flag = RECODE_AUDIO_OFF;
			ret = 0;
			break;
		case 2:									// SDI-1
			info->pull_module = PULL_IN_UDP;
			info->push_module = NO_COMMUNICATION;
			info->pull_udp_port = UDP_SDI1_TO_ROOM;
			info->port = TCP_SDI1_TO_ROOM;
			info->rec_mode = RECODE_MODE_RES;
			info->rec_audio_falg = RECODE_AUDIO_ON;
			info->live_audio_falg = RECODE_AUDIO_OFF;
			info->recv_audio_flag = RECODE_AUDIO_OFF;
			ret = 0;
			break;
		case 3:									// SDI-2
			info->pull_module = PULL_IN_UDP;
			info->push_module = NO_COMMUNICATION;
			info->pull_udp_port = UDP_SDI2_TO_ROOM;
			info->port = TCP_SDI2_TO_ROOM;
			info->rec_mode = RECODE_MODE_RES;
			info->rec_audio_falg = RECODE_AUDIO_ON;
			info->live_audio_falg = RECODE_AUDIO_OFF;
			info->recv_audio_flag = RECODE_AUDIO_OFF;
			ret = 0;
			break;
		case 4:									//VGA-H264(仿ENC1200)
			info->pull_module = PULL_IN_UDP;
			info->push_module = NO_COMMUNICATION;
			info->pull_udp_port = UDP_ENC1200_TO_ROOM;
			info->port = TCP_ENC1200_TO_ROOM;
			info->rec_mode = RECODE_MODE_RES;
			info->rec_audio_falg = RECODE_AUDIO_ON;
			info->live_audio_falg = RECODE_AUDIO_OFF;
			info->recv_audio_flag = RECODE_AUDIO_ON;
			ret = 0;
			break;
		case 5:									//VGA-PPT(仿ENC120E)
			info->pull_module = PULL_IN_UDP;
		//	info->pull_module = PULL_IN_TCP;
			info->push_module = NO_COMMUNICATION;
			info->pull_udp_port = UDP_ENC120_TO_ROOM;
			info->port = TCP_ENC120_TO_ROOM;
			info->rec_mode = RECODE_MODE_ALL;
			info->rec_audio_falg = RECODE_AUDIO_OFF;
			info->live_audio_falg = RECODE_AUDIO_OFF;
			info->recv_audio_flag = RECODE_AUDIO_OFF;
			ret = 0;
			break;
		case 6:									//IP-Carame-1 (防 ENC1200)
			info->pull_module = PULL_IN_TCP;
			info->push_module = PUSH_IN_UDP;
			info->push_udp_port = UDP_ROOM_TO_SYN_IPENC1;
			info->pull_udp_port = UDP_ROOM_TO_SYN_IPENC1_BIND;
			info->port = TCP_IPENC1_TO_ROOM;
			info->rec_mode = RECODE_MODE_RES;
			info->rec_audio_falg = RECODE_AUDIO_OFF;
			info->live_audio_falg = RECODE_AUDIO_OFF;
			info->recv_audio_flag = RECODE_AUDIO_OFF;
			ret = 0;
			break;
		case 7:									//IP-Carame-2 (防 ENC1200)
			info->pull_module = PULL_IN_TCP;
			info->push_module = PUSH_IN_UDP;
			info->push_udp_port = UDP_ROOM_TO_SYN_IPENC2;
			info->pull_udp_port = UDP_ROOM_TO_SYN_IPENC2_BIND;
			info->port = TCP_IPENC2_TO_ROOM;
			info->rec_mode = RECODE_MODE_RES;
			info->rec_audio_falg = RECODE_AUDIO_OFF;
			info->live_audio_falg = RECODE_AUDIO_OFF;
			info->recv_audio_flag = RECODE_AUDIO_OFF;
			ret = 0;
			break;
		case 8:									//IP-Carame-3 (防 ENC1200)
			info->pull_module = PULL_IN_TCP;
			info->push_module = PUSH_IN_UDP;
			info->push_udp_port = UDP_ROOM_TO_SYN_IPENC3;
			info->pull_udp_port = UDP_ROOM_TO_SYN_IPENC3_BIND;
			info->port = TCP_IPENC3_TO_ROOM;
			info->rec_mode = RECODE_MODE_RES;
			info->rec_audio_falg = RECODE_AUDIO_OFF;
			info->live_audio_falg = RECODE_AUDIO_OFF;
			info->recv_audio_flag = RECODE_AUDIO_OFF;
			ret = 0;
			break;
		case 9:									//IP-Carame-4 (防 ENC1200)
			info->pull_module = PULL_IN_TCP;
			info->push_module = PUSH_IN_UDP;
			info->push_udp_port = UDP_ROOM_TO_SYN_IPENC4;
			info->pull_udp_port = UDP_ROOM_TO_SYN_IPENC4_BIND;
			info->port = TCP_IPENC4_TO_ROOM;
			info->rec_mode = RECODE_MODE_RES;
			info->rec_audio_falg = RECODE_AUDIO_OFF;
			info->live_audio_falg = RECODE_AUDIO_OFF;
			info->recv_audio_flag = RECODE_AUDIO_OFF;
			ret = 0;
			break;
		default:
			break;
	}
	if(ret == 0)
	{
	//	info->port = TCP_SYN_TO_ROOM - 1+ enc_id;
	//	info->port = TCP_SYN_TO_ROOM;
		;
	}
	return ret;
}

static int32_t set_stream_audio_info(recv_room_handle *recv_handle)
{
	int32_t ret = -1;
	int32_t enc_id = 0;
	int32_t index = 0;
	stream_handle *info = NULL;
	int32_t vaild_index = 0;
	if(recv_handle == NULL)
	{
		return ret;
	}
	for(index = 0;index <MAX_STREAM ;index ++)
	{
		if(recv_handle->stream_hand[index].media_flag == MEDIA_STREAM_INVALID)
		{
			continue;
		}
		if(recv_handle->stream_hand[index].recv_audio_flag == RECODE_AUDIO_ON)
		{
			// 就一路来声音
			recv_handle->stream_hand[index].audio_info = (media_audio_info_t *)r_malloc(sizeof(media_audio_info_t));
			r_memset(recv_handle->stream_hand[index].audio_info , 0 ,sizeof(media_audio_info_t));
			info = &(recv_handle->stream_hand[index]);
			break;
		}
	}
	if(info == NULL)
	{
		return -1;
	}
	for(index = 0;index <MAX_STREAM ;index ++)
	{
		if(recv_handle->stream_hand[index].media_flag == MEDIA_STREAM_INVALID)
		{
			continue;
		}

		if(recv_handle->stream_hand[index].rec_audio_falg == RECODE_AUDIO_ON &&
			(recv_handle->stream_hand[index].rec_mode == RECODE_MODE_MOVIE ||
			recv_handle->stream_hand[index].rec_mode == RECODE_MODE_ALL))
		{
			vaild_index = info->audio_info->audio_tran_movie_info.stream_num ;
			info->audio_info->audio_tran_movie_info.stream_handle_addr[vaild_index] = &(recv_handle->stream_hand[index]);
			info->audio_info->audio_tran_movie_info.stream_num++;
			nslog(NS_ERROR,"FUCK_GOD_1015 <RECODE_MOVIE_AUDIO> <AUDIO_INDEX : %d> <STREAM_INDEX : %d> <STREAM_ADDR : %p> <NUM : %d>\n",
				info->stream_id ,
				recv_handle->stream_hand[index].stream_id,
				&(info->audio_info->audio_tran_movie_info.stream_handle_addr[vaild_index -1]),
				info->audio_info->audio_tran_movie_info.stream_num);
		}

		if(recv_handle->stream_hand[index].rec_audio_falg == RECODE_AUDIO_ON &&
			(recv_handle->stream_hand[index].rec_mode == RECODE_MODE_RES ||
			recv_handle->stream_hand[index].rec_mode == RECODE_MODE_ALL))
		{
			vaild_index = info->audio_info->audio_tran_res_info.stream_num ;
			info->audio_info->audio_tran_res_info.stream_handle_addr[vaild_index] = &(recv_handle->stream_hand[index]);
			info->audio_info->audio_tran_res_info.stream_num++;

			nslog(NS_ERROR,"FUCK_GOD_1015 <RECODE_RES_AUDIO> <AUDIO_INDEX : %d> <STREAM_INDEX : %d> <STREAM_ADDR : %p> <NUM : %d>\n",
				info->stream_id ,
				recv_handle->stream_hand[index].stream_id,
				&(info->audio_info->audio_tran_res_info.stream_handle_addr[vaild_index -1]),
				info->audio_info->audio_tran_res_info.stream_num);
			
		}

		if(recv_handle->stream_hand[index].live_audio_falg == RECODE_AUDIO_ON)
		{
			vaild_index = info->audio_info->audio_tran_live_into.stream_num ;
			info->audio_info->audio_tran_live_into.stream_handle_addr[vaild_index] = &(recv_handle->stream_hand[index]);
			info->audio_info->audio_tran_live_into.stream_num++;

			nslog(NS_ERROR,"FUCK_GOD_1015 <LIVE_AUDIO> <AUDIO_INDEX : %d> <STREAM_INDEX : %d> <STREAM_ADDR : %p> <NUM : %d>\n",
				info->stream_id ,
				recv_handle->stream_hand[index].stream_id,
				&(info->audio_info->audio_tran_live_into.stream_handle_addr[vaild_index -1]),
				info->audio_info->audio_tran_live_into.stream_num);
		}
	}

	return 0;
	
}

int32_t memset_pri_status(recv_print *pinfo)
{
    if(NULL == pinfo){
        nslog(NS_ERROR, "memset_pri_status failed, params is NULL!\n");
        return -1;
    }

	pthread_mutex_lock(&pinfo->print_mutex);
    pinfo->code_rate = 0;
    pinfo->data_type = 0;
    pinfo->stream_type = 0;
    pinfo->is_data = 0;
    pinfo->frame_rate = 0;
    pinfo->width = 0;
    pinfo->hight = 0;
    pinfo->sample_rate = 0;
	pthread_mutex_unlock(&pinfo->print_mutex);

    return 0;
}


/*==============================================================================
  函数: <register_room_receive_module>
  功能: <注册会议室接收模块:>
  参数: int32_t max_stream_num   设置最大流数.
  返回值: 成功返回recv_room_handle *，失败返回NULL.
  Created By liuchsh 2012.11.16 14:25:23 For EDU
  ==============================================================================*/
recv_room_handle *register_room_receive_module(int32_t max_stream_num)
{
    recv_room_handle *recv_handle = NULL;

    int32_t return_code = 0, i = 0;
    int32_t max_stream = MAX_STREAM;   //日志线程为最后个下标。
    /*Added by liuchsh 2012.11.16 11:42:05 For EDU , BEGIN*/
    /*检测数值合法性*/
    /* Added by liuchsh , End*/
    if(max_stream_num > max_stream)
    {
        max_stream_num = max_stream;
    }
    else if (max_stream_num < 0)
    {
        nslog(NS_ERROR, "max_stream_num = %d", max_stream_num);
        return NULL;
    }

    recv_handle = (recv_room_handle *)r_malloc(RoomLen);

    if(NULL == recv_handle)
    {
        nslog(NS_ERROR, "r_malloc");
        return NULL;
    }
	

    r_memset(recv_handle, 0, RoomLen);
    recv_handle->init_room_module = init_room_module;
    recv_handle->deinit_room_module = deinit_room_module;
    recv_handle->close_room_connect = close_room_connect;
    recv_handle->open_room_connect = open_room_connect;
    recv_handle->set_recv_to_ctrl_msgid = set_recv_to_ctrl_msgid;
    recv_handle->get_stream_socket = get_stream_socket;
    recv_handle->get_stream_connect = get_stream_connect;
    recv_handle->close_stream_connect = close_stream_connect;
    recv_handle->open_stream_connect = open_stream_connect;
    recv_handle->set_room_info = set_room_info;

    recv_handle->set_recv_to_rec_msgid = set_recv_to_rec_msgid;
	recv_handle->set_recv_to_usb_rec_msgid = set_recv_to_usb_rec_msgid;
    recv_handle->set_recv_to_live_msgid = set_recv_to_live_msgid;

    recv_handle->set_rec_status = set_rec_status;
    recv_handle->get_rec_status = get_rec_status;
    recv_handle->set_usb_rec_status = set_usb_rec_status;  //add zl
    recv_handle->get_usb_rec_status = get_usb_rec_status;
    recv_handle->set_live_status = set_live_status;
    recv_handle->get_live_status = get_live_status;


    recv_handle->stream_num = max_stream_num;

    /*Added by liuchsh 2012.11.16 11:49:59 For EDU , BEGIN*/
    //为组帧模块分配空间.
    /* Added by liuchsh , End*/
	#if 0
    for(i = 0; i < recv_handle->stream_num; i++)
    {
        recv_handle->stream_hand[i].pthread_status = 1;

        recv_handle->stream_hand[i].frame_data = (int8_t *)r_malloc(MAX_FRAME_LEN);

        if(NULL == recv_handle->stream_hand[i].frame_data)
        {
            nslog(NS_ERROR, "r_malloc error");

            while(i > -1)
            {
				  if (recv_handle->stream_hand[i].frame_data) {
                		r_free(recv_handle->stream_hand[i].frame_data);
                		recv_handle->stream_hand[i].frame_data = NULL;
				  }
				 i--;
            }
            goto EXIT;
        }

		 r_memset(recv_handle->stream_hand[i].frame_data, 0, MAX_FRAME_LEN);
				
        recv_handle->stream_hand[i].audio_data = (int8_t *)r_malloc(MAX_AUDIO_LEN);
      	//recv_handle->stream_hand[i].audio_data = (int8_t *)r_malloc(12);
        if(NULL == recv_handle->stream_hand[i].audio_data)
        {
            nslog(NS_ERROR, "r_malloc error");

            while(i > -1)
            {
				  if (recv_handle->stream_hand[i].frame_data) {
                		r_free(recv_handle->stream_hand[i].frame_data);
                		recv_handle->stream_hand[i].frame_data = NULL;
				  }
				  if (recv_handle->stream_hand[i].audio_data) {
                		r_free(recv_handle->stream_hand[i].audio_data);
                		recv_handle->stream_hand[i].audio_data = NULL;
				  }
				  i--;
            }
            goto EXIT;
        }

		 r_memset(recv_handle->stream_hand[i].audio_data, 0, MAX_AUDIO_LEN);
    }
    recv_handle->stream_hand[i].pthread_status = 1;
	
	#endif
    return_code = create_room_pthread(recv_handle);
    if(return_code == OPERATION_SUCC)
    {
        init_room_module(recv_handle);
        nslog(NS_INFO, "register_room_receive_module success");
    }
    else
    {
        nslog(NS_ERROR, "create_room_pthread error");
        goto EXIT;
    }
    return recv_handle;
EXIT:
    unregister_room_receive_module(recv_handle);
}

/*==============================================================================
  函数: <init_room_module>
  功能: <初始化接收模块>
  参数: void *recv_handle
  返回值: 成功返回0，失败返回-1.
  Created By liuchsh 2012.11.16 14:26:27 For EDU
  ==============================================================================*/
static int32_t init_room_module(void *recv_handle)
{
    if(NULL == recv_handle)
    {
        nslog(NS_ERROR, "recv_handle is NULL");
        return OPERATION_ERR;
    }

    int index = 0;
    recv_room_handle *recv_hand = (recv_room_handle *)recv_handle;

    for(index = 0; index < recv_hand->stream_num; index++)
    {
        pthread_mutex_init(&recv_hand->stream_hand[index].mutex, NULL);
        pthread_mutex_init(&recv_hand->stream_hand[index].alive_mutex, NULL);
        pthread_mutex_init(&recv_hand->stream_hand[index].status_mutex, NULL);
		 pthread_mutex_init(&recv_hand->stream_hand[index].recv_pri.print_mutex, NULL);

		// add zl
		recv_hand->stream_hand[index].audio_info = NULL;
		recv_hand->stream_hand[index].media_flag = MEDIA_STREAM_INVALID;

	}
	

    return OPERATION_SUCC;
}

static int32_t deinit_room_module(void *recv_handle)
{
    int32_t index = 0;
    int32_t  num = 0;
    if(NULL == recv_handle)
    {
        nslog(NS_ERROR, "recv_handle is NULL");
        return OPERATION_ERR;
    }

    recv_room_handle *recv_hand = (recv_room_handle *)recv_handle;
    num = recv_hand->stream_num;
    recv_hand->stream_num = 0;

    pthread_mutex_destroy(&recv_hand->stream_hand[num].mutex);
    pthread_mutex_destroy(&recv_hand->stream_hand[num].alive_mutex);
    pthread_mutex_destroy(&recv_hand->stream_hand[num].status_mutex);
	 pthread_mutex_destroy(&recv_hand->stream_hand[index].recv_pri.print_mutex);
   	 recv_hand->stream_hand[num].pthread_status = 0;
	if (0 < recv_hand->stream_hand[num].pid) {
    	pthread_join(recv_hand->stream_hand[num].pid, NULL);
	}

    for(index = 0; index < num; index++)
    {
        close_stream_connect(&recv_hand->stream_hand[index]);
        set_rec_status(&recv_hand->stream_hand[index], STOP_REC);
		set_usb_rec_status(&recv_hand->stream_hand[index], STOP_USB_REC);
        set_live_status(&recv_hand->stream_hand[index], STOP_LIVE);
		// add zl question???
        recv_hand->stream_hand[index].pthread_status = 0;
        pthread_mutex_destroy(&recv_hand->stream_hand[index].mutex);
        pthread_mutex_destroy(&recv_hand->stream_hand[index].alive_mutex);
        pthread_mutex_destroy(&recv_hand->stream_hand[index].status_mutex);
		 pthread_mutex_destroy(&recv_hand->stream_hand[index].recv_pri.print_mutex);
        if (0 < recv_hand->stream_hand[index].pid) {
        		pthread_join(recv_hand->stream_hand[index].pid, NULL);
        }
    }

    if(recv_hand)
    {
        r_free(recv_hand);
        recv_hand = NULL;
    }
    nslog(NS_INFO, "unregister_room_module success");
    return OPERATION_SUCC;
}

/*==============================================================================
  函数: <set_room_info>
  功能: <设置流信息>
  参数: recv_param *param, stream_handle *stream_hand
  返回值: 成功返回0，失败返回-1.
  Created By liuchsh 2012.11.16 14:28:21 For EDU
  ==============================================================================*/
static int32_t  set_room_info(recv_param *param, void *recv_hand, int32_t param_len)
{
    int32_t index = 0;
    recv_room_handle *recv_handle = (recv_room_handle *)recv_hand;
    if (NULL == recv_handle  || NULL == param || 0 >= param_len){
        nslog(NS_ERROR, "recv_handle = [%p], param = [%p]", recv_handle, param);
        return OPERATION_ERR;
    }


    for (index = 0; index < param_len; index++) 
	{

		// add zl
		if(INVALID_ENC == param[index].status)
		{
			recv_handle->stream_hand[index].pthread_status = 0;  // 结束空跑线程!!!
			nslog(NS_ERROR,"FUCK ------ \n");

			recv_handle->stream_hand[index].media_flag = MEDIA_STREAM_INVALID;  // add zl

			continue;
		}
		nslog(NS_ERROR,"FUCK-GOD SET FLAG IS OK !<streamid :%d>\n",param[index].stream_id);
		recv_handle->stream_hand[index].media_flag = MEDIA_STREAM_VALID;	// add zl
		
	    if (CONNECT == get_stream_status(&recv_handle->stream_hand[index])) {
            close_stream_connect(&recv_handle->stream_hand[index]);
            //usleep(CLOSE_CONNECT_DELAY);
        }
		// add zl 
		
		recv_handle->stream_hand[index].pthread_status = 1;
		
		if(malloc_recv_stream_pool(recv_handle,index) < 0)
		{
			nslog(NS_ERROR,"MALLOC IS ERROR,<ENC_ID : %d> <IP : %s>\n",param[index].enc_id,param[index].ipaddr);
			free_recv_module_pool(recv_handle);
		}
		
		// add zl
		create_stream_pthread(&(recv_handle->stream_hand[index]));

        r_memset(recv_handle->stream_hand[index].ipaddr, 0, IP_LEN);
        recv_handle->stream_hand[index].port = 0;

        if ((index + 1) == param[index].stream_id && 1 == param[index].status )
		{
            recv_handle->stream_hand[index].stream_id = param[index].stream_id;
            recv_handle->stream_hand[index].enc_id = param[index].enc_id;

			nslog(NS_ERROR,"FUCKGOD ---- INDEX : %d ---- treamid %d   ---- enc_id :%d  --- param_len : %d ---- ip : %s\n",
				index,param[index].stream_id,param[index].enc_id,param_len,param[index].ipaddr);

	        // add zl
			if(set_stream_com_info(&(recv_handle->stream_hand[index])) <0)
			{
				nslog(NS_ERROR,"IMPORT ERROR, <STREAM_ID : %d> <ENC_ID : %d>\n",param[index].stream_id,param[index].enc_id);
				return OPERATION_ERR;
			}

			
			// add zl
			if(recv_handle->stream_hand[index].push_module == PUSH_IN_UDP)
			{
				r_memset(recv_handle->stream_hand[index].push_ip, 0, IP_LEN);
				r_memcpy(recv_handle->stream_hand[index].push_ip,param[0].ipaddr,r_strlen(param[0].ipaddr));
			}
			

		  	if (r_strlen(param[index].ipaddr) < IP_LEN)
		  	{
        		r_memcpy(recv_handle->stream_hand[index].ipaddr, param[index].ipaddr, r_strlen(param[index].ipaddr));
		  	}
			else 
		  	{
				nslog(NS_WARN, "ip addr len error, ignore it");
		  	}
        }
    }

	
	// add zl deal aduio transmit
	if(set_stream_audio_info(recv_handle) < 0)
	{
		nslog(NS_ERROR,"SET STREAM AUDIO IS ERROR!\n");
	}

	nslog(NS_INFO, "set_room_info success");
    return OPERATION_SUCC;
}


static int32_t  close_stream_sock(stream_handle *stream_hand)
{

    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    int32_t sockfd = stream_hand->sockfd;

    if (stream_hand->sockfd > 0) {
        pthread_mutex_lock(&stream_hand->mutex);
        stream_hand->sockfd = CLOSE_SOCK;
        pthread_mutex_unlock(&stream_hand->mutex);
        close_socket(sockfd);
    }
    nslog(NS_INFO, "i will close %d stream, stream_id = %d", stream_hand->stream_id, stream_hand->stream_id);

    return OPERATION_SUCC;
}

static int32_t  close_room_connect(void *recv_hand)
{
    int32_t index = 0;
    recv_room_handle *recv_handle = (recv_room_handle *)recv_hand;
    if (NULL == recv_handle ) {
        nslog(NS_ERROR, "recv_handle = %p", recv_handle);
        return OPERATION_ERR;
    }
    for (index = 0; index < recv_handle->stream_num; index++) {

        recv_handle->close_stream_connect(&recv_handle->stream_hand[index]);
        usleep(CLOSE_CONNECT_DELAY);
    }
    return OPERATION_SUCC;
}

static int32_t  open_room_connect(void *recv_hand)
{
    int32_t index = 0;
    recv_room_handle *recv_handle = (recv_room_handle *)recv_hand;
    if (NULL == recv_handle ) {
        nslog(NS_ERROR, "recv_handle = [%p]", recv_handle);
        return OPERATION_ERR;
    }
    for (index = 0; index < recv_handle->stream_num; index++) {

        if (0 != recv_handle->stream_hand[index].port && (index + 1) == recv_handle->stream_hand[index].stream_id  &&
                0 !=r_strlen(recv_handle->stream_hand[index].ipaddr)) {
            recv_handle->open_stream_connect(&recv_handle->stream_hand[index]);
        }
    }
    return OPERATION_SUCC;
}

static int32_t  close_stream_connect(stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    int32_t sockfd = stream_hand->sockfd;

    pthread_mutex_lock(&stream_hand->status_mutex);
    stream_hand->status = DISCONNECT;
    pthread_mutex_unlock(&stream_hand->status_mutex);

    if (stream_hand->sockfd > 0) {
        pthread_mutex_lock(&stream_hand->mutex);
        stream_hand->sockfd = CLOSE_SOCK;
        pthread_mutex_unlock(&stream_hand->mutex);
        close_socket(sockfd);
    }
		
	 nslog(NS_INFO, "close_stream_connect success status = %d, stream_id = %d", stream_hand->status, stream_hand->stream_id);
    return OPERATION_SUCC;
}

static int32_t  open_stream_connect(stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    pthread_mutex_lock(&stream_hand->status_mutex);
    stream_hand->status = CONNECT;
    pthread_mutex_unlock(&stream_hand->status_mutex);
	 nslog(NS_INFO, "open_stream_connect success status = %d, stream_id = %d", stream_hand->status, stream_hand->stream_id);
    return OPERATION_SUCC;
}

static int32_t  get_stream_connect(stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if(stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
        return stream_hand->recv_pri.status;
    }
    return OPERATION_SUCC;
}

int32_t  get_stream_status(stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if(stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
        return stream_hand->status;
    }
    return OPERATION_SUCC;
}

static int32_t set_recv_to_ctrl_msgid(int32_t msgid, stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }
    if(msgid >= 0 && (stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM))
    {
        stream_hand->msg_recv_to_ctrl = msgid;
		 nslog(NS_INFO, "set msgid success msgid = %d, msg_recv_to_ctrl = %d, stream_id = %d", msgid, stream_hand->msg_recv_to_ctrl, stream_hand->stream_id);
    }
    return OPERATION_SUCC;
}


static int32_t set_recv_to_live_msgid(int32_t msgid, stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }
    if(msgid >= 0 && (stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM))
    {
        stream_hand->msg_recv_to_live = msgid;
		nslog(NS_INFO, "set msgid success msgid = %d, msg_recv_to_live = %d, stream_id = %d", msgid, stream_hand->msg_recv_to_live, stream_hand->stream_id);
    }
    return OPERATION_SUCC;
}

static int32_t set_recv_to_rec_msgid(int32_t msgid, stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if(msgid >= 0 && (stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM))
    {
        stream_hand->msg_recv_to_rec = msgid;
    }
    return OPERATION_SUCC;
}

static int32_t set_recv_to_usb_rec_msgid(int32_t msgid, stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if(msgid >= 0 && (stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM))
    {
        stream_hand->msg_recv_to_usb_rec = msgid;
    }
    return OPERATION_SUCC;
}


static int32_t get_stream_socket(stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if(stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
        return (stream_hand->sockfd);
    }
    else
    {
        return OPERATION_ERR;
    }
}

static int32_t set_rec_status(stream_handle *stream_hand, uint8_t status)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if( stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
        stream_hand->rec_status = status;
        nslog(NS_INFO, "set_rec_status success, status= %d, stream_id = %d", status, stream_hand->stream_id);
    }
    return OPERATION_SUCC;
}

int32_t get_rec_status(stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if(stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
        return stream_hand->rec_status;
    }

    return OPERATION_SUCC;
}


static int32_t set_usb_rec_status(stream_handle *stream_hand, uint8_t status)  //zl
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
		
        return OPERATION_ERR;
    }

    if( stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
    	
        stream_hand->usb_rec_status = status;
        nslog(NS_INFO, "set_usb_rec_status success, status= %d, stream_id = %d", status, stream_hand->stream_id);
    }
	
    return OPERATION_SUCC;
}

int32_t get_usb_rec_status(stream_handle *stream_hand)    //zl
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if(stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
        return stream_hand->usb_rec_status;
    }

    return OPERATION_SUCC;

}
static int32_t set_live_status(stream_handle *stream_hand, uint8_t status)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if( stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
        stream_hand->live_status = status;
        nslog(NS_INFO, "set_live_status success, status= %d, stream_id = %d", status, stream_hand->stream_id);
    }

    return OPERATION_SUCC;
}

int32_t get_live_status(stream_handle *stream_hand)
{
    if (NULL == stream_hand) {
        nslog(NS_ERROR, "stream_hand = [%p]", stream_hand);
        return OPERATION_ERR;
    }

    if(stream_hand->stream_id > 0 || stream_hand->stream_id < MAX_STREAM)
    {
        return stream_hand->live_status;
    }

    return OPERATION_SUCC;
}

/*==============================================================================
  函数: <receive_connect_server>
  功能: <接收模块连接服务器>
  参数: uint8_t *addr, int32_t port
  返回值: 成功返回int32_t sockfd, 失败返回非零ret
  Created By liuchsh 2012.11.16 14:30:18 For EDU
  ==============================================================================*/
static int32_t receive_connect_server(stream_handle *M_handle)
{
    int32_t sockfd, ret = OPERATION_ERR;
    struct sockaddr_in servaddr;
    int optval = 1;// set open keepalive,value 0 is close it.
    int32_t  keepidle = 5;//it will send test data when five seconds isn't receive data;
    int32_t keepintvl = 1; //the interval of each packages.
    int32_t keepcnt = 3; //if it is not responding, it will send 3 times data, so it can test server connect or disconnect.
    socklen_t optlen = sizeof(optval);
    int32_t rcvbuflen;
    int32_t buflen = RECV_BUF;
    socklen_t  rcvbufsize = sizeof(rcvbuflen), bufsize = sizeof(buflen);
    int32_t nreuseaddr = 0;
    int32_t sock_flag = 0;

    if (NULL == M_handle) {
        nslog(NS_ERROR, "M_handle = [%p]", M_handle);
        return OPERATION_ERR;
    }

    struct timeval recv_time; //  set recv  overtime
    recv_time.tv_sec = 6;// it will close socket if recv time cost more than 6 seconds
    recv_time.tv_usec = 0;

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(struct sockaddr_in));

//	nslog(NS_ERROR,"ip:%s  --------port : %d ---- stream_id : %d\n",M_handle->ipaddr,M_handle->port,M_handle->stream_id);
    servaddr.sin_family = PF_INET;
    servaddr.sin_port = htons(M_handle->port);
    servaddr.sin_addr.s_addr = inet_addr(M_handle->ipaddr);

    if(r_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&nreuseaddr, sizeof(int32_t)))
    {
        nslog(NS_ERROR, "setsockopt()  set recv reuseaddr error, errno = %d", errno);
        return OPERATION_ERR;
    }

    if(r_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (int8_t *)&recv_time, sizeof(struct timeval)))
    {
        nslog(NS_ERROR, "setsockopt()  set recv overtime  error, errno = %d", errno);
        return OPERATION_ERR;
    }

    if (r_setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&buflen , bufsize) < 0)
    {
        nslog(NS_INFO, "Setsockopt error%d\n", errno);
    }

#if 1
    if(r_setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
    {
        nslog(NS_ERROR, "setsockopt()");
        return OPERATION_ERR;
    }

    if(r_setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) < 0)
    {
        nslog(NS_ERROR, "setsockopt()");
        return OPERATION_ERR;
    }

    if(r_setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) < 0)
    {
        nslog(NS_ERROR, "setsockopt()");
        return OPERATION_ERR;
    }

    if(r_setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt)) < 0)
    {
        nslog(NS_ERROR, "setsockopt()");
        return OPERATION_ERR;
    }

    if (r_getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuflen , (socklen_t *)&rcvbufsize) < 0)
    {
        nslog(NS_INFO, "Setsockopt error%d\n", errno);
    }
#endif
    ret = r_connect(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));
    if(ret < 0) {
        r_close(sockfd);
        sockfd = CLOSE_SOCK;
        return ret;
    }
    return sockfd;
}

/*==============================================================================
  函数: <receive_parse_stream>
  功能: <分析处理流数据>
  参数: MSGHEAD *msg, stream_handle *M_handle
  返回值: 成功返回0，失败返回小于0.
  Created By liuchsh 2012.11.16 15:02:22 For EDU
  ==============================================================================*/
// add zl
static int32_t receive_parse_stream(MSGHEAD *msg, stream_handle *M_handle, void *in_data, int32_t in_data_len, udp_send_module_handle *p_udp_hand)
{
    int32_t return_code = OPERATION_ERR;
    int32_t nRet = 0;                   //从socket收到返回的字节数.
    m_msgque m_meeting_msgque;
    uint8_t *r_xml_msg_buf = NULL;
    int32_t msg_len = 0;
	int32_t msg_nMsg = msg->nMsg;

	msg_header_t auto_req_msg;
	int8_t auto_req_buf[XML_MSG_LEN] ={0};
	int32_t auto_req_len = 0;

//	nslog(NS_ERROR,"MSG : %d\n",msg_nMsg);
    //assert(M_handle);
    r_memset(&m_meeting_msgque, 0, MsgqueLen);

    //判断消息类型是否是数据或xml，再分别处理。
    if(HIGH_STREAM == msg_nMsg || LOW_STREAM  == msg_nMsg ||
            JPG_STREAM == msg_nMsg || AUDIO_STREAM == msg_nMsg ||
            LOW_STREAM_T == msg_nMsg)
    {
    //	nslog(NS_ERROR ,"SHIRT ----1 --------- %d \n",M_handle->stream_id);
        if (HIGH_STREAM == msg_nMsg) {
            M_handle->recv_pri.stream_type = 0;
            M_handle->recv_pri.data_type = R_VIDEO;
        } else if (LOW_STREAM  == msg_nMsg  || LOW_STREAM_T == msg_nMsg){
            M_handle->recv_pri.stream_type = 1;
            M_handle->recv_pri.data_type = R_VIDEO;
        }else if (JPG_STREAM == msg_nMsg) {
            M_handle->recv_pri.stream_type = 2;
            M_handle->recv_pri.data_type = R_JPEG;
        }

        M_handle->recv_pri.is_data = 1;
		 M_handle->recv_pri.to_live = 0;
		 M_handle->recv_pri.to_rec = 0;
        return_code = send_media_frame_data(M_handle, in_data, in_data_len, p_udp_hand);
        if (return_code < 0)
        {
            nslog(NS_ERROR, "send msg to live and record error return_code = %d", return_code);
            goto EXIT;
        }
    }
    else if ( XML_TYPE == msg_nMsg)
    {
    	
 //   	nslog(NS_WARN,"ip:[%s] streamid:[%d] eid:[%d] len:[%d][%s]\n",
//			M_handle->ipaddr, M_handle->stream_id, M_handle->enc_id,
//			in_data_len, in_data);

		 if (in_data_len > XML_MSG_LEN) {
				nslog(NS_WARN, "in_data_len = %d, XML_MSG_LEN = %d, msg_nMsg = %d,stream_id = %d", in_data_len, XML_MSG_LEN, msg_nMsg, M_handle->stream_id);
				return_code = OPERATION_SUCC;
				goto EXIT;
		 }
		 
        r_xml_msg_buf = (uint8_t *)r_malloc(in_data_len + MsgLen + 1);
        if (NULL == r_xml_msg_buf )
        {
            nslog(NS_ERROR, "malloc live buf");
            return_code = RECEIVE_MALLOC_ERR;
            goto EXIT;
        }	

        r_memset(r_xml_msg_buf, 0, in_data_len + MsgLen + 1);

#if 0
        nslog(NS_DEBUG, "===============start===========\n%s\n", in_data);
        nslog(NS_DEBUG, "==========end========\ntype = 0x%x, id = %d", msg->nMsg, M_handle->stream_id);
        if (r_strlen(in_data) <= 0) {
            nslog(NS_DEBUG, "in_data = \n%s\n", in_data);
            while (1) sleep(1);
        }
#endif

        msg_len = htons(in_data_len + MsgLen);
        r_memcpy(r_xml_msg_buf, msg, MsgLen);
//		nslog(NS_ERROR ,"SHIRT : %s \n",in_data);
        r_memcpy(r_xml_msg_buf + MsgLen, in_data, in_data_len);

        m_meeting_msgque.msgtype = M_handle->stream_id; //the msg type of sending meeting room.
        m_meeting_msgque.msgbuf = r_xml_msg_buf;
        r_xml_msg_buf = NULL;

        if (FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT)) {
					
            return_code = recv_xml_login_response(M_handle, m_meeting_msgque.msgbuf + MsgLen);		
#if 0  //注释上报30087达到最大连接数
   			  if (MAX_CONNECT == return_code) {
                    return_code = r_msg_send(M_handle->msg_recv_to_ctrl, &m_meeting_msgque, MsgqueLen - sizeof(int64_t), IPC_NOWAIT);
                    if (0 > return_code)
                    {
                        if (FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, CTRL_BIT)) { 
                            nslog(NS_WARN, "r_msg_send to meeting ctrl error, errno = %d, stream_id = %d", errno, M_handle->stream_id);
                            SET_FLAG_BIT_TRUE(M_handle->log_flag, CTRL_BIT);
                        }
												
                        if (m_meeting_msgque.msgbuf) {
                            r_free(m_meeting_msgque.msgbuf);
                            m_meeting_msgque.msgbuf = NULL;
                        }
												
							return_code = OPERATION_ERR;
             			   goto EXIT;
                    }
								
                    SET_FLAG_BIT_FALSE(M_handle->log_flag, CTRL_BIT);
						return_code = OPERATION_SUCC;
             			goto EXIT;
										
                }else if (OPERATION_SUCC != return_code) {
                     if (m_meeting_msgque.msgbuf) {
                            r_free(m_meeting_msgque.msgbuf);
                            m_meeting_msgque.msgbuf = NULL;
                        }
							return_code = OPERATION_ERR;
             			   goto EXIT;
                }

#endif
            if (OPERATION_SUCC != return_code) { 						//如果用户名和密码错误，此处还待处理。
                if (m_meeting_msgque.msgbuf) {
                    r_free(m_meeting_msgque.msgbuf);
                    m_meeting_msgque.msgbuf = NULL;
                }
                goto EXIT;
            }

            recv_report_xml(CONNECT, M_handle);
            nslog(NS_INFO, "%d stream connect %s encode success, sockfd = %d", M_handle->stream_id, M_handle->ipaddr, M_handle->sockfd);
            M_handle->recv_pri.status = 1;
            SET_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT);

#if 1
			//add zl login 编码器成功后置位
			M_handle->enc_login_flag = 1;
			
#endif 		
		

			
#if 0
			// add zl 连接编码器后自动请求码流
			r_memset(auto_req_buf, 0, XML_MSG_LEN);
			auto_req_len = 0;
			room_auto_send_req(&(auto_req_buf[MsgLen]),&auto_req_len,M_handle->stream_id%2,M_handle->stream_id);
			nslog(NS_ERROR ,"GOD --- AUTO_REQ :%s	  --- IP <%s>  stream_id :%d\n",&auto_req_buf[MsgLen],M_handle->ipaddr,M_handle->stream_id);
			auto_req_msg.m_len = htons(auto_req_len + MsgLen);
			auto_req_msg.m_ver = htons(ENC_VER);
			auto_req_msg.m_msg_type = XML_TYPE;
			
			r_memcpy(auto_req_buf, &auto_req_msg, MsgLen);

			pthread_mutex_lock(&M_handle->mutex);
			if(r_send(M_handle->sockfd, auto_req_buf, auto_req_len + MsgLen, 0)< 0)
			{
				nslog(NS_ERROR, "r_send AUTO_REQ error, errno = %d, stream_id = %d", errno, M_handle->stream_id);
				return_code = LOSE_CONNECT;
				pthread_mutex_unlock(&M_handle->mutex);
				goto EXIT;
			}
			pthread_mutex_unlock(&M_handle->mutex);
#endif 
        }

#if RECV_KEEPLIVE      //心跳机制
        return_code = recv_xml_analyze_msgcode(M_handle, m_meeting_msgque.msgbuf + MsgLen);
        if (FALSE == M_handle->alive_flag) {
            pthread_mutex_lock(&M_handle->alive_mutex);
            M_handle->alive_flag = (uint8_t)return_code;		 
            pthread_mutex_unlock(&M_handle->alive_mutex);
        }
#endif

//		nslog(NS_ERROR,"SHIRT_FUCK ---- %s     -%d\n", m_meeting_msgque.msgbuf + MsgLen,M_handle->msg_recv_to_ctrl);
        return_code = r_msg_send(M_handle->msg_recv_to_ctrl, &m_meeting_msgque, MsgqueLen - sizeof(long), IPC_NOWAIT);

        if (0 > return_code)
        {
            if (FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, CTRL_BIT)) { 
                nslog(NS_WARN, "r_msg_send to meeting ctrl error, errno = %d, stream_id = %d", errno, M_handle->stream_id);
                SET_FLAG_BIT_TRUE(M_handle->log_flag, CTRL_BIT);
            }
            if (m_meeting_msgque.msgbuf)
            {
                r_free(m_meeting_msgque.msgbuf);
                m_meeting_msgque.msgbuf = NULL;
            }
            return_code = OPERATION_SUCC;
            goto EXIT;
        }
        SET_FLAG_BIT_FALSE(M_handle->log_flag, CTRL_BIT);
    }
    return_code = OPERATION_SUCC;
EXIT:
    return return_code;
}



int32_t recv_sleep_second(stream_handle *p_handle, int32_t count)
{
    int32_t i = 0;
    int32_t second = count * 10;

    if (NULL == p_handle) {
        nslog(NS_ERROR, "p_handle = %p", p_handle);
        r_sleep(count);
    }

    int32_t status = get_stream_status(p_handle);
    for (i = 0; i < second; i++) {
        if (status == get_stream_status(p_handle) && TRUE == p_handle->pthread_status) {
            r_usleep(SLEEP_DELAY);
        }else {
            break;
        }
    }
    return OPERATION_SUCC;
}


static int32_t recv_enc_tcp_data(stream_handle *p_handle, void *buf, int32_t buflen)
{
    int32_t total_recv = 0;
    int32_t recv_len = 0;

    while(total_recv < buflen) {

        recv_len = r_recv(p_handle->sockfd, buf + total_recv, buflen - total_recv, 0);
        if(recv_len < 1 && EINTR != errno) {
            nslog(NS_ERROR, "recv tcp data failed,error message:%s, recv_len = %d, stream_id = %d\n", strerror(errno), recv_len, p_handle->stream_id);

            if(0 == recv_len) {
                return 0;
            }

            return -1;
        }else if (recv_len < 1 && EINTR == errno){
 			  nslog(NS_WARN, "recv tcp data failed, error message:%s, stream_id = %d", strerror(errno), p_handle->stream_id);
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



/*==============================================================================
  函数: <room_receive_deal>
  功能: <处理服务器(编码器)发来的数据。>
  参数: void *arg
  返回值:  无
  Created By liuchsh 2012.11.16 14:35:03 For EDU
  ==============================================================================*/

static void *room_receive_deal(void *arg)
{

    int32_t  return_code = 0;
    int32_t  out_len = 0;
    msg_header_t login_msg;
    int8_t login_buf[XML_MSG_LEN] = {0};
	int8_t send_buf[XML_MSG_LEN + MsgLen] = {0};
    recv_xml_handle recv_xml_hand;
    int32_t sockfd_flag = 0;
    stream_handle *M_handle = (stream_handle *)arg;
    int32_t data_flag = 0, wait_flag = 0, err_flag = 0 ,ver_flag = 0;
    int32_t data_len = 0;
    int8_t *in_data = NULL;
    uint8_t first_login_report = 0;
    int32_t delay_connect_count = 0;
    int32_t ntohs_msglen = 0;
    int32_t ntohs_msgver = 0;
    int32_t connect_fail_count = -1; 
	int32_t connect_log_flag = 0;
	// add zl
	udp_send_module_handle   *udp_send_module_hand = NULL;
	int32_t udp_port 					= 0;

    fd_set readfs;
    struct timeval st;
    int32_t  nRet = -1;
    MSGHEAD msg;
	// add zl
	stream_send_cond_t src;
	stream_recv_cond_t recv_cond;
	udp_recv_handle *p_recv_hand = NULL;

	int32_t login_send_falg = 0;

	int32_t debug_print_num = 0;
	
    if(NULL == M_handle)
    {
        nslog(NS_ERROR, "M_handle = [%p]", M_handle);
        return_code = OPERATION_ERR;
        goto EXIT;
    }

    in_data = (int8_t *)r_malloc(MAX_TCP_PACKAGE);
    if (NULL == in_data){
        nslog(NS_ERROR, "r_malloc can't aloocate enough memory");
        return NULL;
    }
    r_memset(in_data, 0, MAX_TCP_PACKAGE);
    //r_pthread_detach(r_pthread_self());     //用了pthread_join  这个就不能用了.

	nslog(NS_INFO,"CONNECT_ENC <ENC_IP :%s> <SRM_ID : %d> <PORT : %d>  <PUSH_PORT :%d><PULL_PORT %d> <PUSH_IP :%s> <PUSH_MODE: %d> <PULL_MODE :%d>\n",
		M_handle->ipaddr,
		M_handle->stream_id,
		M_handle->port,
		M_handle->push_udp_port,
		M_handle->pull_udp_port,
		M_handle->push_ip,
		M_handle->push_module,
		M_handle->pull_module);
	
    while(M_handle->pthread_status)
    {
		
    	// add zl
		login_send_falg = 0;
		M_handle->enc_login_flag = 0;
		M_handle->pull_module_flag = 0;
		M_handle->push_module_flag = 0;
		
		#if 0
			// zl quesion???
			if (NULL != p_recv_hand){
			 	if (DISCONNECT == get_stream_status(M_handle) && STREAM_START == p_recv_hand->stream_status){
					udp_recv_pause(p_recv_hand);
			 	}

				if (START_CONNECT == get_stream_status(M_handle) && STREAM_START != p_recv_hand->stream_status){
					udp_recv_start(p_recv_hand);
			 	}
		 }
		#endif	
		
      	if(START_CONNECT == get_stream_status(M_handle))
        {
        
#if 0
		 if (PUSH_IN_UDP == M_handle->push_module && M_handle->push_module_flag == 0){
					 udp_port = M_handle->pull_udp_port;
					 src.video_port = M_handle->push_udp_port;
					 src.audio_port = M_handle->push_udp_port;
					 r_memcpy(src.ip,M_handle->push_ip,IP_LEN);
					 udp_send_module_hand = UdpSend_init(&src);
					 M_handle->push_module_flag = 1;
		 }
		 // add zl
		 if (PULL_IN_UDP == M_handle->pull_module && M_handle->pull_module_flag == 0){
				recv_cond.port = M_handle->pull_udp_port;
				recv_cond.func = recv_deal_udp_recv_data;
				recv_cond.user_data = M_handle;
				// add zl question eth1 ???
				get_local_ip("eth1",recv_cond.ip);
				p_recv_hand = udp_recv_init(&recv_cond);
				udp_recv_start(p_recv_hand);
				M_handle->pull_module_flag = 1;
		 }
 #endif
            M_handle->sockfd = receive_connect_server(M_handle);
            if (M_handle->sockfd < 0) {
                return_code = CONNECT_ERROR;
		//		  nslog(NS_ERROR, "receive_connect_server error, stream_id = %d", M_handle->stream_id);

                goto EXIT;
            }
			nslog(NS_INFO,"FUCK-CONNET IS OK!   IP : %s   ---- PORT : %d   --STREAM_ID : %d\n",
				M_handle->ipaddr,M_handle->port,M_handle->stream_id,M_handle->port);
			r_usleep(SLEEP_DELAY);
			#if 0
            r_memset(&recv_xml_hand, 0, sizeof(recv_xml_handle));
            snprintf(recv_xml_hand.msgcode, r_strlen(ENC_MSGCODE) + 1, "%s", ENC_MSGCODE);
            snprintf(recv_xml_hand.passkey, r_strlen(ENC_PASSKEY) + 1, "%s", ENC_PASSKEY);
            snprintf(recv_xml_hand.user, r_strlen(ENC_USER) + 1, "%s", ENC_USER);
            snprintf(recv_xml_hand.password, r_strlen(ENC_PASSWORD) + 1, "%s", ENC_PASSWORD);

            r_memset(login_buf, 0, XML_MSG_LEN);
            r_memset(&login_msg, 0, MsgLen);
            receive_xml_login(&recv_xml_hand, login_buf, &out_len);
			nslog(NS_ERROR ,"GOD --- LOGIN :%s    --- IP <%s>  stream_id :%d\n",login_buf,M_handle->ipaddr,M_handle->stream_id);
            login_msg.m_len = htons(out_len + MsgLen);
            login_msg.m_ver = htons(ENC_VER);
            login_msg.m_msg_type = XML_TYPE;

            r_memcpy(send_buf, &login_msg, MsgLen);
            r_memcpy(send_buf + MsgLen, login_buf, out_len);

            if (M_handle->sockfd >= 0 && START_CONNECT == get_stream_status(M_handle))
            {

                pthread_mutex_lock(&M_handle->mutex);
                return_code = r_send(M_handle->sockfd, send_buf, out_len + MsgLen, 0);
                if (return_code < 0)
                {
                    nslog(NS_ERROR, "r_send login START_CONNECT error, errno = %d, stream_id = %d", errno, M_handle->stream_id);
                    return_code = LOSE_CONNECT;
                    pthread_mutex_unlock(&M_handle->mutex);
                    goto EXIT;
                }
                pthread_mutex_unlock(&M_handle->mutex);
            }
			
            else
            {
                nslog(NS_ERROR, "sockfd = %d, status = %d, stream_id = %d", M_handle->sockfd,  get_stream_status(M_handle), M_handle->stream_id);
                return_code = LOSE_CONNECT;
                goto EXIT;
            }
			#endif
        }
        else
        {
            //nslog(NS_NOTICE, "Meeting control not set connect status, I would not connect encode stream_id = %d", M_handle->stream_id);
            recv_sleep_second(M_handle, LOOP_DELAY);
            continue;
        }

        sockfd_flag = 0;
        data_flag = 0;
        err_flag = 0;
        wait_flag = 0;
        data_flag = 0;
        ver_flag = 0;
        M_handle->log_flag = 0;
        M_handle->offset = 0;
        M_handle->audio_offset = 0;

	
#if 1
	 if (PUSH_IN_UDP == M_handle->push_module && M_handle->push_module_flag == 0){
				 udp_port = M_handle->pull_udp_port;
				 src.video_port = M_handle->push_udp_port;
				 src.audio_port = M_handle->push_udp_port;
				 r_memcpy(src.ip,M_handle->push_ip,IP_LEN);
				 udp_send_module_hand = UdpSend_init(&src);
				 M_handle->push_module_flag = 1;
	 }
	//nslog(NS_WARN ,
	 // add zl
	 if (PULL_IN_UDP == M_handle->pull_module && M_handle->pull_module_flag == 0){
			recv_cond.port = M_handle->pull_udp_port;
			recv_cond.func = recv_deal_udp_recv_data;
			recv_cond.user_data = M_handle;
			// add zl question eth1 ???
			get_local_ip("eth1",recv_cond.ip);

//			printf("---recv_cond.port=%x.\n",recv_cond.port);
			p_recv_hand = udp_recv_init(&recv_cond);
			udp_recv_start(p_recv_hand);
			M_handle->pull_module_flag = 1;
	 }
 #endif

        // add zl  !!!

        while (START_CONNECT == get_stream_status(M_handle))
        {
        //	nslog(NS_ERROR,"FUCK 2---- IP :%s  streamid :%d\n",M_handle->ipaddr,M_handle->stream_id);
            st.tv_sec = 0;
            st.tv_usec = RECV_SELECT_DELAY;
			if(login_send_falg == 0)
			{
#if 1
				r_memset(&recv_xml_hand, 0, sizeof(recv_xml_handle));
				snprintf(recv_xml_hand.msgcode, r_strlen(ENC_MSGCODE) + 1, "%s", ENC_MSGCODE);
				snprintf(recv_xml_hand.passkey, r_strlen(ENC_PASSKEY) + 1, "%s", ENC_PASSKEY);
				snprintf(recv_xml_hand.user, r_strlen(ENC_USER) + 1, "%s", ENC_USER);
				snprintf(recv_xml_hand.password, r_strlen(ENC_PASSWORD) + 1, "%s", ENC_PASSWORD);

				// add zl 设定高清版同步时间只有高清版发送
				
				
				r_memset(login_buf, 0, XML_MSG_LEN);
				r_memset(&login_msg, 0, MsgLen);
				receive_xml_login(&recv_xml_hand, login_buf, &out_len,M_handle->enc_id);
				nslog(NS_ERROR ,"GOD --- LOGIN :%s	  --- IP <%s>  stream_id :%d   enc_id : %d\n",login_buf,M_handle->ipaddr,M_handle->stream_id,M_handle->enc_id);
				login_msg.m_len = htons(out_len + MsgLen);
				login_msg.m_ver = htons(ENC_VER);
				login_msg.m_msg_type = XML_TYPE;
				
				r_memcpy(send_buf, &login_msg, MsgLen);
				r_memcpy(send_buf + MsgLen, login_buf, out_len);
				
				if (M_handle->sockfd >= 0 && START_CONNECT == get_stream_status(M_handle))
				{
				
					pthread_mutex_lock(&M_handle->mutex);
					return_code = r_send(M_handle->sockfd, send_buf, out_len + MsgLen, 0);
					if (return_code < 0)
					{
						nslog(NS_ERROR, "r_send login START_CONNECT error, errno = %d, stream_id = %d", errno, M_handle->stream_id);
						return_code = LOSE_CONNECT;
						pthread_mutex_unlock(&M_handle->mutex);
						goto EXIT;
					}
					pthread_mutex_unlock(&M_handle->mutex);
				}
     		   else
	            {
	                nslog(NS_ERROR, "sockfd = %d, status = %d, stream_id = %d", M_handle->sockfd,  get_stream_status(M_handle), M_handle->stream_id);
	                return_code = LOSE_CONNECT;
	                goto EXIT;
	            }
			   login_send_falg = 1;
			}
#endif 

			#if 0
			 // zl question???
			 if (NULL != p_recv_hand){
			 	if (DISCONNECT == get_stream_status(M_handle) && STREAM_START == p_recv_hand->stream_status){
					udp_recv_pause(p_recv_hand);
			 	}

				if (START_CONNECT == get_stream_status(M_handle) && STREAM_START != p_recv_hand->stream_status){
					udp_recv_start(p_recv_hand);
			 	}
		 	}
			#endif

            FD_ZERO(&readfs);
            if (M_handle->sockfd >= 0 && START_CONNECT == get_stream_status(M_handle))
            {
                FD_SET(M_handle->sockfd, &readfs);
                r_memset(&msg, 0, MsgLen);
                return_code = r_select(M_handle->sockfd + 1, &readfs, NULL, NULL, &st);
                if (return_code < 0)
                {
                    nslog(NS_ERROR, "select error! errno = %d", errno);
                    return_code = LOSE_CONNECT;
                    goto EXIT;
                } else if (0 == return_code) {

                    usleep(20000);
                    if (FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT)) {
						nslog(NS_ERROR,"FUCK OOXX---- IP :%s  streamid :%d\n",M_handle->ipaddr,M_handle->stream_id);
						// add zl question
					//      goto EXIT;
                    } 
                }
            }
            else
            {
                nslog(NS_ERROR, "sockfd = %d, status = %d, stream_id = %d", M_handle->sockfd,  get_stream_status(M_handle), M_handle->stream_id);
                return_code = LOSE_CONNECT;
                goto EXIT;
            }

            if (M_handle->sockfd >= 0 && FD_ISSET(M_handle->sockfd, &readfs))
            {

                nRet = recv_enc_tcp_data(M_handle, &msg, MsgLen);

                ntohs_msglen  = ntohs(msg.nLen);
                ntohs_msgver = ntohs(msg.nVer);

                if (nRet <= 0 || 0 >= ntohs_msglen)
                {
                    if (0 == err_flag && TRUE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT)) {
                        nslog(NS_ERROR, "receive and encode is disconnect, nRet = %d, ntohs(msg.nLen)= %d ,msg.type = %d, errno = %d, stream_id = %d", nRet,  ntohs_msglen, msg.nMsg, errno, M_handle->stream_id);
                    }
                    return_code = LOSE_CONNECT;
					
	//				nslog(NS_ERROR,"FUCK 3---- IP :%s  streamid :%d\n",M_handle->ipaddr,M_handle->stream_id);
                    goto EXIT;
                }							
			
				data_len = ntohs_msglen - MsgLen;
//              	nslog(NS_DEBUG, "recv encode type = %d,ver : %d, msg_len = %d, stream_id = %d", msg.nMsg,ntohs_msgver, data_len, M_handle->stream_id);

				nRet = recv_enc_tcp_data(M_handle, in_data, data_len);
                if (nRet <  data_len || data_len < 0)
                {
                    nslog(NS_ERROR, "receive and encode is disconnect,msg.ver = %d errno = %d, ntohs(msg.nLen) = %d, data_len = %d, stream_id = %d  Ret %d", ntohs_msgver, errno, ntohs_msglen, data_len, M_handle->stream_id,nRet);
                    return_code = LOSE_CONNECT;
                    goto EXIT;
                }
                if (ntohs_msgver != ENC_VER) {
                    if (!ver_flag) {
                        nslog(NS_WARN, "I receive tcp data, msg.ver = %d, msg.nLen = %d, msg.type = %d, stream_id = %d", ntohs_msgver, ntohs_msglen, msg.nMsg, M_handle->stream_id);
                        ver_flag = 1;
                    }
                    usleep(10000);
                    continue;
                }

                if (!data_flag && TRUE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT)) {
                    data_flag = 1;
                    nslog(NS_NOTICE, "I receive tcp data, msg.nLen = %d, msg.type = %d, stream_id = %d", ntohs_msglen, msg.nMsg, M_handle->stream_id);
                }

                ver_flag = 0;
				// add zl
				pthread_mutex_lock(&M_handle->recv_pri.print_mutex);
                return_code = receive_parse_stream(&msg, M_handle, in_data, data_len, udp_send_module_hand);
				pthread_mutex_unlock(&M_handle->recv_pri.print_mutex);
				  if (OPERATION_SUCC  != return_code)
                {
                    if (TRUE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT)) {
                        nslog(NS_WARN, "receive_parse_stream return_code = %d, stream_id = %d", return_code, M_handle->stream_id);
                    } else {
                        connect_fail_count++;
                        if	(0 == connect_fail_count) {
                            nslog(NS_INFO, "%d stream connect %s encode failed, sockfd = %d", M_handle->stream_id, M_handle->ipaddr, M_handle->sockfd);
                            connect_fail_count = -5;
                        }
                    }
                    goto EXIT;
                }

               // r_memset(in_data, 0, msg.nLen);              
               r_memset(in_data, 0, MAX_TCP_PACKAGE);
				  first_login_report = 1;
            }else {            	

                if (!wait_flag) {
                    nslog(NS_DEBUG, "%d stream is live, waiting data", M_handle->stream_id);
                    wait_flag = 1;
                }
                //memset_pri_status(&M_handle->recv_pri);
            }
        }
EXIT:
		debug_print_num ++;
    	 pthread_mutex_lock(&M_handle->mutex);
        if (M_handle->sockfd >= 0) {
            close_socket(M_handle->sockfd);
            M_handle->sockfd = CLOSE_SOCK;
        }

		M_handle->av_connect_flag = 1;

		 if (DISCONNECT == M_handle->status) {
				M_handle->sockfd = 0;
		 }
        pthread_mutex_unlock(&M_handle->mutex);

		if(debug_print_num == 10 )
		{
			
			nslog(NS_INFO,"RECONNECT_ENC <ENC_IP :%s> <SRM_ID : %d> <PORT : %d>  <PUSH_PORT :%d><PULL_PORT %d> <PUSH_IP :%s> <PUSH_MODE: %d> <PULL_MODE :%d>\n",
				M_handle->ipaddr,
				M_handle->stream_id,
				M_handle->port,
				M_handle->push_udp_port,
				M_handle->pull_udp_port,
				M_handle->push_ip,
				M_handle->push_module,
				M_handle->pull_module);
			debug_print_num = 0 ;
		}

		// add zl
		if (NULL != udp_send_module_hand)
		{
			UdpSend_deinit(udp_send_module_hand);
			udp_send_module_hand = NULL;
		}
		if(NULL != p_recv_hand)
		{
			udp_recv_deinit(p_recv_hand);
			p_recv_hand = NULL;
		}
		M_handle->enc_login_flag = 0;
		M_handle->pull_module_flag = 0;
		M_handle->push_module_flag = 0;


        if (LOSE_CONNECT == return_code && 0 == sockfd_flag && 
                TRUE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT))
        {
            sockfd_flag = 1;
			 nslog(NS_ERROR, "first_login_report return_code = %d, stream_id = %d", return_code, M_handle->stream_id);
            recv_report_xml(DISCONNECT, M_handle);//上报连接失败消息。
        }

        if (0 == first_login_report ||(CONNECT_ERROR == return_code && 0 == connect_log_flag)) {
            first_login_report = 1;
			  connect_log_flag = 1;
			  nslog(NS_ERROR,"IP :%s   ------- port :%d\n",M_handle->ipaddr,M_handle->port);
			  nslog(NS_ERROR, "first_login_report return_code = %d, stream_id = %d", return_code, M_handle->stream_id);
            recv_report_xml(DISCONNECT, M_handle);
        }

        delay_connect_count++;   //延长重连时间.

        r_memset(&(M_handle->recv_pri), 0, sizeof(recv_print));
        r_memset(in_data, 0, MAX_TCP_PACKAGE);
        M_handle->recv_pri.status = 0;

        if (!err_flag && DISCONNECT != M_handle->status && TRUE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT)) {
            nslog(NS_ERROR, "waring, errors appear, I need connect server again, stream_id = %d", M_handle->stream_id);
            err_flag =  1;
        }else if (DISCONNECT == M_handle->status) {
            nslog(NS_NOTICE, "%d stream will be closed", M_handle->stream_id);
            recv_report_xml(DISCONNECT, M_handle);
        }else if (FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LOGIN_BIT)) {

            if (0 == delay_connect_count) {	
                nslog(NS_ERROR, "%d stream connect %s encode failed", M_handle->stream_id, M_handle->ipaddr);
                delay_connect_count == -5;
            }
            recv_sleep_second(M_handle, LOOP_DELAY);
            continue;
        }

        SET_FLAG_BIT_FALSE(M_handle->log_flag, LOGIN_BIT);
        recv_sleep_second(M_handle, LOOP_DELAY);
        continue;
    }

	nslog(NS_INFO, "%d stream is exit, pid = %lld", M_handle->stream_id, pthread_self());

    if (in_data) {
        r_free(in_data);
        in_data = NULL;
    }
	// add zl
	if(udp_send_module_hand != NULL)
	{
		UdpSend_rtp_build_uninit(&udp_send_module_hand->rtp_hand);
	}

		
    if (M_handle->frame_data)
    {
    	nslog(NS_ERROR,"FUCK---1 FREE VIDEO_DATA  <%d>\n",M_handle->stream_id);
           r_free(M_handle->frame_data);
           M_handle->frame_data = NULL;
    }
    if (M_handle->audio_data)
    {
    	
    	nslog(NS_ERROR,"FUCK---1 FREE AUDIO_DATA  <%d>\n",M_handle->stream_id);
         r_free(M_handle->audio_data);
         M_handle->audio_data = NULL;
    }
	if(M_handle->audio_info != NULL)
	{
    	nslog(NS_ERROR,"FUCK---1 FREE AUDIO_INFO  <%d>\n",M_handle->stream_id);
		 r_free(M_handle->audio_info);
         M_handle->audio_info = NULL;
	}
		
    return NULL;
}

int32_t recv_print_status(recv_room_handle *recv_handle)
{
    int index = 0;
    if (NULL == recv_handle) {
        nslog(NS_ERROR, "recv_handle = [%p]", recv_handle);
        return OPERATION_ERR;
    }
    //<id_ip_status_is_data_data_type_stream_type_width*hight_FrameRate> <code_rate_SampleRate>
    nslog(NS_INFO, "receive <ID_ST_Conn_Data> <Fd_DataT_StreamT> <toRec_Live> <Widh*High_Fps> <Acrate_Asate> [IP]");
    for (index = 0; index < recv_handle->stream_num; index++) {
		 if (recv_handle->stream_hand[index].status != 0 || 0 != r_strlen(recv_handle->stream_hand[index].ipaddr) 
		 || recv_handle->stream_hand[index].stream_id > 0) {
        nslog(NS_INFO, "receive <%2d%2d%4d%6d > <%2d%3d%8d   > <%3d%6d > <%4d*%4d%4d> <%6d%6d> [%s]", recv_handle->stream_hand[index].stream_id, recv_handle->stream_hand[index].status, 
                recv_handle->stream_hand[index].recv_pri.status, recv_handle->stream_hand[index].recv_pri.is_data,  recv_handle->stream_hand[index].sockfd, 
                recv_handle->stream_hand[index].recv_pri.data_type, recv_handle->stream_hand[index].recv_pri.stream_type,
                 recv_handle->stream_hand[index].recv_pri.to_rec, recv_handle->stream_hand[index].recv_pri.to_live, recv_handle->stream_hand[index].recv_pri.width, recv_handle->stream_hand[index].recv_pri.hight, recv_handle->stream_hand[index].recv_pri.frame_rate, 
                recv_handle->stream_hand[index].recv_pri.code_rate, recv_handle->stream_hand[index].recv_pri.sample_rate, recv_handle->stream_hand[index].ipaddr);
    	}
		// add zl
		memset_pri_status(&recv_handle->stream_hand[index].recv_pri);
    }
    nslog(NS_INFO, "\n");
    return OPERATION_SUCC;
}

static void * recv_real_print(void *recv_handle)
{
    recv_room_handle *recv_hand = (recv_room_handle *)recv_handle;
    int32_t print_count = 0;
    int32_t index = 0;

    while ( (recv_hand->stream_hand[recv_hand->stream_num].pthread_status) && recv_hand->stream_num) {

        r_sleep(RECV_KEEPALIVE_DALAY);
        print_count++;

#if RECV_KEEPLIVE//keepalive  保持心跳包
        for (index = 0; index < recv_hand->stream_num; index++) {
            if (TRUE == recv_hand->stream_hand[index].alive_flag) {
                pthread_mutex_lock(&(recv_hand->stream_hand[index].alive_mutex));
                recv_hand->stream_hand[index].alive_flag = FALSE;
                pthread_mutex_unlock(&(recv_hand->stream_hand[index].alive_mutex));
            } else if (CONNECT == recv_hand->stream_hand[index].recv_pri.status && FALSE == recv_hand->stream_hand[index].alive_flag){
                close_stream_sock(&recv_hand->stream_hand[index]);
            }
        }
#endif

        if (2 == print_count)  {           //延时30秒打印日志
            recv_print_status(recv_handle);
            print_count = 0;
        }
    }

    nslog(NS_INFO, "recv_real_print is exited");
    return 0;
}

static int32_t create_stream_pthread(stream_handle *handle)
{
	pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = RECV_THREAD_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
	
	pthread_attr_setstacksize(&attr,THREAD_POOL_SIZE);
	
	if(r_pthread_create(&(handle->pid), &attr, room_receive_deal, (void *)handle))
    {
        nslog(NS_ERROR, "create streamid:%d  room_pthread error",handle->stream_id);
        return OPERATION_ERR;
    }
    else
    {
        nslog(NS_INFO, "create streamid:%d  room_pthread success, pid = %lld",handle->stream_id, handle->pid);
    }
    pthread_attr_destroy(&attr);
    return OPERATION_SUCC;
}

static int32_t create_room_pthread(recv_room_handle *recv_hand)
{
    if(NULL == recv_hand)
    {
        return OPERATION_ERR;
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,THREAD_POOL_SIZE);
	// add zl
#if 0
    for(i = 0; i <  recv_hand->stream_num; i++)
    {
  		if(r_pthread_create(&(recv_hand->stream_hand[i].pid), &attr, room_receive_deal, (void *) & (recv_hand->stream_hand[i])))
        {
            nslog(NS_ERROR, "create %d num room_pthread error", i + 1);
            return OPERATION_ERR;
        }
        else
        {
            nslog(NS_INFO, "create the %d room  success, pid = %lld",  i + 1, recv_hand->stream_hand[i].pid);
        }
    }
	
#endif	

    if (r_pthread_create(&(recv_hand->stream_hand[recv_hand->stream_num].pid),&attr, recv_real_print, (void *) recv_hand))
    {
        nslog(NS_WARN, "create recv_real_print thread error");
    }
	
	pthread_attr_destroy(&attr);
    return OPERATION_SUCC;

}

/*==============================================================================
  函数: <unregister_room_receive_module>
  功能: <注销会议室接收模块，并释放空间.>
  参数: recv_room_handle *recv_hand
  Created By liuchsh 2012.11.16 14:17:23 For EDU
  ==============================================================================*/
int8_t unregister_room_receive_module(recv_room_handle *recv_hand)
{
    if(NULL == recv_hand)
    {
        nslog(NS_ERROR, "r_malloc");
        return OPERATION_ERR;
    }

    recv_hand->init_room_module = NULL;
    recv_hand->deinit_room_module = NULL;
    recv_hand->set_room_info = NULL;
    recv_hand->close_room_connect = NULL;
    recv_hand->open_room_connect = NULL;
    recv_hand->set_recv_to_ctrl_msgid = NULL;
    recv_hand->get_stream_socket = NULL;
    recv_hand->set_recv_to_rec_msgid = NULL;
	recv_hand->set_recv_to_usb_rec_msgid = NULL;
    recv_hand->set_recv_to_live_msgid = NULL;
    recv_hand->get_stream_connect = NULL;
    recv_hand->close_stream_connect = NULL;
    recv_hand->open_stream_connect = NULL;

    recv_hand->set_rec_status = NULL;
    recv_hand->get_rec_status = NULL;
    recv_hand->set_usb_rec_status = NULL;
    recv_hand->get_usb_rec_status = NULL;

    deinit_room_module(recv_hand);
    return OPERATION_SUCC;

}


