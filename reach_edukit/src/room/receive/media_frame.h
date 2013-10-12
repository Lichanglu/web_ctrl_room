
#ifndef	__MEDIA_FRAME_H__
#define	__MEDIA_FRAME_H__

// add zl
#include "stdint.h"
#include "reach_udp_recv.h"
#include "receive_module.h"

// add zl
int recv_deal_udp_recv_data(udp_recv_stream_info_t *p_stream_info,char *data,int len);
uint32_t getCurrentTime(void);
uint32_t getIntervalTime(uint32_t prev_time, uint32_t cur_time);
uint32_t getJpgIntervalTime(uint32_t prev_time, uint32_t cur_time);
int32_t pkg_placed_in_frame(int8_t *frame_data,  int32_t part_frame_len,  stream_handle *M_handle);
int32_t pkg_placed_in_frame_audio(int8_t *frame_data,  int32_t part_frame_len,  stream_handle *M_handle);

#endif //__MEDIA_FRAME_H__

