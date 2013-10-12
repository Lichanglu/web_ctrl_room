#ifdef HAVE_RTSP_MODULE

#ifndef _TRAFFIC_SHAPING_H__
#define _TRAFFIC_SHAPING_H__


void tc_init(void);
int tc_add_element(int classid, char *dst_ip, unsigned short dst_port, int a_rate, int c_rate);
int tc_del_element(int classid);
int tc_set_element(int classid, char *dst_ip, unsigned short dst_port, int a_rate, int c_rate);
int tc_status_print();
#endif

#endif