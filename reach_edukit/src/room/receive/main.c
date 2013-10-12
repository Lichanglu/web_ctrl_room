#include <stdio.h>
#include "receive_module.h"

#define NSLOG_CONF  					"./nslog.conf"
#define NSLOG_TEST_CONF  				"./nslog.conf"
#define CONTROL_ADDR					"192.168.4.113"
#define ENCODE_PORT  						(3400)
#define RECV_TEST_STREAM        
#define RECV_TEST_WRITE_FILE

int32_t  __main(int32_t argc, int8_t **argv)
{
	int32_t rc, i = 0;
	recv_param r_param[MAX_STREAM];
	recv_room_handle * recv_handle= NULL;
	rc = dzlog_init(NSLOG_CONF, "Rec_server");
	if (rc) {
		printf("dzlog_init failed : <%s>\n", NSLOG_CONF);
		return -1;
	}
	r_memset(r_param, 0, sizeof(recv_param) * MAX_STREAM);

	r_memcpy(r_param[0].ipaddr, CONTROL_ADDR, sizeof(CONTROL_ADDR));
	r_param[0].port = ENCODE_PORT;
	r_param[0]. stream_id = 1;
	r_param[0].status = RUNNING;
	#if 1 
		r_memcpy(r_param[2].ipaddr, "192.168.4.239", sizeof(CONTROL_ADDR));
	r_param[2].port = ENCODE_PORT;
	r_param[2]. stream_id = 3;
	r_param[2].status = RUNNING;


		r_memcpy(r_param[1].ipaddr, "192.168.4.112", sizeof(CONTROL_ADDR));
	r_param[1].port = ENCODE_PORT;
	r_param[1]. stream_id = 2;
	r_param[1].status = RUNNING;


	r_memcpy(r_param[3].ipaddr, "192.168.4.239", sizeof(CONTROL_ADDR));
	r_param[3].port = ENCODE_PORT;
	r_param[3]. stream_id = 4;
	r_param[3].status = RUNNING;

	r_memcpy(r_param[4].ipaddr, "192.168.4.242", sizeof(CONTROL_ADDR));
	r_param[4].port = ENCODE_PORT;
	r_param[4]. stream_id = 5;
	r_param[4].status = RUNNING;

	#endif
	recv_handle = register_room_receive_module(1);
	recv_handle->set_room_info(r_param, recv_handle, 5);


    int count = 0;
	while (1)
    {
        if (60 == (count++)) 
        {
            unregister_room_receive_module(recv_handle);
        }
        if (120 == count)
        {
            recv_handle = register_room_receive_module(5);
             recv_handle->set_room_info(r_param, recv_handle, 5);
            count = 0;
        }
        sleep(1);
    }
	return 0;
}
