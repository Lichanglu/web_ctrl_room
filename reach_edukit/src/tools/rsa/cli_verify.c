#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rsa.h"
#include "cli_verify.h"


char *check_hw_and_get_pri_data(char **pri_setup_data)
{
    int ret 	 = -1;
    char *tmp    = NULL;
    unsigned char *pri_key = NULL;
    int pri_len = 0;
    unsigned char *device_key = NULL;
    int device_key_len = 0;
    unsigned char *out_data = NULL;
    int  out_len = 0;
    unsigned char *hardware_info = NULL;
    int hd_info_len = 0;
    unsigned char *setup_data = NULL;
    int setup_len = 0;
    unsigned char    cur_hardware_info[256];    

    if (*pri_setup_data != NULL) {
    	printf("param error\n");
	return NULL;
    }

    // read device private key
    ret = rsa_read_file(CLIENT_PRI_KEY, pri_key , &pri_len);
    if( ret == -1 )
        return NULL;

    pri_key = calloc(pri_len , sizeof(char));
    if( pri_key == NULL ) {
        printf("malloc failed");
        return NULL;
    }

    ret = rsa_read_file(CLIENT_PRI_KEY, pri_key , &pri_len);
    if( ret == -1) {
        printf("read %s data failed!\n", CLIENT_PRI_KEY);
        goto free_resource;
    }

    // decrypt device key file to setup info and hardware info
    ret = rsa_read_file(CLIENT_DEVICE_KEY, device_key, &device_key_len);
    if( ret == -1) {
        printf("read %s data failed!\n", CLIENT_DEVICE_KEY);
        goto free_resource;
    }

    device_key = calloc(device_key_len ,sizeof(char));
    if( device_key == NULL ) {
        printf("malloc device key buf failed!");
        goto free_resource;
    }

    ret = rsa_read_file(CLIENT_DEVICE_KEY, device_key, &device_key_len);
    if (ret == -1) {
        printf("read %s data failed!\n", CLIENT_DEVICE_KEY);
        goto free_resource;
    }

    // decrypt by device private key
    out_len = 4 * device_key_len;
    out_data = calloc(out_len, sizeof(char));
    if(out_data == NULL) {
        printf("malloc decrypt buf failed! request len = %d \n", out_len);
        goto free_resource;
    }

    ret = rsa_private_decrypt(device_key , device_key_len, out_data, &out_len
            , pri_key, pri_len);
    if( ret == -1 ) {
        printf("rsa device private key decrypt failed!\n");
        goto free_resource;
    }

    //split hardware info and setup data
    hd_info_len = *(int *)out_data;
    hardware_info = out_data + sizeof(int);
    setup_len = *(int *)(out_data + sizeof(int) + hd_info_len);
    setup_data = out_data + sizeof(int) * 2 + hd_info_len;


    if(hd_info_len + setup_len + sizeof(int) *2 != out_len){
        printf("verify device key failed!\n");
        goto free_resource;
    }

    printf("setup_len = %d, setup_buf = %s\n", setup_len, setup_data);

    // read real hardware info and compare 
    memset(cur_hardware_info, 0 , 256);
    ret = rsa_read_hardware_info(cur_hardware_info, 256);
    if ( ret == -1 ) {
        printf("rsa read hardware info failed!\n");
        goto free_resource;
    }

    printf("cur_hardware_info = %s\n hardware_info = %s\n", cur_hardware_info, hardware_info);

    if(memcmp(hardware_info , cur_hardware_info , 256) == 0) {
        printf("compare hardware info success!\n");
    } else {
	ret = -1;
        printf("compare hardware info failed!\n");
    }

    if (ret != -1) {
    	*pri_setup_data = calloc(setup_len, sizeof(char));
	if (*pri_setup_data == NULL){
            printf("malloc pri_setup_data buf failed! request len = %d \n", setup_len);
	    goto free_resource;
	}

	memcpy(*pri_setup_data, setup_data, setup_len);
    	tmp = *pri_setup_data; 
    }

free_resource:
    if( pri_key )
        free(pri_key);
    if( out_data )
        free(out_data);
    if( device_key )
        free(device_key);

    return tmp;
}

