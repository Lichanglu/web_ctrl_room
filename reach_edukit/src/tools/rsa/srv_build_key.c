#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "rsa.h"

static int write_device_key(unsigned char *key , int key_len
        , unsigned char *c_pub_key, int c_pub_key_len );

    int
main( int argc, char **argv )
{
    int ret = 0;

    // read device register file
    unsigned char *reg_data = NULL;
    int            reg_len = 0;

    ret = rsa_read_file(C_REG_FILE, reg_data, &reg_len); 
    if( ret == -1) {
        printf("open %s failed!\n", C_REG_FILE);
        goto release_resource;
    }

    reg_data = calloc(reg_len, sizeof(char));
    if( reg_data == NULL) {
        printf("alloc reg data failed! request len = %d \n", reg_len);
        goto release_resource;
    }

    ret = rsa_read_file(C_REG_FILE, reg_data, &reg_len);
    if (ret == -1) {
        printf("read reg data file failed!\n");
        goto release_resource;
    }

    //read server private key buffer
    unsigned char *s_pri_key = NULL;
    int            s_pri_key_len = 0;
    ret = rsa_read_file(S_PRIVATE_KEY_FILE, s_pri_key, &s_pri_key_len);
    if( ret == -1) {
        printf("open %s failed!\n", S_PRIVATE_KEY_FILE);
        goto release_resource;
    }

    s_pri_key = calloc(s_pri_key_len , sizeof(char));
    if( s_pri_key == NULL) {
        printf("alloc s_pri_key failed! request len = %d \n", s_pri_key_len);
        goto release_resource;
    }

    ret = rsa_read_file(S_PRIVATE_KEY_FILE, s_pri_key, &s_pri_key_len);
    if( ret == -1) {
        printf("read server private key data failed!\n");
        goto release_resource;
    }

    //decrypto register file 
    unsigned char *out_data;
    int out_len = 0;
    out_len = 4 * reg_len;
    out_data = calloc(out_len, sizeof(char));
    if(out_data == NULL ) {
        printf("alloc decrypt data failed! request len = %d\n", out_len);
        goto release_resource;
    }
    ret = rsa_private_decrypt(reg_data, reg_len, out_data, &out_len
               ,s_pri_key, s_pri_key_len);
    if( ret == -1 ) {
        printf("decrypt register file failed!\n");
        goto release_resource;
    }
    printf("decrypt register file success , len = %d\n", out_len);

    //splite hardware info and client public key
    unsigned char   *hardware_info = NULL;
    int              hd_info_len = 0;
    
    memcpy(&hd_info_len, out_data, sizeof(int));
    hardware_info = calloc(hd_info_len, sizeof(char));
    if( hardware_info == NULL ) {
        printf("alloc hardware info mem failed! request len = %d\n"
                , hd_info_len);
        goto release_resource;
    }
    memcpy(hardware_info, out_data + sizeof(int), hd_info_len);

    unsigned char   *pub_key = NULL;
    int              key_len = 0;
    memcpy(&key_len, out_data + sizeof(int) + hd_info_len, sizeof(int));
    pub_key = calloc(key_len , sizeof(char));
    if( pub_key == NULL) {
        printf("calloc pub key mem failed! request len = %d\n", key_len);
        goto release_resource;
    }
    memcpy(pub_key, out_data + 2 * sizeof(int) + hd_info_len, key_len);

    // write device key file
    // hardware info and setup info
    unsigned char setup_data[1024];
    int setup_len = 0;

    FILE *fp = fopen(NRS_SETUP_FILE,"rb");
    if( fp == NULL ) {
        printf("open setup file %s failed!\n",NRS_SETUP_FILE);
        goto release_resource;
    }
    setup_len = fread(setup_data, sizeof(char), 1024, fp);
    if( setup_len < 0) {
        printf("read setup data failed!\n");
        goto release_resource;
    }

    unsigned char  *device_key = NULL;
    int  device_key_len = 0;

    device_key_len = setup_len + hd_info_len + sizeof(int) * 2;
    device_key = calloc(device_key_len , sizeof(char));
    if( device_key == NULL ) {
        printf("alloc device key buf failed! request len = %d\n"
                , device_key_len);
        goto release_resource;
    }
    
    memcpy(device_key, &hd_info_len, sizeof(int));
    memcpy(device_key + sizeof(int), hardware_info, hd_info_len);
    memcpy(device_key + sizeof(int) + hd_info_len , &setup_len, sizeof(int));
    memcpy(device_key + sizeof(int) * 2 + hd_info_len , setup_data, setup_len);
    
    ret = write_device_key(device_key, device_key_len,pub_key, key_len );
    if( ret == -1) {
        printf("rsa write device key file %s failed!\n", C_DEVICE_KEY_FILE);
        goto release_resource;
    }

release_resource:
    if( device_key )
        free(device_key);
    if( hardware_info )
        free(hardware_info);
    if( out_data )
        free(out_data);
    if( s_pri_key )
        free(s_pri_key);
    if( reg_data )
        free( reg_data);
    
    if( fp )
        fclose(fp);

    return 0;
}

static int write_device_key(unsigned char *key , int key_len
        , unsigned char *c_pub_key, int c_pub_key_len )
{
    int ret = 0;
    unsigned char *out_data = NULL;
    int out_len = 0;

    out_len = 4 * c_pub_key_len;
    out_data = calloc(out_len , sizeof(char));
    if( out_data == NULL) {
        printf("malloc encrypt buf failed!\n");
        return -1;
    }
    
    ret = rsa_public_encrypt(key, key_len , out_data, &out_len
            , c_pub_key, c_pub_key_len);
    if( ret == -1) {
        printf("rsa public encrypt failed!\n");
        free(out_data);
        return -1;
    }

    ret = rsa_write_file(C_DEVICE_KEY_FILE, out_data, out_len);
    if( ret == -1) {
        printf("rsa write device key file %s failed!\n", C_DEVICE_KEY_FILE);
    }

    free(out_data);

    return ret;
}
