#ifndef _CLI_VERIFY_H
#define _CLI_VERIFY_H

#ifdef __cplusplus
extern "C"{
#endif

#define CLIENT_PRI_KEY			"/etc/reach/c_pri.key"
#define CLIENT_DEVICE_KEY		"/etc/reach/c_device.key"



char *check_hw_and_get_pri_data(char **pri_setup_data);

#ifdef __cplusplus
}
#endif
#endif

